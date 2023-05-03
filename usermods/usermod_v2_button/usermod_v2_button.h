#pragma once

#include <functional>

#include "wled.h"

#include <AceButton.h>
using namespace ace_button;

class ButtonUsermod : public Usermod {

  private:

    const uint8_t touchButtonPin = 4;

    class CapacitiveConfig: public ButtonConfig {

      protected:

        int readButton(uint8_t pin) override {
          return digitalRead(pin) == LOW;
          // return touchRead(pin) > touchThreshold;
        }
    };

    CapacitiveConfig touchConfig;

    AceButton touch;
    AceButton button;

    int8_t step = 8;

    // Non-static handleEvent method
    void handleEvent(AceButton* /* button */, uint8_t eventType, uint8_t /* buttonState */);

    // Static method that calls the stored std::function
    static void handleEventStatic(AceButton* button, uint8_t eventType, uint8_t buttonState) {
      s_handleEventFunction(button, eventType, buttonState);
    }

    // The stored std::function created with std::bind
    static std::function<void(AceButton*, uint8_t, uint8_t)> s_handleEventFunction;

  public:

    ButtonUsermod() : touch(&touchConfig, touchButtonPin) {
      // Set the event handler using std::bind
      s_handleEventFunction = std::bind(&ButtonUsermod::handleEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
      touchConfig.setEventHandler(handleEventStatic);
    }

    void setup() {
      touchConfig.setFeature(ButtonConfig::kFeatureClick);
      touchConfig.setFeature(ButtonConfig::kFeatureDoubleClick);
      touchConfig.setFeature(ButtonConfig::kFeatureSuppressClickBeforeDoubleClick);
      touchConfig.setFeature(ButtonConfig::kFeatureLongPress);
      touchConfig.setFeature(ButtonConfig::kFeatureRepeatPress);
      touchConfig.setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
    }

    void loop() {
      touch.check();
    }

    bool handleButton(uint8_t b) {
      return btnPin[b] == touchButtonPin;
    }

    uint16_t getId() {
      return USERMOD_ID_BUTTON;
    }
};

// Define the static s_handleEventFunction member variable
std::function<void(AceButton*, uint8_t, uint8_t)> ButtonUsermod::s_handleEventFunction = nullptr;

// Implement the non-static handleEvent method outside the class definition
void ButtonUsermod::handleEvent(AceButton* /* button */, uint8_t eventType, uint8_t /* buttonState */) {
  switch (eventType) {
    case AceButton::kEventPressed:
      DEBUG_PRINTLN(F("PRESSED"));
      break;
    case AceButton::kEventReleased:
      DEBUG_PRINTLN(F("RELEASED"));
      break;
    case AceButton::kEventClicked:
      DEBUG_PRINTLN(F("CLICK"));
      toggleOnOff();
      stateUpdated(CALL_MODE_BUTTON);
      break;
    case AceButton::kEventDoubleClicked:
      DEBUG_PRINTLN(F("DOUBLE CLICK"));
      effectCurrent += 1;
      if (effectCurrent >= MODE_COUNT) effectCurrent = 0;
      stateChanged = true;
      colorUpdated(CALL_MODE_BUTTON);
      break;
    case AceButton::kEventRepeatPressed:
      DEBUG_PRINT(F("LONG: "));
      DEBUG_PRINTLN("S " + String(step) + ", B: " + String(bri));
      bri = constrain((bri + step), 10, 254);
      stateUpdated(CALL_MODE_BUTTON);
      break;
    case  AceButton::kEventLongReleased:
      DEBUG_PRINTLN(F("LONG RELEASED"));
      step = -step;
      break;
    default:
      DEBUG_PRINT(F("OTHER:"));
      DEBUG_PRINTLN(String(eventType));
      break;
  }
}
