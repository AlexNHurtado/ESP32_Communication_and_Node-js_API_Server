# Device Authentication and Whitelist Module Documentation

## Overview
The Generic ESP32 API now includes a comprehensive authentication and whitelist system that:
- **Prevents duplicate device registrations** with the same device ID
- **Uses registered devices as a whitelist** for data submission
- **Implements rate limiting** and IP blacklisting for security
- **Tracks device activity** and provides detailed statistics

## Key Features

### 🔐 Device Registration Control
- **Unique Device IDs**: Each device ID can only be registered once
- **IP-based Rate Limiting**: Prevents spam registration attempts
- **Duplicate Detection**: Blocks multiple registrations of the same device ID from different IPs
- **Re-registration Support**: Allows same device to re-register from same IP (for restarts)

### 📋 Whitelist Enforcement
- **Data Submission Control**: Only registered devices can submit data via `/data` endpoint
- **Header Validation**: Requires `X-ESP32-ID` header matching a registered device
- **IP Validation**: Optional strict mode to verify IP matches registration
- **Activity Tracking**: Monitors submission count and last activity per device

### 🚫 Security Features
- **IP Blacklisting**: Automatic and manual IP blocking
- **Rate Limiting**: Configurable limits on registration attempts
- **Auth Tokens**: Generated tokens for future enhanced authentication
- **Registration Cooldowns**: Time-based limits on failed attempts

## API Changes

### Modified Endpoints

#### POST /register
**Enhanced with duplicate prevention and rate limiting**
```bash
curl -X POST http://localhost:3001/register \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "esp32_sensor_01",
    "ip": "192.168.1.100",
    "port": 80,
    "device_info": {
      "type": "temperature_sensor",
      "location": "kitchen",
      "firmware": "1.0.0"
    }
  }'
```

**Success Response:**
```json
{
  "success": true,
  "message": "Device registered successfully",
  "device": {
    "deviceId": "esp32_sensor_01",
    "ip": "192.168.1.100",
    "port": 80,
    "registeredAt": "2024-01-15T10:30:00.000Z",
    "status": "active"
  },
  "auth_token": "abc123def456...",
  "total_devices": 1
}
```

**Error Responses:**
```json
// Duplicate device ID from different IP
{
  "success": false,
  "error": "Device ID 'esp32_sensor_01' is already registered from different IP (192.168.1.99)",
  "device_id": "esp32_sensor_01"
}

// Too many registration attempts
{
  "success": false,
  "error": "Too many registration attempts",
  "retry_after": 300
}
```

#### POST /data
**Now requires device to be registered (whitelisted)**
```bash
curl -X POST http://localhost:3001/data \
  -H "Content-Type: application/json" \
  -H "X-ESP32-ID: esp32_sensor_01" \
  -d '{
    "temperature": 25.6,
    "humidity": 60.2,
    "timestamp": "2024-01-15T10:30:00Z"
  }'
```

**Success Response:**
```json
{
  "success": true,
  "message": "Data received and logged successfully",
  "device_id": "esp32_sensor_01",
  "device_status": "active",
  "submission_count": 42,
  "mongodb_log_id": "65a5b2c8f1e2d3a4b5c6d7e8"
}
```

**Error Responses:**
```json
// Missing device ID header
{
  "success": false,
  "error": "Missing X-ESP32-ID header",
  "required_header": "X-ESP32-ID"
}

// Device not registered
{
  "success": false,
  "error": "Device not authorized",
  "device_id": "unregistered_device",
  "reason": "Device 'unregistered_device' is not registered or authorized",
  "message": "Only registered devices can submit data. Please register first at POST /register"
}
```

### New Management Endpoints

#### DELETE /devices/:deviceId
**Unregister a device from the whitelist**
```bash
curl -X DELETE http://localhost:3001/devices/esp32_sensor_01 \
  -H "Content-Type: application/json" \
  -d '{"reason": "Device decommissioned"}'
```

#### GET /auth/status
**Get authentication system status and statistics**
```bash
curl http://localhost:3001/auth/status
```

**Response:**
```json
{
  "success": true,
  "authentication_enabled": true,
  "statistics": {
    "totalRegistered": 5,
    "activeDevices": 3,
    "blacklistedIPs": 2,
    "pendingTokens": 5,
    "config": {
      "maxRegistrationAttempts": 5,
      "registrationCooldown": 300000,
      "enableWhitelist": true,
      "requireUniqueIPs": false
    }
  },
  "system_health": {
    "whitelist_active": true,
    "devices_registered": 5,
    "active_devices": 3,
    "blacklisted_ips": 2
  }
}
```

#### POST /auth/blacklist
**Manually blacklist an IP address**
```bash
curl -X POST http://localhost:3001/auth/blacklist \
  -H "Content-Type: application/json" \
  -d '{
    "ip": "192.168.1.200",
    "reason": "Suspicious activity detected"
  }'
```

#### PUT /auth/config
**Update authentication configuration**
```bash
curl -X PUT http://localhost:3001/auth/config \
  -H "Content-Type: application/json" \
  -d '{
    "maxRegistrationAttempts": 3,
    "enableWhitelist": true,
    "requireUniqueIPs": true
  }'
```

## ESP32 Integration

### Required Changes for ESP32 Code

#### 1. Registration Process
```cpp
bool registerWithAPI() {
  HTTPClient http;
  http.begin("http://192.168.1.10:3001/register");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(1024);
  doc["device_id"] = "esp32_kitchen_01";  // Must be unique
  doc["ip"] = WiFi.localIP().toString();
  doc["port"] = 80;
  
  JsonObject deviceInfo = doc.createNestedObject("device_info");
  deviceInfo["type"] = "temperature_sensor";
  deviceInfo["location"] = "kitchen";
  deviceInfo["firmware"] = "2.1.0";
  
  String payload;
  serializeJson(doc, payload);
  
  int responseCode = http.POST(payload);
  
  if (responseCode == 200) {
    String response = http.getString();
    DynamicJsonDocument responseDoc(1024);
    deserializeJson(responseDoc, response);
    
    if (responseDoc["success"]) {
      // Store auth token for future use
      String authToken = responseDoc["auth_token"];
      // Save to EEPROM/preferences for persistence
      Serial.println("✅ Registration successful!");
      return true;
    }
  } else if (responseCode == 409) {
    Serial.println("❌ Device ID already registered");
  } else if (responseCode == 429) {
    Serial.println("❌ Too many registration attempts, wait before retry");
  }
  
  http.end();
  return false;
}
```

#### 2. Data Submission (Updated)
```cpp
bool sendDataToAPI() {
  HTTPClient http;
  http.begin("http://192.168.1.10:3001/data");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-ESP32-ID", "esp32_kitchen_01");  // REQUIRED!
  
  DynamicJsonDocument doc(1024);
  doc["temperature"] = 25.6;
  doc["humidity"] = 62.3;
  doc["timestamp"] = millis();
  
  String payload;
  serializeJson(doc, payload);
  
  int responseCode = http.POST(payload);
  
  if (responseCode == 200) {
    Serial.println("✅ Data sent successfully");
    return true;
  } else if (responseCode == 403) {
    Serial.println("❌ Device not authorized - need to register first");
    // Attempt re-registration
    registerWithAPI();
  } else if (responseCode == 400) {
    Serial.println("❌ Missing X-ESP32-ID header");
  }
  
  http.end();
  return false;
}
```

#### 3. Complete ESP32 Flow
```cpp
void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  // Register with API server
  if (!registerWithAPI()) {
    Serial.println("Failed to register, will retry later");
  }
  
  // Setup web server for receiving commands
  setupWebServer();
}

void loop() {
  // Send data every 30 seconds
  if (millis() - lastDataSend >= 30000) {
    if (!sendDataToAPI()) {
      Serial.println("Data send failed, will retry");
    }
    lastDataSend = millis();
  }
  
  // Handle incoming commands
  server.handleClient();
  delay(100);
}
```

## Configuration Options

### Authentication Settings
The authentication system can be configured via the `/auth/config` endpoint:

```json
{
  "maxRegistrationAttempts": 5,        // Max attempts per IP before cooldown
  "registrationCooldown": 300000,      // Cooldown period in milliseconds (5 min)
  "tokenExpiryTime": 86400000,         // Auth token expiry in milliseconds (24h)
  "requireUniqueIPs": false,           // Require devices to have unique IPs
  "enableWhitelist": true              // Enable/disable whitelist enforcement
}
```

### Disabling Whitelist (for testing)
```bash
curl -X PUT http://localhost:3001/auth/config \
  -H "Content-Type: application/json" \
  -d '{"enableWhitelist": false}'
```

## Security Benefits

### 1. Duplicate Prevention
- Prevents malicious devices from hijacking existing device IDs
- Ensures data integrity by maintaining device identity
- Allows legitimate device restarts while blocking imposters

### 2. Rate Limiting
- Protects against registration spam attacks
- Automatically blacklists persistent attackers
- Configurable thresholds and cooldown periods

### 3. Access Control
- Only registered devices can submit data
- Eliminates unauthorized data pollution
- Provides audit trail of all device activities

### 4. Monitoring and Management
- Real-time statistics on device activity
- Manual device management capabilities
- IP blacklisting for problematic sources

## Migration from Previous Version

### For Existing ESP32 Devices
1. **Add Registration Call**: Implement device registration in setup()
2. **Add Device ID Header**: Include X-ESP32-ID in all data requests
3. **Handle Auth Errors**: Implement retry logic for 403 responses
4. **Update Error Handling**: Check for new error codes (409, 429)

### For Server Administrators
1. **Monitor Auth Status**: Use `/auth/status` to monitor system health
2. **Configure Settings**: Adjust rate limits via `/auth/config` as needed
3. **Manage Devices**: Use `/devices/:id` DELETE to remove problematic devices
4. **Monitor Logs**: Check MongoDB logs for authentication events

## Troubleshooting

### Common Issues

#### Device Registration Fails (409 Conflict)
- **Cause**: Device ID already registered from different IP
- **Solution**: Use unique device ID or unregister existing device
- **Check**: GET /devices to see registered devices

#### Data Submission Fails (403 Forbidden)
- **Cause**: Device not registered or missing X-ESP32-ID header
- **Solution**: Register device first, ensure header is included
- **Check**: Verify device appears in GET /devices response

#### Too Many Registration Attempts (429)
- **Cause**: IP hit rate limit
- **Solution**: Wait for cooldown period or contact administrator
- **Check**: Use GET /auth/status to see current limits

#### IP Blacklisted
- **Cause**: Excessive failed registration attempts
- **Solution**: Contact administrator to remove from blacklist
- **Admin**: Use DELETE /auth/blacklist/:ip to remove

### Debug Commands

```bash
# Check device registration status
curl http://localhost:3001/devices

# Check authentication system status
curl http://localhost:3001/auth/status

# Test data submission
curl -X POST http://localhost:3001/data \
  -H "Content-Type: application/json" \
  -H "X-ESP32-ID: your_device_id" \
  -d '{"test": "data"}'

# Check MongoDB logs for authentication events
# Look for event_type: "device_registration" and "data_received"
```

This authentication system provides a robust foundation for managing ESP32 device access while maintaining compatibility with existing functionality.