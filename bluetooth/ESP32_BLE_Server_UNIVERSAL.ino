/*
 * ESP32 BLE Server - UNIVERSAL FIX for getValue() Type Issues
 * 
 * This version handles both library variants:
 * - Libraries that return std::string from getValue()
 * - Libraries that return Arduino String from getValue()
 */

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
 * @brief BLE Characteristic Callbacks - UNIVERSAL VERSION
 * 
 * This version works with both library variants:
 * - Some ESP32 BLE libraries return std::string from getValue()
 * - Other ESP32 BLE libraries return Arduino String from getValue()
 */
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      
      // UNIVERSAL SOLUTION - Works with both library types
      
      // Method 1: Direct Arduino String assignment (for your current library)
      String rxValue = pCharacteristic->getValue();
      
      if (rxValue.length() > 0) {
        Serial.println("📨 Raw received: " + rxValue);
        processCommand(rxValue);
      }
      
      /* 
       * Alternative Method 2: For libraries that return std::string
       * Uncomment this block and comment out Method 1 if you get std::string errors
       * 
      std::string rxValueStd = pCharacteristic->getValue();
      if (rxValueStd.length() > 0) {
        String rxValue = String(rxValueStd.c_str());
        Serial.println("📨 Raw received: " + rxValue);
        processCommand(rxValue);
      }
      */
      
      /* 
       * Method 3: Auto-detection approach (most universal)
       * This tries to handle both cases automatically
       * 
      auto rxValueAuto = pCharacteristic->getValue();
      String command;
      
      // Try to convert based on type
      if constexpr (std::is_same_v<decltype(rxValueAuto), std::string>) {
        command = String(rxValueAuto.c_str());
      } else {
        command = rxValueAuto;
      }
      
      if (command.length() > 0) {
        processCommand(command);
      }
      */
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
  
  /*
   * BLE CONFIGURATION OVERVIEW
   * 
   * This section configures a complete BLE (Bluetooth Low Energy) GATT server
   * with a three-characteristic architecture for bi-directional communication.
   * 
   * BLE HIERARCHY STRUCTURE:
   *   ESP32 Device (BLE_DEVICE_NAME: "ESP32_BLE_Server")
   *   └── Service (SERVICE_UUID: 12345678-1234-1234-1234-123456789abc)
   *       ├── Command Characteristic (WRITE/READ - receives client commands)
   *       ├── Status Characteristic (NOTIFY/READ - sends device status)
   *       └── Data Characteristic (NOTIFY/read - streams sensor data)
   * 
   * COMMUNICATION PATTERN:
   * 1. Client Discovery: Clients scan and find this device by name
   * 2. Connection: Client establishes connection to GATT server
   * 3. Service Discovery: Client discovers available service and characteristics
   * 4. Command Flow: Client writes to Command characteristic → triggers callback
   * 5. Status Updates: Server notifies Status characteristic every 5 seconds
   * 6. Data Stream: Server notifies Data characteristic every 2 seconds
   * 
   * UUID DESIGN:
   * - Service UUID: Groups all related characteristics under one service
   * - Each characteristic has unique UUID for client identification
   * - Custom 128-bit UUIDs avoid conflicts with standard BLE services
   * 
   * PROPERTIES EXPLAINED:
   * - READ: Client can request current value on-demand
   * - WRITE: Client can send new data to characteristic
   * - NOTIFY: Server can push updates to subscribed clients
   * - BLE2902 Descriptor: Required for notifications (client subscription control)
   */
  
  // Initialize BLE Stack and Device
  // BLE (Bluetooth Low Energy) operates on a hierarchical structure:
  // Device -> Service(s) -> Characteristic(s) -> Descriptor(s)
  Serial.println("Initializing BLE stack...");
  
  // Step 1: Initialize the BLE device with a human-readable name
  // This name will be visible when scanning for BLE devices
  // The device name helps clients identify this specific ESP32
  BLEDevice::init(BLE_DEVICE_NAME);
  
  // Step 2: Create BLE Server (GATT Server)
  // The server hosts services and handles client connections
  // GATT = Generic Attribute Protocol (defines how BLE devices communicate)
  // Multiple clients can connect to one server simultaneously
  pServer = BLEDevice::createServer();
  
  // Step 3: Attach connection callbacks to monitor client connect/disconnect events
  // These callbacks trigger when clients connect or disconnect from this server
  pServer->setCallbacks(new MyServerCallbacks());

  // Step 4: Create BLE Service (contains related characteristics)
  // Services group related functionality together using a unique UUID
  // UUID format: 12345678-1234-1234-1234-123456789abc (128-bit custom UUID)
  // Services organize characteristics logically (like a folder containing files)
  pService = pServer->createService(SERVICE_UUID);

  // Step 5: Create BLE Characteristics (data endpoints within the service)
  // Characteristics are like variables that clients can read, write, or subscribe to
  // Each characteristic has properties defining what operations are allowed
  
  // CHARACTERISTIC 1: Command Input (Client -> Server communication)
  // Purpose: Receives commands from BLE clients (like "on", "off", "toggle")
  // Properties: READ + WRITE
  // - READ: Clients can read current value (shows "Ready for commands")
  // - WRITE: Clients can write new commands to this characteristic
  // UUID: 87654321-4321-4321-4321-cba987654321 (unique identifier)
  pCommandCharacteristic = pService->createCharacteristic(
                      COMMAND_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );
  // Attach callback to process incoming commands when client writes to this characteristic
  pCommandCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  // Set initial value that clients will see when they first read this characteristic
  pCommandCharacteristic->setValue("Ready for commands");

  // CHARACTERISTIC 2: Status Updates (Server -> Client notifications)
  // Purpose: Sends device status information to connected clients
  // Properties: READ + NOTIFY
  // - READ: Clients can read current status on-demand
  // - NOTIFY: Server can push status updates to subscribed clients automatically
  // UUID: 11111111-2222-3333-4444-555555555555
  pStatusCharacteristic = pService->createCharacteristic(
                      STATUS_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  // Add BLE2902 descriptor to enable notifications (required for NOTIFY property)
  // BLE2902 = Client Characteristic Configuration Descriptor (CCCD)
  // Clients write to this descriptor to enable/disable notifications
  pStatusCharacteristic->addDescriptor(new BLE2902());
  // Set initial status value containing device information as JSON
  pStatusCharacteristic->setValue(getDeviceStatus().c_str());

  // CHARACTERISTIC 3: Sensor Data Stream (Server -> Client notifications)
  // Purpose: Continuously streams sensor data to connected clients
  // Properties: READ + NOTIFY (same as status, but for different data type)
  // - READ: Clients can request current sensor reading
  // - NOTIFY: Server automatically sends new sensor data every 2 seconds
  // UUID: 22222222-3333-4444-5555-666666666666
  pDataCharacteristic = pService->createCharacteristic(
                      DATA_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  // Add notification descriptor (same purpose as status characteristic)
  pDataCharacteristic->addDescriptor(new BLE2902());
  // Set initial sensor data value as JSON string
  pDataCharacteristic->setValue(getSensorData().c_str());

  // Step 6: Activate the BLE service
  // This makes the service and its characteristics available to connecting clients
  // Until started, clients cannot discover or interact with the service
  pService->start();

  // Step 7: Configure and start BLE advertising (device discovery)
  // Advertising broadcasts the device's existence so clients can find and connect
  // This is like shouting "I'm here!" periodically for other devices to hear
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  
  // Include service UUID in advertising data
  // This helps clients identify what services are available before connecting
  // Clients can filter scan results to find devices offering specific services
  pAdvertising->addServiceUUID(SERVICE_UUID);
  
  // Disable scan response (optional secondary advertising packet)
  // Scan response provides additional advertising data but we don't need it
  // Setting false reduces advertising overhead and power consumption
  pAdvertising->setScanResponse(false);
  
  // Set minimum preferred connection interval to 0x00
  // This parameter suggests connection timing to clients but 0x00 means "no preference"
  // Clients can choose their own optimal connection interval
  pAdvertising->setMinPreferred(0x0);
  
  // Begin broadcasting advertisements
  // The ESP32 will now periodically transmit advertising packets
  // Other BLE devices can scan and discover this device by name and services
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
 * 🔧 COMPILATION ERROR FIXES:
 * 
 * PROBLEM: Different ESP32 BLE library versions return different types from getValue()
 * 
 * ERROR TYPE 1: "conversion from 'String' to non-scalar type 'std::string'"
 * - Your library returns Arduino String, but code expects std::string
 * - SOLUTION: Use direct String assignment (Method 1 above - currently active)
 * 
 * ERROR TYPE 2: "conversion from 'std::string' to Arduino String"  
 * - Your library returns std::string, but code expects Arduino String
 * - SOLUTION: Use .c_str() conversion (Method 2 above - commented out)
 * 
 * CURRENT FIX (Method 1):
 * String rxValue = pCharacteristic->getValue(); // Direct assignment for String-returning libraries
 * 
 * ALTERNATIVE FIX (Method 2 - if Method 1 fails):
 * std::string rxValueStd = pCharacteristic->getValue();
 * String rxValue = String(rxValueStd.c_str());
 * 
 * HOW TO USE:
 * 1. Try Method 1 first (currently active)
 * 2. If compilation fails, comment out Method 1 and uncomment Method 2
 * 3. One of these will definitely work with your ESP32 BLE library version!
 */