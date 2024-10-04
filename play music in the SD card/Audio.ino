/*
  ESP32 SD I2S Music Player
  esp32-i2s-sd-player.ino
  Plays MP3 file from microSD card
  Uses MAX98357 I2S Amplifier Module
  Uses ESP32-audioI2S Library - https://github.com/schreibfaul1/ESP32-audioI2S
  * 
  DroneBot Workshop 2022
  https://dronebotworkshop.com
*/
 
// Include required libraries
#include "Arduino.h"
#include "Audio.h"
#include "SD.h"
#include "FS.h"
 
// microSD Card Reader connections
#define SD_CS          5
#define SPI_MOSI      23 
#define SPI_MISO      19
#define SPI_SCK       18
 
// I2S Connections
#define I2S_DOUT      25
#define I2S_BCLK      27
#define I2S_LRC       26
 
// Create Audio object
Audio audio;
 
void setup() {
    
    // Start Serial Port
    Serial.begin(115200);
    Serial.println("Initializing ESP32 SD I2S Music Player...");
    
    // Set microSD Card CS as OUTPUT and set HIGH
    pinMode(SD_CS, OUTPUT);      
    digitalWrite(SD_CS, HIGH); 
    Serial.println("microSD Card CS set to HIGH.");
    
    // Initialize SPI bus for microSD Card
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    Serial.println("SPI bus initialized.");
    
    // Start microSD Card
    Serial.print("Initializing microSD card...");
    if (!SD.begin(SD_CS)) {
      Serial.println("Error: Could not access microSD card!");
      while (true);  // Halt execution
    } else {
      Serial.println("microSD card initialized successfully.");
    }
    
    // Setup I2S 
    Serial.println("Setting up I2S interface...");
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    Serial.println("I2S interface set up successfully.");
    
    // Set Volume
    audio.setVolume(10);
    Serial.println("Audio volume set to 5.");
    
    // Open music file
    Serial.println("Attempting to play /MYMUSIC.mp3 from microSD card...");
    audio.connecttoFS(SD, "/MYMUSIC.mp3");
}
 
void loop() {
    audio.loop();    
}
