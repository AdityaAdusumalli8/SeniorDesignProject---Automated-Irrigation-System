#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertisedDevice.h>

// BLE UUIDs for server (sending data to IrrigationSystem)
#define SERVICE_UUID_SERVER "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID_SERVER "87654321-4321-4321-4321-abcdef987654"

// // BLE UUIDs for client (receiving data from another node)
// #define SERVICE_UUID_CLIENT "abcdef12-3456-7890-abcd-ef1234567890"
// #define CHARACTERISTIC_UUID_CLIENT "09876543-21fe-dcba-4321-abcdef098765"

// The remote service we wish to connect to.
static BLEUUID serviceUUIDClient("abcdef12-3456-7890-abcd-ef1234567890");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUIDClient("09876543-21fe-dcba-4321-abcdef098765");

// Define the ID of this node and the target irrigation system
#define NODE_ID "Node1_1"
#define TARGET_ID "IrrigationSystem"

// BLE Variables
BLECharacteristic *pCharacteristic;

// Variables for BLE client
BLEClient* pClient;
BLERemoteCharacteristic* pRemoteCharacteristic;
bool doConnect = false;
bool connected = false;
bool doScan = false;
String receivedData = "";
BLEAdvertisedDevice* myDevice;

// Client callback
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("Connected to BLE Server.");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Disconnected from BLE Server.");
  }
};

// Notification callback
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  String data = "";
  for (int i = 0; i < length; i++) {
    data += (char)pData[i];
  }
  Serial.print("Received data: ");
  Serial.println(data);
  // Serial.println(length);
  receivedData = data; // Store received data
}

// Scanning callback
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUIDClient)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      Serial.println("Found our device!");
    }
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  pClient->connect(myDevice);
  Serial.println("Connected to server");
  pClient->setMTU(517); // NOTE: THIS IS EXTREMELY IMPORTANT, OR BLE WILL RESTRICT BYTES TO 23

  BLERemoteService* pRemoteService = pClient->getService(serviceUUIDClient);
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find our service UUID");
    pClient->disconnect();
    return false;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUIDClient);
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("Failed to find our characteristic UUID");
    pClient->disconnect();
    return false;
  }

  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  connected = true;
  return true;
}

// Function to send test data
void sendTestData() {
  // Example sensor data from this node
  int ownMoisture = 2700; // TODO: Replace with actual sensor reading
  String ownData = NODE_ID + String("|Moisture:") + String(ownMoisture) + String(",Temp:22C,Humidity:60%");

  // Variables to store parsed moisture values
  int receivedMoisture = -1;
  int averageMoisture = ownMoisture;

  // Parse received data for moisture value
  if (receivedData != "") {
    int separatorIndex = receivedData.indexOf("|Moisture:");
    if (separatorIndex != -1) {
      String moistureStr = receivedData.substring(separatorIndex + 10);
      receivedMoisture = moistureStr.toInt(); // Convert extracted value to integer

      // Calculate average moisture
      averageMoisture = (ownMoisture + receivedMoisture) / 2;
    }
    receivedData = ""; // Clear received data after processing
  }

  // Prepare aggregated data with averaged moisture value
  String aggregatedData = NODE_ID + String("|Moisture:") + String(averageMoisture) + String(",Temp:22C,Humidity:60%");
  pCharacteristic->setValue(aggregatedData.c_str());
  pCharacteristic->notify();

  Serial.println("Sent aggregated data: " + aggregatedData);
}

// void sendTestData() {
//   String ownData = NODE_ID + String("|Moisture:2700,Temp:22C,Humidity:60%");
//   String aggregatedData = ownData;

//   if (receivedData != "") {
//     aggregatedData += ";" + receivedData;
//     receivedData = ""; // Clear received data after aggregating
//   }

//   pCharacteristic->setValue(aggregatedData.c_str());
//   pCharacteristic->notify();
//   Serial.println("Sent aggregated data: " + aggregatedData);
// }

void setup() {
  Serial.begin(9600);

  // Initialize BLE
  BLEDevice::init(NODE_ID);

  // Initialize BLE server
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID_SERVER);

  // Create BLE characteristic for sending data
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_SERVER,
    BLECharacteristic::PROPERTY_NOTIFY
  );

  // Start BLE service
  pService->start();

  // Advertise this node
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID_SERVER);
  pAdvertising->start();

  Serial.println("Node ready to send data.");

  // Initialize BLE client
  Serial.println("Starting BLE scan...");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("Connected to the BLE Server.");
    } else {
      Serial.println("Failed to connect to the server; restarting scan");
      BLEDevice::getScan()->start(0);
    }
    doConnect = false;
  }

  if (!connected && doScan) {
    BLEDevice::getScan()->start(0);
  }

  // Send test data every 5 seconds
  sendTestData();
  delay(5000);
}
