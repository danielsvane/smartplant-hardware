/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE"
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second.
*/

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <EEPROM.h>
#include <queue>

using std::string;
using std::queue;

#define EEPROM_SIZE 64

struct WifiSettings {
  char ssid[32];
  char password[32];
};

char ssid[32];
char password[32];

BLECharacteristic *pCharacteristic;

queue<string> messageQueue;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

void saveSettings() {
  WifiSettings settings;

  strcpy(settings.ssid, ssid);
  strcpy(settings.password, password);

  EEPROM.put(0, settings);
  EEPROM.commit();
};

void printString(string value) {
  for (int i = 0; i < value.length(); i++) {
    Serial.print(value[i]);
  }
  Serial.println();
};

class Callbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("Received message");

        printString(rxValue);

        if (rxValue.find("ssid") != -1) {
          Serial.println("SSID: ");
          rxValue.erase(0, 4);
          strcpy(ssid, rxValue.c_str());
          printString(rxValue);
        }
        else if (rxValue.find("pass") != -1) {
          Serial.println("Password: ");
          rxValue.erase(0, 4);
          strcpy(password, rxValue.c_str());
          printString(rxValue);

          saveSettings();

          Serial.println("Saved settings");

          messageQueue.push("ssav");
        }
      }
    }
};

void setup() {
  Serial.begin(115200);

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  };

  // Create the BLE Device
  BLEDevice::init("Smartplant"); // Give it a name

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new Callbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (!messageQueue.empty()) {
    // for (int i = 0; i < messageQueue.size(); i++) {
    Serial.println("Value from message queue:");
    Serial.println(messageQueue.front().c_str());

    pCharacteristic->setValue(messageQueue.front());
    pCharacteristic->notify();

    messageQueue.pop();

  }
  delay(1000);
}
