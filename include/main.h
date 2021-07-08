#ifndef __MAIN_H
#define __MAIN_H

#include <FS.h>
#ifdef USE_LittleFS
  #define SPIFFS LITTLEFS
  #include <LITTLEFS.h> 
#else
  #include <SPIFFS.h>
#endif 

#include <arduino.h>

/*

  This is a simple MJPEG streaming webserver implemented for AI-Thinker ESP32-CAM
  and ESP-EYE modules.
  This is tested to work with VLC and Blynk video widget and can support up to 10
  simultaneously connected streaming clients.
  Simultaneous streaming is implemented with dedicated FreeRTOS tasks.

  Inspired by and based on this Instructable: $9 RTSP Video Streamer Using the ESP32-CAM Board
  (https://www.instructables.com/id/9-RTSP-Video-Streamer-Using-the-ESP32-CAM-Board/)

  Board: AI-Thinker ESP32-CAM or ESP-EYE
  Compile as:
   ESP32 Dev Module
   CPU Freq: 240
   Flash Freq: 80
   Flash mode: QIO
   Flash Size: 4Mb
   Partrition: Minimal SPIFFS
   PSRAM: Enabled
*/

// ESP32 has two cores: APPlication core and PROcess core (the one that runs ESP32 SDK stack)
#define APP_CPU 1
#define PRO_CPU 0

#include "esp_camera.h"
#include "ov2640.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>

#include <esp_bt.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER

#define MAX_CLIENTS   10

#include "camera_pins.h"

#define USE_FTP

#include "Blinker.h"
#include "BlinkerTask.h"
#include "CustomTask.h"
#include <ArduinoJson.h>
#include <string.h>
#include "EmbUI.h"
#ifdef USE_FTP
#include "ftpSrv.h"
#endif
// We will try to achieve 24 FPS frame rate
const int FPS = 24;

// We will handle web client requests every 100 ms (10 Hz)
const int WSINTERVAL = 100;

//OV2640 cam;

void camCB(void* pvParameters);
void streamCB(void * pvParameters);
void handleJPGSstream(AsyncWebServerRequest *request);
void handleJPG(AsyncWebServerRequest *request);
void streamJpg(AsyncWebServerRequest *request);
void handleNotFound(AsyncWebServerRequest *request);
char* allocateMemory(char* aPtr, size_t aSize);
void mjpegCB(void* pvParameters);
void sendBMP(AsyncWebServerRequest *request);
void sendJpg(AsyncWebServerRequest *request);
void setCameraVar(AsyncWebServerRequest *request);
void getCameraStatus(AsyncWebServerRequest *request);

void create_parameters();
void sync_parameters();

extern SemaphoreHandle_t frameSync;
extern WebServer server;
extern TaskHandle_t tMjpeg;
extern TaskHandle_t tCam;
extern uint8_t       noActiveClients;
extern BlinkerTask *btask;

#define LED_PIN GPIO_NUM_4
#define LED_LEVEL HIGH

#endif