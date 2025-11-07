#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// Hardware Configuration
const uint8_t LED_PIN = 2;
const uint16_t HTTP_PORT = 80;
const uint16_t WEBSOCKET_PORT = 81;

// Timing Configuration
const uint32_t WIFI_TIMEOUT_MS = 10000;
const uint32_t WIFI_CHECK_INTERVAL_MS = 30000;
const uint32_t SERIAL_TIMEOUT_MS = 30000;
const uint32_t STATUS_BROADCAST_INTERVAL_MS = 5000;

// Global Objects
WebServer httpServer(HTTP_PORT);
WebSocketsServer webSocket = WebSocketsServer(WEBSOCKET_PORT);

// State
bool ledState = false;
uint32_t lastWifiCheck = 0;
uint32_t lastStatusBroadcast = 0;
String wifiSSID = "";
String wifiPassword = "";
uint32_t sessionCounter = 0;  // Global session counter for unique IDs

// Track active connections
struct ClientInfo {
  uint32_t sessionId;
  IPAddress ip;
  unsigned long connectTime;
  bool active;
};

ClientInfo clients[WEBSOCKETS_SERVER_CLIENT_MAX];  // Default is 8

/**
 * @brief Initialize client tracking
 */
void initClientTracking() {
  for (int i = 0; i < WEBSOCKETS_SERVER_CLIENT_MAX; i++) {
    clients[i].active = false;
    clients[i].sessionId = 0;
  }
}

/**
 * @brief Register new client connection
 */
void registerClient(uint8_t clientNum, IPAddress ip) {
  clients[clientNum].sessionId = ++sessionCounter;
  clients[clientNum].ip = ip;
  clients[clientNum].connectTime = millis();
  clients[clientNum].active = true;
  
  Serial.print("🔌 Session #");
  Serial.print(clients[clientNum].sessionId);
  Serial.print(" (Slot ");
  Serial.print(clientNum);
  Serial.print(") connected from ");
  Serial.print(ip);
  Serial.print(" | Active connections: ");
  Serial.println(getActiveClientCount());
}

/**
 * @brief Unregister client disconnection
 */
void unregisterClient(uint8_t clientNum) {
  Serial.print("❌ Session #");
  Serial.print(clients[clientNum].sessionId);
  Serial.print(" (Slot ");
  Serial.print(clientNum);
  Serial.print(") disconnected | Active connections: ");
  clients[clientNum].active = false;
  Serial.println(getActiveClientCount());
}

/**
 * @brief Get count of active clients
 */
int getActiveClientCount() {
  int count = 0;
  for (int i = 0; i < WEBSOCKETS_SERVER_CLIENT_MAX; i++) {
    if (clients[i].active) count++;
  }
  return count;
}

/**
 * @brief Print all active connections
 */
void printActiveConnections() {
  Serial.println("\n📊 Active WebSocket Connections:");
  int count = 0;
  for (int i = 0; i < WEBSOCKETS_SERVER_CLIENT_MAX; i++) {
    if (clients[i].active) {
      count++;
      Serial.print("  Session #");
      Serial.print(clients[i].sessionId);
      Serial.print(" (Slot ");
      Serial.print(i);
      Serial.print(") | IP: ");
      Serial.print(clients[i].ip);
      Serial.print(" | Uptime: ");
      Serial.print((millis() - clients[i].connectTime) / 1000);
      Serial.println("s");
    }
  }
  if (count == 0) {
    Serial.println("  No active connections");
  }
  Serial.println();
}

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
 * @brief Control LED state and notify all WebSocket clients
 */
void setLED(bool state) {
  ledState = state;
  digitalWrite(LED_PIN, state ? HIGH : LOW);
  
  // Broadcast state change to WebSocket clients
  String json = "{";
  json += "\"type\":\"led_update\"";
  json += ",\"led\":" + String(ledState ? "true" : "false");
  json += ",\"timestamp\":" + String(millis());
  json += "}";
  webSocket.broadcastTXT(json);
  
  Serial.print("💡 LED ");
  Serial.print(state ? "ON" : "OFF");
  Serial.print(" | Broadcast to ");
  Serial.print(getActiveClientCount());
  Serial.println(" client(s)");
}

/**
 * @brief Build status JSON
 */
String buildStatusJson() {
  String json = "{";
  json += "\"device\":\"ESP32\"";
  json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
  json += ",\"ssid\":\"" + WiFi.SSID() + "\"";
  json += ",\"rssi\":" + String(WiFi.RSSI());
  json += ",\"led\":" + String(ledState ? "true" : "false");
  json += ",\"uptime\":" + String(millis() / 1000);
  json += ",\"heap\":" + String(ESP.getFreeHeap());
  json += ",\"ws_clients\":" + String(getActiveClientCount());
  json += ",\"timestamp\":" + String(millis());
  json += "}";
  return json;
}

// ========== HTTP REST HANDLERS ==========

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
  
  httpServer.send(code, "application/json", json);
}

/**
 * @brief GET /status - Device status
 */
void handleStatus() {
  String json = buildStatusJson();
  httpServer.send(200, "application/json", json);
}

/**
 * @brief GET /led/on - Turn LED on
 */
void handleLedOn() {
  Serial.println("📡 HTTP: LED ON command received");
  setLED(true);
  sendJson(200, "LED ON");
}

/**
 * @brief GET /led/off - Turn LED off
 */
void handleLedOff() {
  Serial.println("📡 HTTP: LED OFF command received");
  setLED(false);
  sendJson(200, "LED OFF");
}

/**
 * @brief POST /led - Control LED with JSON
 */
void handleLedControl() {
  if (!httpServer.hasArg("plain")) {
    sendJson(400, "Missing JSON body", false);
    return;
  }
  
  String body = httpServer.arg("plain");
  body.toLowerCase();
  
  if (body.indexOf("\"state\":true") >= 0) {
    Serial.println("📡 HTTP: LED ON command received (POST)");
    setLED(true);
    sendJson(200, "LED ON");
  } else if (body.indexOf("\"state\":false") >= 0) {
    Serial.println("📡 HTTP: LED OFF command received (POST)");
    setLED(false);
    sendJson(200, "LED OFF");
  } else {
    sendJson(400, "Invalid JSON format", false);
  }
}

/**
 * @brief Handle 404
 */
void handleNotFound() {
  String json = "{\"error\":\"Not found\",\"path\":\"";
  json += httpServer.uri();
  json += "\"}";
  httpServer.send(404, "application/json", json);
}

// ========== WEBSOCKET HANDLERS ==========

/**
 * @brief Build WebSocket response JSON
 */
String buildWsResponseJson(bool success, const char* message) {
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
 * @brief Handle WebSocket messages
 */
void handleWebSocketMessage(uint8_t clientNum, String payload) {
  if (!clients[clientNum].active) return;
  
  Serial.print("📨 Session #");
  Serial.print(clients[clientNum].sessionId);
  Serial.print(" (Slot ");
  Serial.print(clientNum);
  Serial.print("): ");
  Serial.println(payload);
  
  payload.toLowerCase();
  
  if (payload.indexOf("\"command\":\"led_on\"") >= 0) {
    setLED(true);
    String response = buildWsResponseJson(true, "LED ON");
    webSocket.sendTXT(clientNum, response);
    
  } else if (payload.indexOf("\"command\":\"led_off\"") >= 0) {
    setLED(false);
    String response = buildWsResponseJson(true, "LED OFF");
    webSocket.sendTXT(clientNum, response);
    
  } else if (payload.indexOf("\"command\":\"toggle\"") >= 0) {
    setLED(!ledState);
    String response = buildWsResponseJson(true, ledState ? "LED ON" : "LED OFF");
    webSocket.sendTXT(clientNum, response);
    
  } else if (payload.indexOf("\"command\":\"status\"") >= 0) {
    String status = buildStatusJson();
    webSocket.sendTXT(clientNum, status);
    
  } else if (payload.indexOf("\"command\":\"list\"") >= 0) {
    printActiveConnections();
    
  } else {
    String response = buildWsResponseJson(false, "Unknown command");
    webSocket.sendTXT(clientNum, response);
  }
}

/**
 * @brief WebSocket event handler
 */
void webSocketEvent(uint8_t clientNum, WStype_t type, uint8_t* payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      unregisterClient(clientNum);
      break;
      
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(clientNum);
      registerClient(clientNum, ip);
      
      // Send initial status with session info
      String status = "{\"type\":\"status\",\"session_id\":" + 
                     String(clients[clientNum].sessionId) + "," + 
                     buildStatusJson().substring(1);
      webSocket.sendTXT(clientNum, status);
      break;
    }
    
    case WStype_TEXT:
      handleWebSocketMessage(clientNum, String((char*)payload));
      break;
      
    default:
      break;
  }
}

/**
 * @brief Broadcast status to all WebSocket clients
 */
void broadcastStatus() {
  uint32_t now = millis();
  if (now - lastStatusBroadcast < STATUS_BROADCAST_INTERVAL_MS) return;
  
  lastStatusBroadcast = now;
  
  int activeCount = getActiveClientCount();
  if (activeCount == 0) return;  // Don't broadcast if no clients
  
  String status = "{\"type\":\"status\"," + buildStatusJson().substring(1);
  webSocket.broadcastTXT(status);
}

// ========== WIFI FUNCTIONS ==========

/**
 * @brief Connect to WiFi
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
  
  while (Serial.available()) {
    Serial.read();
  }
  
  pinMode(LED_PIN, OUTPUT);
  setLED(false);
  
  initClientTracking();  // Initialize connection tracking
  
  Serial.println("\n\n=== ESP32 Hybrid Server (REST + WebSocket) ===");
  Serial.println("Firmware Version: 1.1 - Enhanced Connection Tracking");
  Serial.println();
  
  // Get WiFi credentials
  while (!getWiFiCredentials()) {
    delay(2000);
  }
  
  // Connect to WiFi
  Serial.println("\n=== Connecting to WiFi ===");
  if (!connectWiFi()) {
    Serial.println("\nERROR: WiFi connection failed");
    Serial.println("Please reset and check your credentials");
    while(1) delay(1000);
  }
  
  // Start HTTP REST server
  Serial.println("\n=== Starting HTTP REST Server ===");
  httpServer.on("/status", HTTP_GET, handleStatus);
  httpServer.on("/led/on", HTTP_GET, handleLedOn);
  httpServer.on("/led/off", HTTP_GET, handleLedOff);
  httpServer.on("/led", HTTP_POST, handleLedControl);
  httpServer.onNotFound(handleNotFound);
  httpServer.begin();
  Serial.println("HTTP server started on port 80");
  
  // Start WebSocket server
  Serial.println("\n=== Starting WebSocket Server ===");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started on port 81");
  
  Serial.println("\n=== Server Information ===");
  Serial.println("HTTP REST API:");
  Serial.print("  http://");
  Serial.println(WiFi.localIP());
  Serial.println("  GET  /status   - Device status");
  Serial.println("  GET  /led/on   - Turn LED on");
  Serial.println("  GET  /led/off  - Turn LED off");
  Serial.println("  POST /led      - Control LED (JSON)");
  Serial.println();
  Serial.println("WebSocket API:");
  Serial.print("  ws://");
  Serial.print(WiFi.localIP());
  Serial.println(":81");
  Serial.println("  Commands:");
  Serial.println("    {\"command\":\"led_on\"}");
  Serial.println("    {\"command\":\"led_off\"}");
  Serial.println("    {\"command\":\"toggle\"}");
  Serial.println("    {\"command\":\"status\"}");
  Serial.println("    {\"command\":\"list\"}  ← List active connections");
  Serial.println("==========================");
  Serial.println("\nBoth servers ready!\n");
}

void loop() {
  httpServer.handleClient();  // Handle HTTP requests
  webSocket.loop();            // Handle WebSocket connections
  checkWiFi();                 // Monitor WiFi
  broadcastStatus();           // Broadcast status to WebSocket clients
  delay(1);
}