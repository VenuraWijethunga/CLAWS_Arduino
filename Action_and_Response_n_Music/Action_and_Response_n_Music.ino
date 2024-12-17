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
#define API_KEY "AIzaSyBKBp7ZU6qyemAcp2pAw6WSB5QpOViMUt4"
#define DATABASE_URL "https://claws-441617-default-rtdb.asia-southeast1.firebasedatabase.app/"

// SD Card and I2S pin configurations
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26

// Firebase objects
FirebaseData fbdo;             // General Firebase object
FirebaseData stream1;          // Stream for first path (animalName)
FirebaseData stream2;          // Stream for second path (Action)
FirebaseData stream3;          // Stream for third path (ChangedAnimal)
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;
bool actionState = false;  // Track whether action == 1 or 0
Audio audio;

// Function to play music based on the animal name
void playAnimalSound(String animalName) {
  if (animalName == "elephant") {
    audio.connecttoFS(SD, "/ELEPHANT.mp3");
  } else if (animalName == "wildboar") {
    audio.connecttoFS(SD, "/WILDBOAR.mp3");
  } else if (animalName == "peacock") {
    audio.connecttoFS(SD, "/PEACOCK.mp3");
  } else if (animalName == "common") {
    audio.connecttoFS(SD, "/COMMON.mp3");
  }else {
    Serial.println("Unknown animal name, no music played.");
  }
}

// Callback function for the first path: animalName (Response)
void streamCallback1(FirebaseStream data) {
  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_string) {
    String animalName = data.stringData();
    Serial.printf("Animal Name Updated: %s\n", animalName.c_str());
    if (actionState) {  // Play sound only if action == 1
      playAnimalSound(animalName);
    }
  }
}

// Callback function for the third path: ChangedAnimal (Request Update)
void streamCallback3(FirebaseStream data) {
  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_string) {
    String changedAnimal = data.stringData();
    Serial.printf("Changed Animal Updated: %s\n", changedAnimal.c_str());
    if (actionState) {  // Play sound only if action == 1
      playAnimalSound(changedAnimal);
    }
  }
}

// Callback function for the second path: Action
void streamCallback2(FirebaseStream data) {
  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_string) {
    String action = data.stringData();
    Serial.printf("Action Updated: %s\n", action.c_str());

    if (action == "1") {
      actionState = true;
      Serial.println("Action is 1. Listening to updates...");
      
      // Start streaming both paths when action is 1
      if (!Firebase.RTDB.beginStream(&stream1, "response/detectedAnimalName/animalName")) {
        Serial.printf("Stream 1 error: %s\n", stream1.errorReason().c_str());
      }
      Firebase.RTDB.setStreamCallback(&stream1, streamCallback1, NULL);
      
      if (!Firebase.RTDB.beginStream(&stream3, "RequestUpdate/animalUpdate/ChangedAnimal")) {
        Serial.printf("Stream 3 error: %s\n", stream3.errorReason().c_str());
      }
      Firebase.RTDB.setStreamCallback(&stream3, streamCallback3, NULL);
    } 
    else if (action == "0") {
      actionState = false;
      Serial.println("Action is 0. Stopping all playback...");
      audio.stopSong();  // Stop currently playing music
      // Stop listening to updates
      Firebase.RTDB.endStream(&stream1);
      Firebase.RTDB.endStream(&stream3);
    }
  }
}

// Timeout callbacks
void streamTimeoutCallback1(bool timeout) {
  if (timeout) {
    Serial.println("Stream 1 timeout, resuming...");
  }
}

void streamTimeoutCallback2(bool timeout) {
  if (timeout) {
    Serial.println("Stream 2 timeout, resuming...");
  }
}

void streamTimeoutCallback3(bool timeout) {
  if (timeout) {
    Serial.println("Stream 3 timeout, resuming...");
  }
}

// Initialize SD card
void initializeSDCard() {
  const int maxRetries = 5;
  int retryCount = 0;

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  while (!SD.begin(SD_CS) && retryCount < maxRetries) {
    retryCount++;
    Serial.printf("SD card initialization failed. Retry %d of %d\n", retryCount, maxRetries);
    delay(1000);
  }

  if (retryCount >= maxRetries) {
    Serial.println("SD card initialization failed. Halting.");
    while (true) delay(1000);
  }

  Serial.println("SD card initialized successfully.");
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

  // Firebase configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  // Start Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Sign up
  if (Firebase.signUp(&config, &auth, "", "")) {
    signupOK = true;
  } else {
    Serial.printf("SignUp error: %s\n", config.signer.signupError.message.c_str());
  }

  // Start Firebase stream for Action path
  if (!Firebase.RTDB.beginStream(&stream2, "RequestUpdate/Action")) {
    Serial.printf("Stream 2 error: %s\n", stream2.errorReason().c_str());
  }
  Firebase.RTDB.setStreamCallback(&stream2, streamCallback2, streamTimeoutCallback2);

  // Initialize SD card and I2S
  initializeSDCard();
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(10);
}

void loop() {
  audio.loop();  // Keep checking for audio playback
}