#include "esp_camera.h"
#include "base64.h"
#include <WiFi.h>
#include <HTTPClient.h>

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

#define FLASH_GPIO_NUM     4

const char* ssid = "Venura";
const char* password = "12345678";
const char* serverName = "https://0e343f79-2075-47d9-a6f8-eb71a020257a.mock.pstmn.io/success"; // Replace with your endpoint URL

camera_fb_t *fb = NULL;
String base64String = "";
unsigned long startTime = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize the flash GPIO pin
  pinMode(FLASH_GPIO_NUM, OUTPUT);
  digitalWrite(FLASH_GPIO_NUM, LOW); // Turn off the flash initially

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Configure camera settings
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Turn on the flash
  digitalWrite(FLASH_GPIO_NUM, HIGH);
  delay(100); // Short delay to ensure flash is on before capturing

  // Capture a picture
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    digitalWrite(FLASH_GPIO_NUM, LOW); // Turn off the flash if capture fails
    return;
  }

  // Turn off the flash
  digitalWrite(FLASH_GPIO_NUM, LOW);

  // Encode the image to Base64
  base64String = base64::encode((uint8_t *)fb->buf, fb->len);
  Serial.println("Base64 Encoded Image:");
  Serial.println(base64String);

  // Send the Base64 encoded image as JSON to the endpoint
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{\"image\":\"" + base64String + "\"}";
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
      Serial.print("HTTP error: ");
      Serial.println(http.errorToString(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }

  // Start timer
  startTime = millis();
}

void loop() {
  // Check if 30 seconds have passed
  if (millis() - startTime >= 30000 && base64String != "") {
    // Delete the image and Base64 string
    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
    }
    base64String = "";
    Serial.println("Image and Base64 string deleted after 30 seconds");

    // Stop further execution
    while (true) {
      // Do nothing, effectively halting the program
    }
  }
}
