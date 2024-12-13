#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// BLE UUIDs
// #define SERVICE_UUID "abcdef12-3456-7890-abcd-ef1234567890"
// #define CHARACTERISTIC_UUID "09876543-21fe-dcba-4321-abcdef098765"
#define SERVICE_UUID "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-abcdef987654"

// #define SOIL_MOISTURE_PIN 1 
#define SOIL_MOISTURE_PIN 2 
// #define SOIL_MOISTURE_PIN 3 

// Define the ID of this node and the target irrigation system
#define NODE_ID "Node1_AGG"
#define TARGET_ID "IrrigationSystem"
// #define NODE_ID "Node1_2"
// #define TARGET_ID "Node1_1"

// BLE Variables
BLECharacteristic *pCharacteristic;

// Function to send test data
void sendTestData(int moistureFake) {
  // String testData = NODE_ID + String("|Moisture:2500");
  // String ownData = NODE_ID + String("|Moisture:2700,Temp:22C,Humidity:60%");
  // testData += '\0';
  // pCharacteristic->setValue((unsigned char *) testData.c_str(), testData.length());
  // Serial.println(ownData.length());

  int moisture = analogRead(SOIL_MOISTURE_PIN);
  // int moisture = moisture;
  // float temperature = dht.readTemperature();
  // float humidity = dht.readHumidity();
  String testData = NODE_ID + String("|Moisture: "+ String(moisture)+",Temp:22C,Humidity:60%");
  pCharacteristic->setValue(testData);
  pCharacteristic->notify();
  Serial.println("Sent test data: " + testData);
}

void setup() {
  // Serial.begin(19200);
  Serial.begin(9600);

  // Initialize BLE
  BLEDevice::init(NODE_ID);
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE characteristic for sending data
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);

  // Start BLE service
  pService->start();

  // Advertise this node
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("Sensing system node ready to send data.");
}

void loop() {
  // Send test data every 5 seconds
  sendTestData(1900);
  delay(5000);
  sendTestData(3000);
  delay(5000);
}
