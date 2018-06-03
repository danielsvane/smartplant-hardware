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
 #include <BLEUtils.h>
 #include <BLE2902.h>
#include <EEPROM.h>
#include <queue>
#include <WiFi.h>
#include <IOXhop_FirebaseESP32.h>

using std::string;
using std::queue;

#define EEPROM_SIZE 64

struct WifiSettings {
  char ssid[32];
  char password[32];
};

int mode = 1; // bluetooth mode

// char ssid[32];
// char password[32];

 BLECharacteristic *pCharacteristic;

 // queue<string> messageQueue;
 //
 // WiFiClientSecure client;

 #define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
 #define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
 // #define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

 char const *ssid = "kanel28trick65rabat";
 char const *password = "f495bcabd22133098862e7f760";

 const char *host = "api.github.com";
 const int httpsPort = 443;

 // Use web browser to view and copy
 // SHA1 fingerprint of the certificate
 const char* fingerprint = "B8 4F 40 70 0C 63 90 E0 07 E8 7D BD B4 11 D0 4A EA 9C 90 F6";


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

void setupWifi() {
  WiFi.begin(ssid, password);

  Serial.print(F("Connecting to "));
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();

  Serial.println(F("Connected to the WiFi network"));

  Serial.println(F("Memory after wifi setup"));
  Serial.println(ESP.getFreeHeap());

  Firebase.begin("smartplant-df86c.firebaseio.com", "oFbdyoTQz3t3zTlX6SlDPQoYJXlRap4uq06kBu5m");
}

void setupBluetooth () {
  class Callbacks: public BLECharacteristicCallbacks {
     void onWrite(BLECharacteristic *pCharacteristic) {
       string rxValue = pCharacteristic->getValue();

       if (rxValue.length() > 0) {
         Serial.println("Received message");

         printString(rxValue);

         if (rxValue.find("ssid") != -1) {
           Serial.println("SSID: ");
           rxValue.erase(0, 4);
           // strcpy(ssid, rxValue.c_str());
           printString(rxValue);
         }
         else if (rxValue.find("pass") != -1) {
           Serial.println("Password: ");
           rxValue.erase(0, 4);
           // strcpy(password, rxValue.c_str());
           printString(rxValue);

           saveSettings();

           Serial.println("Saved settings");

           // messageQueue.push("ssav");
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
  //   pCharacteristic = pService->createCharacteristic(
  //     CHARACTERISTIC_UUID_TX,
  //     BLECharacteristic::PROPERTY_NOTIFY
  //   );
  //
  //   pCharacteristic->addDescriptor(new BLE2902());

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
};

void setup() {

  Serial.begin(115200);

  Serial.println(F("memory after serial"));
  Serial.println(ESP.getFreeHeap());


  if (mode == 0) setupBluetooth();
  if (mode == 1) setupWifi();

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println(F("failed to initialise EEPROM")); delay(1000000);
  };

  Serial.println(F("memory after EEPROM"));
  Serial.println(ESP.getFreeHeap());
}

void loop() {
  if (mode == 1) {
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
    Serial.println(F("memory after wifi connected"));
    Serial.println(ESP.getFreeHeap());

    Firebase.setInt("foo", 15);

    if (Firebase.failed()) {
      Serial.print("setting /number failed:");
      Serial.println(Firebase.error());
      return;
  }
    // if (!client.connect(host, httpsPort)) {
    // Serial.println(F("connection failed"));


    // return;
}

  // if (client.verify(fingerprint, host)) {
  //   Serial.println("certificate matches");
  // } else {
  //   Serial.println("certificate doesn't match");
  // }
  //
  // String url = "/repos/esp8266/Arduino/commits/master/status";
  // Serial.print("requesting URL: ");
  // Serial.println(url);
  //
  // client.print(String("GET ") + url + " HTTP/1.1\r\n" +
  //              "Host: " + host + "\r\n" +
  //              "User-Agent: BuildFailureDetectorESP8266\r\n" +
  //              "Connection: close\r\n\r\n");
  //
  // Serial.println("request sent");
  // while (client.connected()) {
  //   String line = client.readStringUntil('\n');
  //   if (line == "\r") {
  //     Serial.println("headers received");
  //     break;
  //   }
  // }
  // String line = client.readStringUntil('\n');
  // if (line.startsWith("{\"state\":\"success\"")) {
  //   Serial.println("esp8266/Arduino CI successfull!");
  // } else {
  //   Serial.println("esp8266/Arduino CI has failed");
  // }
  // Serial.println("reply was:");
  // Serial.println("==========");
  // Serial.println(line);
  // Serial.println("==========");
  // Serial.println("closing connection");
  // }
  //
  //
  // Serial.println(F("memory after https"));
  // Serial.println(ESP.getFreeHeap());

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
