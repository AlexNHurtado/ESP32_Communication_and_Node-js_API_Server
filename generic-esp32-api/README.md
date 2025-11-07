# Generic ESP32 REST API Server

A universal REST API server that accepts connections from any ESP32 device, logs all data to MongoDB, and provides 4 dedicated endpoints for sending data back to ESP32 devices.

## 🎯 Features

### Core Functionality
- **Universal ESP32 Support**: Accepts data from any ESP32 device
- **MongoDB Logging**: Automatically logs all incoming data and API activities
- **Device Registry**: Maintains a registry of connected ESP32 devices
- **4 Communication Endpoints**: Send commands, config, updates, and custom data to ESP32s
- **Real-time Statistics**: Monitor API usage and device connections
- **Error Handling**: Comprehensive error handling and logging
- **CORS Enabled**: Cross-origin requests supported
- **External Network Access**: Listens on all network interfaces (0.0.0.0)

### 🔐 Security & Authentication (NEW!)
- **Device Whitelist**: Only registered devices can submit data
- **Duplicate Prevention**: Blocks multiple registrations of same device ID
- **Rate Limiting**: Protects against spam registration attempts
- **IP Blacklisting**: Automatic and manual IP blocking capabilities
- **Access Control**: Header-based authentication (X-ESP32-ID required)
- **Activity Tracking**: Monitors device submissions and last activity
- **Configuration Management**: Real-time authentication settings updates

## 🏗️ Architecture

```
ESP32 Devices ←→ Generic REST API Server ←→ MongoDB Database
     ↑                    ↑                      ↑
   Any ESP32          Node.js/Express      Data Logging
  (Sensors,           (Port 3001)         (localhost:27017)
   Actuators,
   Controllers)
```

## 📋 Prerequisites

- **Node.js** (v14 or higher)
- **MongoDB** running on localhost:27017
- **ESP32 devices** with WiFi capability

## 🚀 Quick Start

### 1. Install Dependencies
```bash
cd generic-esp32-api
npm install
```

### 2. Configure Environment
Edit the `.env` file or use the defaults:
```env
PORT=3001
MONGODB_URI=mongodb://localhost:27017
MONGODB_DATABASE=generic_esp32_iot
MONGODB_COLLECTION=device_data
```

### 3. Start MongoDB
```bash
# Windows
net start MongoDB

# macOS/Linux
brew services start mongodb-community
# or
sudo systemctl start mongod
```

### 4. Start the Server
```bash
npm start
```

The server will start on:
- **Local Access**: `http://localhost:3001`
- **External Access**: `http://YOUR-IP:3001` (automatically detected and displayed)
- **Dashboard**: `http://localhost:3001/dashboard` or `http://YOUR-IP:3001/dashboard`

**Note**: The server listens on `0.0.0.0` (all network interfaces) to accept connections from any IP address, including external ESP32 devices on your network.

## 📡 API Endpoints

### Core Endpoints

#### 1. Register ESP32 Device
**POST** `/register`
```json
{
  "device_id": "esp32_sensor_01",
  "ip": "192.168.1.100",
  "port": 80,
  "device_info": {
    "type": "sensor",
    "location": "kitchen",
    "firmware": "1.0.0"
  }
}
```

#### 2. Receive Data from ESP32 🔐 (Whitelist Protected)
**POST** `/data`
- **REQUIRED Header**: `X-ESP32-ID: your_device_id` (must be registered)
- **Authentication**: Device must be registered via `/register` endpoint first
- Body: Any JSON data
```json
{
  "temperature": 25.6,
  "humidity": 60.2,
  "sensor_status": "ok",
  "timestamp": "2024-01-01T12:00:00Z"
}
```

**Error Responses:**
- `403`: Device not registered (requires registration first)
- `400`: Missing X-ESP32-ID header

#### 3. List Registered Devices
**GET** `/devices`

### ESP32 Communication Endpoints (4 endpoints as requested)

#### 1. Send Commands
**POST** `/send/command`
```json
{
  "device_id": "esp32_001",
  "command": "turn_led",
  "parameters": {
    "state": true,
    "brightness": 255,
    "color": "red"
  },
  "endpoint": "/command"
}
```

#### 2. Send Configuration
**POST** `/send/config`
```json
{
  "device_id": "esp32_001",
  "config": {
    "wifi_ssid": "NewNetwork",
    "sensor_interval": 30000,
    "mqtt_broker": "192.168.1.1",
    "deep_sleep_duration": 600
  },
  "endpoint": "/config"
}
```

#### 3. Send Updates/Firmware
**POST** `/send/update`
```json
{
  "device_id": "esp32_001",
  "update_info": {
    "firmware_version": "2.1.0",
    "download_url": "https://example.com/firmware.bin",
    "checksum": "sha256:abc123...",
    "force_update": false
  },
  "endpoint": "/update"
}
```

#### 4. Send Custom Data
**POST** `/send/custom`
```json
{
  "device_id": "esp32_001",
  "data": {
    "message": "Hello ESP32",
    "action": "display_text",
    "parameters": {
      "color": "blue",
      "duration": 5000
    }
  },
  "endpoint": "/custom",
  "method": "POST"
}
```

### Authentication Management Endpoints 🔐

#### Get Authentication Status
**GET** `/auth/status`
- Returns system statistics, configuration, and health info

#### Update Authentication Configuration
**PUT** `/auth/config`
```json
{
  "maxRegistrationAttempts": 5,
  "enableWhitelist": true,
  "requireUniqueIPs": false
}
```

#### Unregister Device
**DELETE** `/devices/:deviceId`
```json
{
  "reason": "Device decommissioned"
}
```

#### Blacklist IP Address
**POST** `/auth/blacklist`
```json
{
  "ip": "192.168.1.200",
  "reason": "Suspicious activity"
}
```

#### Remove IP from Blacklist
**DELETE** `/auth/blacklist/:ip`

#### Clear Authentication Data
**DELETE** `/auth/reset`
```json
{
  "component": "devices|blacklist|tokens",
  "confirm": true
}
```

### Monitoring Endpoints

#### Get API Statistics
**GET** `/stats`

#### Get API Documentation
**GET** `/`

## 🔧 ESP32 Integration Examples

### ESP32 Code with Authentication 🔐

#### 1. Register Device First (Required)
```cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

bool registerDevice() {
  HTTPClient http;
  http.begin("http://192.168.1.10:3001/register");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(1024);
  doc["device_id"] = "esp32_sensor_01";  // Must be unique!
  doc["ip"] = WiFi.localIP().toString();
  doc["port"] = 80;
  
  JsonObject deviceInfo = doc.createNestedObject("device_info");
  deviceInfo["type"] = "temperature_sensor";
  deviceInfo["location"] = "kitchen";
  deviceInfo["firmware"] = "1.0.0";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode == 200) {
    Serial.println("✅ Device registered successfully!");
    return true;
  } else if (httpResponseCode == 409) {
    Serial.println("⚠️ Device already registered (OK if same device)");
    return true;  // Might be re-registration from same IP
  } else {
    Serial.printf("❌ Registration failed: %d\n", httpResponseCode);
    return false;
  }
  
  http.end();
}

#### 2. Send Data (After Registration)
void sendDataToAPI() {
  HTTPClient http;
  http.begin("http://192.168.1.10:3001/data");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-ESP32-ID", "esp32_sensor_01");  // REQUIRED!
  
  DynamicJsonDocument doc(1024);
  doc["temperature"] = 25.6;
  doc["humidity"] = 60.2;
  doc["device_status"] = "online";
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("✅ Data sent successfully: " + response);
  } else if (httpResponseCode == 403) {
    Serial.println("❌ Device not authorized - registering...");
    if (registerDevice()) {
      // Retry sending data
      sendDataToAPI();
    }
  } else {
    Serial.printf("❌ Error sending data: %d\n", httpResponseCode);
  }
  
  http.end();
}

#### 3. Complete Setup Flow
void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");
  
  // Register with API (required for data submission)
  if (!registerDevice()) {
    Serial.println("Failed to register, will retry later");
  }
  
  setupWebServer();  // Setup for receiving commands
}
```

### ESP32 Code to Handle Commands
```cpp
void handleCommand() {
  // This would be called when API sends POST to /command
  server.on("/command", HTTP_POST, []() {
    String body = server.arg("plain");
    
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, body);
    
    String command = doc["command"];
    JsonObject parameters = doc["parameters"];
    
    if (command == "turn_led") {
      bool state = parameters["state"];
      int brightness = parameters["brightness"];
      // Execute LED command
      digitalWrite(LED_PIN, state);
    }
    
    server.send(200, "application/json", "{\"success\": true, \"executed\": \"" + command + "\"}");
  });
}
```

## 📊 MongoDB Data Structure

### Device Registration Log
```json
{
  "_id": "ObjectId(...)",
  "event_type": "device_registration",
  "device_id": "esp32_sensor_01",
  "ip": "192.168.1.100",
  "port": 80,
  "device_info": { ... },
  "client_ip": "192.168.1.100",
  "logged_at": "2024-01-01T12:00:00Z",
  "logged_timestamp": 1704110400000
}
```

### Data Received Log
```json
{
  "_id": "ObjectId(...)",
  "event_type": "data_received",
  "device_id": "esp32_sensor_01",
  "client_ip": "192.168.1.100",
  "payload": {
    "temperature": 25.6,
    "humidity": 60.2
  },
  "payload_size": 45,
  "logged_at": "2024-01-01T12:00:00Z"
}
```

### Command Sent Log
```json
{
  "_id": "ObjectId(...)",
  "event_type": "command_sent",
  "device_id": "esp32_001",
  "command": "turn_led",
  "parameters": { "state": true },
  "result": true,
  "response": { "success": true },
  "logged_at": "2024-01-01T12:00:00Z"
}
```

## 🔍 Usage Examples

### Register a New ESP32 Device
```bash
curl -X POST http://localhost:3001/register \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "esp32_kitchen_sensor",
    "ip": "192.168.1.105",
    "port": 80,
    "device_info": {
      "type": "temperature_sensor",
      "location": "kitchen",
      "firmware": "1.2.0"
    }
  }'
```

### Send Data from ESP32
```bash
curl -X POST http://localhost:3001/data \
  -H "Content-Type: application/json" \
  -H "X-ESP32-ID: esp32_kitchen_sensor" \
  -d '{
    "temperature": 23.5,
    "humidity": 65.1,
    "battery_level": 87,
    "signal_strength": -42
  }'
```

### Send Command to ESP32
```bash
curl -X POST http://localhost:3001/send/command \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "esp32_kitchen_sensor",
    "command": "take_reading",
    "parameters": {
      "sensor_type": "all",
      "interval": 5000
    }
  }'
```

### Get Device Statistics
```bash
curl http://localhost:3001/stats
```

## 🛠️ Configuration Options

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `PORT` | 3001 | Server port |
| `MONGODB_URI` | mongodb://localhost:27017 | MongoDB connection string |
| `MONGODB_DATABASE` | generic_esp32_iot | Database name |
| `MONGODB_COLLECTION` | device_data | Collection name |
| `MAX_PAYLOAD_SIZE` | 10mb | Maximum request payload size |
| `REQUEST_TIMEOUT` | 30000 | Timeout for ESP32 requests (ms) |
| `LOG_REQUESTS` | true | Log all incoming requests |

## 🔒 Security Considerations

- **External Access**: Server accepts connections from any IP (0.0.0.0)
  - ⚠️ **Important**: This allows external devices to access your API
  - Use firewall rules to restrict access to trusted networks only
  - Consider VPN or network isolation for production environments
- **Network Security**: Configure firewalls appropriately
- **API Keys**: Implement API key authentication if needed
- **Input Validation**: All inputs are validated and sanitized
- **Rate Limiting**: Consider implementing rate limiting for production
- **MongoDB Security**: Enable authentication for production deployments
- **CORS Policy**: Currently set to allow all origins (*) - restrict in production

## 🚨 Troubleshooting

### MongoDB Connection Issues
```bash
# Check if MongoDB is running
mongo --eval "db.adminCommand('ismaster')"

# Check logs
tail -f /var/log/mongodb/mongod.log
```

### ESP32 Communication Issues
- Verify ESP32 IP addresses are correct
- Check network connectivity between server and ESP32s
- Ensure ESP32 devices are accepting POST requests
- Verify firewall settings

### Common Error Responses
- **400**: Bad request (missing required fields)
- **503**: Service unavailable (device not reachable)
- **500**: Internal server error (check logs)

## � Authentication & Security Features

### Device Whitelisting
The API now uses registered devices as a **whitelist** for data submission:
- Only devices registered via `/register` can submit data via `/data`
- Prevents unauthorized data pollution from unknown devices
- Requires `X-ESP32-ID` header matching a registered device ID

### Duplicate Prevention
- **Unique Device IDs**: Each device ID can only be registered once
- **IP Validation**: Blocks duplicate device IDs from different IP addresses
- **Re-registration Support**: Allows same device to re-register from same IP

### Rate Limiting & Security
- **Registration Rate Limiting**: Prevents spam registration attempts
- **IP Blacklisting**: Automatic blocking of persistent attackers
- **Configurable Limits**: Adjustable thresholds via `/auth/config`
- **Activity Tracking**: Monitors device submission counts and last activity

### Authentication Management
- **Device Management**: Unregister devices via `DELETE /devices/:id`
- **Blacklist Control**: Manually blacklist/unblacklist IP addresses
- **System Statistics**: Monitor authentication status via `/auth/status`
- **Configuration Updates**: Real-time setting changes via `/auth/config`

For detailed authentication documentation, see [`AUTHENTICATION.md`](./AUTHENTICATION.md)

## �📈 Monitoring

Access real-time statistics at:
- **API Documentation**: `GET http://localhost:3001/`
- **Device List**: `GET http://localhost:3001/devices`
- **Statistics**: `GET http://localhost:3001/stats`
- **Authentication Status**: `GET http://localhost:3001/auth/status`

## 🔄 Data Flow

1. **ESP32 Registration**: Device registers with API providing IP and info
2. **Data Transmission**: ESP32 sends sensor data to `/data` endpoint
3. **MongoDB Logging**: All data automatically logged with timestamps
4. **Command Transmission**: API sends commands/config to ESP32 devices
5. **Response Logging**: All responses and errors logged for monitoring

This generic API provides a flexible foundation for any ESP32 IoT project with comprehensive logging and bidirectional communication capabilities.