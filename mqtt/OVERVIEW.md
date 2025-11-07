# ESP32 MQTT Communication Example

A complete MQTT implementation for ESP32 microcontroller communication, organized in a clean folder structure.

## 📁 Folder Structure

```
mqtt/
├── ESP32_MQTT_Minimal.cpp     # ESP32 Arduino code
├── README.md                  # Complete documentation and guide
├── requirements.txt           # Python dependencies
├── package.json              # Node.js dependencies
└── clients/                   # Test client applications
    ├── mqtt-test-client.py    # Python MQTT test client
    └── mqtt-test-client.js    # Node.js MQTT test client
```

## 🚀 Quick Start

### **1. ESP32 Setup**
1. Open `ESP32_MQTT_Minimal.cpp` in Arduino IDE
2. Install required libraries:
   - `PubSubClient` by Nick O'Leary
   - `ArduinoJson` by Benoit Blanchon
3. Upload to ESP32 and enter WiFi credentials

### **2. Test with Python Client**
```powershell
cd mqtt
pip install -r requirements.txt
python clients/mqtt-test-client.py
```

### **3. Test with Node.js Client**
```powershell
cd mqtt
npm install
npm start
```

## 🔧 Features

### **ESP32 MQTT Device:**
- ✅ **WiFi auto-connection** with credential input
- ✅ **MQTT auto-reconnection** with retry logic
- ✅ **LED control** via MQTT messages
- ✅ **Status publishing** every 30 seconds
- ✅ **Command handling** (status request, remote restart)
- ✅ **JSON standardization** with improved format
- ✅ **Device identification** using MAC address
- ✅ **Memory optimization** for ESP32 constraints

### **Test Clients:**
- ✅ **Real-time messaging** with timestamp display
- ✅ **Interactive CLI** for easy testing
- ✅ **Connection monitoring** and error handling
- ✅ **Clean output** without emojis for professional use

## 📡 MQTT Topics

| Topic | Direction | Purpose | Format |
|-------|-----------|---------|---------|
| `esp32/led/control` | Subscribe | LED control | `{"state": true/false}` |
| `esp32/device/command` | Subscribe | Device commands | `"status"` or `"restart"` |
| `esp32/led/status` | Publish | LED state changes | JSON response |
| `esp32/device/status` | Publish | Full device status | JSON with device info |

## 🎯 MQTT vs REST Benefits

| Advantage | Description |
|-----------|-------------|
| **Real-time** | Instant push notifications, no polling |
| **Efficient** | Lower bandwidth and power consumption |
| **Scalable** | Multiple clients without server overhead |
| **Reliable** | Built-in QoS levels and persistent connections |
| **Bidirectional** | ESP32 can push updates to all clients |

## 📖 Full Documentation

See [`mqtt/README.md`](mqtt/README.md) for complete setup instructions, message formats, troubleshooting, and advanced configuration options.

## 🔗 Related Examples

- **REST API**: [`http-REST/`](http-REST/) - HTTP-based communication
- **WebSocket**: [`websocket/`](websocket/) - Real-time web communication
- **Hybrid**: [`ESP32_Hybrid_REST_WebSocket.cpp`](ESP32_Hybrid_REST_WebSocket.cpp) - Combined approach

---
*Part of the ESP32 Communication Examples collection*