#include <esp_log.h>
#include "CustomTask.h"

static const char TAG[] = "CustomTask";

CustomTask *customTask = NULL;

void CustomTask::halt(const char *msg) {
  if (customTask)
    delete customTask;
  Serial.println(msg);
  Serial.flush();
  esp_deep_sleep_start();
}

CustomTask::CustomTask(const char *name, uint32_t stack, uint8_t priority, core_t core) {
  BaseType_t result;

  _flags = xEventGroupCreate();
  if (! _flags) {
    _task = NULL;
    ESP_LOGE(TAG, "Error creating RTOS event group!\r\n");
    return;
  }
  if (core < CORE_ANY)
    result = xTaskCreatePinnedToCore((TaskFunction_t)&CustomTask::taskHandler, name, stack, this, priority, &_task, core);
  else
    result = xTaskCreate((TaskFunction_t)&CustomTask::taskHandler, name, stack, this, priority, &_task);
  if (result != pdPASS) {
    _task = NULL;
    vEventGroupDelete(_flags);
    ESP_LOGE(TAG, "Error creating RTOS CustomTask!\r\n");
  }
}

bool CustomTask::isRunning() {
  if (_task) {
    return eTaskGetState(_task) == eRunning;
  }
  return false;
}

void CustomTask::pause() {
  if (_task) {
    vTaskSuspend(_task);
  }
}

void CustomTask::resume() {
  if (_task) {
    vTaskResume(_task);
  }
}

void CustomTask::destroy() {
  if (_task) {
    xEventGroupSetBits(_flags, FLAG_DESTROYING);
    xEventGroupWaitBits(_flags, FLAG_DESTROYED, pdFALSE, pdTRUE, portMAX_DELAY);
    vEventGroupDelete(_flags);
    _task = NULL;
  }
}

void CustomTask::notify(uint32_t value) {
  if (_task) {
    xTaskNotify(_task, value, eSetValueWithOverwrite);
  }
}

void CustomTask::lock() {
  portENTER_CRITICAL(&_mutex);
}

void CustomTask::unlock() {
  portEXIT_CRITICAL(&_mutex);
}

void CustomTask::taskHandler() {
  setup();
  while (! (xEventGroupWaitBits(_flags, FLAG_DESTROYING, pdFALSE, pdTRUE, 0) & FLAG_DESTROYING)) {
    loop();
  }
  cleanup();
  xEventGroupSetBits(_flags, FLAG_DESTROYED);
  vTaskDelete(_task);
}

portMUX_TYPE CustomTask::_mutex = portMUX_INITIALIZER_UNLOCKED;
