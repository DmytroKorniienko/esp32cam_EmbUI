#include "main.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

EMBUICAMERA *camera;
EmbUI *embui = nullptr;

// ==== SETUP method ==================================================================
void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  // Setup Serial connection:
  Serial.begin(460800); // 115200
  //delay(1000); // wait for a second to let Serial connect
  Serial.printf("\n\nsetup: free heap  : %d\n", ESP.getFreeHeap());
  Serial.printf("setup: free PSRAM  : %d\n", ESP.getFreePsram());

  embui = EmbUI::GetInstance(); // create embui instance
  embui->begin(); // Инициализируем EmbUI фреймворк.

  camera = new EMBUICAMERA();

  //  Registering webserver handling routines
  embui->server.onNotFound(EMBUICAMERA::handleNotFound);

  embui->server.on("/bmp", HTTP_GET, EMBUICAMERA::sendBMP);

  embui->server.on("/jpg", HTTP_GET, EMBUICAMERA::sendJpg);
  embui->server.on("/stream", HTTP_GET, EMBUICAMERA::streamJpg);

  embui->server.on("/capture", HTTP_GET, EMBUICAMERA::sendJpg2);
  embui->server.on("/mjpeg/1", HTTP_GET, EMBUICAMERA::streamJpg2);

  embui->server.on("/control", HTTP_GET, EMBUICAMERA::setCameraVar);
  embui->server.on("/status", HTTP_GET, EMBUICAMERA::getCameraStatus);

  Serial.printf("setup complete: free heap  : %d\n", ESP.getFreeHeap());
}

void loop() {
  embui->handle(); // цикл, необходимый фреймворку (временно)
  //LOG(printf_P,"IsDirty()=%d\n", embui->timeProcessor.isDirtyTime());
  vTaskDelay(100);
}
