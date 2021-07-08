//https://github.com/MoonFox2006/ESP32_BlinkerButtons
#pragma once

#include <esp_timer.h>
#include <driver/ledc.h>

class Blinker {
public:
  enum blinkmode_t { BLINK_OFF, BLINK_ON, BLINK_TOGGLE, BLINK_05HZ, BLINK_1HZ, BLINK_2HZ, BLINK_4HZ, BLINK_FADEIN, BLINK_FADEOUT, BLINK_FADEINOUT, BLINK_PWM };

  Blinker(uint8_t pin, bool level, uint32_t freq = 1000, ledc_timer_t timer_num = LEDC_TIMER_3, ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE, ledc_channel_t channel = LEDC_CHANNEL_7);
  ~Blinker();

  operator bool() {
    return _timer != NULL;
  }
  void operator=(blinkmode_t mode) {
    setMode(mode);
  }
  void operator<<(int8_t value) {
    setValue(value);
  }
  blinkmode_t getMode() const {
    return _mode;
  }
  int8_t getValue() const {
    return _value;
  }
  void setMode(blinkmode_t mode);
  void setValue(int8_t value) {
    _value=value;
    if(_mode == BLINK_PWM){
      ledc_set_duty(_speed_mode, _channel, _value);
      ledc_update_duty(_speed_mode, _channel);
    }
  };

protected:
  void timerCallback();

  struct __attribute__((__packed__)) {
    uint8_t _pin : 7;
    bool _level : 1;
    blinkmode_t _mode : 4;
    ledc_timer_t _timer_num : 3;
    ledc_mode_t _speed_mode : 2;
    ledc_channel_t _channel : 4;
    volatile int8_t _value;
    esp_timer_handle_t _timer;
  };
};
