#ifndef __MISC_H
#define __MISC_H

//#pragma once
#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined ESP32
#include <WiFi.h>
#endif
//#include "user_config.h" - если включить, то UI тоже будет слать лог по телнет и это вызовет падения при большом потоке лога

// Задержка после записи в ФС, не менять, если не сказано дополнительно!
#ifndef DELAY_AFTER_FS_WRITING
#define DELAY_AFTER_FS_WRITING       (50U)                        // 50мс, меньшие значения могут повлиять на стабильность
#endif


//----------------------------------------------------
// мини-класс таймера, версия 1.0

class timerMinim
{
  public:
	timerMinim() {_interval = 0;} // по дефолту - отключен

    timerMinim(uint32_t interval)				                  // объявление таймера с указанием интервала
    {
      _interval = interval;
      _timer = millis();
    }

    uint32_t getInterval()	                  						 // получение интервала работы таймера
    {
    	return _interval;
    }

    void setInterval(uint32_t interval)	                   // установка интервала работы таймера
    {
      _interval = interval;
    }

    bool isReady()						                             // возвращает true, когда пришло время. Сбрасывается в false сам (AUTO) или вручную (MANUAL)
    {
      if ((uint32_t)millis() - _timer >= _interval && _interval!=0){
        _timer = millis();
        return true;
      }
      else {
        return false;
      }
    }

    bool isReadyManual()                                   // возвращает true, когда пришло время. Без сбороса
    {
      if ((uint32_t)millis() - _timer >= _interval && _interval!=0){
        return true;
      }
      else {
        return false;
      }
    }

    void reset()							                              // ручной сброс таймера на установленный интервал
    {
      _timer = millis();
    }

  private:
    uint32_t _timer = 0;
    uint32_t _interval = 0;
};

//----------------------------------------------------
#if defined(SYS_DEBUG) && DEBUG_TELNET_OUTPUT
	//#define LOG                   telnet
	#define LOG(func, ...) telnet.func(__VA_ARGS__)
#elif defined(SYS_DEBUG)
	//#define LOG                   Serial
	#define LOG(func, ...) Serial.func(__VA_ARGS__)
#else
	#define LOG(func, ...) ;
#endif

#if defined(SYS_DEBUG) && DEBUG_TELNET_OUTPUT
#define TELNET_PORT           (23U)                         // номер telnet порта
extern WiFiServer telnetServer;
extern WiFiClient telnet;
extern bool telnetGreetingShown;
void handleTelnetClient();

// WiFiServer telnetServer(TELNET_PORT);                       // telnet сервер
// WiFiClient telnet;                                          // обработчик событий telnet клиента
// bool telnetGreetingShown = false;                           // признак "показано приветствие в telnet"

#endif

#endif