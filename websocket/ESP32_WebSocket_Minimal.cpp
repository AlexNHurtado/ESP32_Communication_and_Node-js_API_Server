#include <WiFi.h>
#include <WebSocketsServer.h>

// Hardware Configuration
const uint8_t LED_PIN = 2;
const uint16_t WEBSOCKET_PORT = 81;

// Timing Configuration
const uint32_t WIFI_TIMEOUT_MS = 10000;
const uint32_t WIFI_CHECK_INTERVAL_MS = 30000;
const uint32_t SERIAL_TIMEOUT_MS = 30000;
const uint32_t STATUS_BROADCAST_INTERVAL_MS = 5000;

// Global Objects
WebSocketsServer webSocket = WebSocketsServer(WEBSOCKET_PORT);

// State
bool ledState = false;
uint32_t lastWifiCheck = 0;
uint32_t lastStatusBroadcast = 0;
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
  
  // Get SSID
  Serial.print("Enter WiFi SSID: ");
  wifiSSID = readSerialLine(SERIAL_TIMEOUT_MS);
  
  if (wifiSSID.length() == 0) {
    Serial.println("\nERROR: Empty input - SSID is required");
    Serial.println("Please try again\n");
    return false;
  }
  
  Serial.println(wifiSSID);
  
  // Get Password (can be empty for open networks)
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
}

/**
 * @brief Build JSON status message
 */
String buildStatusJson() {
  String json = "{";
  json += "\"type\":\"status\"";
  json += ",\"device\":\"ESP32\"";
  json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
  json += ",\"ssid\":\"" + WiFi.SSID() + "\"";
  json += ",\"rssi\":" + String(WiFi.RSSI());
  json += ",\"led\":" + String(ledState ? "true" : "false");
  json += ",\"uptime\":" + String(millis() / 1000);
  json += ",\"heap\":" + String(ESP.getFreeHeap());
  json += ",\"timestamp\":" + String(millis());
  json += "}";
  return json;
}

/**
 * @brief Build JSON response message
 */
String buildResponseJson(bool success, const char* message) {
  String json = "{";
  json += "\"type\":\"response\"";
  json += ",\"success\":";
  json += success ? "true" : "false";
  json += ",\"message\":\"";
  json += message;
  json += "\"";
  json += ",\"led\":";
  json += ledState ? "true" : "false";
  json += ",\"timestamp\":" + String(millis());
  json += "}";
  return json;
}

/**
 * @brief Handle incoming WebSocket messages
 */
void handleWebSocketMessage(uint8_t clientNum, String payload) {
  Serial.print("Message from client ");
  Serial.print(clientNum);
  Serial.print(": ");
  Serial.println(payload);
  
  payload.toLowerCase();
  
  // Parse JSON command
  if (payload.indexOf("\"command\":\"led_on\"") >= 0) {
    setLED(true);
    String response = buildResponseJson(true, "LED ON");
    webSocket.sendTXT(clientNum, response);
    Serial.println("LED turned ON");
    
  } else if (payload.indexOf("\"command\":\"led_off\"") >= 0) {
    setLED(false);
    String response = buildResponseJson(true, "LED OFF");
    webSocket.sendTXT(clientNum, response);
    Serial.println("LED turned OFF");
    
  } else if (payload.indexOf("\"command\":\"toggle\"") >= 0) {
    setLED(!ledState);
    String response = buildResponseJson(true, ledState ? "LED ON" : "LED OFF");
    webSocket.sendTXT(clientNum, response);
    Serial.println("LED toggled");
    
  } else if (payload.indexOf("\"command\":\"status\"") >= 0) {
    String status = buildStatusJson();
    webSocket.sendTXT(clientNum, status);
    Serial.println("Status sent");
    
  } else {
    String response = buildResponseJson(false, "Unknown command");
    webSocket.sendTXT(clientNum, response);
    Serial.println("Unknown command received");
  }
}

/**
 * @brief WebSocket event handler
 */
void webSocketEvent(uint8_t clientNum, WStype_t type, uint8_t* payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.print("Client ");
      Serial.print(clientNum);
      Serial.println(" disconnected");
      break;
      
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(clientNum);
      Serial.print("Client ");
      Serial.print(clientNum);
      Serial.print(" connected from ");
      Serial.println(ip);
      
      // Send initial status
      String status = buildStatusJson();
      webSocket.sendTXT(clientNum, status);
      break;
    }
    
    case WStype_TEXT:
      handleWebSocketMessage(clientNum, String((char*)payload));
      break;
      
    case WStype_ERROR:
      Serial.print("WebSocket error on client ");
      Serial.println(clientNum);
      break;
      
    default:
      break;
  }
}

/**
 * @brief Broadcast status to all connected clients
 */
void broadcastStatus() {
  uint32_t now = millis();
  if (now - lastStatusBroadcast < STATUS_BROADCAST_INTERVAL_MS) return;
  
  lastStatusBroadcast = now;
  String status = buildStatusJson();
  webSocket.broadcastTXT(status);
  Serial.println("Status broadcast to all clients");
}

/**
 * @brief Connect to WiFi with timeout
 */
bool connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(wifiSSID);
  
  WiFi.mode(WIFI_STA);
  
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
  
  Serial.println("\n\n=== ESP32 WebSocket Server ===");
  Serial.println("Firmware Version: 1.0");
  Serial.println();
  
  // Get WiFi credentials (retry until valid input)
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
  
  // Start WebSocket server
  Serial.println("\n=== Starting WebSocket Server ===");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started successfully!");
  Serial.println();
  Serial.println("=== Available Commands ===");
  Serial.println("{\"command\":\"led_on\"}    - Turn LED on");
  Serial.println("{\"command\":\"led_off\"}   - Turn LED off");
  Serial.println("{\"command\":\"toggle\"}    - Toggle LED");
  Serial.println("{\"command\":\"status\"}    - Get status");
  Serial.println();
  Serial.println("=== Connection Info ===");
  Serial.print("ws://");
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.println(WEBSOCKET_PORT);
  Serial.println("========================");
  Serial.println("\nReady! Waiting for connections...\n");
}

void loop() {
  webSocket.loop();
  checkWiFi();
  broadcastStatus();  // Auto-broadcast status every 5 seconds
  delay(1);
}
