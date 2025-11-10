/*
 * ESP32 Bluetooth Discoverability Test - Simple Version
 * 
 * This is a minimal test to verify continuous Bluetooth discoverability.
 * Upload this code and monitor the Serial output to confirm the fix works.
 */

#include "BluetoothSerial.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

BluetoothSerial SerialBT;
const char* BT_NAME = "ESP32_Test_Always_Visible";
unsigned long lastDiscoverCheck = 0;
bool wasConnected = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("🧪 ESP32 Bluetooth Discoverability Test");
  Serial.println("=====================================");
  
  // Initialize Bluetooth
  if (!SerialBT.begin(BT_NAME)) {
    Serial.println("❌ Bluetooth init failed!");
    while(1) delay(1000);
  }
  
  // Enable continuous discoverability
  SerialBT.enableSSP();
  esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
  
  Serial.println("✅ Bluetooth initialized");
  Serial.println("📡 Device Name: " + String(BT_NAME));
  Serial.println("🔍 Status: ALWAYS DISCOVERABLE");
  Serial.println();
  Serial.println("📱 Now scan for Bluetooth devices on your phone!");
  Serial.println("   You should see: " + String(BT_NAME));
  Serial.println();
  Serial.println("🔄 Monitoring discoverability...");
  Serial.println("=====================================");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Refresh discoverability every 30 seconds
  if (currentTime - lastDiscoverCheck > 30000) {
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    Serial.println("🔄 " + String(currentTime/1000) + "s: Discoverability refreshed");
    lastDiscoverCheck = currentTime;
  }
  
  // Check for connections
  bool isConnected = SerialBT.hasClient();
  if (isConnected && !wasConnected) {
    Serial.println("📱 " + String(currentTime/1000) + "s: Device connected!");
  } else if (!isConnected && wasConnected) {
    Serial.println("📱 " + String(currentTime/1000) + "s: Device disconnected - re-enabling discovery");
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
  }
  wasConnected = isConnected;
  
  // Handle incoming data
  if (SerialBT.available()) {
    String msg = SerialBT.readString();
    msg.trim();
    Serial.println("📨 Received: " + msg);
    SerialBT.println("✅ ESP32 received: " + msg);
    SerialBT.println("⏰ Uptime: " + String(millis()/1000) + " seconds");
    SerialBT.println("🔍 Still discoverable as: " + String(BT_NAME));
  }
  
  delay(100);
}

/*
 * Expected Serial Monitor Output:
 * 
 * 🧪 ESP32 Bluetooth Discoverability Test
 * =====================================
 * ✅ Bluetooth initialized
 * 📡 Device Name: ESP32_Test_Always_Visible
 * 🔍 Status: ALWAYS DISCOVERABLE
 * 
 * 📱 Now scan for Bluetooth devices on your phone!
 *    You should see: ESP32_Test_Always_Visible
 * 
 * 🔄 Monitoring discoverability...
 * =====================================
 * 🔄 30s: Discoverability refreshed
 * 🔄 60s: Discoverability refreshed
 * 📱 75s: Device connected!
 * 📨 Received: hello
 * 📱 120s: Device disconnected - re-enabling discovery
 * 🔄 150s: Discoverability refreshed
 * 
 * SUCCESS: Device should remain visible in Bluetooth scans continuously!
 */