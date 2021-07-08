#ifndef __BLINKERTASK_H
#define __BLINKERTASK_H

#include <arduino.h>
#include "CustomTask.h"
#include "Blinker.h"

class BlinkerTask : public CustomTask {
public:
  BlinkerTask(uint8_t pin, bool level) : CustomTask("BlinkerTask", 1024, 0, CustomTask::CORE_1), _blinker(NULL), _pin(pin), _level(level) {}
  Blinker &getInstance() {return *_blinker;}
  void Demo();

protected:
  void setup();
  void loop();
  void cleanup();

  struct __attribute__((__packed__)) {
    Blinker *_blinker;
    uint8_t _pin : 7;
    bool _level : 1;
  };
};
#endif