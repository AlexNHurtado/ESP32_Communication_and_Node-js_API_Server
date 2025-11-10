# ESP32 Bluetooth MAC Address - Quick Fix Guide

## 🐛 The Problem

**Error Message:**
```
error: no matching function for call to 'BluetoothSerial::getBtAddress()'
candidate expects 1 argument, 0 provided
```

## ✅ The Solution

The `getBtAddress()` function requires a `uint8_t*` parameter (byte array) to store the MAC address.

### ❌ Incorrect Usage:
```cpp
// This causes compilation error
String mac = SerialBT.getBtAddress();
SerialBT.println("MAC: " + String(SerialBT.getBtAddress()));
```

### ✅ Correct Usage:

#### Method 1: Helper Function (Recommended)
```cpp
/**
 * @brief Get Bluetooth MAC address as formatted string
 */
String getBluetoothMAC() {
  uint8_t mac[6];
  SerialBT.getBtAddress(mac);
  
  char macStr[18];  // 17 chars + null terminator
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  return String(macStr);
}

// Usage:
SerialBT.println("Bluetooth MAC: " + getBluetoothMAC());
```

#### Method 2: Inline Implementation
```cpp
uint8_t mac[6];
SerialBT.getBtAddress(mac);

char macStr[18];
sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

SerialBT.println("Bluetooth MAC: " + String(macStr));
```

#### Method 3: Alternative Format
```cpp
String formatBluetoothMAC() {
  uint8_t mac[6];
  SerialBT.getBtAddress(mac);
  
  String macStr = "";
  for (int i = 0; i < 6; i++) {
    if (i > 0) macStr += ":";
    if (mac[i] < 0x10) macStr += "0";
    macStr += String(mac[i], HEX);
  }
  macStr.toUpperCase();
  return macStr;
}
```

## 🔧 Complete Example

```cpp
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

String getBluetoothMAC() {
  uint8_t mac[6];
  SerialBT.getBtAddress(mac);
  
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  return String(macStr);
}

void setup() {
  Serial.begin(115200);
  
  if (!SerialBT.begin("ESP32_Device")) {
    Serial.println("Bluetooth init failed");
    return;
  }
  
  Serial.println("Bluetooth MAC: " + getBluetoothMAC());
  SerialBT.println("My MAC: " + getBluetoothMAC());
}

void loop() {
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    if (command.trim() == "mac") {
      SerialBT.println("Bluetooth MAC: " + getBluetoothMAC());
    }
  }
}
```

## 🎯 Key Points

1. **Parameter Required**: `getBtAddress()` needs a `uint8_t[6]` array parameter
2. **No Return Value**: The function doesn't return the MAC address directly
3. **Formatting Needed**: Raw bytes need to be formatted as readable string
4. **Buffer Size**: MAC address string needs 18 characters (17 + null terminator)
5. **Format Options**: Common formats are `XX:XX:XX:XX:XX:XX` or `XXXXXXXXXXXX`

## 🚀 Quick Fix Steps

1. **Create helper function** `getBluetoothMAC()` as shown above
2. **Replace all instances** of `SerialBT.getBtAddress()` with `getBluetoothMAC()`
3. **Test compilation** - error should be resolved
4. **Verify output** - MAC address should display correctly

## 📱 Expected Output

```
Bluetooth initialized successfully!
Bluetooth MAC: A4:CF:12:34:56:78
Device ready for connections...
```

This fix works for all ESP32 Bluetooth Serial applications! 🎉