#include "../shared.h"
#include "esp_camera.h"
#include "esp_http_server.h" 

// Pin Map for AI-Thinker
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

bool is_capturing_snapshot = false;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  char * part_buf[64];

  // This header tells Blynk: "Incoming flip-book of JPEGs"
  httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);

  while(true){
    // Safety check for the PIR trigger
    if(is_capturing_snapshot) { delay(100); continue; }

    fb = esp_camera_fb_get();
    if (!fb) { 
      Serial.println("Camera capture failed");
      res = ESP_FAIL; 
    } else {
      // 1. Send Boundary 
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
      
      // 2. Send Image Header
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, fb->len);
      if(res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
      
      // 3. Send Image Data
      if(res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
      
      esp_camera_fb_return(fb);
    }
    if(res != ESP_OK) break;
  }
  return res;
}

static esp_err_t capture_handler(httpd_req_t *req){
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Set the response type to image/jpeg so the phone knows it's a photo
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");

    esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    
    // Always return the frame buffer to avoid memory leaks!
    esp_camera_fb_return(fb); 
    return res;
}

void startCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80; 
  config.ctrl_port = 32768; 

  httpd_handle_t stream_httpd = NULL;

  // 1. The Video Stream URI (Existing)
  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  // 2. The Mugshot Capture URI (NEW)
  httpd_uri_t capture_uri = {
    .uri       = "/capture",
    .method    = HTTP_GET,
    .handler   = capture_handler, // We need to define this function too!
    .user_ctx  = NULL
  };

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &index_uri);
    httpd_register_uri_handler(stream_httpd, &capture_uri); // Register the new path
    Serial.println("Web Server Started with Mugshot support!");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(4, OUTPUT); // Flash LED initialization
  digitalWrite(4, LOW);

  //connect to wifi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected!");
  
  Blynk.begin(BLYNK_AUTH_TOKEN1, ssid, pass);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM; config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM; config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM; config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM; config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA; 
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err == ESP_OK) {
    startCameraServer();
    Serial.println("CAMERA READY!");
    Blynk.virtualWrite(V5, "http://" + WiFi.localIP().toString() + "/");
  } else {
    Serial.printf("Camera init failed: 0x%x", err);
  }
}

void loop() { 
    Blynk.run();
 }

 BLYNK_WRITE(V6) {
  if (param.asInt() == 1) {
    is_capturing_snapshot = true; 
    Serial.println("PIR Motion: Capturing High-Res...");

    sensor_t * s = esp_camera_sensor_get();
    s->set_framesize(s, FRAMESIZE_VGA); 
    
    delay(500); // Give the sensor time to adjust to light
    digitalWrite(4, HIGH); // Flash ON
    camera_fb_t * fb = esp_camera_fb_get();
    digitalWrite(4, LOW);  // Flash OFF
    
    if (fb) {
  String mugshotURL = "http://" + WiFi.localIP().toString() + "/capture?t=" + String(millis());
  
  // 1. Tell the Gallery which image 'index' to show (e.g., the 1st one)
  Blynk.virtualWrite(V7, 1); 
  
  // 2. The "Property Bridge": This injects the URL into that index
  Blynk.setProperty(V7, "urls", mugshotURL); 
  
  Serial.println("Mugshot URL injected into V7!");
  esp_camera_fb_return(fb);
}
    
    s->set_framesize(s, FRAMESIZE_QVGA); // Return to stream size
    is_capturing_snapshot = false;       // Resume stream
  }
}