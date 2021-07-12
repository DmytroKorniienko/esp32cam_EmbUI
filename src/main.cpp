#include "main.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ===== rtos task handles =========================
// Streaming is implemented with 3 tasks:
TaskHandle_t tMjpeg;   // handles client connections to the webserver
TaskHandle_t tCam;     // handles getting picture frames from the camera and storing them locally
uint8_t       noActiveClients;       // number of active clients
// frameSync semaphore is used to prevent streaming buffer as it is replaced with the next frame
SemaphoreHandle_t frameSync = NULL;
BlinkerTask *btask;

// ==== SETUP method ==================================================================
void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  // Setup Serial connection:
  Serial.begin(460800);
  //delay(1000); // wait for a second to let Serial connect
  Serial.printf("setup: free heap  : %d\n", ESP.getFreeHeap());

  //create_parameters(); // создаем дефолтные параметры, отсутствующие в текущем загруженном конфиге
#ifdef USE_FTP
  ftp_setup(); // запуск ftp-сервера
#endif
  embui.begin(); // Инициализируем EmbUI фреймворк.

#if defined(CAMERA_MODEL_AI_THINKER)
  btask = new BlinkerTask(LED_PIN, LED_LEVEL);
  if ((! btask) || (! *btask))
    BlinkerTask::halt("Error initializing blinker task!");
#endif

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

    .xclk_freq_hz   = 20000000,
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
    .fb_count       = 2
  };

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  //  Registering webserver handling routines
  //embui.server.on("/mjpeg/1", HTTP_GET, handleJPGSstream);
  embui.server.on("/mjpeg/1", HTTP_GET, streamJpg);
  embui.server.on("/jpg", HTTP_GET, handleJPG);
  embui.server.onNotFound(handleNotFound);

  embui.server.on("/bmp", HTTP_GET, sendBMP);
  embui.server.on("/capture", HTTP_GET, sendJpg);
  embui.server.on("/stream", HTTP_GET, streamJpg);
  embui.server.on("/control", HTTP_GET, setCameraVar);
  embui.server.on("/status", HTTP_GET, getCameraStatus);

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

  Serial.printf("setup complete: free heap  : %d\n", ESP.getFreeHeap());

  // sensor_t * s = esp_camera_sensor_get();
  // if(s){
  //   s->set_framesize(s, FRAMESIZE_HVGA);
  //   s->set_quality(s,8);
  // }
}

void loop() {
  embui.handle(); // цикл, необходимый фреймворку (временно)
#ifdef USE_FTP
  ftp_loop(); // цикл обработки событий фтп-сервера
#endif
  vTaskDelay(100);
}
