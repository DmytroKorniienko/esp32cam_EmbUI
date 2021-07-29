//https://github.com/MoonFox2006/ESP32_BlinkerButtons
#pragma once

#include <esp_timer.h>
#include <driver/ledc.h>
#include "CustomTask.h"
#include "Blinker.h"

const uint8_t FADES_SIZE = 16;
const uint8_t FADES[FADES_SIZE] = { 0, 1, 2, 4, 8, 16, 32, 64, 128, 192, 224, 240, 248, 252, 254, 255 };

int8_t abs8(int8_t value);

class Blinker {
public:
  typedef enum blinkmode_t { BLINK_OFF, BLINK_ON, BLINK_TOGGLE, BLINK_05HZ, BLINK_1HZ, BLINK_2HZ, BLINK_4HZ, BLINK_FADEIN, BLINK_FADEOUT, BLINK_FADEINOUT, BLINK_PWM } BLINKMODE;

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
      ledc_set_duty(_speed_mode, _channel, FADES[abs8(_value)%FADES_SIZE]); // 16 градаций яркости
      ledc_update_duty(_speed_mode, _channel);
    }
  }
  void setupTimeout(uint16_t timeout) {_ms = millis(); _timeout = timeout;}
protected:
  static void timerCallback(void* pObjInstance);
  struct __attribute__((__packed__)) {
    uint8_t _pin : 7;
    bool _level : 1;
    blinkmode_t _mode : 4;
    ledc_timer_t _timer_num : 3;
    ledc_mode_t _speed_mode : 2;
    ledc_channel_t _channel : 4;
    volatile int8_t _value;
    volatile uint16_t _timeout;
    volatile unsigned long _ms;
    esp_timer_handle_t _timer;
  };
};

class BlinkerTask : public CustomTask {
public:
  BlinkerTask(uint8_t pin, bool level) : CustomTask("BlinkerTask", 2048, 0, CustomTask::CORE_1), _blinker(NULL), _pin(pin), _level(level) {}
  Blinker &getBlinker() {return *_blinker;}
  void off() { if(_blinker) _blinker->setMode(Blinker::BLINK_OFF); }
  void toggle() { if(_blinker){
      if(_blinker->getValue() && _blinker->getMode() == Blinker::BLINK_PWM){
          _ledbright = _blinker->getValue();
          _blinker->setMode(Blinker::BLINK_OFF);
      }
      else {
          _blinker->setMode(Blinker::BLINK_PWM);
          _blinker->setValue(_ledbright);
      } 
    }
  }
  void setBright(int8_t val) { if(_blinker){ _blinker->setMode(Blinker::BLINK_PWM); _blinker->setValue(val); _ledbright = val; } }
  int8_t getBright() {return _ledbright;}
  void setLedOffAfterMS(uint16_t ms) { if(_blinker){ _blinker->setupTimeout(ms); } }
  void Demo();

protected:
  void setup();
  void loop();
  void cleanup();

  struct __attribute__((__packed__)) {
    Blinker *_blinker;
    int8_t _ledbright = 1;
    uint8_t _pin : 7;
    bool _level : 1;
  };
};