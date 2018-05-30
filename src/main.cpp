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

#define EEPROM_SIZE 64

struct WifiSettings {
  char ssid[32];
  char password[32];
};

char ssid[32];
char password[32];

int incomingByte = 0;
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
float txValue = 0;
const int readPin = 32; // Use GPIO number. See ESP32 board pinouts
const int LED = 2; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.

char buf[50];

//std::string rxValue; // Could also make this a global var to access it in loop()

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void saveSettings() {
  WifiSettings settings;

  strcpy(settings.ssid, ssid);
  strcpy(settings.password, password);

  EEPROM.put(0, settings);
  EEPROM.commit();
};

void printString(std::string value) {
  for (int i = 0; i < value.length(); i++) {
    Serial.print(value[i]);
  }
  Serial.println();
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");

        printString(rxValue);

        if (rxValue.find("ssid") != -1) {
          Serial.println("received ssid");
          rxValue.erase(0, 4);
          strcpy(ssid, rxValue.c_str());
          printString(rxValue);
        }
        else if (rxValue.find("pass") != -1) {
          Serial.println("received pass");
          rxValue.erase(0, 4);
          strcpy(password, rxValue.c_str());
          printString(rxValue);

          saveSettings();
        }
        else if (rxValue.find("read") != -1) {
          Serial.println("reading eeprom");

          WifiSettings settings;

          EEPROM.get(0, settings);
          Serial.println(settings.ssid);
          Serial.println(settings.password);
        }

        Serial.println();
        Serial.println("*********");
      }
    }
};

void setup() {
  Serial.begin(115200);

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }

  pinMode(LED, OUTPUT);

  // Create the BLE Device
  BLEDevice::init("Smartplant"); // Give it a name

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

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

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();

    // say what you got:
    Serial.print("I received: ");
    Serial.println(incomingByte, DEC);

    if (incomingByte == 97) {
      WifiSettings settings = {
        "longtest",
        "mypassword"
      };
      // Settings settings = {
      //   "test"
      // };

      // int val = byte(random(10020));
      // write the value to the appropriate byte of the EEPROM.
      // these values will remain there when the board is
      // turned off.
      EEPROM.put(0, settings);
      EEPROM.commit();

      Serial.println("Added settings to EEPROM");
      Serial.println(settings.ssid);
      // Serial.println(settings.password);
    }

    if (incomingByte == 103) {
      // Settings settings;
      // byte lol;
      WifiSettings settings;
      // lol = EEPROM.read(0);
      Serial.println("Loaded settings from EEPROM");
      // int i;
      // for (i = 0; i < 5; i++) {
      //   Serial.println(settings.ssid[i]);
      // }
      // Serial.print(lol, DEC);
      // Serial.println();

      EEPROM.get(0, settings);
      Serial.println(settings.ssid);
      Serial.println(settings.password);

      // Serial.print(byte(EEPROM.read(0)));
      // Serial.println();
    }
  }
}
