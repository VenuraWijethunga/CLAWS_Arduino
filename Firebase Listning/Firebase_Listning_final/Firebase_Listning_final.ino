#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "Venura"
#define WIFI_PASSWORD "12345678"

// Insert Firebase project API Key
#define API_KEY "AIzaSyB-LLg5PNlmCXmySOW90O5tcs1uDttea24"

// Insert RTDB URL
#define DATABASE_URL "https://claws-423416-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Define Firebase Data object
FirebaseData fbdo;
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

// Function to retrieve and print the values of animalName and detTime
void getAnimalInfo() {
  if (Firebase.RTDB.getString(&fbdo, "response/detectedAnimalName/animalName")) {
    String animalName = fbdo.stringData();
    Serial.printf("Animal Name: %s\n", animalName.c_str());
  } else {
    Serial.printf("Failed to get animalName: %s\n", fbdo.errorReason().c_str());
  }

  if (Firebase.RTDB.getString(&fbdo, "response/detectedAnimalName/detTime")) {
    String detTime = fbdo.stringData();
    Serial.printf("Detection Time: %s\n", detTime.c_str());
  } else {
    Serial.printf("Failed to get detTime: %s\n", fbdo.errorReason().c_str());
  }
}

// Callback function that runs when a change is detected
void streamCallback(FirebaseStream data) {
  Serial.printf("Stream path: %s\n", data.streamPath().c_str());
  Serial.printf("Event path: %s\n", data.dataPath().c_str());
  Serial.printf("Data type: %s\n", data.dataType().c_str());
  Serial.printf("Event type: %s\n\n", data.eventType().c_str());

  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_json || data.dataTypeEnum() == fb_esp_rtdb_data_type_string) {
    // Retrieve the values of animalName and detTime whenever an update is detected
    getAnimalInfo();
  } else {
    Serial.printf("Data: %s\n", data.stringData().c_str());   
  }
}

// Callback function that runs on stream timeout
void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("Stream timeout, resuming...");
  }
  if (!stream.httpConnected()) {
    Serial.printf("Connection error: %s\n", stream.errorReason().c_str());
  }
}

void setup() {
  Serial.begin(115200);
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

  // Assign the API key (required)
  config.api_key = API_KEY;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  // Set the authentication method to anonymous
  auth.user.email = "";
  auth.user.password = "";

  // Sign up
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Start listening to changes on the specified path
  if (!Firebase.RTDB.beginStream(&stream, "response/detectedAnimalName")) {
    Serial.printf("Stream begin error: %s\n", stream.errorReason().c_str());
  }
  Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);
}

void loop() {
  // No need to do anything in the loop, as the stream callback will handle updates
}
