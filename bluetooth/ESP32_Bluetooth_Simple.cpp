#include "BluetoothSerial.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

// Check if Bluetooth is available
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Hardware Configuration
const uint8_t LED_PIN = 2;

// Bluetooth Configuration
const char* BT_DEVICE_NAME = "ESP32_BT_Server";

// Global Objects
BluetoothSerial SerialBT;

// State Variables
bool ledState = false;
bool bluetoothConnected = false;
String deviceName = "";
unsigned long lastDiscoverabilityCheck = 0;
const unsigned long DISCOVERABILITY_INTERVAL = 30000; // Check every 30 seconds

/**
 * @brief Ensure Bluetooth remains discoverable
 */
void maintainDiscoverability() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastDiscoverabilityCheck > DISCOVERABILITY_INTERVAL) {
    // Re-enable discoverability periodically
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    Serial.println("Refreshed Bluetooth discoverability");
    lastDiscoverabilityCheck = currentTime;
  }
}

/**
 * @brief Get Bluetooth MAC address as string
 */
String getBluetoothMAC() {
  uint8_t mac[6];
  SerialBT.getBtAddress(mac);
  
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  return String(macStr);
}

/**
 * @brief Control LED state
 */
void setLED(bool state) {
  ledState = state;
  digitalWrite(LED_PIN, state ? HIGH : LOW);
  Serial.println(state ? "LED: ON" : "LED: OFF");
}

/**
 * @brief Bluetooth event callback
 */
void bluetoothCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  switch (event) {
    case ESP_SPP_SRV_OPEN_EVT:
      Serial.println("Bluetooth client connected");
      bluetoothConnected = true;
      SerialBT.println("Welcome to ESP32 Bluetooth Server!");
      SerialBT.println("Available commands:");
      SerialBT.println("  led on    - Turn LED ON");
      SerialBT.println("  led off   - Turn LED OFF");
      SerialBT.println("  status    - Get device status");
      SerialBT.println("  help      - Show this help");
      break;
      
    case ESP_SPP_CLOSE_EVT:
      Serial.println("Bluetooth client disconnected");
      bluetoothConnected = false;
      // Re-enable discoverability after disconnection
      Serial.println("Re-enabling discoverability...");
      esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
      break;
      
    case ESP_SPP_START_EVT:
      Serial.println("Bluetooth SPP server started");
      break;
      
    case ESP_SPP_INIT_EVT:
      Serial.println("Bluetooth SPP initialized");
      break;
      
    default:
      Serial.printf("Bluetooth event: %d\n", event);
      break;
  }
}

/**
 * @brief Process incoming Bluetooth commands
 */
void processBluetoothCommand(String command) {
  command.trim();
  command.toLowerCase();
  
  Serial.println("Received command: " + command);
  
  if (command == "led on") {
    setLED(true);
    SerialBT.println("LED turned ON");
    
  } else if (command == "led off") {
    setLED(false);
    SerialBT.println("LED turned OFF");
    
  } else if (command == "status") {
    SerialBT.println("=== Device Status ===");
    SerialBT.println("Device: ESP32");
    SerialBT.println("Bluetooth Name: " + String(BT_DEVICE_NAME));
    SerialBT.println("Bluetooth MAC: " + getBluetoothMAC());
    SerialBT.println("LED State: " + String(ledState ? "ON" : "OFF"));
    SerialBT.println("Uptime: " + String(millis() / 1000) + " seconds");
    SerialBT.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    SerialBT.println("Bluetooth Connected: " + String(bluetoothConnected ? "Yes" : "No"));
    SerialBT.println("====================");
    
  } else if (command == "help") {
    SerialBT.println("Available commands:");
    SerialBT.println("  led on    - Turn LED ON");
    SerialBT.println("  led off   - Turn LED OFF");
    SerialBT.println("  status    - Get device status");
    SerialBT.println("  help      - Show this help");
    
  } else if (command.length() > 0) {
    SerialBT.println("Unknown command: '" + command + "'");
    SerialBT.println("Type 'help' for available commands");
  }
}

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 Bluetooth Server ===");
  Serial.println("Firmware Version: 1.0");
  Serial.println();
  
  // Initialize hardware
  pinMode(LED_PIN, OUTPUT);
  setLED(false);
  
  // Initialize Bluetooth
  Serial.println("Initializing Bluetooth...");
  SerialBT.register_callback(bluetoothCallback);
  
  if (!SerialBT.begin(BT_DEVICE_NAME)) {
    Serial.println("ERROR: Bluetooth initialization failed!");
    Serial.println("Please check:");
    Serial.println("  1. Bluetooth is enabled in ESP32 configuration");
    Serial.println("  2. Device supports Bluetooth Classic");
    while(1) {
      delay(1000);
    }
  }
  
  // Enable discoverability and connectability
  SerialBT.enableSSP();  // Enable Secure Simple Pairing
  
  // Set device as discoverable and connectable
  esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
  
  Serial.println("Bluetooth initialized successfully!");
  Serial.println();
  Serial.println("=== Device Information ===");
  Serial.println("Bluetooth Name: " + String(BT_DEVICE_NAME));
  Serial.println("Bluetooth MAC: " + getBluetoothMAC());
  Serial.println("Device discoverable as: " + String(BT_DEVICE_NAME));
  Serial.println("Discoverability: ENABLED (Continuous)");
  Serial.println("==========================");
  Serial.println();
  Serial.println("🔍 Device is now DISCOVERABLE and ready for connections!");
  Serial.println("📱 To connect:");
  Serial.println("   1. Open Bluetooth settings on your phone/computer");
  Serial.println("   2. Scan for new devices");
  Serial.println("   3. Look for '" + String(BT_DEVICE_NAME) + "'");
  Serial.println("   4. Pair and connect");
  Serial.println("   5. Send commands like 'led on', 'status', 'help'");
  Serial.println();
  Serial.println("💡 Tip: Device stays discoverable continuously!");
  Serial.println();
  
  // Initial status
  Serial.println("Ready! LED is OFF, waiting for commands...");
}

void loop() {
  // Maintain Bluetooth discoverability
  maintainDiscoverability();
  
  // Check for incoming Bluetooth data
  if (SerialBT.available()) {
    String command = SerialBT.readString();
    processBluetoothCommand(command);
  }
  
  // Check for Serial Monitor input (for testing)
  if (Serial.available()) {
    String command = Serial.readString();
    Serial.println("Testing command locally: " + command);
    processBluetoothCommand(command);
  }
  
  delay(10);
}