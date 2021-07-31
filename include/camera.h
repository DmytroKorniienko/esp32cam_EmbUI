#ifndef _CAMERA_H
#define _CAMERA_H

#include "config.h"
#include "EmbUI.h"

#include <esp_bt.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include "esp_camera.h"
#include "camera_pins.h"

#include "Blinker.h"
#include "CustomTask.h"

// extern SemaphoreHandle_t frameSync;
// extern WebServer server;
// extern TaskHandle_t tMjpeg;
// extern TaskHandle_t tCam;
// extern uint8_t       noActiveClients;
// extern BlinkerTask *btask;

class CAMERA {
  // // ===== rtos task handles =========================
  // // Streaming is implemented with 3 tasks:
  // TaskHandle_t tMjpeg;   // handles client connections to the webserver
  // TaskHandle_t tCam;     // handles getting picture frames from the camera and storing them locally
  // uint8_t       noActiveClients;       // number of active clients
  // // frameSync semaphore is used to prevent streaming buffer as it is replaced with the next frame
  // SemaphoreHandle_t frameSync = NULL;

  // // We will try to achieve 24 FPS frame rate
  // const int FPS = 24;
  // // We will handle web client requests every 100 ms (10 Hz)
  // const int WSINTERVAL = 100;
public:
  CAMERA() {
    // Configure the camera
    //  camera_config_t config;
    //  config.ledc_channel = LEDC_CHANNEL_0;
    //  config.ledc_timer = LEDC_TIMER_0;
    //  config.pin_d0 = Y2_GPIO_NUM;
    //  config.pin_d1 = Y3_GPIO_NUM;
    //  config.pin_d2 = Y4_GPIO_NUM;
    //  config.pin_d3 = Y5_GPIO_NUM;
    //  config.pin_d4 = Y6_GPIO_NUM;
    //  config.pin_d5 = Y7_GPIO_NUM;
    //  config.pin_d6 = Y8_GPIO_NUM;
    //  config.pin_d7 = Y9_GPIO_NUM;
    //  config.pin_xclk = XCLK_GPIO_NUM;
    //  config.pin_pclk = PCLK_GPIO_NUM;
    //  config.pin_vsync = VSYNC_GPIO_NUM;
    //  config.pin_href = HREF_GPIO_NUM;
    //  config.pin_sscb_sda = SIOD_GPIO_NUM;
    //  config.pin_sscb_scl = SIOC_GPIO_NUM;
    //  config.pin_pwdn = PWDN_GPIO_NUM;
    //  config.pin_reset = RESET_GPIO_NUM;
    //  config.xclk_freq_hz = 20000000;
    //  config.pixel_format = PIXFORMAT_JPEG;
    //
    //  // Frame parameters: pick one
    //  //  config.frame_size = FRAMESIZE_UXGA;
    //  //  config.frame_size = FRAMESIZE_SVGA;
    //  //  config.frame_size = FRAMESIZE_QVGA;
    //  config.frame_size = FRAMESIZE_VGA;
    //  config.jpeg_quality = 12;
    //  config.fb_count = 2;

    static camera_config_t camera_config = {
      .pin_pwdn       = PWDN_GPIO_NUM,
      .pin_reset      = RESET_GPIO_NUM,
      .pin_xclk       = XCLK_GPIO_NUM,
      .pin_sscb_sda   = SIOD_GPIO_NUM,
      .pin_sscb_scl   = SIOC_GPIO_NUM,
      .pin_d7         = Y9_GPIO_NUM,
      .pin_d6         = Y8_GPIO_NUM,
      .pin_d5         = Y7_GPIO_NUM,
      .pin_d4         = Y6_GPIO_NUM,
      .pin_d3         = Y5_GPIO_NUM,
      .pin_d2         = Y4_GPIO_NUM,
      .pin_d1         = Y3_GPIO_NUM,
      .pin_d0         = Y2_GPIO_NUM,
      .pin_vsync      = VSYNC_GPIO_NUM,
      .pin_href       = HREF_GPIO_NUM,
      .pin_pclk       = PCLK_GPIO_NUM,

      .xclk_freq_hz   = 20000000, //16500000, // 20000000,
      .ledc_timer     = LEDC_TIMER_0,
      .ledc_channel   = LEDC_CHANNEL_0,
      .pixel_format   = PIXFORMAT_JPEG,
      /*
          FRAMESIZE_96X96,    // 96x96
          FRAMESIZE_QQVGA,    // 160x120
          FRAMESIZE_QCIF,     // 176x144
          FRAMESIZE_HQVGA,    // 240x176
          FRAMESIZE_240X240,  // 240x240
          FRAMESIZE_QVGA,     // 320x240
          FRAMESIZE_CIF,      // 400x296
          FRAMESIZE_HVGA,     // 480x320
          FRAMESIZE_VGA,      // 640x480
          FRAMESIZE_SVGA,     // 800x600
          FRAMESIZE_XGA,      // 1024x768
          FRAMESIZE_HD,       // 1280x720
          FRAMESIZE_SXGA,     // 1280x1024
          FRAMESIZE_UXGA,     // 1600x1200
      */
      //    .frame_size     = FRAMESIZE_QVGA,
      //    .frame_size     = FRAMESIZE_UXGA,
      //    .frame_size     = FRAMESIZE_SVGA,
      //.frame_size     = FRAMESIZE_VGA,
      .frame_size     = FRAMESIZE_UXGA,
      .jpeg_quality   = 16,
      .fb_count       = 16 // 2
    };

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  if (esp_camera_init(&camera_config) != ESP_OK) {
    Serial.println("Error initializing the camera");
    delay(10000);
    ESP.restart();
  }

  // Serial.println("/mjpeg/1");
  // Start mainstreaming RTOS task
  // xTaskCreatePinnedToCore(
  //   mjpegCB,
  //   "mjpeg",
  //   2*1024,
  //   NULL,
  //   2,
  //   &tMjpeg,
  //   APP_CPU);

  // sensor_t * s = esp_camera_sensor_get();
  // if(s){
  //   s->set_framesize(s, FRAMESIZE_HVGA);
  //   s->set_quality(s,8);
  // }
  }
};

class EMBUICAMERA : public CAMERA {
private:
  BlinkerTask *btask = nullptr;
public:
  EMBUICAMERA() : CAMERA() {
    #if defined(CAMERA_MODEL_AI_THINKER)
      btask = new BlinkerTask(LED_PIN, LED_LEVEL);
      if ((!btask) || (!*btask))
        BlinkerTask::halt("Error initializing blinker task!", btask);
    #endif
  }
  int8_t getLedBright(){return btask->getBright();}
  void toggleLed(){btask->toggle();}
  void setLedOff(){btask->off();}
  void setLedBright(int8_t val){btask->setBright(val);}
  void setLedOffAfterMS(uint16_t ms) {btask->setLedOffAfterMS(ms);}

  static void getCameraStatus(AsyncWebServerRequest *request);
  static void streamJpg(AsyncWebServerRequest *request);
  static void setCameraVar(AsyncWebServerRequest *request);
  static void sendJpg(AsyncWebServerRequest *request);
  static void sendBMP(AsyncWebServerRequest *request);
  static void handleNotFound(AsyncWebServerRequest *request);
};

#endif