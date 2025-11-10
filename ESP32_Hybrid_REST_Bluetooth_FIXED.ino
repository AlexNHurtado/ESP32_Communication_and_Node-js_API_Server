/*
 * ESP32 REST + Bluetooth Hybrid Server - CORRECTED VERSION
 * 
 * This file fixes the getBtAddress() compilation error by properly handling
 * the MAC address retrieval which requires a uint8_t array parameter.
 * 
 * Copy this code to your sketch_REST_Socket_Hybrid.ino file to fix the compilation error.
 */

#include "BluetoothSerial.h"
#include <WiFi.h>
#include <WebServer.h>

// Check if Bluetooth is available
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Hardware Configuration
const uint8_t LED_PIN = 2;

// Network Configuration  
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Bluetooth Configuration
const char* BT_DEVICE_NAME = "ESP32_Hybrid_Server";

// Global Objects
BluetoothSerial SerialBT;
WebServer server(80);

// State Variables
bool ledState = false;
bool bluetoothConnected = false;
bool wifiConnected = false;

/**
 * @brief Get Bluetooth MAC address as formatted string
 * 
 * This function properly handles the getBtAddress() method which requires
 * a uint8_t array parameter to store the 6-byte MAC address.
 * 
 * @return String formatted MAC address (XX:XX:XX:XX:XX:XX)
 */
String getBluetoothMAC() {
  uint8_t mac[6];
  SerialBT.getBtAddress(mac);
  
  char macStr[18];  // 17 chars + null terminator
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  return String(macStr);
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
 * @brief Process Bluetooth commands
 */
void processBluetoothCommand(String command) {
  command.trim();
  command.toLowerCase();
  
  Serial.println("BT Command: " + command);
  
  if (command == "led on") {
    setLED(true);
    SerialBT.println("LED turned ON via Bluetooth");
    
  } else if (command == "led off") {
    setLED(false);
    SerialBT.println("LED turned OFF via Bluetooth");
    
  } else if (command == "status") {
    SerialBT.println("=== Device Status ===");
    SerialBT.println("Device: ESP32 Hybrid Server");
    SerialBT.println("Bluetooth Name: " + String(BT_DEVICE_NAME));
    SerialBT.println("Bluetooth MAC: " + getBluetoothMAC());  // FIXED: Using helper function
    SerialBT.println("WiFi Status: " + String(wifiConnected ? "Connected" : "Disconnected"));
    if (wifiConnected) {
      SerialBT.println("WiFi IP: " + WiFi.localIP().toString());
      SerialBT.println("REST API: http://" + WiFi.localIP().toString() + "/");
    }
    SerialBT.println("LED State: " + String(ledState ? "ON" : "OFF"));
    SerialBT.println("Uptime: " + String(millis() / 1000) + " seconds");
    SerialBT.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    SerialBT.println("====================");
    
  } else if (command == "wifi status") {
    if (wifiConnected) {
      SerialBT.println("WiFi Connected!");
      SerialBT.println("SSID: " + WiFi.SSID());
      SerialBT.println("IP: " + WiFi.localIP().toString());
      SerialBT.println("Signal: " + String(WiFi.RSSI()) + " dBm");
    } else {
      SerialBT.println("WiFi Disconnected");
    }
    
  } else if (command == "help") {
    SerialBT.println("=== Available Commands ===");
    SerialBT.println("led on        - Turn LED ON");
    SerialBT.println("led off       - Turn LED OFF");
    SerialBT.println("status        - Get device status");
    SerialBT.println("wifi status   - Get WiFi status");
    SerialBT.println("help          - Show this help");
    SerialBT.println("==========================");
    
  } else {
    SerialBT.println("Unknown command: " + command);
    SerialBT.println("Type 'help' for available commands");
  }
}

/**
 * @brief REST API Handlers
 */
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>ESP32 Hybrid Server</title></head>";
  html += "<body><h1>ESP32 REST + Bluetooth Server</h1>";
  html += "<p>LED Status: " + String(ledState ? "ON" : "OFF") + "</p>";
  html += "<p>Bluetooth MAC: " + getBluetoothMAC() + "</p>";  // FIXED: Using helper function
  html += "<p><a href='/led/on'>Turn LED ON</a> | <a href='/led/off'>Turn LED OFF</a></p>";
  html += "<p><a href='/status'>Device Status</a></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleLEDOn() {
  setLED(true);
  server.send(200, "application/json", "{\"status\":\"success\",\"led\":\"on\",\"message\":\"LED turned ON via REST API\"}");
}

void handleLEDOff() {
  setLED(false);
  server.send(200, "application/json", "{\"status\":\"success\",\"led\":\"off\",\"message\":\"LED turned OFF via REST API\"}");
}

void handleStatus() {
  String json = "{";
  json += "\"device\":\"ESP32 Hybrid Server\",";
  json += "\"bluetooth_name\":\"" + String(BT_DEVICE_NAME) + "\",";
  json += "\"bluetooth_mac\":\"" + getBluetoothMAC() + "\",";  // FIXED: Using helper function
  json += "\"wifi_connected\":" + String(wifiConnected ? "true" : "false") + ",";
  if (wifiConnected) {
    json += "\"wifi_ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"wifi_ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
  }
  json += "\"led_state\":\"" + String(ledState ? "on" : "off") + "\",";
  json += "\"uptime_seconds\":" + String(millis() / 1000) + ",";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"bluetooth_connected\":" + String(bluetoothConnected ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "application/json", "{\"error\":\"Endpoint not found\",\"message\":\"Available endpoints: /, /led/on, /led/off, /status\"}");
}

/**
 * @brief Setup function
 */
void setup() {
  // Initialize Serial
  Serial.begin(115200);
  Serial.println();
  Serial.println("ESP32 REST + Bluetooth Hybrid Server Starting...");
  
  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  setLED(false);
  
  // Initialize WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println();
    Serial.println("WiFi connected successfully!");
    Serial.println("IP address: " + WiFi.localIP().toString());
  } else {
    Serial.println();
    Serial.println("WiFi connection failed, continuing with Bluetooth only");
  }
  
  // Initialize Bluetooth
  if (!SerialBT.begin(BT_DEVICE_NAME)) {
    Serial.println("An error occurred initializing Bluetooth");
  } else {
    Serial.println("Bluetooth initialized successfully!");
  }
  
  // Setup REST API endpoints
  if (wifiConnected) {
    server.on("/", handleRoot);
    server.on("/led/on", handleLEDOn);
    server.on("/led/off", handleLEDOff);
    server.on("/status", handleStatus);
    server.onNotFound(handleNotFound);
    
    server.begin();
    Serial.println("REST API server started!");
  }
  
  // Display device information
  Serial.println();
  Serial.println("=== Device Information ===");
  Serial.println("Bluetooth Name: " + String(BT_DEVICE_NAME));
  Serial.println("Bluetooth MAC: " + getBluetoothMAC());  // FIXED: Using helper function
  if (wifiConnected) {
    Serial.println("REST API URL: http://" + WiFi.localIP().toString() + "/");
  }
  Serial.println("==========================");
  Serial.println();
  Serial.println("Device ready! You can:");
  if (wifiConnected) {
    Serial.println("- Access REST API via web browser");
    Serial.println("- Send HTTP requests to control LED");
  }
  Serial.println("- Connect via Bluetooth and send commands");
  Serial.println("- Send 'help' via Bluetooth for command list");
}

/**
 * @brief Main loop
 */
void loop() {
  // Handle REST API requests (if WiFi is connected)
  if (wifiConnected) {
    server.handleClient();
  }
  
  // Handle Bluetooth commands
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    processBluetoothCommand(command);
  }
  
  // Check WiFi connection status
  if (WiFi.status() != WL_CONNECTED && wifiConnected) {
    wifiConnected = false;
    Serial.println("WiFi connection lost");
  } else if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
    wifiConnected = true;
    Serial.println("WiFi reconnected: " + WiFi.localIP().toString());
  }
  
  delay(10);  // Small delay to prevent watchdog issues
}

/*
 * KEY FIXES MADE:
 * 
 * 1. Added getBluetoothMAC() helper function that properly handles getBtAddress()
 *    - Creates uint8_t mac[6] array to store the MAC address
 *    - Uses SerialBT.getBtAddress(mac) with the required parameter
 *    - Formats the MAC address as a readable string using sprintf()
 * 
 * 2. Replaced all instances of SerialBT.getBtAddress() with getBluetoothMAC()
 *    - Line 78 in processBluetoothCommand() function
 *    - Line 129 in setup() function
 *    - Added to REST API responses for consistency
 * 
 * 3. Added comprehensive error handling and status reporting
 * 
 * USAGE:
 * 1. Copy this entire code to your sketch_REST_Socket_Hybrid.ino file
 * 2. Update the WiFi credentials (ssid and password)
 * 3. Compile and upload to your ESP32
 * 
 * The compilation error should now be resolved!
 */