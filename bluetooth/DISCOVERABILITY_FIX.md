# ESP32 Bluetooth Discoverability Issues - Complete Fix Guide

## 🔍 The Problem: Brief Discoverability

**Common Issue**: ESP32 Bluetooth devices become discoverable for only a few seconds after boot, then disappear from Bluetooth scans.

### Why This Happens:
1. **Default Timeout**: ESP32 Bluetooth has a default discoverability timeout
2. **Power Management**: Bluetooth may enter sleep mode to save power
3. **Connection State**: Some ESP32 implementations disable discoverability after first connection
4. **GAP Settings**: Generic Access Profile settings may not be configured for continuous discoverability

## ✅ Complete Solution

### 1. Required Includes
```cpp
#include "BluetoothSerial.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h" 
#include "esp_gap_bt_api.h"
```

### 2. Discoverability Maintenance Function
```cpp
unsigned long lastDiscoverabilityCheck = 0;
const unsigned long DISCOVERABILITY_INTERVAL = 30000; // 30 seconds

void maintainDiscoverability() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastDiscoverabilityCheck > DISCOVERABILITY_INTERVAL) {
    // Re-enable discoverability periodically
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    Serial.println("🔄 Refreshed Bluetooth discoverability");
    lastDiscoverabilityCheck = currentTime;
  }
}
```

### 3. Enhanced Setup Configuration
```cpp
void setup() {
  // ... basic setup code ...
  
  // Initialize Bluetooth with callback
  SerialBT.register_callback(bluetoothCallback);
  
  if (!SerialBT.begin(BT_DEVICE_NAME)) {
    Serial.println("Bluetooth init failed!");
    return;
  }
  
  // KEY FIX: Enable continuous discoverability
  SerialBT.enableSSP();  // Enable Secure Simple Pairing
  esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
  
  Serial.println("✅ Bluetooth is now continuously discoverable!");
}
```

### 4. Enhanced Bluetooth Callback
```cpp
void bluetoothCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  switch (event) {
    case ESP_SPP_SRV_OPEN_EVT:
      Serial.println("📱 Client connected");
      bluetoothConnected = true;
      break;
      
    case ESP_SPP_CLOSE_EVT:
      Serial.println("📱 Client disconnected");
      bluetoothConnected = false;
      // IMPORTANT: Re-enable discoverability after disconnection
      esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
      Serial.println("🔍 Re-enabled discoverability after disconnect");
      break;
      
    case ESP_SPP_START_EVT:
      Serial.println("🚀 SPP server started");
      break;
  }
}
```

### 5. Main Loop Integration
```cpp
void loop() {
  // Maintain continuous discoverability
  maintainDiscoverability();
  
  // Handle Bluetooth communication
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    processCommand(command);
  }
  
  delay(10);
}
```

## 🎯 Advanced Solutions

### Option 1: Aggressive Discoverability (Every 10 seconds)
```cpp
const unsigned long DISCOVERABILITY_INTERVAL = 10000; // 10 seconds

void aggressiveDiscoverability() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > DISCOVERABILITY_INTERVAL) {
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    lastCheck = millis();
  }
}
```

### Option 2: Connection State Management
```cpp
void manageBluetoothState() {
  static bool wasConnected = false;
  
  if (bluetoothConnected && !wasConnected) {
    Serial.println("📱 First connection - maintaining discoverability");
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
  } else if (!bluetoothConnected && wasConnected) {
    Serial.println("📱 Disconnected - re-enabling discoverability");
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
  }
  
  wasConnected = bluetoothConnected;
}
```

### Option 3: Manual Re-discoverability Command
```cpp
void processCommand(String command) {
  command.trim().toLowerCase();
  
  if (command == "rediscover") {
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    SerialBT.println("🔍 Device is now discoverable again!");
    Serial.println("🔍 Manual re-discoverability activated");
    
  } else if (command == "status") {
    SerialBT.println("📊 Bluetooth Status: Always Discoverable");
    SerialBT.println("📱 Device Name: " + String(BT_DEVICE_NAME));
    SerialBT.println("🔍 Discoverability: ENABLED");
    
  } else {
    // ... other commands
  }
}
```

## 🛠️ Troubleshooting Steps

### 1. Check Bluetooth Configuration
```cpp
// Add to setup() for debugging
void checkBluetoothConfig() {
  Serial.println("=== Bluetooth Configuration ===");
  Serial.println("Controller enabled: " + String(esp_bt_controller_get_status()));
  Serial.println("Bluedroid enabled: " + String(esp_bluedroid_get_status()));
  Serial.println("Device name: " + String(BT_DEVICE_NAME));
  Serial.println("==============================");
}
```

### 2. Monitor Discoverability Status
```cpp
void monitorDiscoverability() {
  static unsigned long lastReport = 0;
  
  if (millis() - lastReport > 60000) { // Every minute
    Serial.println("📊 Discoverability Check:");
    Serial.println("   - Device should be visible as: " + String(BT_DEVICE_NAME));
    Serial.println("   - Status refreshed every 30 seconds");
    Serial.println("   - Connected clients: " + String(bluetoothConnected ? "1" : "0"));
    lastReport = millis();
  }
}
```

### 3. Test Connection Recovery
```cpp
void testConnectionRecovery() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim().toLowerCase();
    
    if (input == "test") {
      Serial.println("🧪 Testing discoverability...");
      esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
      delay(2000);
      esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
      Serial.println("✅ Discoverability test complete");
    }
  }
}
```

## 📱 Client-Side Tips

### For Android Devices:
1. **Clear Bluetooth Cache**: Settings → Apps → Bluetooth → Storage → Clear Cache
2. **Forget and Re-pair**: If device was paired before, forget it first
3. **Use Bluetooth Terminal Apps**: Apps like "Bluetooth Terminal" work better than built-in terminals

### For iOS Devices:
1. **Use Third-Party Apps**: iOS requires apps that support Bluetooth Classic SPP
2. **Reset Network Settings**: If having connection issues
3. **Check App Permissions**: Ensure Bluetooth permissions are granted

### For Windows/Linux:
1. **Use Bluetooth Serial Terminal**: PuTTY, Arduino Serial Monitor, or dedicated Bluetooth terminals
2. **Check COM Port Assignment**: Windows assigns COM ports to Bluetooth Serial connections
3. **Pair Before Connecting**: Device must be paired in OS Bluetooth settings first

## 🎯 Key Points for Success

1. **Always Call `esp_bt_gap_set_scan_mode()`** after Bluetooth initialization
2. **Re-enable After Disconnections** in the callback function
3. **Periodic Refresh** every 30-60 seconds as backup
4. **Monitor Serial Output** to confirm discoverability status
5. **Test with Multiple Devices** to ensure compatibility
6. **Use Proper Bluetooth Apps** on client devices

## 🚀 Expected Behavior After Fix

```
Bluetooth initialized successfully!
Device Name: ESP32_BT_Server
Bluetooth MAC: AA:BB:CC:DD:EE:FF
🔍 Device is now DISCOVERABLE and ready for connections!

📱 To connect:
   1. Open Bluetooth settings
   2. Scan for devices  
   3. Look for 'ESP32_BT_Server'
   4. Connect and send commands

🔄 Refreshed Bluetooth discoverability (every 30s)
📱 Client connected
📱 Client disconnected  
🔍 Re-enabled discoverability after disconnect
```

With these fixes, your ESP32 should remain discoverable **continuously** instead of just briefly! 🎉