/*
 * ESP32 BLE Client - Test Your BLE Server
 * 
 * This code creates a BLE client that can connect to your ESP32_BLE_Server
 * and test all the characteristics and functionality.
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// BLE Server Configuration (must match your server)
const char* SERVER_NAME = "ESP32_BLE_Server";
const char* SERVICE_UUID = "12345678-1234-1234-1234-123456789abc";
const char* COMMAND_CHAR_UUID = "87654321-4321-4321-4321-cba987654321";
const char* STATUS_CHAR_UUID = "11111111-2222-3333-4444-555555555555";
const char* DATA_CHAR_UUID = "22222222-3333-4444-5555-666666666666";

// Hardware
const uint8_t BUTTON_PIN = 0; // Boot button
const uint8_t LED_PIN = 2;

// BLE Objects
BLEClient* pClient = nullptr;
BLERemoteService* pRemoteService = nullptr;
BLERemoteCharacteristic* pCommandCharacteristic = nullptr;
BLERemoteCharacteristic* pStatusCharacteristic = nullptr;
BLERemoteCharacteristic* pDataCharacteristic = nullptr;

// State Variables
bool connected = false;
bool doConnect = false;
bool doScan = false;
BLEAdvertisedDevice* myDevice = nullptr;
unsigned long lastCommand = 0;
int commandIndex = 0;

// Test commands to cycle through
String testCommands[] = {"on", "off", "toggle", "status", "data", "help"};
const int numCommands = sizeof(testCommands) / sizeof(testCommands[0]);

/**
 * @brief Notification callback for Status characteristic
 */
static void statusNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                               uint8_t* pData, size_t length, bool isNotify) {
  Serial.printf("📊 Status Notify: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)pData[i]);
  }
  Serial.println();
}

/**
 * @brief Notification callback for Data characteristic  
 */
static void dataNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                             uint8_t* pData, size_t length, bool isNotify) {
  Serial.printf("📡 Data Notify: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)pData[i]);
  }
  Serial.println();
}

/**
 * @brief BLE Client event callbacks
 */
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("✅ Connected to BLE Server!");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("❌ Disconnected from BLE Server");
    digitalWrite(LED_PIN, LOW);
  }
};

/**
 * @brief Connect to the BLE Server
 */
bool connectToServer() {
  Serial.println("🔄 Connecting to BLE Server...");
  
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server
  if (!pClient->connect(myDevice)) {
    Serial.println("❌ Failed to connect to server");
    return false;
  }
  Serial.println("✅ Connected to server");

  // Obtain a reference to the service
  pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.println("❌ Failed to find our service UUID: " + String(SERVICE_UUID));
    pClient->disconnect();
    return false;
  }
  Serial.println("✅ Found service");

  // Get Command characteristic
  pCommandCharacteristic = pRemoteService->getCharacteristic(COMMAND_CHAR_UUID);
  if (pCommandCharacteristic == nullptr) {
    Serial.println("❌ Failed to find Command characteristic");
    pClient->disconnect();
    return false;
  }
  Serial.println("✅ Found Command characteristic");

  // Get Status characteristic and register for notifications
  pStatusCharacteristic = pRemoteService->getCharacteristic(STATUS_CHAR_UUID);
  if (pStatusCharacteristic == nullptr) {
    Serial.println("❌ Failed to find Status characteristic");
    pClient->disconnect();
    return false;
  }
  
  if (pStatusCharacteristic->canNotify()) {
    pStatusCharacteristic->registerForNotify(statusNotifyCallback);
    Serial.println("✅ Registered for Status notifications");
  }

  // Get Data characteristic and register for notifications  
  pDataCharacteristic = pRemoteService->getCharacteristic(DATA_CHAR_UUID);
  if (pDataCharacteristic == nullptr) {
    Serial.println("❌ Failed to find Data characteristic");
    pClient->disconnect();
    return false;
  }
  
  if (pDataCharacteristic->canNotify()) {
    pDataCharacteristic->registerForNotify(dataNotifyCallback);
    Serial.println("✅ Registered for Data notifications");
  }

  connected = true;
  digitalWrite(LED_PIN, HIGH); // Indicate connection
  return true;
}

/**
 * @brief Scan for BLE servers and find our target
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.printf("🔍 Found device: %s\n", advertisedDevice.toString().c_str());

    // Check if this is our target server
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
      Serial.println("🎯 Found our BLE Server!");
    } else if (advertisedDevice.getName() == SERVER_NAME) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
      Serial.println("🎯 Found server by name!");
    }
  }
};

/**
 * @brief Send command to server
 */
void sendCommand(String command) {
  if (connected && pCommandCharacteristic != nullptr) {
    Serial.println("📤 Sending: " + command);
    pCommandCharacteristic->writeValue(command.c_str(), command.length());
  } else {
    Serial.println("❌ Not connected - cannot send command");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n🔷 ESP32 BLE Client Starting...");
  Serial.println("==================================");
  
  // Initialize hardware
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize BLE
  BLEDevice::init("ESP32_BLE_Client");

  // Retrieve a Scanner and set the callback
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  
  Serial.println("🔍 Starting BLE scan...");
  Serial.println("Looking for: " + String(SERVER_NAME));
  Serial.println("Service UUID: " + String(SERVICE_UUID));
  Serial.println();
  
  pBLEScan->start(5, false);
}

void loop() {
  // If we found a server and should connect
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("🎉 Successfully connected to BLE Server!");
      Serial.println("📱 Press boot button to send commands");
      Serial.println("🔄 Auto-sending test commands every 10 seconds");
    } else {
      Serial.println("❌ Failed to connect to server");
    }
    doConnect = false;
  }

  // If we're connected, handle communication
  if (connected) {
    // Button press - send next test command
    static bool lastButtonState = HIGH;
    bool currentButtonState = digitalRead(BUTTON_PIN);
    
    if (lastButtonState == HIGH && currentButtonState == LOW) {
      // Button pressed
      sendCommand(testCommands[commandIndex]);
      commandIndex = (commandIndex + 1) % numCommands;
      delay(200); // Debounce
    }
    lastButtonState = currentButtonState;
    
    // Auto-send commands every 10 seconds
    if (millis() - lastCommand > 10000) {
      sendCommand(testCommands[commandIndex]);
      commandIndex = (commandIndex + 1) % numCommands;
      lastCommand = millis();
    }
    
  } else if (doScan) {
    // Restart scanning if not connected
    BLEDevice::getScan()->start(0);
    doScan = false;
  }
  
  // Handle Serial commands for testing
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "scan") {
      Serial.println("🔍 Restarting BLE scan...");
      BLEDevice::getScan()->start(5, false);
    } else if (command == "disconnect") {
      if (connected) {
        pClient->disconnect();
        Serial.println("🔌 Disconnected from server");
      }
    } else if (command.length() > 0) {
      sendCommand(command);
    }
  }
  
  delay(100);
}

/*
 * 🧪 BLE Client Test Features:
 * 
 * ✅ Auto-Discovery:
 *    - Scans for ESP32_BLE_Server automatically
 *    - Connects when found
 *    - LED indicates connection status
 * 
 * ✅ Characteristic Access:
 *    - Connects to all three characteristics
 *    - Enables notifications for Status and Data
 *    - Can send commands via Command characteristic
 * 
 * ✅ Interactive Testing:
 *    - Press boot button to cycle through commands
 *    - Auto-sends test commands every 10 seconds
 *    - Serial monitor commands: "scan", "disconnect", or any BLE command
 * 
 * ✅ Real-time Monitoring:
 *    - Receives Status notifications 
 *    - Receives Data notifications
 *    - Shows all communication in Serial Monitor
 * 
 * 📋 Testing Procedure:
 *    1. Upload BLE Server code to one ESP32
 *    2. Upload this BLE Client code to another ESP32
 *    3. Power both devices
 *    4. Watch Serial Monitor - should auto-connect
 *    5. Press boot button or wait for auto-commands
 *    6. Monitor real-time data exchange!
 * 
 * 💡 Serial Commands:
 *    - "scan" - Restart BLE scanning
 *    - "disconnect" - Disconnect from server
 *    - "on", "off", "toggle", etc. - Send to server
 */