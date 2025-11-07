#include <WiFi.h>
#include <WebServer.h>

// Configuration Constants
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const uint8_t LED_PIN = 2;
const uint16_t SERVER_PORT = 80;
const uint32_t WIFI_TIMEOUT_MS = 10000;
const uint32_t WIFI_CHECK_INTERVAL_MS = 30000;

// Global Objects
WebServer server(SERVER_PORT);

// State Variables
bool ledState = false;
uint32_t lastWifiCheck = 0;

// Helper Functions
void sendJsonResponse(int code, bool success, const char* message, bool includeLedState = true) {
  String response = "{\"success\":";
  response += success ? "true" : "false";
  response += ",\"message\":\"";
  response += message;
  response += "\"";
  
  if (includeLedState) {
    response += ",\"led_state\":";
    response += ledState ? "true" : "false";
  }
  
  response += ",\"device\":\"ESP32\"";
  response += ",\"ip\":\"";
  response += WiFi.localIP().toString();
  response += "\"}";
  
  server.send(code, "application/json", response);
}

void setLED(bool state) {
  ledState = state;
  digitalWrite(LED_PIN, state ? HIGH : LOW);
}

// Endpoint Handlers
void handleRoot() {
  const char* html = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 LED Control</title>
  <style>
    *{margin:0;padding:0;box-sizing:border-box}
    body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#667eea,#764ba2);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
    .container{background:#fff;border-radius:20px;padding:40px;max-width:500px;width:100%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}
    h1{color:#333;text-align:center;margin-bottom:30px}
    .status{background:#f8f9fa;padding:20px;border-radius:10px;margin-bottom:30px;text-align:center}
    .status-label{color:#666;font-size:14px;margin-bottom:10px}
    .status-value{font-size:24px;font-weight:bold;color:#333}
    .status-value.on{color:#28a745}
    .status-value.off{color:#dc3545}
    .controls{display:grid;gap:15px}
    button{padding:15px;border:none;border-radius:10px;font-size:16px;font-weight:bold;cursor:pointer;color:#fff;transition:all 0.3s}
    button:hover{transform:translateY(-2px);box-shadow:0 5px 15px rgba(0,0,0,0.2)}
    .btn-on{background:#28a745}
    .btn-off{background:#dc3545}
    .btn-status{background:#6c757d}
    .info{background:#e7f3ff;padding:15px;border-radius:5px;margin-top:20px;font-size:14px;color:#0c5460}
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 LED Control</h1>
    <div class="status">
      <div class="status-label">LED Status</div>
      <div class="status-value" id="status">Unknown</div>
    </div>
    <div class="controls">
      <button class="btn-on" onclick="control('on')">Turn LED ON</button>
      <button class="btn-off" onclick="control('off')">Turn LED OFF</button>
      <button class="btn-status" onclick="getStatus()">Refresh Status</button>
    </div>
    <div class="info">
      <strong>Device IP:</strong> <span id="ip">Loading...</span><br>
      <strong>Uptime:</strong> <span id="uptime">0s</span>
    </div>
  </div>
  <script>
    async function control(action){
      try{
        const res=await fetch(`/led/${action}`);
        const data=await res.json();
        updateUI(data);
      }catch(e){
        alert('Error: '+e.message);
      }
    }
    async function getStatus(){
      try{
        const res=await fetch('/status');
        const data=await res.json();
        updateUI(data);
      }catch(e){
        alert('Error: '+e.message);
      }
    }
    function updateUI(data){
      const status=document.getElementById('status');
      status.textContent=data.led_state?'ON':'OFF';
      status.className='status-value '+(data.led_state?'on':'off');
      if(data.ip)document.getElementById('ip').textContent=data.ip;
      if(data.uptime_seconds)document.getElementById('uptime').textContent=data.uptime_seconds+'s';
    }
    getStatus();
    setInterval(getStatus,5000);
  </script>
</body>
</html>
)";
  
  server.send(200, "text/html", html);
}
  html += ".status.error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }";
  html += ".status.info { background: #d1ecf1; color: #0c5460; border: 1px solid #bee5eb; }";
  html += ".code-block { background: #282c34; color: #abb2bf; padding: 15px; border-radius: 8px; margin-top: 10px; overflow-x: auto; font-family: 'Courier New', monospace; font-size: 13px; }";
  html += ".method { display: inline-block; padding: 3px 8px; border-radius: 4px; font-weight: bold; font-size: 12px; margin-right: 5px; }";
  html += ".method.get { background: #28a745; color: white; }";
  html += ".method.post { background: #fd7e14; color: white; }";
  html += ".architecture { display: grid; grid-template-columns: 1fr auto 1fr auto 1fr; align-items: center; gap: 10px; text-align: center; margin: 20px 0; }";
  html += ".arch-box { background: #667eea; color: white; padding: 15px; border-radius: 8px; font-weight: bold; }";
  html += ".arch-arrow { font-size: 24px; color: #667eea; }";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>ESP32 + REST API Backend Demo</h1>";
  html += "<p class='subtitle'>Learn how HTML/JavaScript connects to REST APIs</p>";
  
  // Architecture Diagram
  html += "<div class='section'>";
  html += "<h2>Architecture Flow</h2>";
  html += "<div class='architecture'>";
  html += "<div class='arch-box'>Browser<br>HTML/JS</div>";
  html += "<div class='arch-arrow'>&rarr;</div>";
  html += "<div class='arch-box'>Backend API<br>Node.js</div>";
  html += "<div class='arch-arrow'>&rarr;</div>";
  html += "<div class='arch-box'>ESP32<br>Device</div>";
  html += "</div>";
  html += "</div>";
  
  // Configuration Info
  html += "<div class='section'>";
  html += "<h2>Configuration</h2>";
  html += "<div class='info-box'>";
  html += "<strong>ESP32 IP Address:</strong> <code>" + WiFi.localIP().toString() + "</code>";
  html += "</div>";
  html += "<div class='info-box'>";
  html += "<strong>Backend API URL:</strong> <code>" + String(BACKEND_API_URL) + "</code>";
  html += "</div>";
  html += "<div class='info-box'>";
  html += "<strong>Current Page URL:</strong> <code id='currentUrl'></code>";
  html += "</div>";
  html += "</div>";
  
  // Direct ESP32 Control
  html += "<div class='section'>";
  html += "<h2>Method 1: Direct ESP32 Control (No Backend)</h2>";
  html += "<p>These buttons communicate directly with the ESP32 device:</p>";
  html += "<div class='button-grid'>";
  html += "<button class='btn-direct' onclick=\"directControl('/led/on', 'GET')\">LED ON (Direct)</button>";
  html += "<button class='btn-direct' onclick=\"directControl('/led/off', 'GET')\">LED OFF (Direct)</button>";
  html += "<button class='btn-direct' onclick=\"directControl('/status', 'GET')\">Get Status (Direct)</button>";
  html += "</div>";
  html += "<div class='code-block'>fetch('http://" + WiFi.localIP().toString() + "/led/on')</div>";
  html += "</div>";
  
  // Backend API Control
  html += "<div class='section'>";
  html += "<h2>Method 2: Through Backend API (Recommended)</h2>";
  html += "<p>These buttons go through the Node.js backend API:</p>";
  html += "<div class='button-grid'>";
  html += "<button class='btn-api' onclick=\"apiControl('/api/led/on', 'POST')\">LED ON (via API)</button>";
  html += "<button class='btn-api' onclick=\"apiControl('/api/led/off', 'POST')\">LED OFF (via API)</button>";
  html += "<button class='btn-api' onclick=\"apiControl('/api/led/toggle', 'POST')\">Toggle LED (via API)</button>";
  html += "<button class='btn-status' onclick=\"apiControl('/api/status', 'GET')\">Get Status (via API)</button>";
  html += "</div>";
  html += "<div class='code-block'>fetch('" + String(BACKEND_API_URL) + "/api/led/on', { method: 'POST' })</div>";
  html += "</div>";
  
  // REST Methods Demo
  html += "<div class='section'>";
  html += "<h2>REST Methods Examples</h2>";
  html += "<div class='button-grid'>";
  html += "<button class='btn-get' onclick=\"restDemo('GET')\"><span class='method get'>GET</span> Request</button>";
  html += "<button class='btn-post' onclick=\"restDemo('POST')\"><span class='method post'>POST</span> Request</button>";
  html += "<button class='btn-post' onclick=\"restDemo('POST-JSON')\"><span class='method post'>POST</span> with JSON</button>";
  html += "</div>";
  html += "</div>";
  
  // Response Display
  html += "<div class='section'>";
  html += "<h2>API Response</h2>";
  html += "<div id='response' class='status info'>Click any button to see the API response here...</div>";
  html += "</div>";
  
  html += "</div>";
  
  // JavaScript
  html += "<script>";
  html += "document.getElementById('currentUrl').textContent = window.location.href;";
  
  // Direct ESP32 control function
  html += "async function directControl(endpoint, method) {";
  html += "  const url = 'http://" + WiFi.localIP().toString() + "' + endpoint;";
  html += "  displayResponse('info', 'Sending ' + method + ' request to ESP32...\\n' + url);";
  html += "  try {";
  html += "    const response = await fetch(url, { method: method });";
  html += "    const data = await response.json();";
  html += "    displayResponse('success', 'SUCCESS - Direct ESP32:\\n' + JSON.stringify(data, null, 2));";
  html += "  } catch (error) {";
  html += "    displayResponse('error', 'ERROR: ' + error.message);";
  html += "  }";
  html += "}";
  
  // Backend API control function
  html += "async function apiControl(endpoint, method) {";
  html += "  const url = '" + String(BACKEND_API_URL) + "' + endpoint;";
  html += "  displayResponse('info', 'Sending ' + method + ' request to Backend API...\\n' + url);";
  html += "  try {";
  html += "    const response = await fetch(url, { ";
  html += "      method: method,";
  html += "      headers: { 'Content-Type': 'application/json' }";
  html += "    });";
  html += "    const data = await response.json();";
  html += "    displayResponse('success', 'SUCCESS - Backend API Response:\\n' + JSON.stringify(data, null, 2));";
  html += "  } catch (error) {";
  html += "    displayResponse('error', 'ERROR: ' + error.message + '\\n\\nMake sure backend is running at: " + String(BACKEND_API_URL) + "');";
  html += "  }";
  html += "}";
  
  // REST methods demo
  html += "async function restDemo(type) {";
  html += "  const url = '" + String(BACKEND_API_URL) + "';";
  html += "  let config = {};";
  html += "  switch(type) {";
  html += "    case 'GET':";
  html += "      config = { method: 'GET' };";
  html += "      displayResponse('info', 'GET Request Example:\\n' + url + '/api/status');";
  html += "      try {";
  html += "        const response = await fetch(url + '/api/status', config);";
  html += "        const data = await response.json();";
  html += "        displayResponse('success', 'GET Response:\\n' + JSON.stringify(data, null, 2));";
  html += "      } catch (error) { displayResponse('error', 'ERROR: ' + error.message); }";
  html += "      break;";
  html += "    case 'POST':";
  html += "      config = { method: 'POST', headers: { 'Content-Type': 'application/json' } };";
  html += "      displayResponse('info', 'POST Request Example:\\n' + url + '/api/led/on');";
  html += "      try {";
  html += "        const response = await fetch(url + '/api/led/on', config);";
  html += "        const data = await response.json();";
  html += "        displayResponse('success', 'POST Response:\\n' + JSON.stringify(data, null, 2));";
  html += "      } catch (error) { displayResponse('error', 'ERROR: ' + error.message); }";
  html += "      break;";
  html += "    case 'POST-JSON':";
  html += "      config = { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ state: true }) };";
  html += "      displayResponse('info', 'POST with JSON Body:\\n' + url + '/api/led/control\\nBody: {\"state\": true}');";
  html += "      try {";
  html += "        const response = await fetch(url + '/api/led/control', config);";
  html += "        const data = await response.json();";
  html += "        displayResponse('success', 'POST Response:\\n' + JSON.stringify(data, null, 2));";
  html += "      } catch (error) { displayResponse('error', 'ERROR: ' + error.message); }";
  html += "      break;";
  html += "  }";
  html += "}";
  
  // Display response function
  html += "function displayResponse(type, message) {";
  html += "  const responseDiv = document.getElementById('response');";
  html += "  responseDiv.className = 'status ' + type;";
  html += "  responseDiv.textContent = message;";
  html += "}";
  
  html += "</script>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleLedOn() {
  setLED(true);
  sendJsonResponse(200, true, "LED turned ON");
}

void handleLedOff() {
  setLED(false);
  sendJsonResponse(200, true, "LED turned OFF");
}

void handleStatus() {
  String response = "{";
  response += "\"success\":true";
  response += ",\"device\":\"ESP32\"";
  response += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
  response += ",\"wifi_signal\":" + String(WiFi.RSSI());
  response += ",\"led_state\":" + String(ledState ? "true" : "false");
  response += ",\"uptime_seconds\":" + String(millis() / 1000);
  response += ",\"free_memory\":" + String(ESP.getFreeHeap());
  response += "}";
  
  server.send(200, "application/json", response);
}

void handleLedControl() {
  if (!server.hasArg("plain")) {
    sendJsonResponse(400, false, "No JSON payload received", false);
    return;
  }
  
  String body = server.arg("plain");
  body.toLowerCase();
  
  if (body.indexOf("\"state\":true") != -1 || body.indexOf("\"state\": true") != -1) {
    setLED(true);
    sendJsonResponse(200, true, "LED turned ON");
  } else if (body.indexOf("\"state\":false") != -1 || body.indexOf("\"state\": false") != -1) {
    setLED(false);
    sendJsonResponse(200, true, "LED turned OFF");
  } else {
    sendJsonResponse(400, false, "Invalid JSON. Expected {\"state\": true/false}", false);
  }
}

void handleNotFound() {
  sendJsonResponse(404, false, "Endpoint not found", false);
}

bool connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  uint32_t startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > WIFI_TIMEOUT_MS) {
      Serial.println("\nWiFi connection failed!");
      return false;
    }
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signal: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  
  return true;
}

void checkWiFiConnection() {
  if (millis() - lastWifiCheck < WIFI_CHECK_INTERVAL_MS) {
    return;
  }
  
  lastWifiCheck = millis();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    WiFi.reconnect();
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  
  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  setLED(false);
  
  Serial.println("\n=== ESP32 REST API Server ===");
  
  // Connect to WiFi
  if (!connectWiFi()) {
    Serial.println("ERROR: WiFi connection failed. Halting.");
    while(1) delay(1000);
  }
  
  // Configure routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/led/on", HTTP_GET, handleLedOn);
  server.on("/led/off", HTTP_GET, handleLedOff);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/led", HTTP_POST, handleLedControl);
  server.onNotFound(handleNotFound);
  
  // Start server
  server.begin();
  
  Serial.println("=== Server Started ===");
  Serial.println("Endpoints:");
  Serial.println("  GET  /");
  Serial.println("  GET  /led/on");
  Serial.println("  GET  /led/off");
  Serial.println("  GET  /status");
  Serial.println("  POST /led");
  Serial.println("======================");
}

void loop() {
  server.handleClient();
  checkWiFiConnection();
  delay(1);
}