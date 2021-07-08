#include "blinkerTask.h"

void BlinkerTask::setup() {
  if (_task) {
    _blinker = new Blinker(_pin, _level, 4096, LEDC_TIMER_1, LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1); // LEDC_LOW_SPEED_MODE
    if ((! _blinker) || (! *_blinker)) {
      if (_blinker) {
        delete _blinker;
        _blinker= NULL;
      }
      destroy();
    }
  }
  *_blinker = Blinker::BLINK_PWM; // фиксированный уровень
  //*_blinker<<(int8_t)16;
  *_blinker<<(int8_t)0;
}

void BlinkerTask::Demo()
{
  const char *BLINKS[] = { "OFF", "ON", "TOGGLE", "0.5 Hz", "1 Hz", "2 Hz", "4 Hz", "FADE IN", "FADE OUT", "FADE IN/OUT", "PWM" };

  Blinker::blinkmode_t mode = _blinker->getMode();

  if (mode < Blinker::BLINK_PWM)
    mode = (Blinker::blinkmode_t)((uint8_t)mode + 1);
  else
    mode = Blinker::BLINK_OFF;
  *_blinker = mode;
  lock();
  Serial.print("Blinker switch to ");
  Serial.println(BLINKS[mode]);
  unlock();
}

void BlinkerTask::loop() {
  // *_blinker = Blinker::BLINK_PWM; // фиксированный уровень
  // Demo();
  vTaskDelay(pdMS_TO_TICKS(5000));
}

void BlinkerTask::cleanup() {
  if (_blinker) {
    delete _blinker;
    _blinker = NULL;
  }
}