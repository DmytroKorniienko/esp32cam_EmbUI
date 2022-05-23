#include "main.h"
#include "uistrings.h"
// https://gist.github.com/me-no-dev/d34fba51a8f059ac559bf62002e61aa3
typedef struct {
        camera_fb_t * fb;
        size_t index;
} camera_frame_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART = "Content-Type: %s\r\nContent-Length: %u\r\n\r\n";

static const char * JPG_CONTENT_TYPE = "image/jpeg";
static const char * BMP_CONTENT_TYPE = "image/x-windows-bmp";

class AsyncBufferResponse: public AsyncAbstractResponse {
    private:
        uint8_t * _buf;
        size_t _len;
        size_t _index;
    public:
        AsyncBufferResponse(uint8_t * buf, size_t len, const char * contentType){
            _buf = buf;
            _len = len;
            _callback = nullptr;
            _code = 200;
            _contentLength = _len;
            _contentType = contentType;
            _index = 0;
        }
        ~AsyncBufferResponse(){
            if(_buf != nullptr){
                free(_buf);
            }
        }
        bool _sourceValid() const { return _buf != nullptr; }
        virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override{
            size_t ret = _content(buf, maxLen, _index);
            if(ret != RESPONSE_TRY_AGAIN){
                _index += ret;
            }
            return ret;
        }
        size_t _content(uint8_t *buffer, size_t maxLen, size_t index){
            memcpy(buffer, _buf+index, maxLen);
            if((index+maxLen) == _len){
                free(_buf);
                _buf = nullptr;
            }
            return maxLen;
        }
};

class AsyncFrameResponse: public AsyncAbstractResponse {
    private:
        camera_fb_t * fb;
        size_t _index;
    public:
        AsyncFrameResponse(camera_fb_t * frame, const char * contentType){
            _callback = nullptr;
            _code = 200;
            _contentLength = frame->len;
            _contentType = contentType;
            _index = 0;
            fb = frame;
        }
        ~AsyncFrameResponse(){
            if(fb != nullptr){
                esp_camera_fb_return(fb);
            }
        }
        bool _sourceValid() const { return fb != nullptr; }
        virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override{
            size_t ret = _content(buf, maxLen, _index);
            if(ret != RESPONSE_TRY_AGAIN){
                _index += ret;
            }
            return ret;
        }
        size_t _content(uint8_t *buffer, size_t maxLen, size_t index){
            memcpy(buffer, fb->buf+index, maxLen);
            if((index+maxLen) == fb->len){
                esp_camera_fb_return(fb);
                fb = nullptr;
            }
            return maxLen;
        }
};

class AsyncJpegStreamResponse: public AsyncAbstractResponse {
    private:
        camera_frame_t _frame;
        size_t _index;
        size_t _jpg_buf_len;
        uint8_t * _jpg_buf;
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO        
        uint64_t lastAsyncRequest = 0;
        float fps = 0.0;
        uint8_t frameCnt = 0;
        uint64_t frame_ms_1s = 0;
#endif
    public:
        AsyncJpegStreamResponse(){
            _callback = nullptr;
            _code = 200;
            _contentLength = 0;
            _contentType = STREAM_CONTENT_TYPE;
            _sendContentLength = false;
            _chunked = true;
            _index = 0;
            _jpg_buf_len = 0;
            _jpg_buf = NULL;
            memset(&_frame, 0, sizeof(camera_frame_t));
        }
        ~AsyncJpegStreamResponse(){
            if(_frame.fb){
                if(_frame.fb->format != PIXFORMAT_JPEG){
                    free(_jpg_buf);
                }
                esp_camera_fb_return(_frame.fb);
            }
            log_i("Stream closed");
        }
        bool _sourceValid() const {
            return true;
        }
        virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override {
            size_t ret = _content(buf, maxLen, _index);
            if(ret != RESPONSE_TRY_AGAIN){
                _index += ret;
            }
            return ret;
        }
        size_t _content(uint8_t *buffer, size_t maxLen, size_t index){
            camera->setLedOffAfterMS(5000); // 5 sec delayed off
            if(!_frame.fb || _frame.index == _jpg_buf_len){
                if(index && _frame.fb){
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
                    uint64_t end = (uint64_t)micros();
                    frameCnt++;
                    if(frame_ms_1s+1000<end/1000){
                        fps = (fps + (float)frameCnt / (0.001*(0.001*end - frame_ms_1s))) / 2.0;
                        frame_ms_1s = end/1000; // ms
                        frameCnt = 0;
                        log_i("Size: %uKB, Time: %lums (%.1ffps)\n", _jpg_buf_len/1024, (unsigned long)((end - lastAsyncRequest) / 1000), fps); // ~1 sec period
                    }
                    lastAsyncRequest = end;
#endif
                    if(_frame.fb->format != PIXFORMAT_JPEG){
                        free(_jpg_buf);
                    }
                    esp_camera_fb_return(_frame.fb);
                    _frame.fb = NULL;
                    _jpg_buf_len = 0;
                    _jpg_buf = NULL;
                }
                if(maxLen < (strlen(STREAM_BOUNDARY) + strlen(STREAM_PART) + strlen(JPG_CONTENT_TYPE) + 8)){
                    //log_w("Not enough space for headers");
                    return RESPONSE_TRY_AGAIN;
                }
                //get frame
                _frame.index = 0;

                _frame.fb = esp_camera_fb_get();
                if (_frame.fb == NULL) {
                    log_e("Camera frame failed");
                    return 0;
                }

                if(_frame.fb->format != PIXFORMAT_JPEG){
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
                    unsigned long st = millis();
#endif
                    bool jpeg_converted = frame2jpg(_frame.fb, 80, &_jpg_buf, &_jpg_buf_len);
                    if(!jpeg_converted){
                        log_e("JPEG compression failed");
                        esp_camera_fb_return(_frame.fb);
                        _frame.fb = NULL;
                        _jpg_buf_len = 0;
                        _jpg_buf = NULL;
                        return 0;
                    }
                    log_i("JPEG: %lums, %uB", millis() - st, _jpg_buf_len);
                } else {
                    _jpg_buf_len = _frame.fb->len;
                    _jpg_buf = _frame.fb->buf;
                }

                //send boundary
                size_t blen = 0;
                if(index){
                    blen = strlen(STREAM_BOUNDARY);
                    memcpy(buffer, STREAM_BOUNDARY, blen);
                    buffer += blen;
                }
                //send header
                size_t hlen = sprintf((char *)buffer, STREAM_PART, JPG_CONTENT_TYPE, _jpg_buf_len);
                buffer += hlen;
                //send frame
                hlen = maxLen - hlen - blen;
                if(hlen > _jpg_buf_len){
                    maxLen -= hlen - _jpg_buf_len;
                    hlen = _jpg_buf_len;
                }
                memcpy(buffer, _jpg_buf, hlen);
                _frame.index += hlen;
                return maxLen;
            }

            size_t available = _jpg_buf_len - _frame.index;
            if(maxLen > available){
                maxLen = available;
            }
            memcpy(buffer, _jpg_buf+_frame.index, maxLen);
            _frame.index += maxLen;

            return maxLen;
        }
};

void EMBUICAMERA::sendBMP(AsyncWebServerRequest *request){
    camera_fb_t * fb = esp_camera_fb_get();
    if (fb == NULL) {
        log_e("Camera frame failed");
        request->send(501);
        return;
    }

    uint8_t * buf = NULL;
    size_t buf_len = 0;
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
    unsigned long st = millis();
#endif
    bool converted = frame2bmp(fb, &buf, &buf_len);
    log_i("BMP: %lums, %uB", millis() - st, buf_len);
    esp_camera_fb_return(fb);
    if(!converted){
        request->send(501);
        return;
    }

    AsyncBufferResponse * response = new AsyncBufferResponse(buf, buf_len, BMP_CONTENT_TYPE);
    if (response == NULL) {
        log_e("Response alloc failed");
        request->send(501);
        return;
    }
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void EMBUICAMERA::sendJpg(AsyncWebServerRequest *request){
    camera_fb_t * fb = esp_camera_fb_get();
    if (fb == NULL) {
        log_e("Camera frame failed");
        request->send(501);
        return;
    }

    if(fb->format == PIXFORMAT_JPEG){
        AsyncFrameResponse * response = new AsyncFrameResponse(fb, JPG_CONTENT_TYPE);
        if (response == NULL) {
            log_e("Response alloc failed");
            request->send(501);
            return;
        }
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        return;
    }

    size_t jpg_buf_len = 0;
    uint8_t * jpg_buf = NULL;
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
    unsigned long st = millis();
#endif
    bool jpeg_converted = frame2jpg(fb, 80, &jpg_buf, &jpg_buf_len);
    esp_camera_fb_return(fb);
    if(!jpeg_converted){
        log_e("JPEG compression failed: %lu", millis());
        request->send(501);
        return;
    }
    log_i("JPEG: %lums, %uB", millis() - st, jpg_buf_len);

    AsyncBufferResponse * response = new AsyncBufferResponse(jpg_buf, jpg_buf_len, JPG_CONTENT_TYPE);
    if (response == NULL) {
        log_e("Response alloc failed");
        request->send(501);
        return;
    }
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void EMBUICAMERA::sendJpg2(AsyncWebServerRequest *request){
    camera->setLedBright(camera->getLedBright());
    vTaskDelay(1500 / portTICK_PERIOD_MS); // The LED needs to be turned on ~150ms before the call to esp_camera_fb_get()
    sendJpg(request);
    //camera->setLedOffAfterMS(200);
    camera->setLedOff();
}

void EMBUICAMERA::streamJpg(AsyncWebServerRequest *request){
    AsyncJpegStreamResponse *response = new AsyncJpegStreamResponse();
    if(!response){
        request->send(501);
        return;
    }
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void EMBUICAMERA::streamJpg2(AsyncWebServerRequest *request){
    //if(!camera->getLedBright())
    //camera->setLedOffAfterMS(5000);
    LOG(println,"Starting stream...");
    camera->setLedBright(camera->getLedBright());
    streamJpg(request);
}

void EMBUICAMERA::getCameraStatus(AsyncWebServerRequest *request){
    static char json_response[1024];

    sensor_t * s = esp_camera_sensor_get();
    if(s == NULL){
        request->send(501);
        return;
    }
    char * p = json_response;
    *p++ = '{';

    p+=sprintf(p, "\"framesize\":%u,", s->status.framesize);
    p+=sprintf(p, "\"quality\":%u,", s->status.quality);
    p+=sprintf(p, "\"brightness\":%d,", s->status.brightness);
    p+=sprintf(p, "\"contrast\":%d,", s->status.contrast);
    p+=sprintf(p, "\"saturation\":%d,", s->status.saturation);
    p+=sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
    p+=sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
    p+=sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
    p+=sprintf(p, "\"awb\":%u,", s->status.awb);
    p+=sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
    p+=sprintf(p, "\"aec\":%u,", s->status.aec);
    p+=sprintf(p, "\"aec2\":%u,", s->status.aec2);
    p+=sprintf(p, "\"denoise\":%u,", s->status.denoise);
    p+=sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
    p+=sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
    p+=sprintf(p, "\"agc\":%u,", s->status.agc);
    p+=sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
    p+=sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
    p+=sprintf(p, "\"bpc\":%u,", s->status.bpc);
    p+=sprintf(p, "\"wpc\":%u,", s->status.wpc);
    p+=sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
    p+=sprintf(p, "\"lenc\":%u,", s->status.lenc);
    p+=sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
    p+=sprintf(p, "\"vflip\":%u,", s->status.vflip);
    p+=sprintf(p, "\"dcw\":%u,", s->status.dcw);
    p+=sprintf(p, "\"colorbar\":%u", s->status.colorbar);
    *p++ = '}';
    *p++ = 0;

    AsyncWebServerResponse * response = request->beginResponse(200, "application/json", json_response);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void EMBUICAMERA::setCameraVar(AsyncWebServerRequest *request){
    if(!request->hasArg("var") || !request->hasArg("val")){
        request->send(404);
        return;
    }
    String var = request->arg("var");
    const char * variable = var.c_str();
    int val = atoi(request->arg("val").c_str());

    sensor_t * s = esp_camera_sensor_get();
    if(s == NULL){
        request->send(501);
        return;
    }

    int res = 0;
    if(!strcmp(variable, PSTR(T_FRAMESIZE))) res = s->set_framesize(s, (framesize_t)val);
    else if(!strcmp(variable, PSTR(T_QUALITY))) res = s->set_quality(s, val);
    else if(!strcmp(variable, PSTR(T_CONTRAST))) res = s->set_contrast(s, val);
    else if(!strcmp(variable, PSTR(T_BRIGHTNESS))) res = s->set_brightness(s, val);
    else if(!strcmp(variable, PSTR(T_SATURATION))) res = s->set_saturation(s, val);
    else if(!strcmp(variable, PSTR(T_SHARPNESS))) res = s->set_sharpness(s, val);
    else if(!strcmp(variable, PSTR(T_GAINCEILING))) res = s->set_gainceiling(s, (gainceiling_t)val);
    else if(!strcmp(variable, PSTR(T_COLORBAR))) res = s->set_colorbar(s, val);
    else if(!strcmp(variable, PSTR(T_AWB))) res = s->set_whitebal(s, val);
    else if(!strcmp(variable, PSTR(T_AGC))) res = s->set_gain_ctrl(s, val);
    else if(!strcmp(variable, PSTR(T_AEC))) res = s->set_exposure_ctrl(s, val);
    else if(!strcmp(variable, PSTR(T_HMIRROR))) res = s->set_hmirror(s, val);
    else if(!strcmp(variable, PSTR(T_VFLIP))) res = s->set_vflip(s, val);
    else if(!strcmp(variable, PSTR(T_AWB_GAIN))) res = s->set_awb_gain(s, val);
    else if(!strcmp(variable, PSTR(T_AGC_GAIN))) res = s->set_agc_gain(s, val);
    else if(!strcmp(variable, PSTR(T_AEC_VALUE))) res = s->set_aec_value(s, val);
    else if(!strcmp(variable, PSTR(T_AEC2))) res = s->set_aec2(s, val);
    else if(!strcmp(variable, PSTR(T_DENOISE))) res = s->set_denoise(s, val);
    else if(!strcmp(variable, PSTR(T_DCW))) res = s->set_dcw(s, val);
    else if(!strcmp(variable, PSTR(T_BPC))) res = s->set_bpc(s, val);
    else if(!strcmp(variable, PSTR(T_WPC))) res = s->set_wpc(s, val);
    else if(!strcmp(variable, PSTR(T_RAW_GMA))) res = s->set_raw_gma(s, val);
    else if(!strcmp(variable, PSTR(T_LENC))) res = s->set_lenc(s, val);
    else if(!strcmp(variable, PSTR(T_SPECIAL_EFFECT))) res = s->set_special_effect(s, val);
    else if(!strcmp(variable, PSTR(T_WB_MODE))) res = s->set_wb_mode(s, val);
    else if(!strcmp(variable, PSTR(T_AE_LEVEL))) res = s->set_ae_level(s, val);
    else {
        log_e("unknown setting %s", var.c_str());
        request->send(404);
        return;
    }
    log_d("Got setting %s with value %d. Res: %d", var.c_str(), val, res);
    res = res;

    AsyncWebServerResponse * response = request->beginResponse(200);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

// ==== Handle invalid URL requests ============================================
void EMBUICAMERA::handleNotFound(AsyncWebServerRequest *request)
{
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += request->args();
  message += "\n";
  request->send(200, "text / plain", message);
}

// deprecated ------------------------------------------------------------------

/*
const char JHEADER[] = "HTTP/1.1 200 OK\r\n" \
                       "Content-disposition: inline; filename=capture.jpg\r\n" \
                       "Content-type: image/jpeg\r\n\r\n";
const int jhdLen = strlen(JHEADER);

// ======== Server Connection Handler Task ==========================
void mjpegCB(void* pvParameters) {
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(WSINTERVAL);

  // Creating frame synchronization semaphore and initializing it
  frameSync = xSemaphoreCreateBinary();
  xSemaphoreGive( frameSync );

  //=== setup section  ==================

  //  Creating RTOS task for grabbing frames from the camera
  xTaskCreatePinnedToCore(
    camCB,        // callback
    "cam",        // name
    4 * 1024,       // stacj size
    NULL,         // parameters
    2,            // priority
    &tCam,        // RTOS task handle
    PRO_CPU);     // core

  noActiveClients = 0;

  Serial.printf("\nmjpegCB: free heap (start)  : %d\n", ESP.getFreeHeap());
  //=== loop() section  ===================
  xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    //server.handleClient();

    //  After every server client handling request, we let other tasks run and then pause
    taskYIELD();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}


// Current frame information
volatile uint32_t frameNumber;
volatile size_t   camSize;    // size of the current frame, byte
volatile char*    camBuf;      // pointer to the current frame


// ==== RTOS task to grab frames from the camera =========================
void camCB(void* pvParameters) {

  TickType_t xLastWakeTime;

  //  A running interval associated with currently desired frame rate
  const TickType_t xFrequency = pdMS_TO_TICKS(1000 / FPS);

  //  Pointers to the 2 frames, their respective sizes and index of the current frame
  char* fbs[2] = { NULL, NULL };
  size_t fSize[2] = { 0, 0 };
  int ifb = 0;
  frameNumber = 0;

  //=== loop() section  ===================
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {

    //  Grab a frame from the camera and query its size
    camera_fb_t* fb = NULL;

    fb = esp_camera_fb_get();
    size_t s = fb->len;

    //  If frame size is more that we have previously allocated - request  125% of the current frame space
    if (s > fSize[ifb]) {
      fSize[ifb] = s + s;
      fbs[ifb] = allocateMemory(fbs[ifb], fSize[ifb]);
    }

    //  Copy current frame into local buffer
    char* b = (char *)fb->buf;
    memcpy(fbs[ifb], b, s);
    esp_camera_fb_return(fb);

    //  Let other tasks run and wait until the end of the current frame rate interval (if any time left)
    taskYIELD();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    //  Only switch frames around if no frame is currently being streamed to a client
    //  Wait on a semaphore until client operation completes
    //    xSemaphoreTake( frameSync, portMAX_DELAY );

    //  Do not allow frame copying while switching the current frame
    xSemaphoreTake( frameSync, xFrequency );
    camBuf = fbs[ifb];
    camSize = s;
    ifb++;
    ifb &= 1;  // this should produce 1, 0, 1, 0, 1 ... sequence
    frameNumber++;
    //  Let anyone waiting for a frame know that the frame is ready
    xSemaphoreGive( frameSync );

    //  Immediately let other (streaming) tasks run
    taskYIELD();

    //  If streaming task has suspended itself (no active clients to stream to)
    //  there is no need to grab frames from the camera. We can save some juice
    //  by suspedning the tasks
    if ( noActiveClients == 0 ) {
      Serial.printf("mjpegCB: free heap           : %d\n", ESP.getFreeHeap());
      Serial.printf("mjpegCB: min free heap)      : %d\n", ESP.getMinFreeHeap());
      Serial.printf("mjpegCB: max alloc free heap : %d\n", ESP.getMaxAllocHeap());
      Serial.printf("mjpegCB: tCam stack wtrmark  : %d\n", uxTaskGetStackHighWaterMark(tCam));
      Serial.flush();
      vTaskSuspend(NULL);  // passing NULL means "suspend yourself"
    }
  }
}

// ==== Memory allocator that takes advantage of PSRAM if present =======================
char* allocateMemory(char* aPtr, size_t aSize) {

  //  Since current buffer is too smal, free it
  if (aPtr != NULL) free(aPtr);

  char* ptr = NULL;
  ptr = (char*) ps_malloc(aSize);

  // If the memory pointer is NULL, we were not able to allocate any memory, and that is a terminal condition.
  if (ptr == NULL) {
    Serial.println("Out of memory!");
    delay(5000);
    ESP.restart();
  }
  return ptr;
}


// ==== STREAMING ======================================================
const char HEADER[] = "HTTP/1.1 200 OK\r\n" \
                      "Access-Control-Allow-Origin: *\r\n" \
                      "Content-Type: multipart/x-mixed-replace; boundary=123456789000000000000987654321\r\n";
const char BOUNDARY[] = "\r\n--123456789000000000000987654321\r\n";
const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";
const int hdrLen = strlen(HEADER);
const int bdrLen = strlen(BOUNDARY);
const int cntLen = strlen(CTNTTYPE);


struct streamInfo {
  uint32_t        frame;
  AsyncClient    *client;
  TaskHandle_t    task;
  char*           buffer;
  size_t          len;
};

// ==== Handle connection request from clients ===============================
void handleJPGSstream(AsyncWebServerRequest *request)
{
  if ( noActiveClients >= MAX_CLIENTS ) return;
  Serial.printf("handleJPGSstream start: free heap  : %d\n", ESP.getFreeHeap());

  streamInfo* info = new streamInfo;

  info->frame = frameNumber - 1;
  info->client = request->client();
  info->buffer = NULL;
  info->len = 0;

  //  Creating task to push the stream to all connected clients
  int rc = xTaskCreatePinnedToCore(
             streamCB,
             "strmCB",
             3 * 1024,
             (void*) info,
             2,
             &info->task,
             APP_CPU);
  if ( rc != pdPASS ) {
    Serial.printf("handleJPGSstream: error creating RTOS task. rc = %d\n", rc);
    Serial.printf("handleJPGSstream: free heap  : %d\n", ESP.getFreeHeap());
    //    Serial.printf("stk high wm: %d\n", uxTaskGetStackHighWaterMark(tSend));
    delete info;
  }

  noActiveClients++;

  // Wake up streaming tasks, if they were previously suspended:
  if ( eTaskGetState( tCam ) == eSuspended ) vTaskResume( tCam );
}


// ==== Actually stream content to all connected clients ========================
void streamCB(void * pvParameters) {
  char buf[16];
  TickType_t xLastWakeTime;
  TickType_t xFrequency;

  streamInfo* info = (streamInfo*) pvParameters;

  if ( info == NULL ) {
    Serial.println("streamCB: a NULL pointer passed");
  }
  //  Immediately send this client a header
  info->client->write(HEADER, hdrLen);
  info->client->write(BOUNDARY, bdrLen);
  taskYIELD();

  xLastWakeTime = xTaskGetTickCount();
  xFrequency = pdMS_TO_TICKS(1000 / FPS);

  for (;;) {
    //  Only bother to send anything if there is someone watching
    if ( info->client->connected() ) {

      if ( info->frame != frameNumber) {
        xSemaphoreTake( frameSync, portMAX_DELAY );
        if ( info->buffer == NULL ) {
          info->buffer = allocateMemory (info->buffer, camSize);
          info->len = camSize;
        }
        else {
          if ( camSize > info->len ) {
            info->buffer = allocateMemory (info->buffer, camSize);
            info->len = camSize;
          }
        }
        memcpy(info->buffer, (const void*) camBuf, info->len);
        xSemaphoreGive( frameSync );
        taskYIELD();

        info->frame = frameNumber;
        info->client->write(CTNTTYPE, cntLen);
        sprintf(buf, "%d\r\n\r\n", info->len);
        info->client->write(buf, strlen(buf));
        info->client->write((char*) info->buffer, (size_t)info->len);
        info->client->write(BOUNDARY, bdrLen);
        info->client->send();
        //info->client->flush();
      }
    }
    else {
      //  client disconnected - clean up.
      noActiveClients--;
      Serial.printf("streamCB: Stream Task stack wtrmark  : %d\n", uxTaskGetStackHighWaterMark(info->task));
      Serial.flush();
      //info->client->flush();
      info->client->stop();
      if ( info->buffer ) {
        free( info->buffer );
        info->buffer = NULL;
      }
      delete info;
      info = NULL;
      vTaskDelete(NULL);
    }
    //  Let other tasks run after serving every client
    taskYIELD();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}
*/
