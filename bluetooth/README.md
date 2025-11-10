# ESP32 Bluetooth Communication Examples

Complete examples for **both Bluetooth Classic and BLE (Bluetooth Low Energy)** communication with ESP32, including LED control, sensor data, and device status monitoring.

## � Files Overview

| File | Type | Description |
|------|------|-------------|
| `ESP32_Bluetooth_Simple.cpp` | **Classic** | Bluetooth Classic server with LED control |
| `ESP32_BLE_Server.ino` | **BLE** | BLE server with three characteristics |
| `ESP32_BLE_Client.ino` | **BLE** | BLE client for testing the server |
| `BLE_vs_Classic_Guide.md` | **Guide** | Complete comparison and migration guide |
| `DISCOVERABILITY_FIX.md` | **Fix** | Solution for discoverability issues |

## 🔷 BLE vs Bluetooth Classic

### **NEW: BLE Server Available!** 🎉

| Feature | **Bluetooth Classic** | **BLE (Bluetooth Low Energy)** |
|---------|----------------------|--------------------------------|
| **Power** | 100-200mA active | 1-50mA active ⚡ |
| **Battery Life** | Hours to days | Months to years 🔋 |
| **Range** | ~10 meters | ~10-100+ meters 📡 |
| **Connection** | ~6 seconds | ~0.5 seconds ⚡ |
| **Mobile Apps** | Limited iOS support | Full iOS/Android support 📱 |
| **Web Support** | None | Web Bluetooth API 🌐 |
| **Complexity** | Simple serial | Characteristic-based |
| **Best For** | Audio, file transfer | IoT sensors, wearables |

### **🎯 Recommendation:**
- **Use BLE** for new projects, mobile apps, battery devices
- **Use Classic** for audio streaming, legacy compatibility

## 🚀 Quick Start

### 🔷 Option 1: BLE Server (Recommended)

#### Requirements:
- ESP32 development board
- Arduino IDE with ESP32 board support  
- BLE scanner app (nRF Connect, LightBlue)

#### Steps:
1. Open `ESP32_BLE_Server.ino` in Arduino IDE
2. Upload to ESP32
3. Open Serial Monitor (115200 baud)
4. Install **nRF Connect** app on your phone
5. Scan for "ESP32_BLE_Server"
6. Connect and explore characteristics!

### 🔵 Option 2: Bluetooth Classic (Legacy)

#### Requirements:
- ESP32 development board
- Arduino IDE with ESP32 board support
- Bluetooth terminal app

#### Steps:
1. Open `ESP32_Bluetooth_Simple.cpp` in Arduino IDE  
2. Upload to ESP32
3. Open Serial Monitor (115200 baud)
4. Pair device from phone Bluetooth settings
5. Use Bluetooth terminal app to connect

### 2. Connect from Your Device

#### Android Phone:

Step 1: Pair the Device
1. Open Settings → Connected devices → Bluetooth
2. Tap "Pair new device" or "Add device"
3. Wait for ESP32_BT_Server to appear in the list
4. Tap ESP32_BT_Server to pair (no PIN required)
5. Wait for "Connected" status

Step 2: Install a Bluetooth Terminal App
- Recommended apps:
  - "Serial Bluetooth Terminal" (by Kai Morich) - Most popular
  - "Bluetooth Terminal" (by Next Prototypes)
  - "BlueTerm" - Simple and reliable

Step 3: Connect and Send Commands
1. Open the Bluetooth terminal app
2. Tap "Connect" or "Devices"
3. Select ESP32_BT_Server from the paired devices list
4. Wait for connection (you'll see "Connected" status)
5. Type commands like `led on` and press Enter
6. You'll see responses from the ESP32

#### Windows Computer:

Step 1: Pair the Device
1. Open Settings → Bluetooth & devices
2. Click "Add device" → Bluetooth
3. Select ESP32_BT_Server when it appears
4. Click "Connect" (no PIN required)
5. Wait for "Connected" status

Step 2: Find the Bluetooth COM Port
1. Right-click "This PC" → Properties → Device Manager
2. Expand "Ports (COM & LPT)"
3. Look for "Standard Serial over Bluetooth link (COMX)"
4. Note the COM port number (e.g., COM5, COM8)

Step 3: Connect Using Terminal Software

Option A: Using PuTTY (Recommended)
1. Download PuTTY from https://putty.org/
2. Open PuTTY
3. Select "Serial" connection type
4. Enter the COM port (e.g., COM5)
5. Set Speed to 115200
6. Click "Open"
7. Type commands and press Enter

Option B: Using Windows Terminal/Command Prompt
1. Open Command Prompt as Administrator
2. Type: `mode COM5 BAUD=115200 PARITY=N DATA=8 STOP=1`
3. Type: `copy con COM5` 
4. Type your commands (Ctrl+Z to end)

Option C: Using Arduino IDE Serial Monitor
1. In Arduino IDE, go to Tools → Port
2. Select the Bluetooth COM port
3. Open Serial Monitor
4. Set baud rate to 115200
5. Send commands

#### Mac Computer:

Step 1: Pair the Device
1. Apple Menu → System Preferences → Bluetooth
2. Wait for ESP32_BT_Server to appear
3. Click "Connect" next to ESP32_BT_Server
4. Wait for "Connected" status

Step 2: Find the Bluetooth Device Path
1. Open Terminal
2. Type: `ls /dev/tty.*` 
3. Look for something like `/dev/tty.ESP32_BT_Server-ESP32SPP`
4. Note the exact path

Step 3: Connect Using Terminal
```bash
# Method 1: Using screen
screen /dev/tty.ESP32_BT_Server-ESP32SPP 115200

# Method 2: Using cu
cu -l /dev/tty.ESP32_BT_Server-ESP32SPP -s 115200

# Method 3: Using minicom (if installed)
minicom -D /dev/tty.ESP32_BT_Server-ESP32SPP
```

#### Linux Computer:

Step 1: Pair the Device
```bash
# Start bluetoothctl
bluetoothctl

# In bluetoothctl:
scan on
pair XX:XX:XX:XX:XX:XX  # ESP32's Bluetooth MAC
connect XX:XX:XX:XX:XX:XX
exit
```

Step 2: Create Serial Connection
```bash
# Bind to RFCOMM
sudo rfcomm bind rfcomm0 XX:XX:XX:XX:XX:XX

# Connect using screen
screen /dev/rfcomm0 115200

# Or using minicom
minicom -D /dev/rfcomm0
```

## � What is a Bluetooth Serial Terminal?

A Bluetooth Serial Terminal is an application that allows you to send and receive text data over a Bluetooth connection. It acts as a bridge between you and the Bluetooth device.

### How It Works:
```
┌─────────────────┐    Bluetooth     ┌─────────────────┐
│  Your Device    │    Serial        │      ESP32      │
│                 │    Protocol      │                 │
│ Terminal App    │ ◄────────────► │ BluetoothSerial │
│ (Phone/PC)      │    (SPP)         │   Library       │
│                 │                  │                 │
│ You type: "led on"                 │ Receives: "led on" │
│ You see: "LED turned ON"           │ Sends: "LED turned ON" │
└─────────────────┘                  └─────────────────┘
```

### Key Concepts:

#### 1. Bluetooth SPP (Serial Port Profile)
- SPP creates a virtual serial cable over Bluetooth
- Works exactly like a USB serial connection but wireless
- No special protocols - just plain text communication
- Bidirectional - you can send commands and receive responses

#### 2. COM Ports (Windows)
- Windows assigns a COM port number (like COM5) to Bluetooth connections
- Same as USB devices - treated identically by applications  
- Virtual port - no physical cable, but software sees it as real
- Multiple connections can have different COM numbers

#### 3. Device Files (Mac/Linux)
- Unix systems create device files like `/dev/tty.ESP32_BT_Server`
- Character devices that you can read from and write to
- Standard I/O - use normal file operations to communicate

### Popular Terminal Applications:

#### Mobile (Android/iOS):
- Serial Bluetooth Terminal - Full-featured with logging
- Bluetooth Terminal - Simple and clean interface  
- BlueTerm - Lightweight and fast
- BluetoothTerminalHC-05 - Specifically for HC-05 modules

#### Desktop (Windows/Mac/Linux):
- PuTTY - Professional terminal emulator
- Tera Term - Advanced features for debugging
- Arduino Serial Monitor - Built into Arduino IDE
- CoolTerm - Cross-platform with many features
- minicom/screen - Command-line tools for Unix systems

## �📡 Available Commands

Send these text commands via Bluetooth:

| Command | Description | Response |
|---------|-------------|----------|
| `led on` | Turn LED ON | "LED turned ON" |
| `led off` | Turn LED OFF | "LED turned OFF" |
| `status` | Get device info | Full status report |
| `help` | Show commands | Command list |

### Example Session:
```
> led on
LED turned ON

> status
=== Device Status ===
Device: ESP32
Bluetooth Name: ESP32_BT_Server
MAC Address: AA:BB:CC:DD:EE:FF
LED State: ON
Uptime: 145 seconds
Free Heap: 287432 bytes
Bluetooth Connected: Yes
====================

> led off
LED turned OFF
```

## 🔧 How It Works

### ESP32 Bluetooth Architecture:
```
┌─────────────────┐    Bluetooth     ┌─────────────────┐
│   Phone/PC      │    Classic       │      ESP32      │
│   Client App    │ ◄────────────► │  BluetoothSerial │
│                 │                  │                 │
└─────────────────┘                  └─────────────────┘
                                            │
                                     ┌─────────────┐
                                     │ LED Control │
                                     │   (GPIO 2)  │
                                     └─────────────┘
```

### Code Structure:

#### 1. Bluetooth Initialization:
```cpp
BluetoothSerial SerialBT;
SerialBT.begin("ESP32_BT_Server");
```

#### 2. Connection Events:
- `ESP_SPP_SRV_OPEN_EVT` - Client connected
- `ESP_SPP_CLOSE_EVT` - Client disconnected

#### 3. Command Processing:
- Receives text commands via `SerialBT.available()`
- Parses and executes commands
- Sends responses back via `SerialBT.println()`

#### 4. LED Control:
- Simple digital write to GPIO 2
- State tracking for status reporting

## 🛠️ Features

### What This Example Demonstrates:
- Bluetooth Classic setup on ESP32
- Device discovery and pairing
- Bidirectional communication (send commands, receive responses)
- Connection status monitoring
- Simple command parsing
- Hardware control via Bluetooth

### Key Benefits:
- No WiFi required - works offline
- Direct connection - no server needed
- Mobile friendly - easy to connect from phones
- Simple setup - no network configuration
- Low latency - direct device communication

## 🔍 Troubleshooting

### Common Issues:

#### 1. Bluetooth Not Found:
- Ensure ESP32 has Bluetooth support (not all ESP32 variants do)
- Check if Bluetooth is enabled in Arduino IDE board settings
- Restart ESP32 and try again
- Make sure ESP32 is not already paired to another device

#### 2. Connection Fails:
- Pairing vs Connection: Device must be paired first, then connected
- Clear Bluetooth cache: Android Settings → Apps → Bluetooth → Storage → Clear Cache
- On Windows: Remove device and re-pair
- Check if ESP32 is already connected to another device (only one connection at a time)

#### 3. COM Port Issues (Windows):
- COM port not appearing:
  - Restart Bluetooth service: `services.msc` → Bluetooth Support Service → Restart
  - Update Bluetooth drivers through Device Manager
  - Try unpairing and re-pairing the device

- "COM port in use" error:
  - Close other applications using the COM port
  - Check Task Manager for hidden serial applications
  - Restart computer to reset COM port assignments

- Finding the correct COM port:
  ```
  Device Manager → Ports (COM & LPT) → 
  Look for "Standard Serial over Bluetooth link (COMxx)"
  ```

#### 4. Terminal Connection Problems:
- No response to commands:
  - Check baud rate is set to 115200
  - Ensure line ending is set to "Newline" or "CR+LF"
  - Try typing commands in lowercase
  - Verify Bluetooth connection is active (check Serial Monitor)

- Garbled text:
  - Wrong baud rate - must be 115200
  - Check data bits (8), parity (None), stop bits (1)

#### 5. Commands Not Working:
- Ensure commands are sent as plain text (not binary)
- Check for extra spaces or special characters
- Commands are case-insensitive but avoid mixed case
- Use Serial Monitor to test commands locally first

#### 6. LED Not Responding:
- Verify LED connection (built-in LED on GPIO 2)
- Check if LED is working with other code (try Blink example)
- Some boards may have inverted LED logic (HIGH = OFF)

#### 7. Multiple Device Connections:
- ESP32 Bluetooth Classic supports only ONE connection at a time
- If another device is connected, new connections will fail
- Disconnect other devices before connecting new ones

### Debug Information:
- Serial Monitor shows all Bluetooth events
- Connection status is displayed
- Commands are echoed for verification

## 💡 Next Steps

This simple example can be extended with:

1. **JSON Communication** - Structured data exchange
2. **Multiple Commands** - More device control options
3. **Authentication** - PIN-based security
4. **File Transfer** - Send/receive files via Bluetooth
5. **BLE Version** - Lower power consumption variant
6. **Mobile App** - Custom Android/iOS application

## 🔗 Related Examples

- **MQTT**: [`../mqtt/`](../mqtt/) - Internet-based messaging
- **REST API**: [`../http-REST/`](../http-REST/) - HTTP communication
- **WebSocket**: [`../websocket/`](../websocket/) - Real-time web communication

---

This example provides the foundation for understanding ESP32 Bluetooth communication and can be built upon for more complex applications.