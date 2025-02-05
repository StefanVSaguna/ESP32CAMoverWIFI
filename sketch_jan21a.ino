#define CAMERA_MODEL_ESP32S3_EYE
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "esp_camera.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "img_converters.h"
#include "Arduino.h"
#include <SPIFFS.h>
#include "camera_pins.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <HTTPClient.h>
#include <FS.h>
// Network credentials
const char* ssid = "StefanV";
const char* password = "Password";
const char* plantnet_api_key = "2b1Mg5LKuOxxxDGdqYzn7aO"; // Replace with your API Key
const char* plantnet_endpoint = "https://my-api.plantnet.org/v2/identify/all";
IPAddress staticIP(192, 168, 79, 120); // ESP32 static IP
IPAddress gateway(192, 168, 1, 1);    // IP Address of your network gateway (router)
IPAddress subnet(255, 255, 255, 0);   // Subnet mask
IPAddress primaryDNS(192, 168, 1, 1); // Primary DNS (optional)
IPAddress secondaryDNS(0, 0, 0, 0);   // Secondary DNS (optional)

AsyncWebServer server(80);
boolean takeNewPhoto = false;
#define FILE_PHOTO "/photo.jpg"

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>STEFAN CAM</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f4f4f4;
      color: #333;
      margin: 0;
      padding: 0;
      text-align: center;
    }
    header {
      background-color: #0073e6;
      color: white;
      padding: 1em 0;
    }
    h2 {
      margin: 0;
    }
    .container {
      margin: 20px auto;
      max-width: 90%;
      padding: 10px;
      background: #fff;
      border-radius: 8px;
      box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
    }
    button {
      margin: 10px;
      padding: 10px 20px;
      border: none;
      border-radius: 5px;
      background: #0073e6;
      color: white;
      font-size: 16px;
      cursor: pointer;
    }
    button:hover {
      background: #005bb5;
    }
    img {
      margin-top: 20px;
      max-width: 100%;
      height: auto;
      border-radius: 10px;
      box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
    }
     #responseMessage {
      margin-top: 20px;
      font-size: 18px;
      color: #0073e6;
    }
  </style>
</head>
<body>
  <div id="container">
    <h2>Galerie foto</h2>
    <div class="container">
    <p>Poate dura cateva secunde pentru salvarea foto</p>
    <button onclick="detectPlant()">DETECT(in progress)</button>
    <button onclick="capturePhoto()">FOTOGRAFIAZA</button>
    <button onclick="location.reload();">REINCARCARE</button>
    <img src="saved-photo" id="photo" alt="Captured Photo">
  <div id="responseMessage">Informatii depre planta:
  <p>In progres...</p>
  </div>
</body>
<script>
  var deg = 0;
  function capturePhoto() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/capture", true);
    xhr.send();
  }
  function rotatePhoto() {
    var img = document.getElementById("photo");
    deg += 90;
    if(isOdd(deg/90)){ document.getElementById("container").className = "vert"; }
    else{ document.getElementById("container").className = "hori"; }
    img.style.transform = "rotate(" + deg + "deg)";
  }
  function isOdd(n) { return Math.abs(n % 2) == 1; }
</script>
</html>)rawliteral";

void setup() {
  Serial.begin(9600);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
    // Configuring static IP
  if(!WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Failed to configure Static IP");
  } else {
    Serial.println("Static IP configured!");
  }
  
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP()); 
 if (!SPIFFS.begin(true)) {
  Serial.println("An Error has occurred while mounting SPIFFS");
  ESP.restart();
}
else {
  delay(500);
  Serial.println("SPIFFS mounted successfully");
}
  // Print the ESP32's IP address
Serial.print("IP Address: http://");
Serial.println(WiFi.localIP());
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
 // config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 2 ;
  config.fb_count = 2;
  //init with high specs to pre-allocate larger buffers
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 2 ;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();

    if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if(config.pixel_format == PIXFORMAT_JPEG){
   // s->set_framesize(s, FRAMESIZE_QVGA);
    s->set_framesize(s, FRAMESIZE_SVGA);
   //  s->set_framesize(s, FRAMESIZE_SXGA);
  // s->set_framesize(s, FRAMESIZE_UXGA);
   // s->set_framesize(s, FRAMESIZE_QSXGA);


  }

  // Define a route to serve the HTML page
server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
  request->send(200, "text/html", index_html);
});

server.on("/capture", HTTP_GET, [](AsyncWebServerRequest * request) {
  takeNewPhoto = true;
  request->send(200, "text/plain", "Taking Photo");
});

server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest * request) {
  request->send(SPIFFS, FILE_PHOTO, "image/jpg", false);
});

  // server.on("/capture", HTTP_GET, [](AsyncWebServerRequest * request) {
  //   takeNewPhoto = true;
  //   request->send(200, "text/plain", "Taking Photo");
  // });

  // Start the server
  server.begin();
}

void loop() {
   if (takeNewPhoto) {
    capturePhotoSaveSpiffs();
    takeNewPhoto = false;
  }
  delay(1);
}
// Check if photo capture was successful
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

void capturePhotoSaveSpiffs( void ) {
  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly

  do {
    // Take a photo with the camera
    Serial.println("Taking a photo...");

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    // Photo file name
    Serial.printf("Picture file name: %s\n", FILE_PHOTO);
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

    // Insert the data in the photo file
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("The picture has been saved in ");
      Serial.print(FILE_PHOTO);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    // Close the file
    file.close();
    esp_camera_fb_return(fb);

    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);
  } while ( !ok );
}