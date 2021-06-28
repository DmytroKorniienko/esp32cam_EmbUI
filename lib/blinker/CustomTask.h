#ifndef __CUSTOMTASK_H
#define __CUSTOMTASK_H
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <freertos/portmacro.h>
#include <HardwareSerial.h>

class CustomTask {
public:
  enum core_t : uint8_t { CORE_0 = 0, CORE_1, CORE_ANY };

  CustomTask(const char *name, uint32_t stack, uint8_t priority = 1, core_t core = CORE_ANY);
  virtual ~CustomTask() {
    destroy();
  }
  static void halt(const char *msg);

  operator bool() {
    return _task != NULL;
  }
  bool isRunning();
  void pause();
  void resume();
  void destroy();
  void notify(uint32_t value);

  static void lock();
  static void unlock();

protected:
  enum flag_t : uint8_t { FLAG_DESTROYING = 1, FLAG_DESTROYED = 2, FLAG_USER = 4 };

  virtual void setup() {};
  virtual void loop() = 0;
  virtual void cleanup() {};

  void taskHandler();

  static portMUX_TYPE _mutex;
  EventGroupHandle_t _flags;
  TaskHandle_t _task;
};

extern CustomTask *task;

#endif