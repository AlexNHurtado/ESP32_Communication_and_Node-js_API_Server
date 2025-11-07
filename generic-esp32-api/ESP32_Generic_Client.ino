/*
 * ESP32 Generic API Client Example
 * 
 * This example demonstrates how to:
 * 1. Register with the Generic ESP32 API
 * 2. Send sensor data periodically
 * 3. Handle incoming commands/config/updates
 * 4. Respond to custom requests
 * 
 * Compatible with the Generic ESP32 REST API server
 * API Server: http://your-server-ip:3001
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// API Server Configuration  
const char* apiServer = "192.168.1.10";  // Change to your server's IP address
const int apiPort = 3001;                 // Default port (change if modified in .env)
const String deviceId = "esp32_generic_001";  // Unique device identifier

// Note: The server listens on all network interfaces (0.0.0.0)
// You can use:
// - Local network IP (192.168.x.x, 10.x.x.x, etc.)
// - Public IP (if server is accessible from internet)
// - localhost/127.0.0.1 (if ESP32 and server are on same machine)

// Hardware Configuration
const int LED_PIN = 2;
const int SENSOR_PIN = A0;  // Analog sensor pin
const int BUTTON_PIN = 0;   // Boot button

// Timing Configuration
unsigned long lastDataSend = 0;
unsigned long lastHeartbeat = 0;
const unsigned long DATA_INTERVAL = 30000;    // Send data every 30 seconds
const unsigned long HEARTBEAT_INTERVAL = 60000; // Heartbeat every minute

// Device State
struct DeviceState {
  bool ledState = false;
  int sensorInterval = 30000;
  String deviceLocation = "unknown";
  String firmwareVersion = "1.0.0";
  bool deepSleepEnabled = false;
  int batteryLevel = 100;
} deviceState;

// Web Server for receiving API commands
WebServer server(80);

// ============================================
// WiFi Functions
// ============================================

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected successfully!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength: ");
    Serial.println(WiFi.RSSI());
  } else {
    Serial.println();
    Serial.println("Failed to connect to WiFi!");
  }
}

// ============================================
// API Communication Functions
// ============================================

bool registerWithAPI() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, cannot register");
    return false;
  }

  HTTPClient http;
  String url = String("http://") + apiServer + ":" + apiPort + "/register";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  // Create registration payload
  DynamicJsonDocument doc(1024);
  doc["device_id"] = deviceId;
  doc["ip"] = WiFi.localIP().toString();
  doc["port"] = 80;
  
  JsonObject deviceInfo = doc.createNestedObject("device_info");
  deviceInfo["type"] = "sensor_controller";
  deviceInfo["location"] = deviceState.deviceLocation;
  deviceInfo["firmware"] = deviceState.firmwareVersion;
  deviceInfo["capabilities"] = "led_control,sensor_reading,remote_config";
  deviceInfo["mac_address"] = WiFi.macAddress();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.println("Registering with API server...");
  Serial.println("URL: " + url);
  Serial.println("Payload: " + jsonString);
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Registration response: " + response);
    
    // Parse response
    DynamicJsonDocument responseDoc(1024);
    deserializeJson(responseDoc, response);
    
    if (responseDoc["success"]) {
      Serial.println("✅ Successfully registered with API!");
      http.end();
      return true;
    } else {
      Serial.println("❌ Registration failed: " + responseDoc["error"].as<String>());
    }
  } else {
    Serial.println("❌ HTTP Error: " + String(httpResponseCode));
  }
  
  http.end();
  return false;
}

bool sendDataToAPI() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  HTTPClient http;
  String url = String("http://") + apiServer + ":" + apiPort + "/data";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-ESP32-ID", deviceId);
  
  // Read sensor data
  int sensorValue = analogRead(SENSOR_PIN);
  float voltage = sensorValue * (3.3 / 4095.0);
  bool buttonPressed = digitalRead(BUTTON_PIN) == LOW;
  
  // Create data payload
  DynamicJsonDocument doc(1024);
  doc["device_id"] = deviceId;
  doc["timestamp"] = millis();
  doc["uptime_seconds"] = millis() / 1000;
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["led_state"] = deviceState.ledState;
  
  // Sensor data
  JsonObject sensors = doc.createNestedObject("sensors");
  sensors["analog_value"] = sensorValue;
  sensors["voltage"] = voltage;
  sensors["button_pressed"] = buttonPressed;
  sensors["temperature"] = random(200, 300) / 10.0; // Simulated temperature
  sensors["humidity"] = random(400, 800) / 10.0;    // Simulated humidity
  
  // Device status
  JsonObject status = doc.createNestedObject("status");
  status["battery_level"] = deviceState.batteryLevel;
  status["sensor_interval"] = deviceState.sensorInterval;
  status["location"] = deviceState.deviceLocation;
  status["firmware_version"] = deviceState.firmwareVersion;
  status["deep_sleep_enabled"] = deviceState.deepSleepEnabled;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.println("Sending data to API...");
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Data sent successfully: " + response);
    http.end();
    return true;
  } else {
    Serial.println("❌ Failed to send data: " + String(httpResponseCode));
  }
  
  http.end();
  return false;
}

// ============================================
// Web Server Endpoints (Receive from API)
// ============================================

void handleCommand() {
  Serial.println("📨 Received command from API");
  
  String body = server.arg("plain");
  Serial.println("Command payload: " + body);
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(400, "application/json", "{\"success\": false, \"error\": \"Invalid JSON\"}");
    return;
  }
  
  String command = doc["command"];
  JsonObject parameters = doc["parameters"];
  
  DynamicJsonDocument response(512);
  response["success"] = true;
  response["device_id"] = deviceId;
  response["command"] = command;
  response["executed_at"] = millis();
  
  // Execute commands
  if (command == "turn_led") {
    bool state = parameters["state"] | false;
    deviceState.ledState = state;
    digitalWrite(LED_PIN, state ? HIGH : LOW);
    response["led_state"] = state;
    Serial.println("💡 LED turned " + String(state ? "ON" : "OFF"));
    
  } else if (command == "get_sensor_reading") {
    int sensorValue = analogRead(SENSOR_PIN);
    response["sensor_value"] = sensorValue;
    response["voltage"] = sensorValue * (3.3 / 4095.0);
    Serial.println("📊 Sensor reading: " + String(sensorValue));
    
  } else if (command == "set_interval") {
    int newInterval = parameters["interval"] | deviceState.sensorInterval;
    deviceState.sensorInterval = newInterval;
    response["new_interval"] = newInterval;
    Serial.println("⏱️ Sensor interval changed to: " + String(newInterval) + "ms");
    
  } else if (command == "restart") {
    response["message"] = "Restarting in 2 seconds...";
    String responseString;
    serializeJson(response, responseString);
    server.send(200, "application/json", responseString);
    delay(2000);
    ESP.restart();
    return;
    
  } else {
    response["success"] = false;
    response["error"] = "Unknown command: " + command;
    Serial.println("❌ Unknown command: " + command);
  }
  
  String responseString;
  serializeJson(response, responseString);
  server.send(200, "application/json", responseString);
}

void handleConfig() {
  Serial.println("⚙️ Received configuration from API");
  
  String body = server.arg("plain");
  Serial.println("Config payload: " + body);
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(400, "application/json", "{\"success\": false, \"error\": \"Invalid JSON\"}");
    return;
  }
  
  JsonObject config = doc["config"];
  
  DynamicJsonDocument response(512);
  response["success"] = true;
  response["device_id"] = deviceId;
  response["config_applied"] = true;
  response["applied_at"] = millis();
  
  JsonArray appliedConfigs = response.createNestedArray("applied_settings");
  
  // Apply configuration settings
  if (config.containsKey("sensor_interval")) {
    deviceState.sensorInterval = config["sensor_interval"];
    appliedConfigs.add("sensor_interval");
    Serial.println("📊 Sensor interval set to: " + String(deviceState.sensorInterval));
  }
  
  if (config.containsKey("device_location")) {
    deviceState.deviceLocation = config["device_location"].as<String>();
    appliedConfigs.add("device_location");
    Serial.println("📍 Location set to: " + deviceState.deviceLocation);
  }
  
  if (config.containsKey("deep_sleep_duration")) {
    int sleepDuration = config["deep_sleep_duration"];
    // Note: Implementing deep sleep would require careful handling
    appliedConfigs.add("deep_sleep_duration");
    Serial.println("💤 Deep sleep duration set to: " + String(sleepDuration));
  }
  
  String responseString;
  serializeJson(response, responseString);
  server.send(200, "application/json", responseString);
}

void handleUpdate() {
  Serial.println("🔄 Received update information from API");
  
  String body = server.arg("plain");
  Serial.println("Update payload: " + body);
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(400, "application/json", "{\"success\": false, \"error\": \"Invalid JSON\"}");
    return;
  }
  
  String newVersion = doc["firmware_version"];
  String downloadUrl = doc["download_url"];
  bool forceUpdate = doc["force_update"] | false;
  
  DynamicJsonDocument response(512);
  response["success"] = true;
  response["device_id"] = deviceId;
  response["current_version"] = deviceState.firmwareVersion;
  response["new_version"] = newVersion;
  response["update_status"] = "acknowledged";
  
  // In a real implementation, you would:
  // 1. Compare version numbers
  // 2. Download the firmware file
  // 3. Verify checksum
  // 4. Apply the update
  // 5. Restart with new firmware
  
  if (newVersion != deviceState.firmwareVersion) {
    response["message"] = "Update available: " + newVersion;
    response["will_update"] = forceUpdate;
    Serial.println("🚀 Update available: " + newVersion + " (Force: " + String(forceUpdate) + ")");
  } else {
    response["message"] = "Already running latest version";
    response["will_update"] = false;
    Serial.println("✅ Already running latest version");
  }
  
  String responseString;
  serializeJson(response, responseString);
  server.send(200, "application/json", responseString);
}

void handleCustom() {
  Serial.println("🎯 Received custom data from API");
  
  String body = server.arg("plain");
  Serial.println("Custom payload: " + body);
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(400, "application/json", "{\"success\": false, \"error\": \"Invalid JSON\"}");
    return;
  }
  
  DynamicJsonDocument response(512);
  response["success"] = true;
  response["device_id"] = deviceId;
  response["received_at"] = millis();
  
  // Handle custom actions based on the data received
  if (doc.containsKey("message")) {
    String message = doc["message"];
    response["message_received"] = message;
    Serial.println("💬 Message: " + message);
  }
  
  if (doc.containsKey("action")) {
    String action = doc["action"];
    response["action_executed"] = action;
    
    if (action == "blink_led") {
      int duration = doc["parameters"]["duration"] | 1000;
      response["blink_duration"] = duration;
      
      // Blink LED
      for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(duration / 10);
        digitalWrite(LED_PIN, LOW);
        delay(duration / 10);
      }
      Serial.println("💫 LED blinked for " + String(duration) + "ms");
    }
  }
  
  String responseString;
  serializeJson(response, responseString);
  server.send(200, "application/json", responseString);
}

void handleStatus() {
  DynamicJsonDocument doc(1024);
  
  doc["device_id"] = deviceId;
  doc["uptime"] = millis();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["led_state"] = deviceState.ledState;
  doc["firmware_version"] = deviceState.firmwareVersion;
  doc["location"] = deviceState.deviceLocation;
  doc["sensor_interval"] = deviceState.sensorInterval;
  doc["last_data_send"] = lastDataSend;
  doc["ip_address"] = WiFi.localIP().toString();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleNotFound() {
  server.send(404, "application/json", 
    "{\"success\": false, \"error\": \"Endpoint not found\", \"device_id\": \"" + deviceId + "\"}");
}

// ============================================
// Setup and Main Loop
// ============================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n🚀 ESP32 Generic API Client Starting...");
  
  // Initialize hardware
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);
  
  // Connect to WiFi
  connectToWiFi();
  
  if (WiFi.status() == WL_CONNECTED) {
    // Register with API
    if (registerWithAPI()) {
      Serial.println("✅ Ready to communicate with API!");
    } else {
      Serial.println("⚠️ Failed to register, but continuing...");
    }
    
    // Setup web server endpoints
    server.on("/command", HTTP_POST, handleCommand);
    server.on("/config", HTTP_POST, handleConfig);
    server.on("/update", HTTP_POST, handleUpdate);
    server.on("/custom", HTTP_POST, handleCustom);
    server.on("/status", HTTP_GET, handleStatus);
    server.onNotFound(handleNotFound);
    
    server.begin();
    Serial.println("🌐 Web server started on port 80");
    Serial.println("📡 Endpoints available:");
    Serial.println("   POST /command - Receive commands");
    Serial.println("   POST /config - Receive configuration");
    Serial.println("   POST /update - Receive update info");
    Serial.println("   POST /custom - Receive custom data");
    Serial.println("   GET /status - Device status");
  }
  
  Serial.println("=================================");
  Serial.println("ESP32 Generic API Client Ready!");
  Serial.println("Device ID: " + deviceId);
  Serial.println("API Server: " + String(apiServer) + ":" + String(apiPort));
  Serial.println("=================================");
}

void loop() {
  // Handle web server requests
  server.handleClient();
  
  unsigned long currentTime = millis();
  
  // Send data periodically
  if (currentTime - lastDataSend >= deviceState.sensorInterval) {
    if (WiFi.status() == WL_CONNECTED) {
      sendDataToAPI();
      lastDataSend = currentTime;
    } else {
      Serial.println("WiFi disconnected, attempting reconnection...");
      connectToWiFi();
    }
  }
  
  // Optional: Send heartbeat less frequently
  if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("💓 Heartbeat - Device online");
      // Could send a simple ping to API here
    }
    lastHeartbeat = currentTime;
  }
  
  // Check button for manual data send
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(200); // Debounce
    Serial.println("🔘 Button pressed - sending immediate data");
    sendDataToAPI();
    while (digitalRead(BUTTON_PIN) == LOW) delay(100); // Wait for release
  }
  
  delay(100); // Small delay to prevent watchdog issues
}