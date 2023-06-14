#pragma once

#include "wled.h"
#include "Adafruit_VEML7700.h"

// the frequency to check photoresistor, 1 second
#ifndef USERMOD_SN_PHOTORESISTOR_MEASUREMENT_INTERVAL
#define USERMOD_SN_PHOTORESISTOR_MEASUREMENT_INTERVAL 1000
#endif

// how many seconds after boot to take first measurement, 10 seconds
#ifndef USERMOD_SN_PHOTORESISTOR_FIRST_MEASUREMENT_AT
#define USERMOD_SN_PHOTORESISTOR_FIRST_MEASUREMENT_AT 10000
#endif

// only report if differance grater than offset value
#ifndef USERMOD_SN_PHOTORESISTOR_OFFSET_VALUE
#define USERMOD_SN_PHOTORESISTOR_OFFSET_VALUE 5
#endif

class Usermod_SN_Photoresistor : public Usermod {

private:
  int8_t offset = USERMOD_SN_PHOTORESISTOR_OFFSET_VALUE;

  unsigned long readingInterval = USERMOD_SN_PHOTORESISTOR_MEASUREMENT_INTERVAL;

  long lastMeasurement;
  long last;
  uint16_t lastLDRValue = -1;

  // flag set at startup
  bool enabled = true;
  bool initDone = false;

  // strings to reduce flash memory usage (used more than twice)
  static const char _name[];
  static const char _enabled[];
  static const char _photoresistorPin[];
  static const char _readInterval[];
  static const char _resistorValue[];
  static const char _offset[];

  Adafruit_VEML7700* _photocell;

  template <typename T> T CLAMP(const T& value, const T& low, const T& high) 
  {
    return value < low ? low : (value > high ? high : value); 
  }

  bool checkBoundSensor(float newValue, float prevValue, float maxDiff) {
    return isnan(prevValue) || newValue <= prevValue - maxDiff || newValue >= prevValue + maxDiff;
  }

  uint16_t getLuminance() {
    float lux = _photocell->readLux();

    // DEBUG_PRINTLN("Lux: " + String(lux));
    return (uint16_t) lux;
  }

public:
  void setup() {
    lastMeasurement = millis() + USERMOD_SN_PHOTORESISTOR_FIRST_MEASUREMENT_AT;
    last = millis() + USERMOD_SN_PHOTORESISTOR_FIRST_MEASUREMENT_AT;

    _photocell = new Adafruit_VEML7700();
    _photocell->begin();
    _photocell->setGain(VEML7700_GAIN_1_4);
    _photocell->setIntegrationTime(VEML7700_IT_100MS);
    _photocell->interruptEnable(false);

    initDone = true;
  }

  void loop() {
    if (!enabled || strip.isUpdating() || !initDone)
      return;

    long now = millis();
    // check to see if we are due for taking a measurement
    // lastMeasurement will not be updated until the conversion
    // is complete the the reading is finished
    if (now - lastMeasurement < readingInterval) {
      return;
    }
    lastMeasurement = millis();

    uint16_t currentLDRValue = getLuminance();
    if (checkBoundSensor(currentLDRValue, lastLDRValue, offset)) {
      lastLDRValue = currentLDRValue;

      bri = map(lastLDRValue, 0, 100, 30, 255);
      stateUpdated(CALL_MODE_USERMODE);

#ifndef WLED_DISABLE_MQTT
      if (WLED_MQTT_CONNECTED) {
        char subuf[45];
        strcpy(subuf, mqttDeviceTopic);
        strcat_P(subuf, PSTR("/luminance"));
        mqtt->publish(subuf, 0, true, String(lastLDRValue).c_str());
      }
    }
#endif
  }

  uint16_t getLastLDRValue() {
    return lastLDRValue;
  }

  void addToJsonInfo(JsonObject &root) {
    JsonObject user = root[F("u")];
    if (user.isNull())
      user = root.createNestedObject(F("u"));

    JsonArray lux = user.createNestedArray(F("Autodimming"));

    String uiDomString = F("<button class=\"btn btn-xs\" onclick=\"requestJson({");
    uiDomString += FPSTR(_name);
    uiDomString += F(":{");
    uiDomString += FPSTR(_enabled);
    uiDomString += enabled ? F(":false}});\">") : F(":true}});\">");
    uiDomString += F("<i class=\"icons ");
    uiDomString += enabled ? "on" : "off";
    uiDomString += F("\">&#xe08f;</i></button>");
    lux.add(uiDomString);

    if (enabled) {
      lux = user.createNestedArray(F("Luminance"));
      lux.add(lastLDRValue);
      lux.add(F(" lux"));
    }
  }

  /*
    * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
    * Values in the state object may be modified by connected clients
    */
  void addToJsonState(JsonObject& root) {
    if (!initDone) return;  // prevent crash on boot applyPreset()
    JsonObject usermod = root[FPSTR(_name)];
    if (usermod.isNull()) {
      usermod = root.createNestedObject(FPSTR(_name));
    }
    usermod["on"] = enabled;
  }

  /*
    * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
    * Values in the state object may be modified by connected clients
    */
  void readFromJsonState(JsonObject& root) {
    if (!initDone) return;  // prevent crash on boot applyPreset()
    bool en = enabled;
    JsonObject um = root[FPSTR(_name)];
    if (!um.isNull()) {
      if (um[FPSTR(_enabled)].is<bool>()) {
        en = um[FPSTR(_enabled)].as<bool>();
      } else {
        String str = um[FPSTR(_enabled)]; // checkbox -> off or on
        en = (bool)(str!="off"); // off is guaranteed to be present
      }
      if (en != enabled) {
        enabled = en;
      }
    }
  }

  /**
     * addToConfig() (called from set.cpp) stores persistent properties to cfg.json
     */
  void addToConfig(JsonObject &root) {
    // we add JSON object.
    JsonObject top = root.createNestedObject(FPSTR(_name)); // usermodname
    top[FPSTR(_enabled)] = enabled;
    top[FPSTR(_readInterval)] = readingInterval / 1000;
    top[FPSTR(_offset)] = offset;

    DEBUG_PRINTLN(F("Photoresistor config saved."));
  }

  /**
  * readFromConfig() is called before setup() to populate properties from values stored in cfg.json
  */
  bool readFromConfig(JsonObject &root) {
    // we look for JSON object.
    JsonObject top = root[FPSTR(_name)];
    if (top.isNull()) {
      DEBUG_PRINT(FPSTR(_name));
      DEBUG_PRINTLN(F(": No config found. (Using defaults.)"));
      return false;
    }

    enabled          = top[FPSTR(_enabled)] | enabled;
    readingInterval  = (top[FPSTR(_readInterval)] | readingInterval/1000) * 1000; // convert to ms
    offset           = top[FPSTR(_offset)] | offset;
    DEBUG_PRINT(FPSTR(_name));
    DEBUG_PRINTLN(F(" config (re)loaded."));

    // use "return !top["newestParameter"].isNull();" when updating Usermod with new features
    return true;
  }
};

uint16_t getId() {
  return USERMOD_ID_SN_PHOTORESISTOR;
}

// strings to reduce flash memory usage (used more than twice)
const char Usermod_SN_Photoresistor::_name[] PROGMEM = "Photoresistor";
const char Usermod_SN_Photoresistor::_enabled[] PROGMEM = "enabled";
const char Usermod_SN_Photoresistor::_readInterval[] PROGMEM = "read-interval-s";
const char Usermod_SN_Photoresistor::_offset[] PROGMEM = "offset";