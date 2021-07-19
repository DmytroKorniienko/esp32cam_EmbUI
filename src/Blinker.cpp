//https://github.com/MoonFox2006/ESP32_BlinkerButtons
#include <esp_log.h>
#include "Blinker.h"

static const char TAG[] = "Blinker";

int8_t abs8(int8_t value) {
  return (value < 0) ? -value : value;
}

Blinker::Blinker(uint8_t pin, bool level, uint32_t freq, ledc_timer_t timer_num, ledc_mode_t speed_mode, ledc_channel_t channel) {
  _pin = pin;
  _level = level;
  _mode = BLINK_OFF;
  _timer_num = timer_num;
  _speed_mode = speed_mode;
  _channel = channel;
  _value = 0;
  {
    esp_timer_create_args_t timer_cfg = {
      .callback =  (esp_timer_cb_t)&Blinker::timerCallback,
      .arg = this
    };

    if (esp_timer_create(&timer_cfg, &_timer) != ESP_OK) {
      _timer = NULL;
      ESP_LOGE(TAG, "Error creating blinker timer!\r\n");
      return;
    }
  }
  {
    ledc_timer_config_t ledc_timer_cfg = {
      .speed_mode = speed_mode,
      { .duty_resolution = LEDC_TIMER_8_BIT },
      .timer_num = timer_num,
      .freq_hz = freq
    };

    if (ledc_timer_config(&ledc_timer_cfg) != ESP_OK) {
      esp_timer_delete(_timer);
      _timer = NULL;
      ESP_LOGE(TAG, "Error configuring LEDC timer!\r\n");
      return;
    }
  }
  gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)pin, ! level);
}

Blinker::~Blinker() {
  if (_timer) {
    esp_timer_stop(_timer);
    esp_timer_delete(_timer);
    if (_mode >= BLINK_FADEIN) {
      ledc_stop(_speed_mode, _channel, ! _level);
    }
  }
  gpio_reset_pin((gpio_num_t)_pin);
}

void Blinker::setMode(blinkmode_t mode) {
  if (_mode != mode) {
    if (mode <= BLINK_TOGGLE) {
      if (_mode > BLINK_TOGGLE) {
        esp_timer_stop(_timer);
        if (_mode >= BLINK_FADEIN) {
          ledc_stop(_speed_mode, _channel, ! _level);
          gpio_set_direction((gpio_num_t)_pin, GPIO_MODE_OUTPUT);
        }
      }
      if (mode == BLINK_TOGGLE)
        gpio_set_level((gpio_num_t)_pin, ! gpio_get_level((gpio_num_t)_pin));
      else
        gpio_set_level((gpio_num_t)_pin, mode == BLINK_ON ? _level : ! _level);
    } else { // mode > BLINK_TOGGLE
      esp_timer_stop(_timer);
      if (mode < BLINK_FADEIN) {
        if (_mode >= BLINK_FADEIN) {
          ledc_stop(_speed_mode, _channel, ! _level);
          gpio_set_direction((gpio_num_t)_pin, GPIO_MODE_OUTPUT);
        }
      } else { // mode >= BLINK_FADEIN
        if (_mode < BLINK_FADEIN) {
          ledc_channel_config_t ledc_channel_cfg = {
            .gpio_num = _pin,
            .speed_mode = _speed_mode,
            .channel = _channel,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = _timer_num,
            .duty = _level ? 0U : 255U
          };

          if (ledc_channel_config(&ledc_channel_cfg) != ESP_OK) {
            ESP_LOGE(TAG, "Error configuring LEDC channel!\r\n");
            return;
          }
        } else if(_mode == BLINK_PWM){
          //ledc_set_duty(_speed_mode, _channel, _value);
          //ledc_update_duty(_speed_mode, _channel);
          setValue(_value);
          return;
        }
      }
      _value = 0;
      {
        uint32_t period;

        if (mode <= BLINK_4HZ)
          period = 25000; // 25 ms.
        else if (mode <= BLINK_FADEOUT)
          period = 93750; // 93.75 ms.
        else // mode == BLINK_FADEINOUT
          period = 100000; // 100 ms.
        if (esp_timer_start_periodic(_timer, period) != ESP_OK) {
          ESP_LOGE(TAG, "Error starting blink timer!\r\n");
          return;
        }
      }
    }
    _mode = mode;
  } else if (mode == BLINK_TOGGLE) {
    gpio_set_level((gpio_num_t)_pin, ! gpio_get_level((gpio_num_t)_pin));
  }
}

void Blinker::timerCallback(void* pObjInstance) {
  Blinker* pObj = (Blinker*)pObjInstance;
  
  if (pObj->_mode <= BLINK_4HZ) {
    if (pObj->_value == 0) {
      gpio_set_level((gpio_num_t)pObj->_pin, pObj->_level);
    } else if (pObj->_value == 1) {
      gpio_set_level((gpio_num_t)pObj->_pin, !pObj->_level);
    }
    if (++pObj->_value >= (pObj->_mode == BLINK_05HZ ? 2000 : pObj->_mode == BLINK_1HZ ? 1000 : pObj->_mode == BLINK_2HZ ? 500 : 250) / 25)
      pObj->_value = 0;
  } else { // _mode >= BLINK_FADEIN
    if (pObj->_mode != BLINK_PWM) {
      ledc_set_duty(pObj->_speed_mode, pObj->_channel, pObj->_level ? FADES[abs8(pObj->_value)] : 255 - FADES[abs8(pObj->_value)]);
      ledc_update_duty(pObj->_speed_mode, pObj->_channel);
    }
    if (pObj->_mode == BLINK_FADEIN) {
      if (++pObj->_value >= FADES_SIZE)
        pObj->_value = 0;
    } else if (pObj->_mode == BLINK_FADEOUT) {
      if (--pObj->_value < 0)
        pObj->_value = FADES_SIZE - 1;
    } else if (pObj->_mode == BLINK_FADEINOUT) {
      if (++pObj->_value >= FADES_SIZE)
        pObj->_value = -(FADES_SIZE - 2);
    } else if (pObj->_mode == BLINK_PWM) {
      if(pObj->_timeout){ // таймаут отключения
        if(millis()-pObj->_ms>pObj->_timeout){
          pObj->_timeout = 0;
          pObj->setMode(BLINK_OFF);
        }
      }
    }
  }
}

//------------------------

void BlinkerTask::setup() {
  if (_task) {
    _blinker = new Blinker(_pin, _level, 4096, LEDC_TIMER_3, LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_7); // LEDC_LOW_SPEED_MODE LEDC_HIGH_SPEED_MODE
    if ((! _blinker) || (! *_blinker)) {
      if (_blinker) {
        delete _blinker;
        _blinker= NULL;
      }
      destroy();
    }
  }
  *_blinker = Blinker::BLINK_PWM; // фиксированный уровень
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
  // Demo();
  vTaskDelay(pdMS_TO_TICKS(5000));
}

void BlinkerTask::cleanup() {
  if (_blinker) {
    delete _blinker;
    _blinker = NULL;
  }
}
