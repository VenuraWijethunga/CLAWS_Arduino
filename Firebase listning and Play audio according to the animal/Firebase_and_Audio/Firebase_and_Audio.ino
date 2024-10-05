#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "Audio.h"
#include "SD.h"
#include "FS.h"

// WiFi credentials
#define WIFI_SSID "WENURA"
#define WIFI_PASSWORD "wijethunga1953"

// Firebase credentials
#define API_KEY "AIzaSyB-LLg5PNlmCXmySOW90O5tcs1uDttea24"
#define DATABASE_URL "https://claws-423416-default-rtdb.asia-southeast1.firebasedatabase.app/"

// SD Card and I2S pin configurations
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26

// Firebase objects
FirebaseData fbdo;
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

// Audio object
Audio audio;

// Function to play music based on the animal name
void playAnimalSound(String animalName) {
  if (animalName == "elephant") {
    audio.connecttoFS(SD, "/ELEPHANT.mp3");
  } else if (animalName == "wildboar") {
    audio.connecttoFS(SD, "/WILDBOAR.mp3");
  } else if (animalName == "peacock") {
    audio.connecttoFS(SD, "/PEACOCK.mp3");
  } else {
    Serial.println("Unknown animal name, no music played.");
  }
}

// Callback function to handle Firebase updates
void streamCallback(FirebaseStream data) {
  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_string) {
    String animalName = data.stringData();
    Serial.printf("Animal Name: %s\n", animalName.c_str());
    playAnimalSound(animalName);  // Play corresponding sound
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("Stream timeout, resuming...");
  }
  if (!stream.httpConnected()) {
    Serial.printf("Connection error: %s\n", stream.errorReason().c_str());
  }
}

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  
  // Initialize WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Firebase configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;  // Handle token generation
  
  // Start Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Sign up and set up Firebase stream
  if (Firebase.signUp(&config, &auth, "", "")) {
    signupOK = true;
  } else {
    Serial.printf("SignUp error: %s\n", config.signer.signupError.message.c_str());
  }

  if (!Firebase.RTDB.beginStream(&stream, "response/detectedAnimalName/animalName")) {
    Serial.printf("Stream begin error: %s\n", stream.errorReason().c_str());
  }

  Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);

  // Initialize SD card
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed.");
    while (true);
  }

  // Initialize I2S for audio
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(10);  // Set volume to 10
}

void loop() {
  audio.loop();  // Keep audio playing in loop
}
