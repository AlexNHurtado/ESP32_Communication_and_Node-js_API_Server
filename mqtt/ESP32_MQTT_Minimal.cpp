#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Hardware Configuration
const uint8_t LED_PIN = 2;

// Timing Configuration
const uint32_t WIFI_TIMEOUT_MS = 10000;
const uint32_t WIFI_CHECK_INTERVAL_MS = 30000;
const uint32_t SERIAL_TIMEOUT_MS = 30000;  // 30 seconds to enter info
const uint32_t MQTT_RECONNECT_INTERVAL_MS = 5000;
const uint32_t STATUS_PUBLISH_INTERVAL_MS = 30000; // Publish status every 30 seconds

// MQTT Configuration
const char* MQTT_SERVER = "broker.hivemq.com"; // Public MQTT broker
const int MQTT_PORT = 1883;
const char* CLIENT_ID = "ESP32_Device";

// MQTT Topics
const char* TOPIC_LED_CONTROL = "esp32/led/control";     // Subscribe: {"state": true/false}
const char* TOPIC_LED_STATUS = "esp32/led/status";       // Publish: LED state changes
const char* TOPIC_DEVICE_STATUS = "esp32/device/status"; // Publish: Full device status
const char* TOPIC_DEVICE_COMMAND = "esp32/device/command"; // Subscribe: Commands like "status", "restart"

// Global Objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// State
bool ledState = false;
uint32_t lastWifiCheck = 0;
uint32_t lastMqttReconnect = 0;
uint32_t lastStatusPublish = 0;
String wifiSSID = "";
String wifiPassword = "";
String deviceId = "";

/**
 * @brief Read line from Serial with timeout
 */
String readSerialLine(uint32_t timeout) {
  String input = "";
  uint32_t start = millis();
  
  while (millis() - start < timeout) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        return input;
      } else {
        input += c;
      }
    }
    delay(10);
  }
  return input;
}

/**
 * @brief Get WiFi credentials from Serial Monitor
 */
bool getWiFiCredentials() {
  Serial.println("\n=== WiFi Configuration ===");
  Serial.println("Please enter your WiFi credentials");
  Serial.println();
  
  Serial.print("Enter WiFi SSID: ");
  wifiSSID = readSerialLine(SERIAL_TIMEOUT_MS);
  
  if (wifiSSID.length() == 0) {
    Serial.println("\nERROR: Empty input - SSID is required");
    Serial.println("Please try again\n");
    return false;
  }
  
  Serial.println(wifiSSID);
  
  Serial.print("Enter WiFi Password (press Enter if open network): ");
  wifiPassword = readSerialLine(SERIAL_TIMEOUT_MS);
  
  if (wifiPassword.length() == 0) {
    Serial.println("[OPEN NETWORK]");
    Serial.println();
    Serial.println("Credentials received:");
    Serial.print("  SSID: ");
    Serial.println(wifiSSID);
    Serial.println("  Password: [NONE - Open Network]");
  } else {
    Serial.println("********");
    Serial.println();
    Serial.println("Credentials received:");
    Serial.print("  SSID: ");
    Serial.println(wifiSSID);
    Serial.print("  Password: ");
    for (int i = 0; i < wifiPassword.length(); i++) {
      Serial.print("*");
    }
    Serial.println();
  }
  
  return true;
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
 * @brief Create standardized JSON response
 */
String createJsonResponse(bool success, const char* message, bool includeState = true) {
  DynamicJsonDocument doc(256);
  
  doc["success"] = success;
  doc["message"] = message;
  
  if (includeState) {
    doc["led_state"] = ledState;
  }
  
  doc["device_id"] = deviceId;
  doc["api_version"] = "1.0";
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

/**
 * @brief Create device status JSON
 */
String createStatusJson() {
  DynamicJsonDocument doc(512);
  
  doc["device"] = "ESP32";
  doc["device_id"] = deviceId;
  doc["ip"] = WiFi.localIP().toString();
  doc["ssid"] = WiFi.SSID();
  doc["rssi"] = WiFi.RSSI();
  doc["led_state"] = ledState;
  doc["uptime"] = millis() / 1000;
  doc["heap"] = ESP.getFreeHeap();
  doc["mqtt_connected"] = mqttClient.connected();
  doc["api_version"] = "1.0";
  
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

/**
 * @brief Publish LED status change
 */
void publishLedStatus() {
  String response = createJsonResponse(true, ledState ? "LED ON" : "LED OFF");
  if (mqttClient.publish(TOPIC_LED_STATUS, response.c_str())) {
    Serial.println("Published LED status: " + response);
  } else {
    Serial.println("Failed to publish LED status");
  }
}

/**
 * @brief Publish device status
 */
void publishDeviceStatus() {
  String status = createStatusJson();
  if (mqttClient.publish(TOPIC_DEVICE_STATUS, status.c_str())) {
    Serial.println("Published device status");
  } else {
    Serial.println("Failed to publish device status");
  }
}

/**
 * @brief Handle MQTT message callback
 */
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  // Convert payload to string
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("Received MQTT message:");
  Serial.println("  Topic: " + String(topic));
  Serial.println("  Message: " + message);
  
  // Handle LED control
  if (String(topic) == TOPIC_LED_CONTROL) {
    DynamicJsonDocument doc(128);
    deserializeJson(doc, message);
    
    if (doc.containsKey("state")) {
      bool newState = doc["state"];
      setLED(newState);
      publishLedStatus();
    } else {
      Serial.println("Invalid LED control message - missing 'state' field");
    }
  }
  // Handle device commands
  else if (String(topic) == TOPIC_DEVICE_COMMAND) {
    if (message == "status") {
      publishDeviceStatus();
    } else if (message == "restart") {
      Serial.println("Restart command received - restarting in 3 seconds...");
      delay(3000);
      ESP.restart();
    } else {
      Serial.println("Unknown command: " + message);
    }
  }
}

/**
 * @brief Connect to WiFi with timeout
 */
bool connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(wifiSSID);
  
  WiFi.mode(WIFI_STA);
  
  // Connect with or without password
  if (wifiPassword.length() > 0) {
    Serial.println("(Secured network)");
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  } else {
    Serial.println("(Open network)");
    WiFi.begin(wifiSSID.c_str());
  }
  
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_TIMEOUT_MS) {
      Serial.println(" FAILED");
      Serial.println("Connection timeout - check credentials");
      return false;
    }
    delay(500);
    Serial.print(".");
  }
  
  Serial.println(" CONNECTED");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signal: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  
  // Generate device ID from MAC address
  deviceId = WiFi.macAddress();
  deviceId.replace(":", "");
  Serial.print("Device ID: ");
  Serial.println(deviceId);
  
  return true;
}

/**
 * @brief Connect to MQTT broker
 */
bool connectMqtt() {
  Serial.print("Connecting to MQTT broker: ");
  Serial.println(MQTT_SERVER);
  
  String clientId = CLIENT_ID;
  clientId += "_" + deviceId;
  
  if (mqttClient.connect(clientId.c_str())) {
    Serial.println("MQTT connected successfully");
    
    // Subscribe to topics
    mqttClient.subscribe(TOPIC_LED_CONTROL);
    mqttClient.subscribe(TOPIC_DEVICE_COMMAND);
    
    Serial.println("Subscribed to topics:");
    Serial.println("  " + String(TOPIC_LED_CONTROL));
    Serial.println("  " + String(TOPIC_DEVICE_COMMAND));
    
    // Publish initial status
    publishDeviceStatus();
    
    return true;
  } else {
    Serial.print("MQTT connection failed, rc=");
    Serial.println(mqttClient.state());
    return false;
  }
}

/**
 * @brief Monitor WiFi connection
 */
void checkWiFi() {
  uint32_t now = millis();
  if (now - lastWifiCheck < WIFI_CHECK_INTERVAL_MS) return;
  
  lastWifiCheck = now;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost - reconnecting");
    WiFi.reconnect();
  }
}

/**
 * @brief Monitor MQTT connection
 */
void checkMqtt() {
  if (!mqttClient.connected()) {
    uint32_t now = millis();
    if (now - lastMqttReconnect > MQTT_RECONNECT_INTERVAL_MS) {
      lastMqttReconnect = now;
      Serial.println("MQTT disconnected - attempting reconnection");
      connectMqtt();
    }
  }
}

/**
 * @brief Publish status periodically
 */
void handlePeriodicStatus() {
  uint32_t now = millis();
  if (now - lastStatusPublish > STATUS_PUBLISH_INTERVAL_MS) {
    lastStatusPublish = now;
    if (mqttClient.connected()) {
      publishDeviceStatus();
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Clear serial buffer
  while (Serial.available()) {
    Serial.read();
  }
  
  // Initialize hardware
  pinMode(LED_PIN, OUTPUT);
  setLED(false);
  
  Serial.println("\n\n=== ESP32 MQTT Controller ===");
  Serial.println("Firmware Version: 1.0");
  Serial.println();
  
  // Get WiFi credentials from user (retry until valid input)
  while (!getWiFiCredentials()) {
    delay(2000);  // Wait 2 seconds before prompting again
  }
  
  // Connect to WiFi
  Serial.println("\n=== Connecting to WiFi ===");
  if (!connectWiFi()) {
    Serial.println("\nERROR: WiFi connection failed");
    Serial.println("Please reset and check:");
    Serial.println("  1. SSID is correct");
    Serial.println("  2. Password is correct (if secured)");
    Serial.println("  3. WiFi is 2.4GHz (ESP32 doesn't support 5GHz)");
    Serial.println("  4. Router is powered on");
    while(1) {
      delay(1000);
    }
  }
  
  // Configure MQTT
  Serial.println("\n=== Setting up MQTT ===");
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(onMqttMessage);
  
  // Connect to MQTT
  if (!connectMqtt()) {
    Serial.println("WARNING: MQTT connection failed - will retry in loop");
  }
  
  Serial.println("\n=== MQTT Topics ===");
  Serial.println("Subscribe to control LED:");
  Serial.println("  Topic: " + String(TOPIC_LED_CONTROL));
  Serial.println("  Payload: {\"state\": true} or {\"state\": false}");
  Serial.println();
  Serial.println("Subscribe to device commands:");
  Serial.println("  Topic: " + String(TOPIC_DEVICE_COMMAND));
  Serial.println("  Payload: \"status\" or \"restart\"");
  Serial.println();
  Serial.println("Device publishes to:");
  Serial.println("  " + String(TOPIC_LED_STATUS) + " (LED state changes)");
  Serial.println("  " + String(TOPIC_DEVICE_STATUS) + " (Full status)");
  Serial.println();
  Serial.println("=== Device Information ===");
  Serial.println("Device ID: " + deviceId);
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MQTT Broker: " + String(MQTT_SERVER) + ":" + String(MQTT_PORT));
  Serial.println("=======================");
  Serial.println("\nReady! Listening for MQTT messages...\n");
}

void loop() {
  // Handle MQTT
  mqttClient.loop();
  
  // Monitor connections
  checkWiFi();
  checkMqtt();
  
  // Periodic status updates
  handlePeriodicStatus();
  
  delay(10);
}