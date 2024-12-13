/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

#include "BLEDevice.h"
//#include "BLEScan.h"

// The remote service we wish to connect to.
static BLEUUID serviceUUID("12345678-1234-1234-1234-1234567890ab");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID("87654321-4321-4321-4321-abcdef987654");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;

#define VALVE_PIN_1 4
#define VALVE_PIN_2 5
#define FLOW_SENSOR_PIN 4 

int currentTotalFlow = 0;

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);

  // Convert received data to a String
  char data[length + 1];
  memcpy(data, pData, length);
  data[length] = '\0'; // Null-terminate the string

  Serial.print("Received data: ");
  Serial.println(data);

  // Convert to String for easier manipulation
  String receivedString = String(data);
  
  // Initialize variables to store parsed values
  int moistureValue1 = -1;
  int moistureValue2 = -1;
  float temperature = -1;
  float humidity = -1;

  // Parse Moisture1
  int moisture1Index = receivedString.indexOf("Moisture1:");
  if (moisture1Index != -1) {
    String moisture1Str = receivedString.substring(moisture1Index + 10, receivedString.indexOf(",", moisture1Index));
    moistureValue1 = moisture1Str.toInt();
    Serial.print("Parsed Moisture1 Value: ");
    Serial.println(moistureValue1);
  } else {
    Serial.println("Moisture1 value not found.");
  }

  // Parse Moisture2
  int moisture2Index = receivedString.indexOf("Moisture2:");
  if (moisture2Index != -1) {
    String moisture2Str = receivedString.substring(moisture2Index + 10, receivedString.indexOf(",", moisture2Index));
    moistureValue2 = moisture2Str.toInt();
    Serial.print("Parsed Moisture2 Value: ");
    Serial.println(moistureValue2);
  } else {
    Serial.println("Moisture2 value not found.");
  }

  // Calculate the average moisture value
  int averageMoisture = (moistureValue1 + moistureValue2) / 2;
  Serial.print("Average Moisture Value: ");
  Serial.println(averageMoisture);

  // Parse Temperature
  int tempIndex = receivedString.indexOf("Temp:");
  if (tempIndex != -1) {
    String tempStr = receivedString.substring(tempIndex + 5, receivedString.indexOf(",", tempIndex));
    temperature = tempStr.toFloat();
    Serial.print("Parsed Temperature Value: ");
    Serial.println(temperature);
  } else {
    Serial.println("Temperature value not found.");
  }

  // Parse Humidity
  int humidityIndex = receivedString.indexOf("Humidity:");
  if (humidityIndex != -1) {
    String humidityStr = receivedString.substring(humidityIndex + 9);
    humidity = humidityStr.toFloat();
    Serial.print("Parsed Humidity Value: ");
    Serial.println(humidity);
  } else {
    Serial.println("Humidity value not found.");
  }

  // Define thresholds
  int moistureUpper = 2400;
  int moistureLower = 2400;  // Adjust this as needed
  float tempUpper = 86.0;    // Temperature threshold in Fahrenheit
  float humidityLower = 40.0; // Humidity threshold in percentage

  // Control the valves based on parsed values
  if (averageMoisture > moistureUpper || temperature > tempUpper) {
    // Open valve if dry or too hot
    digitalWrite(VALVE_PIN_1, HIGH);
    digitalWrite(VALVE_PIN_2, LOW);
    Serial.println("Valve state: OPEN (Dry/Hot)");
    currentTotalFlow += random(3, 11);
  } else if (averageMoisture < moistureLower && humidity < humidityLower) {
    // Close valve if moisture is sufficient or humidity is high
    digitalWrite(VALVE_PIN_1, LOW);
    digitalWrite(VALVE_PIN_2, LOW);
    Serial.println("Valve state: CLOSED (Wet/High Humidity)");
    currentTotalFlow = 0;
  } else {
    currentTotalFlow = 0;
    digitalWrite(VALVE_PIN_1, LOW);
    digitalWrite(VALVE_PIN_2, LOW);
    Serial.println("Conditions within acceptable range. No action taken.");
  }

  Serial.print("Current flow output (in mL): "); 
  Serial.println(currentTotalFlow); 
}


class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {}

  void onDisconnect(BLEClient *pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");
  pClient->setMTU(517);  //set client to request maximum MTU from server (default is 23 otherwise)

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic.
  if (pRemoteCharacteristic->canRead()) {
    String value = pRemoteCharacteristic->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }

  connected = true;
  return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    }  // Found our server
  }  // onResult
};  // MyAdvertisedDeviceCallbacks

// volatile int pulseCount = 0;  // Variable to store pulse count

// // ISR to count pulses
// void IRAM_ATTR countPulse() {
//   pulseCount++;
// }

void setup() {
  Serial.begin(115200);

  pinMode(VALVE_PIN_1, OUTPUT);
  pinMode(VALVE_PIN_2, OUTPUT);

  digitalWrite(VALVE_PIN_1, LOW);
  digitalWrite(VALVE_PIN_2, LOW);

  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

  // pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), countPulse, FALLING);
}  // End of setup.

// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
    doConnect = false;
    // static unsigned long lastTime = 0;
    // unsigned long currentTime = millis();
    
    // if (currentTime - lastTime >= 1000) {  // Measure every 1 second
    //   lastTime = currentTime;

    //   // Calculate flow rate (example formula, adjust as needed)
    //   float flowRate = (pulseCount / 7.5);  // Adjust based on calibration
    //   pulseCount = 0;  // Reset pulse count

    //   Serial.print("Flow Rate: ");
    //   Serial.print(flowRate);
    //   Serial.println(" L/min");
    // }
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    String newValue = "Time since boot: " + String(millis() / 1000);
    // Serial.println("Setting new characteristic value to \"" + newValue + "\"");

    // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  } else if (doScan) {
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }

  delay(1000);  // Delay a second between loops.
}  // End of loop
