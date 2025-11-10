#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Hardware Configuration
const uint8_t LED_PIN = 2;

// BLE Configuration
const char* BLE_DEVICE_NAME = "ESP32_BLE_Server";
const char* SERVICE_UUID = "12345678-1234-1234-1234-123456789abc";
const char* COMMAND_CHAR_UUID = "87654321-4321-4321-4321-cba987654321";
const char* STATUS_CHAR_UUID = "11111111-2222-3333-4444-555555555555";
const char* DATA_CHAR_UUID = "22222222-3333-4444-5555-666666666666";

// Global Objects
BLEServer* pServer = nullptr;
BLEService* pService = nullptr;
BLECharacteristic* pCommandCharacteristic = nullptr;
BLECharacteristic* pStatusCharacteristic = nullptr;
BLECharacteristic* pDataCharacteristic = nullptr;

// State Variables
bool ledState = false;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long lastStatusUpdate = 0;
unsigned long lastDataSend = 0;
const unsigned long STATUS_UPDATE_INTERVAL = 5000; // 5 seconds
const unsigned long DATA_SEND_INTERVAL = 2000; // 2 seconds
uint32_t dataCounter = 0;

/**
 * @brief Control LED state and notify clients
 */
void setLED(bool state) {
  ledState = state;
  digitalWrite(LED_PIN, state ? HIGH : LOW);
  Serial.println(state ? "🔵 LED: ON" : "⚫ LED: OFF");
  
  // Update status immediately when LED changes
  updateStatusCharacteristic();
}

/**
 * @brief Get device status as JSON string
 */
String getDeviceStatus() {
  String status = "{";
  status += "\"device\":\"ESP32_BLE\",";
  status += "\"name\":\"" + String(BLE_DEVICE_NAME) + "\",";
  status += "\"led\":\"" + String(ledState ? "on" : "off") + "\",";
  status += "\"uptime\":" + String(millis() / 1000) + ",";
  status += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  status += "\"connected\":" + String(deviceConnected ? "true" : "false") + ",";
  status += "\"counter\":" + String(dataCounter);
  status += "}";
  return status;
}

/**
 * @brief Get sensor data as JSON string (simulated data)
 */
String getSensorData() {
  dataCounter++;
  
  // Simulate sensor readings
  float temperature = 20.0 + (random(-50, 150) / 10.0); // 15.0 to 35.0
  float humidity = 45.0 + (random(-200, 300) / 10.0);    // 25.0 to 75.0
  int light = random(0, 1024);                           // 0 to 1023
  
  String data = "{";
  data += "\"timestamp\":" + String(millis()) + ",";
  data += "\"counter\":" + String(dataCounter) + ",";
  data += "\"temperature\":" + String(temperature, 1) + ",";
  data += "\"humidity\":" + String(humidity, 1) + ",";
  data += "\"light\":" + String(light) + ",";
  data += "\"led\":\"" + String(ledState ? "on" : "off") + "\"";
  data += "}";
  
  return data;
}

/**
 * @brief Update status characteristic with current device state
 */
void updateStatusCharacteristic() {
  if (deviceConnected && pStatusCharacteristic) {
    String status = getDeviceStatus();
    pStatusCharacteristic->setValue(status.c_str());
    pStatusCharacteristic->notify();
    Serial.println("📊 Status: " + status);
  }
}

/**
 * @brief Send sensor data via data characteristic
 */
void sendSensorData() {
  if (deviceConnected && pDataCharacteristic) {
    String data = getSensorData();
    pDataCharacteristic->setValue(data.c_str());
    pDataCharacteristic->notify();
    Serial.println("📡 Data: " + data);
  }
}

/**
 * @brief Process incoming BLE commands
 */
void processCommand(String command) {
  command.trim();
  command.toLowerCase();
  
  Serial.println("📨 Command: " + command);
  
  if (command == "led on" || command == "on") {
    setLED(true);
    
  } else if (command == "led off" || command == "off") {
    setLED(false);
    
  } else if (command == "toggle") {
    setLED(!ledState);
    
  } else if (command == "status") {
    updateStatusCharacteristic();
    
  } else if (command == "data") {
    sendSensorData();
    
  } else if (command == "help") {
    // Send help as status update
    if (deviceConnected && pStatusCharacteristic) {
      String help = "{\"help\":[\"on\",\"off\",\"toggle\",\"status\",\"data\",\"help\"]}";
      pStatusCharacteristic->setValue(help.c_str());
      pStatusCharacteristic->notify();
    }
    
  } else if (command.length() > 0) {
    Serial.println("❌ Unknown: " + command);
    // Send error as status
    if (deviceConnected && pStatusCharacteristic) {
      String error = "{\"error\":\"Unknown command: " + command + "\"}";
      pStatusCharacteristic->setValue(error.c_str());
      pStatusCharacteristic->notify();
    }
  }
}

/**
 * @brief BLE Server Callbacks
 */
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("📱 BLE Client Connected!");
      
      // Send welcome message
      if (pStatusCharacteristic) {
        String welcome = "{\"message\":\"Welcome to ESP32 BLE Server!\",\"commands\":[\"on\",\"off\",\"toggle\",\"status\",\"data\"]}";
        pStatusCharacteristic->setValue(welcome.c_str());
        pStatusCharacteristic->notify();
      }
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("📱 BLE Client Disconnected");
    }
};

/**
 * @brief BLE Characteristic Callbacks
 */
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      // FIX: Your library returns String, not std::string
      String rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        processCommand(rxValue);
      }
    }
};

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n🔷 ESP32 BLE Server Starting...");
  Serial.println("====================================");
  
  // Initialize hardware
  pinMode(LED_PIN, OUTPUT);
  setLED(false);
  
  // Initialize BLE
  Serial.println("🔧 Initializing BLE...");
  BLEDevice::init(BLE_DEVICE_NAME);
  
  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE Service
  pService = pServer->createService(SERVICE_UUID);

  // Create Command Characteristic (Write)
  pCommandCharacteristic = pService->createCharacteristic(
                      COMMAND_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );
  pCommandCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  pCommandCharacteristic->setValue("Ready for commands");

  // Create Status Characteristic (Read + Notify)
  pStatusCharacteristic = pService->createCharacteristic(
                      STATUS_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pStatusCharacteristic->addDescriptor(new BLE2902());
  pStatusCharacteristic->setValue(getDeviceStatus().c_str());

  // Create Data Characteristic (Read + Notify) 
  pDataCharacteristic = pService->createCharacteristic(
                      DATA_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pDataCharacteristic->addDescriptor(new BLE2902());
  pDataCharacteristic->setValue(getSensorData().c_str());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  
  Serial.println("✅ BLE Server initialized successfully!");
  Serial.println();
  Serial.println("🔷 === Device Information ===");
  Serial.println("📡 BLE Name: " + String(BLE_DEVICE_NAME));
  Serial.println("🔑 Service UUID: " + String(SERVICE_UUID));
  Serial.println("📝 Command UUID: " + String(COMMAND_CHAR_UUID));
  Serial.println("📊 Status UUID: " + String(STATUS_CHAR_UUID));
  Serial.println("📡 Data UUID: " + String(DATA_CHAR_UUID));
  Serial.println("============================");
  Serial.println();
  Serial.println("📱 BLE Device is now ADVERTISING!");
  Serial.println("🔍 To connect:");
  Serial.println("   1. Open BLE scanner app (nRF Connect, LightBlue, etc.)");
  Serial.println("   2. Look for '" + String(BLE_DEVICE_NAME) + "'");
  Serial.println("   3. Connect to the device");
  Serial.println("   4. Find characteristics and enable notifications");
  Serial.println("   5. Write commands to Command characteristic");
  Serial.println();
  Serial.println("💡 Available Commands: on, off, toggle, status, data, help");
  Serial.println("📊 Status updates every 5 seconds");
  Serial.println("📡 Sensor data sent every 2 seconds");
  Serial.println();
  Serial.println("Ready! 🚀");
}

void loop() {
  // Handle connection state changes
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack a chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("🔄 Restarting BLE advertising...");
    oldDeviceConnected = deviceConnected;
  }
  
  // Connecting
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  
  // Send periodic status updates
  if (deviceConnected && (millis() - lastStatusUpdate > STATUS_UPDATE_INTERVAL)) {
    updateStatusCharacteristic();
    lastStatusUpdate = millis();
  }
  
  // Send periodic sensor data
  if (deviceConnected && (millis() - lastDataSend > DATA_SEND_INTERVAL)) {
    sendSensorData();
    lastDataSend = millis();
  }
  
  // Check for Serial Monitor commands (for testing)
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    Serial.println("🧪 Testing: " + command);
    processCommand(command);
  }
  
  delay(10);
}

/*
 * 🔷 ESP32 BLE Server Features:
 * 
 * ✅ Three BLE Characteristics:
 *    📝 Command (Write): Send commands to ESP32
 *    📊 Status (Read/Notify): Get device status updates
 *    📡 Data (Read/Notify): Receive sensor data stream
 * 
 * ✅ Commands Available:
 *    - "on" / "led on": Turn LED on
 *    - "off" / "led off": Turn LED off  
 *    - "toggle": Toggle LED state
 *    - "status": Request status update
 *    - "data": Request sensor data
 *    - "help": Show available commands
 * 
 * ✅ Automatic Features:
 *    - Status updates every 5 seconds
 *    - Sensor data every 2 seconds
 *    - Auto-restart advertising after disconnect
 *    - JSON formatted responses
 *    - Connection state management
 * 
 * 🔧 BLE Client Apps to Test:
 *    - nRF Connect (Android/iOS)
 *    - LightBlue Explorer (iOS)
 *    - BLE Scanner (Android)
 *    - Serial Bluetooth Terminal (Android)
 * 
 * 📱 How to Use:
 *    1. Upload code to ESP32
 *    2. Open BLE scanner app
 *    3. Connect to "ESP32_BLE_Server"
 *    4. Enable notifications on Status and Data characteristics
 *    5. Write commands to Command characteristic
 *    6. Watch real-time data updates!
 */