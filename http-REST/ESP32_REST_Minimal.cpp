#include <WiFi.h>
#include <WebServer.h>

// Hardware Configuration
const uint8_t LED_PIN = 2;
const uint16_t SERVER_PORT = 80;

// Timing Configuration
const uint32_t WIFI_TIMEOUT_MS = 10000;
const uint32_t WIFI_CHECK_INTERVAL_MS = 30000;
const uint32_t SERIAL_TIMEOUT_MS = 30000;  // 30 seconds to enter WiFi info

// Global Objects
WebServer server(SERVER_PORT);

// State
bool ledState = false;
uint32_t lastWifiCheck = 0;
String wifiSSID = "";
String wifiPassword = "";

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
        // Return even if empty (for open networks)
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
 * @brief Send standardized JSON response
 */
void sendJson(int code, const char* message, bool includeState = true) {
  String json = "{\"success\":";
  json += (code == 200) ? "true" : "false";
  json += ",\"message\":\"";
  json += message;
  json += "\"";
  
  if (includeState) {
    json += ",\"led\":";
    json += ledState ? "true" : "false";
  }
  
  json += ",\"timestamp\":";
  json += millis();
  json += "}";
  
  server.send(code, "application/json", json);
}

/**
 * @brief Control LED state
 */
void setLED(bool state) {
  ledState = state;
  digitalWrite(LED_PIN, state ? HIGH : LOW);
}

/**
 * @brief GET /status - Device status
 */
void handleStatus() {
  String json = "{";
  json += "\"device\":\"ESP32\"";
  json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
  json += ",\"ssid\":\"" + WiFi.SSID() + "\"";
  json += ",\"rssi\":" + String(WiFi.RSSI());
  json += ",\"led\":" + String(ledState ? "true" : "false");
  json += ",\"uptime\":" + String(millis() / 1000);
  json += ",\"heap\":" + String(ESP.getFreeHeap());
  json += "}";
  
  server.send(200, "application/json", json);
}

/**
 * @brief GET /led/on - Turn LED on
 */
void handleLedOn() {
  setLED(true);
  sendJson(200, "LED ON");
}

/**
 * @brief GET /led/off - Turn LED off
 */
void handleLedOff() {
  setLED(false);
  sendJson(200, "LED OFF");
}

/**
 * @brief POST /led - Control LED with JSON
 * Expected: {"state": true/false}
 */
void handleLedControl() {
  if (!server.hasArg("plain")) {
    sendJson(400, "Missing JSON body", false);
    return;
  }
  
  String body = server.arg("plain");
  body.toLowerCase();
  
  if (body.indexOf("\"state\":true") >= 0) {
    setLED(true);
    sendJson(200, "LED ON");
  } else if (body.indexOf("\"state\":false") >= 0) {
    setLED(false);
    sendJson(200, "LED OFF");
  } else {
    sendJson(400, "Invalid JSON format", false);
  }
}

/**
 * @brief Handle undefined endpoints
 */
void handleNotFound() {
  String json = "{\"error\":\"Not found\",\"path\":\"";
  json += server.uri();
  json += "\"}";
  server.send(404, "application/json", json);
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
  return true;
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
  
  Serial.println("\n\n=== ESP32 REST API ===");
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
  
  // Configure endpoints
  Serial.println("\n=== Starting HTTP Server ===");
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/led/on", HTTP_GET, handleLedOn);
  server.on("/led/off", HTTP_GET, handleLedOff);
  server.on("/led", HTTP_POST, handleLedControl);
  server.onNotFound(handleNotFound);
  
  // Start server
  server.begin();
  Serial.println("Server started successfully!");
  Serial.println();
  Serial.println("=== Available Endpoints ===");
  Serial.println("GET  /status   - Device status");
  Serial.println("GET  /led/on   - Turn LED on");
  Serial.println("GET  /led/off  - Turn LED off");
  Serial.println("POST /led      - Control LED (JSON)");
  Serial.println();
  Serial.println("=== Access URLs ===");
  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.println("=======================");
  Serial.println("\nReady! Waiting for requests...\n");
}

void loop() {
  server.handleClient();
  checkWiFi();
  delay(1);
}