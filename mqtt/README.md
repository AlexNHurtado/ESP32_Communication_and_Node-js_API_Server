# ESP32 MQTT Communication Demo

This project demonstrates MQTT-based communication for controlling an ESP32 microcontroller device. Unlike REST APIs that use request-response patterns, MQTT uses a publish-subscribe messaging protocol that's ideal for IoT devices.

## Architecture Overview

```
┌─────────────┐         ┌──────────────┐         ┌──────────────┐
│   MQTT      │  MQTT   │   MQTT       │  MQTT   │    ESP32     │
│   Client    │ ◄─────► │   Broker     │ ◄─────► │    Device    │
│ (Publisher/ │         │ (HiveMQ/     │         │ (Publisher/  │
│ Subscriber) │         │  Mosquitto)  │         │ Subscriber)  │
└─────────────┘         └──────────────┘         └──────────────┘
```

### Communication Flow:

1. **Client → MQTT Broker**: Clients publish control messages to topics
2. **MQTT Broker → ESP32**: Broker forwards messages to subscribed ESP32
3. **ESP32 → MQTT Broker**: ESP32 publishes status updates and responses
4. **MQTT Broker → Clients**: Broker distributes ESP32 messages to subscribers

## MQTT vs REST Comparison

| Aspect | MQTT | REST |
|--------|------|------|
| **Communication** | Asynchronous pub/sub | Synchronous request/response |
| **Overhead** | Low (binary protocol) | Higher (HTTP headers) |
| **Real-time** | Excellent (push notifications) | Limited (polling required) |
| **Connection** | Persistent connection | Stateless |
| **Reliability** | Built-in QoS levels | Must implement separately |
| **Scalability** | High (broker handles routing) | Limited by server capacity |
| **Power Usage** | Lower (fewer connections) | Higher (frequent HTTP requests) |

## MQTT Topics Structure

### **Subscribe Topics (ESP32 listens):**
- `esp32/led/control` - LED control commands
- `esp32/device/command` - General device commands

### **Publish Topics (ESP32 sends):**
- `esp32/led/status` - LED state changes
- `esp32/device/status` - Complete device status

## Message Formats

### **LED Control (Subscribe: `esp32/led/control`)**
```json
{
  "state": true
}
```
```json
{
  "state": false
}
```

### **Device Commands (Subscribe: `esp32/device/command`)**
```
"status"
```
```
"restart"
```

### **LED Status Response (Publish: `esp32/led/status`)**
```json
{
  "success": true,
  "message": "LED ON",
  "led_state": true,
  "device_id": "AABBCCDDEEFF",
  "api_version": "1.0",
  "timestamp": 12345
}
```

### **Device Status Response (Publish: `esp32/device/status`)**
```json
{
  "device": "ESP32",
  "device_id": "AABBCCDDEEFF",
  "ip": "192.168.1.100",
  "ssid": "MyNetwork",
  "rssi": -45,
  "led_state": true,
  "uptime": 3600,
  "heap": 45000,
  "mqtt_connected": true,
  "api_version": "1.0"
}
```

## Setup Instructions

### **1. ESP32 Setup**

#### Requirements:
- ESP32 development board
- Arduino IDE with ESP32 board support
- Required libraries:
  ```
  WiFi (built-in)
  PubSubClient by Nick O'Leary
  ArduinoJson by Benoit Blanchon
  ```

#### Installation:
1. Install libraries via Library Manager:
   - PubSubClient
   - ArduinoJson

2. Open `ESP32_MQTT_Minimal.cpp` in Arduino IDE

3. Upload to ESP32

4. Open Serial Monitor (115200 baud)

5. Enter WiFi credentials when prompted

### **2. MQTT Broker**

#### Option A: Public Broker (Testing)
The code uses `broker.hivemq.com` by default - no setup required.

#### Option B: Local Broker (Production)
Install Mosquitto MQTT broker:

**Windows:**
```powershell
# Download from https://mosquitto.org/download/
# Or use Chocolatey:
choco install mosquitto
```

**Linux/Mac:**
```bash
sudo apt-get install mosquitto mosquitto-clients  # Ubuntu/Debian
brew install mosquitto                            # macOS
```

**Start broker:**
```bash
mosquitto -v
```

Update ESP32 code to use your broker:
```cpp
const char* MQTT_SERVER = "your-broker-ip";
```

## Testing the MQTT Communication

### **Using MQTT Client Tools**

#### **1. MQTT Explorer (GUI)**
Download from: http://mqtt-explorer.com/
- Connect to `broker.hivemq.com:1883`
- Subscribe to `esp32/#` to see all ESP32 messages
- Publish to control topics

#### **2. Mosquitto Command Line**

**Subscribe to all ESP32 topics:**
```bash
mosquitto_sub -h broker.hivemq.com -t "esp32/#" -v
```

**Control LED:**
```bash
# Turn LED ON
mosquitto_pub -h broker.hivemq.com -t "esp32/led/control" -m '{"state":true}'

# Turn LED OFF
mosquitto_pub -h broker.hivemq.com -t "esp32/led/control" -m '{"state":false}'
```

**Send device commands:**
```bash
# Request status
mosquitto_pub -h broker.hivemq.com -t "esp32/device/command" -m "status"

# Restart device
mosquitto_pub -h broker.hivemq.com -t "esp32/device/command" -m "restart"
```

#### **3. Python MQTT Client**

```python
import paho.mqtt.client as mqtt
import json
import time

# MQTT Configuration
BROKER = "broker.hivemq.com"
PORT = 1883

def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    # Subscribe to ESP32 status topics
    client.subscribe("esp32/led/status")
    client.subscribe("esp32/device/status")

def on_message(client, userdata, msg):
    topic = msg.topic
    message = msg.payload.decode()
    print(f"Received: {topic} -> {message}")

# Create MQTT client
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

# Connect to broker
client.connect(BROKER, PORT, 60)
client.loop_start()

# Control LED
def control_led(state):
    payload = json.dumps({"state": state})
    client.publish("esp32/led/control", payload)
    print(f"Sent LED command: {payload}")

def request_status():
    client.publish("esp32/device/command", "status")
    print("Requested device status")

# Example usage
try:
    while True:
        command = input("Enter command (on/off/status/quit): ").lower()
        if command == "on":
            control_led(True)
        elif command == "off":
            control_led(False)
        elif command == "status":
            request_status()
        elif command == "quit":
            break
        time.sleep(0.1)
        
except KeyboardInterrupt:
    pass

client.loop_stop()
client.disconnect()
```

#### **4. Node.js MQTT Client**

```bash
npm install mqtt
```

```javascript
const mqtt = require('mqtt');

// Connect to MQTT broker
const client = mqtt.connect('mqtt://broker.hivemq.com:1883');

client.on('connect', () => {
    console.log('Connected to MQTT broker');
    
    // Subscribe to ESP32 responses
    client.subscribe('esp32/led/status');
    client.subscribe('esp32/device/status');
    
    console.log('Subscribed to ESP32 topics');
});

client.on('message', (topic, message) => {
    console.log(`Received: ${topic} -> ${message.toString()}`);
});

// Control functions
function controlLED(state) {
    const payload = JSON.stringify({ state: state });
    client.publish('esp32/led/control', payload);
    console.log(`Sent LED command: ${payload}`);
}

function requestStatus() {
    client.publish('esp32/device/command', 'status');
    console.log('Requested device status');
}

// Example usage
setTimeout(() => controlLED(true), 1000);   // Turn LED ON after 1 second
setTimeout(() => controlLED(false), 3000);  // Turn LED OFF after 3 seconds
setTimeout(() => requestStatus(), 5000);    // Request status after 5 seconds
```

## Features

### **ESP32 MQTT Features:**
- ✅ **Persistent MQTT connection** with auto-reconnection
- ✅ **WiFi monitoring** and auto-reconnection
- ✅ **JSON message formatting** with standardized structure
- ✅ **Device identification** using MAC address
- ✅ **Periodic status publishing** (every 30 seconds)
- ✅ **Command handling** (status request, remote restart)
- ✅ **Error handling** and connection monitoring
- ✅ **Low memory footprint** optimized for ESP32

### **Advantages over REST:**
1. **Real-time communication** - instant notifications
2. **Lower power consumption** - persistent connection
3. **Better for multiple clients** - broker handles distribution
4. **Built-in reliability** - MQTT QoS levels
5. **Efficient protocol** - binary, low overhead
6. **Scalable** - add devices without server changes

### **Use Cases:**
- **Home automation systems**
- **IoT sensor networks** 
- **Real-time monitoring dashboards**
- **Mobile app notifications**
- **Multi-device coordination**

## Troubleshooting

### **Common Issues:**

1. **MQTT connection fails:**
   - Check broker address and port
   - Verify network connectivity
   - Try different broker (test with public broker first)

2. **Messages not received:**
   - Verify topic names match exactly
   - Check if ESP32 is connected to MQTT broker
   - Ensure JSON format is correct

3. **WiFi keeps disconnecting:**
   - Check signal strength (RSSI)
   - Verify 2.4GHz network (ESP32 doesn't support 5GHz)
   - Check power supply stability

4. **Memory issues:**
   - Reduce JSON document sizes if needed
   - Monitor heap usage in status messages
   - Avoid keeping large strings in memory

### **Monitoring:**
- Serial Monitor shows all connection events and messages
- Device publishes status every 30 seconds
- MQTT clients can subscribe to `esp32/#` for all messages

## Security Considerations

### **For Production Use:**
1. **Use secure broker** (MQTT over TLS/SSL)
2. **Authentication** (username/password or certificates)
3. **Authorization** (ACL rules for topic access)
4. **Private broker** (don't use public brokers for sensitive data)
5. **Message encryption** (encrypt JSON payloads if needed)

### **Example Secure Configuration:**
```cpp
// Use secure port
const int MQTT_PORT = 8883;

// Add authentication
const char* MQTT_USERNAME = "your_username";
const char* MQTT_PASSWORD = "your_password";

// In connectMqtt():
if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
    // Connection with authentication
}
```

This MQTT implementation provides a robust, efficient, and scalable communication method for IoT devices, offering significant advantages over REST APIs for real-time applications.