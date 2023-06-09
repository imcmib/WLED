#pragma once

#include "wled.h"
#include "animations/mario_1.h"
#include "animations/mario_3.h"
#include "animations/pacman_2.h"
#include "animations/tiktok_1.h"
#include "animations/tiktok_2.h"
#include "animations/twitch_2.h"

class MatrixAnimationsUsermod : public Usermod {

private:
  // Private class members. You can declare variables and functions only accessible to your usermod here
  unsigned long lastTime = 0;

  // ---- Variables modified by settings below -----
  // set your config variables to their boot default value (this can also be done in readFromConfig() or a constructor if you prefer)
  bool _enabled = true;

public:
  void setup() {
    strip.addEffect(255, &mario_1, MARIO_1);
    strip.addEffect(255, &mario_3, MARIO_3);
    strip.addEffect(255, &pacman_2, PACMAN_2);
    strip.addEffect(255, &tiktok_1, TIKTOK_1);
    strip.addEffect(255, &tiktok_2, TIKTOK_2);
    strip.addEffect(255, &twitch_2, TWITCH_2);
  }

  void loop() {
    if (!_enabled || strip.isUpdating()) {
      return;
    }

    if (millis() - lastTime > 1000) {
      lastTime = millis();
    }
  }

  void addToJsonInfo(JsonObject& root) {
    JsonObject user = root["u"];
    if (user.isNull()) {
      user = root.createNestedObject("u");
    }

    JsonArray lightArr = user.createNestedArray("m_anim"); //name
    lightArr.add(_enabled ? "enabled" : "disabled"); //value
  }

  void addToConfig(JsonObject &root) {
    JsonObject top = root.createNestedObject("m_anim");
    top["enabled"] = _enabled;
  }

  bool readFromConfig(JsonObject &root) {
    JsonObject top = root["m_anim"];

    bool configComplete = !top.isNull();
    configComplete &= getJsonValue(top["enabled"], _enabled);

    return configComplete;
  }

  uint16_t getId() {
    return USERMOD_ID_MATRIX_ANIMATIONS;
  }
};
