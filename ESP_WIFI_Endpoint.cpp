#include <WiFi.h>
#include <WebServer.h>

// Replace with your network credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Create WebServer object on port 80
WebServer server(80);

// Pin for the built-in LED (GPIO 2 for most ESP32 boards)
const int ledPin = 2;
bool ledState = false;

/**
 * @brief Root endpoint - Shows available endpoints and LED status
 */
void handleRoot() {
  Serial.println("GET / - Root endpoint accessed");
  
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>ESP32 LED Controller</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;text-align:center;margin:50px;background:#f0f2f5;}";
  html += ".container{max-width:500px;margin:0 auto;background:white;padding:30px;border-radius:15px;box-shadow:0 4px 20px rgba(0,0,0,0.1);}";
  html += "h1{color:#333;margin-bottom:30px;}";
  html += ".status{font-size:18px;margin:20px 0;padding:15px;border-radius:8px;}";
  html += ".led-on{background:#d4edda;color:#155724;border:1px solid #c3e6cb;}";
  html += ".led-off{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb;}";
  html += "button{font-size:18px;padding:15px 30px;margin:10px;border:none;border-radius:8px;cursor:pointer;transition:all 0.3s;}";
  html += ".btn-on{background:#28a745;color:white;} .btn-on:hover{background:#218838;}";
  html += ".btn-off{background:#dc3545;color:white;} .btn-off:hover{background:#c82333;}";
  html += ".info{background:#e7f3ff;padding:15px;border-radius:8px;margin:20px 0;color:#0c5460;}";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>ESP32 LED Controller</h1>";
  
  html += "<div class='info'>";
  html += "<strong>Device IP:</strong> " + WiFi.localIP().toString() + "<br>";
  html += "<strong>Signal:</strong> " + String(WiFi.RSSI()) + " dBm";
  html += "</div>";
  
  html += "<div class='status " + String(ledState ? "led-on" : "led-off") + "'>";
  html += "LED Status: <strong>" + String(ledState ? "ON" : "OFF") + "</strong>";
  html += "</div>";
  
  html += "<button class='btn-on' onclick=\"controlLED(true)\">Turn LED ON</button>";
  html += "<button class='btn-off' onclick=\"controlLED(false)\">Turn LED OFF</button>";
  
  html += "<script>";
  html += "function controlLED(state) {";
  html += "  const endpoint = state ? '/led/on' : '/led/off';";
  html += "  fetch(endpoint)";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      console.log('Success:', data);";
  html += "      setTimeout(() => location.reload(), 500);";
  html += "    })";
  html += "    .catch(error => console.error('Error:', error));";
  html += "}";
  html += "</script>";
  
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

/**
 * @brief GET /led/on - Turn LED on
 */
void handleLedOn() {
  Serial.println("GET /led/on - Turning LED ON");
  
  ledState = true;
  digitalWrite(ledPin, HIGH);
  
  // Send JSON response
  String response = "{";
  response += "\"success\": true,";
  response += "\"action\": \"LED turned ON\",";
  response += "\"led_state\": true,";
  response += "\"timestamp\": " + String(millis());
  response += "}";
  
  server.send(200, "application/json", response);
}

/**
 * @brief GET /led/off - Turn LED off
 */
void handleLedOff() {
  Serial.println("GET /led/off - Turning LED OFF");
  
  ledState = false;
  digitalWrite(ledPin, LOW);
  
  // Send JSON response
  String response = "{";
  response += "\"success\": true,";
  response += "\"action\": \"LED turned OFF\",";
  response += "\"led_state\": false,";
  response += "\"timestamp\": " + String(millis());
  response += "}";
  
  server.send(200, "application/json", response);
}

/**
 * @brief GET /status - Get current LED status
 */
void handleStatus() {
  Serial.println("GET /status - Status requested");
  
  String response = "{";
  response += "\"device\": \"ESP32\",";
  response += "\"ip\": \"" + WiFi.localIP().toString() + "\",";
  response += "\"wifi_signal\": " + String(WiFi.RSSI()) + ",";
  response += "\"led_state\": " + String(ledState ? "true" : "false") + ",";
  response += "\"uptime_seconds\": " + String(millis() / 1000) + ",";
  response += "\"free_memory\": " + String(ESP.getFreeHeap());
  response += "}";
  
  server.send(200, "application/json", response);
}

/**
 * @brief POST /led - Control LED with JSON payload
 */
void handleLedControl() {
  Serial.println("POST /led - LED control via JSON");
  
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    Serial.println("Received JSON: " + body);
    
    // Simple JSON parsing (looking for "state":true or "state":false)
    if (body.indexOf("\"state\":true") != -1 || body.indexOf("\"state\": true") != -1) {
      ledState = true;
      digitalWrite(ledPin, HIGH);
      
      String response = "{\"success\": true, \"led_state\": true, \"message\": \"LED turned ON\"}";
      server.send(200, "application/json", response);
      
    } else if (body.indexOf("\"state\":false") != -1 || body.indexOf("\"state\": false") != -1) {
      ledState = false;
      digitalWrite(ledPin, LOW);
      
      String response = "{\"success\": true, \"led_state\": false, \"message\": \"LED turned OFF\"}";
      server.send(200, "application/json", response);
      
    } else {
      String response = "{\"error\": \"Invalid JSON. Expected {\\\"state\\\": true/false}\"}";
      server.send(400, "application/json", response);
    }
  } else {
    String response = "{\"error\": \"No JSON payload received\"}";
    server.send(400, "application/json", response);
  }
}

/**
 * @brief Handle 404 errors
 */
void handleNotFound() {
  String message = "{";
  message += "\"error\": \"Endpoint not found\",";
  message += "\"requested_path\": \"" + server.uri() + "\",";
  message += "\"available_endpoints\": [";
  message += "\"/\", \"/led/on\", \"/led/off\", \"/status\", \"POST /led\"";
  message += "]}";
  
  server.send(404, "application/json", message);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize LED pin
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // Start with LED off
  
  Serial.println("\nESP32 LED Controller Starting...");
  Serial.println("===================================");
  
  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // Wait for connection with timeout
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\nWiFi Connection Failed!");
    Serial.println("Please check your credentials and try again.");
    return;
  }
  
  // Configure server endpoints
  server.on("/", HTTP_GET, handleRoot);
  server.on("/led/on", HTTP_GET, handleLedOn);
  server.on("/led/off", HTTP_GET, handleLedOff);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/led", HTTP_POST, handleLedControl);
  server.onNotFound(handleNotFound);
  
  // Start server
  server.begin();
  
  Serial.println("HTTP Server Started!");
  Serial.println("===================================");
  Serial.println("Available Endpoints:");
  Serial.println("   GET  / - Web interface");
  Serial.println("   GET  /led/on - Turn LED on");
  Serial.println("   GET  /led/off - Turn LED off");
  Serial.println("   GET  /status - Get device status");
  Serial.println("   POST /led - Control LED with JSON");
  Serial.println("===================================");
  Serial.print("Access your device at: http://");
  Serial.println(WiFi.localIP());
  Serial.println("===================================");
}

void loop() {
  // Handle incoming HTTP requests
  server.handleClient();
  
  // Check WiFi connection status every 30 seconds
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 30000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected! Attempting reconnection...");
      WiFi.reconnect();
    }
    lastCheck = millis();
  }
  
  // Small delay to prevent watchdog timeout
  delay(10);
}
