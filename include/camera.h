#ifndef _CAMERA_H
#define _CAMERA_H

#include "config.h"
#include "EmbUI.h"
#include "main.h"

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

enum {
  FRAMESIZE, //0 - 10
  SCALE, //bool
  BINNING, //bool
  QUALITY,//0 - 63
  BRIGHTNESS,//-2 - 2
  CONTRAST,//-2 - 2
  SATURATION,//-2 - 2
  SHARPNESS,//-2 - 2
  DENOISE,
  SPECIAL_EFFECT,//0 - 6
  WB_MODE,//0 - 4
  AWB,
  AWB_GAIN,
  AEC,
  AEC2,
  AE_LEVEL,//-2 - 2
  AEC_VALUE,//0 - 1200
  AGC,
  AGC_GAIN,//0 - 30
  GAINCEILING,//0 - 6
  BPC,
  WPC,
  RAW_GMA,
  LENC,
  HMIRROR,
  VFLIP,
  DCW,
  COLORBAR
};

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
      .fb_count       = 8, // 2
      .fb_location    = CAMERA_FB_IN_PSRAM, /*!< The location where the frame buffer will be allocated */
      .grab_mode      = CAMERA_GRAB_LATEST  /*!< When buffers should be filled */
    };

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // // https://github.com/espressif/esp32-camera/issues/370#issuecomment-1070791435
  // digitalWrite(PWDN_GPIO_NUM, LOW);
  // delay(10);
  // digitalWrite(PWDN_GPIO_NUM, HIGH);
  // delay(10);
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
  static void setCameraVar(AsyncWebServerRequest *request);

  static void sendJpg(AsyncWebServerRequest *request);
  static void streamJpg(AsyncWebServerRequest *request);

  static void sendJpg2(AsyncWebServerRequest *request);
  static void streamJpg2(AsyncWebServerRequest *request);

  static void sendBMP(AsyncWebServerRequest *request);
  static void handleNotFound(AsyncWebServerRequest *request);

  static void setParam(uint8_t param, String val){
    sensor_t * s = esp_camera_sensor_get();
    if(s == NULL){
        return;
    }

    switch (param)
    {
    case FRAMESIZE:
      s->set_framesize(s, (framesize_t)val.toInt());
      break;

    case QUALITY:
      s->set_quality(s, val.toInt());
      break;


    case BRIGHTNESS:
      s->set_brightness(s, val.toInt());
      break;


    case CONTRAST:
      s->set_contrast(s, val.toInt());
      break;


    case SATURATION:
      s->set_saturation(s, val.toInt());
      break;


    case SHARPNESS:
      s->set_sharpness(s, val.toInt());
      break;


    case DENOISE:
      s->set_denoise(s, val.toInt());
      break;


    case SPECIAL_EFFECT:
      s->set_special_effect(s, val.toInt());
      break;


    case WB_MODE:
      s->set_wb_mode(s, val.toInt());
      break;


    case AWB:
      s->set_whitebal(s, val.toInt());
      break;


    case AWB_GAIN:
      s->set_awb_gain(s, val.toInt());
      break;


    case AEC:
      s->set_exposure_ctrl(s, val.toInt());
      break;


    case AEC2:
      s->set_aec2(s, val.toInt());
      break;


    case AE_LEVEL:
      s->set_ae_level(s, val.toInt());
      break;


    case AEC_VALUE:
      s->set_aec_value(s, val.toInt());
      break;


    case AGC:
      s->set_gain_ctrl(s, val.toInt());
      break;


    case AGC_GAIN:
      s->set_agc_gain(s, val.toInt());
      break;


    case GAINCEILING:
      s->set_gainceiling(s, (gainceiling_t)val.toInt());
      break;


    case BPC:
      s->set_bpc(s, val.toInt());
      break;


    case WPC:
      s->set_wpc(s, val.toInt());
      break;

    case RAW_GMA:
      s->set_raw_gma(s, val.toInt());
      break;

    case LENC:
      s->set_lenc(s, val.toInt());
      break;

    case HMIRROR:
      s->set_hmirror(s, val.toInt());
      break;

    case VFLIP:
      s->set_vflip(s, val.toInt());
      break;

    case DCW:
      s->set_dcw(s, val.toInt());
      break;

    case COLORBAR:
      s->set_colorbar(s, val.toInt());
      break;
    
    default:
      break;
    }
  }

static String getParam(uint8_t param){
    sensor_t * s = esp_camera_sensor_get();
    String val = "";
    if(s == NULL){
        return val;
    }

    switch (param)
    {
    case FRAMESIZE:
      val = String(s->status.framesize);
      break;

    case QUALITY:
      val = String(s->status.quality);
      break;


    case BRIGHTNESS:
      val = String(s->status.brightness);
      break;


    case CONTRAST:
      val = String(s->status.contrast);
      break;


    case SATURATION:
      val = String(s->status.saturation);
      break;


    case SHARPNESS:
      val = String(s->status.sharpness);
      break;


    case DENOISE:
      val = String(s->status.denoise);
      break;


    case SPECIAL_EFFECT:
      val = String(s->status.special_effect);
      break;


    case WB_MODE:
      val = String(s->status.wb_mode);
      break;


    case AWB:
      val = String(s->status.awb);
      break;


    case AWB_GAIN:
      val = String(s->status.awb_gain);
      break;


    case AEC:
      val = String(s->status.aec);
      break;


    case AEC2:
      val = String(s->status.aec2);
      break;


    case AE_LEVEL:
      val = String(s->status.ae_level);
      break;


    case AEC_VALUE:
      val = String(s->status.aec_value);
      break;


    case AGC:
      val = String(s->status.agc);
      break;


    case AGC_GAIN:
      val = String(s->status.agc_gain);
      break;


    case GAINCEILING:
      val = String(s->status.gainceiling);
      break;


    case BPC:
      val = String(s->status.bpc);
      break;


    case WPC:
      val = String(s->status.wpc);
      break;

    case RAW_GMA:
      val = String(s->status.raw_gma);
      break;

    case LENC:
      val = String(s->status.lenc);
      break;

    case HMIRROR:
      val = String(s->status.hmirror);
      break;

    case VFLIP:
      val = String(s->status.vflip);
      break;

    case DCW:
      val = String(s->status.dcw);
      break;

    case COLORBAR:
      val = String(s->status.colorbar);
      break;
    
    default:
      break;
    }
  return val;
  }
};

#endif