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

// Callback function to handle Firebase updates for RequestUpdate and Response
void streamCallback(FirebaseStream data) {
  // Handle Action from RequestUpdate
  if (data.dataPath() == "/RequestUpdate/Action" && data.dataTypeEnum() == fb_esp_rtdb_data_type_integer) {
    int action = data.intData();
    Serial.printf("Action received: %d\n", action);

    if (action == 1) {
      // Action 1: Check the animal name and play music
      if (Firebase.RTDB.getString(&fbdo, "/RequestUpdate/animalUpdate/ChangeAnimal")) {
        String animalName = fbdo.stringData();
        Serial.printf("Animal to play: %s\n", animalName.c_str());
        playAnimalSound(animalName);  // Play corresponding sound
      } else {
        Serial.printf("Failed to get ChangeAnimal: %s\n", fbdo.errorReason().c_str());
      }
    }
    else if (action == 0) {
      // Action 0: Stop music if playing
      audio.stopSong();
      Serial.println("Music stopped.");
    }
  }
  
  // Handle ChangeAnimal from RequestUpdate (If needed)
  else if (data.dataPath() == "/RequestUpdate/animalUpdate/ChangeAnimal" && data.dataTypeEnum() == fb_esp_rtdb_data_type_string) {
    String animalName = data.stringData();
    Serial.printf("ChangeAnimal received: %s\n", animalName.c_str());

    // If Action is 1, play the animal sound
    if (Firebase.RTDB.getInt(&fbdo, "/RequestUpdate/Action")) {
      int action = fbdo.intData();
      if (action == 1) {
        playAnimalSound(animalName);  // Play corresponding sound
      } else {
        Serial.println("Action is not 1, no music played.");
      }
    } else {
      Serial.printf("Failed to read Action: %s\n", fbdo.errorReason().c_str());
    }
  }

  // Handle the response part updates (this part is already fine based on your previous logic)
  if (data.dataPath() == "/response/detectedAnimalName/animalName" && data.dataTypeEnum() == fb_esp_rtdb_data_type_string) {
    String animalName = data.stringData();
    Serial.printf("Response Animal Name: %s\n", animalName.c_str());
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

// Initialize SD card with retry mechanism
void initializeSDCard() {
  const int maxRetries = 5;  // Maximum number of retries
  int retryCount = 0;

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  while (!SD.begin(SD_CS) && retryCount < maxRetries) {
    retryCount++;
    Serial.printf("SD card initialization failed. Retry %d of %d\n", retryCount, maxRetries);
    delay(1000);  // Wait before retrying
  }

  if (retryCount >= maxRetries) {
    Serial.println("SD card initialization failed after maximum retries. Halting.");
    while (true) {
      // Optionally blink an LED or provide another visual indicator
      delay(1000);
    }
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

  // Set up Firebase stream to listen to both Action, ChangeAnimal, and Response
  if (!Firebase.RTDB.beginStream(&stream, "/RequestUpdate/Action") || 
      !Firebase.RTDB.beginStream(&stream, "/RequestUpdate/animalUpdate/ChangeAnimal") ||
      !Firebase.RTDB.beginStream(&stream, "/response/detectedAnimalName/animalName")) {
    Serial.printf("Stream begin error: %s\n", stream.errorReason().c_str());
  }

  Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);

  // Initialize SD card
  initializeSDCard();

  // Initialize I2S for audio
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(10);  // Set volume to 10
}

void loop() {
  audio.loop();  // Keep audio playing in loop
}
