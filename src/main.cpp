#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <EEPROM.h>
#include <queue>
#include <WiFi.h>
#include <IOXhop_FirebaseESP32.h>

using std::string;
using std::queue;

#define EEPROM_SIZE 120
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define RESET_PIN 14

struct Settings {
  char mode[5];
  char ssid[32];
  char password[32];
  char uid[29];
  char id[21];
};

Settings settings;

BLECharacteristic *pCharacteristic;

 // queue<string> messageQueue;



void saveSettings() {
   EEPROM.put(0, settings);
   EEPROM.commit();
};

void loadSettings() {
   EEPROM.get(0, settings);
};

void clearSettings() {
  Serial.println(F("Clearing EEPROM"));
  detachInterrupt(RESET_PIN); // Detach interrupt for debouncing
  for (int i = 0 ; i < EEPROM_SIZE ; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  Serial.println(F("EEPROM cleared, resetting"));
  ESP.restart();
}

void printString(string value) {
  for (int i = 0; i < value.length(); i++) {
    Serial.print(value[i]);
  }
  Serial.println();
};

void setColor (int red, int green, int blue) {
  digitalWrite(25, red);
  digitalWrite(26, green);
  digitalWrite(27, blue);
};

void setupWifi() {
  WiFi.begin(settings.ssid, settings.password);

  Serial.print(F("Connecting to "));
  Serial.print(settings.ssid);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();

  Serial.println(F("Connected to the WiFi network"));

  Serial.println(F("Memory after wifi setup"));
  Serial.println(ESP.getFreeHeap());

  Firebase.begin("smartplant-df86c.firebaseio.com", "oFbdyoTQz3t3zTlX6SlDPQoYJXlRap4uq06kBu5m");

  setColor(LOW, LOW, HIGH);
}

void setupBluetooth () {
  class Callbacks: public BLECharacteristicCallbacks {
     void onWrite(BLECharacteristic *pCharacteristic) {
       string rxValue = pCharacteristic->getValue();

       if (rxValue.length() > 0) {
         Serial.println("Received message");

         printString(rxValue);

         if (rxValue.compare(0, 4, "ssid") == 0) {
           Serial.print("SSID: ");
           rxValue.erase(0, 4);
           strcpy(settings.ssid, rxValue.c_str());
           // strcpy(ssid, rxValue.c_str());
           printString(rxValue);
         }
         else if (rxValue.compare(0, 4, "pass") == 0) {
           Serial.print("Password: ");
           rxValue.erase(0, 4);
           strcpy(settings.password, rxValue.c_str());
           // strcpy(password, rxValue.c_str());
           printString(rxValue);

           // messageQueue.push("ssav");
         } else if (rxValue.compare(0, 4, "usid") == 0) {
           Serial.print("User ID: ");
           rxValue.erase(0, 4);
           strcpy(settings.uid, rxValue.c_str());

           printString(rxValue);
         } else if (rxValue.compare(0, 4, "plid") == 0) {
           Serial.print("Plant ID: ");
           rxValue.erase(0, 4);
           strcpy(settings.id, rxValue.c_str());

           printString(rxValue);
         } else if (rxValue.compare(0, 4, "ssav") == 0) {
           strcpy(settings.mode, "wifi");
           saveSettings();
           Serial.println("Saved settings");
           Serial.println();
         }
       }
     }
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

  Serial.println(F("memory after bluetooth"));
  Serial.println(ESP.getFreeHeap());

  Serial.println(F("Waiting a client connection to notify..."));

  setColor(LOW, HIGH, LOW);
};

void setup() {

  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);
  pinMode(27, OUTPUT);

  setColor(HIGH, LOW, HIGH);

  Serial.begin(115200);

  Serial.println(F("memory after serial"));
  Serial.println(ESP.getFreeHeap());

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println(F("failed to initialise EEPROM")); delay(1000000);
  };

  // clearSettings();

  Serial.println(F("memory after EEPROM"));
  Serial.println(ESP.getFreeHeap());

  loadSettings();

  // If mode has been set to wifi in settings, start wifi, otherwise bluetooth
  if (strcmp(settings.mode, "wifi") == 0) setupWifi();
  else setupBluetooth();

  // Setup interrupt for clearing EEPROM and resetting
  pinMode(RESET_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RESET_PIN), clearSettings, FALLING);
}

void loop() {
  if (strcmp(settings.mode, "wifi") == 0) {
    if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
      Serial.println(F("memory after wifi connected"));
      Serial.println(ESP.getFreeHeap());

      char url[100];
      sprintf(url, "plants/%s/%s/value", settings.uid, settings.id);
      Serial.print("Firebase path: ");
      Serial.println(url);

      Firebase.setInt(url, 64);

      if (Firebase.failed()) {
        Serial.print("setting /number failed:");
        Serial.println(Firebase.error());
        return;
      }
    }

  // if (!messageQueue.empty()) {
  //   // char msg = messageQueue.front()
  //   // for (int i = 0; i < messageQueue.size(); i++) {
  //   Serial.println("Value from message queue:");
  //   Serial.println(messageQueue.front().c_str());
  //
  //   // if (msg.compare("ssav")) {
  //   //   Serial.println("Setting up WIFI");
  //   //
  //   // }
  //
  //   pCharacteristic->setValue(messageQueue.front());
  //   pCharacteristic->notify();
  //
  //   messageQueue.pop();
  //
  // }


  }
  delay(5000);
}
