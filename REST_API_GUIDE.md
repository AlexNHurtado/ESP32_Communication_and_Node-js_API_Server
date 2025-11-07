# ESP32 REST API Communication Demo

This project demonstrates how HTML/JavaScript connects to a REST API backend (Node.js) to control an ESP32 microcontroller device.

## Architecture Overview

```
┌─────────────┐         ┌──────────────┐         ┌──────────────┐
│   Browser   │  HTTP   │   Node.js    │  HTTP   │    ESP32     │
│ HTML/CSS/JS │ ◄─────► │   Backend    │ ◄─────► │    Device    │
│             │         │   (API)      │         │              │
└─────────────┘         └──────────────┘         └──────────────┘
     Port 80              Port 3000                Port 80
```

### Communication Flow:

1. **Browser → Backend API**: User interacts with HTML interface, JavaScript sends HTTP requests
2. **Backend API → ESP32**: Node.js server forwards requests to ESP32 device
3. **ESP32 → Backend API**: ESP32 responds with JSON data
4. **Backend API → Browser**: Response is sent back and displayed in the UI

## Project Files

### ESP32 Code
- **`ESP32_REST_Demo.cpp`** - ESP32 code that:
  - Serves an interactive HTML page demonstrating REST API calls
  - Provides REST endpoints for LED control
  - Shows two methods: Direct ESP32 access and API backend access

### Backend API (Node.js)
- **`esp32-controller.js`** - Express.js backend that:
  - Acts as middleware between browser and ESP32
  - Provides RESTful API endpoints
  - Handles CORS and serves static files
  - Monitors ESP32 connectivity

### Web Interface
- **`public/index.html`** - Main HTML interface
- **`public/styles.css`** - Styling for the web interface
- **`public/app.js`** - JavaScript for API communication

### Utility Scripts
- **`led-toggle.js`** - Interactive command-line LED controller
- **`led-simple.js`** - Simple command-line LED controller

## Setup Instructions

### 1. ESP32 Setup

#### Requirements:
- ESP32 development board
- Arduino IDE with ESP32 board support
- WiFi network

#### Steps:
1. Open `ESP32_REST_Demo.cpp` in Arduino IDE
2. Update WiFi credentials:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
3. Update backend API URL (after starting backend):
   ```cpp
   const char* BACKEND_API_URL = "http://YOUR_COMPUTER_IP:3000";
   ```
4. Upload to ESP32
5. Open Serial Monitor (115200 baud) to see the ESP32's IP address

### 2. Backend API Setup

#### Requirements:
- Node.js (v14 or higher)
- npm

#### Steps:
1. Install dependencies:
   ```bash
   npm install
   ```

2. Update `.env` file with your ESP32's IP address:
   ```
   ESP32_IP=192.168.1.XXX
   PORT=3000
   ```

3. Start the backend server:
   ```bash
   npm start
   ```

4. Server will display available network addresses:
   ```
   Local: http://localhost:3000
   Network access:
     http://192.168.1.XXX:3000
   ```

### 3. Access the Web Interface

Open your browser to:
- **Backend-served interface**: `http://localhost:3000`
- **ESP32-served interface**: `http://[ESP32_IP_ADDRESS]`

## REST API Endpoints

### Backend API (Node.js - Port 3000)

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/` | Serves web interface |
| GET | `/api/health` | Check backend and ESP32 connectivity |
| GET | `/api/status` | Get device status from ESP32 |
| POST | `/api/led/on` | Turn LED on |
| POST | `/api/led/off` | Turn LED off |
| POST | `/api/led/toggle` | Toggle LED state |
| POST | `/api/led/control` | Control LED with JSON body |

### ESP32 Endpoints (Port 80)

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/` | Serves interactive REST demo page |
| GET | `/led/on` | Turn LED on |
| GET | `/led/off` | Turn LED off |
| GET | `/status` | Get device status |
| POST | `/led` | Control LED with JSON: `{"state": true/false}` |

## Usage Examples

### JavaScript (Browser)

```javascript
// Turn LED ON via backend API
async function turnLedOn() {
  const response = await fetch('/api/led/on', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    }
  });
  const data = await response.json();
  console.log(data);
}

// Control LED with JSON
async function controlLed(state) {
  const response = await fetch('/api/led/control', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({ state: state })
  });
  const data = await response.json();
  console.log(data);
}
```

### cURL

```bash
# Turn LED ON (via backend)
curl -X POST http://localhost:3000/api/led/on

# Turn LED OFF (via backend)
curl -X POST http://localhost:3000/api/led/off

# Control with JSON (via backend)
curl -X POST http://localhost:3000/api/led/control \
  -H "Content-Type: application/json" \
  -d '{"state": true}'

# Get status
curl http://localhost:3000/api/status

# Direct ESP32 control
curl http://[ESP32_IP]/led/on
```

### Python

```python
import requests

# Via backend API
response = requests.post('http://localhost:3000/api/led/on')
print(response.json())

# Control with JSON
response = requests.post('http://localhost:3000/api/led/control',
                        json={'state': True})
print(response.json())

# Get status
response = requests.get('http://localhost:3000/api/status')
print(response.json())
```

### Node.js Command Line

```bash
# Interactive controller
node led-toggle.js

# Simple commands
node led-simple.js on
node led-simple.js off
node led-simple.js test
```

## How REST Communication Works

### 1. HTTP Methods

- **GET**: Retrieve data (read-only)
  - Example: Getting device status
  - No request body needed
  
- **POST**: Create or trigger action
  - Example: Turning LED on/off
  - Can include JSON body with parameters

- **PUT**: Update existing resource
  - Example: Updating configuration
  
- **DELETE**: Remove resource

### 2. Request/Response Cycle

#### Request Components:
```
POST /api/led/on HTTP/1.1
Host: localhost:3000
Content-Type: application/json

{
  "state": true
}
```

#### Response Components:
```
HTTP/1.1 200 OK
Content-Type: application/json

{
  "success": true,
  "led_state": true,
  "message": "LED turned ON"
}
```

### 3. JSON Data Format

All API responses use JSON format:

```json
{
  "success": true,
  "data": {
    "led_state": true,
    "device": "ESP32",
    "ip": "192.168.1.100"
  },
  "timestamp": 1234567890
}
```

## Why Use a Backend API?

### Benefits of Backend Middleware:

1. **Security**: 
   - Backend can authenticate requests
   - Hide ESP32 IP from public
   - Rate limiting and request validation

2. **Scalability**:
   - Control multiple ESP32 devices
   - Load balancing
   - Caching responses

3. **Data Processing**:
   - Store data in database
   - Process and transform data
   - Aggregate data from multiple devices

4. **Reliability**:
   - Retry failed requests
   - Queue commands
   - Error handling and logging

5. **CORS Handling**:
   - Backend handles cross-origin requests
   - Simplifies web application development

## Troubleshooting

### ESP32 Not Responding

1. Check WiFi connection:
   - Verify ESP32 is connected to WiFi (check Serial Monitor)
   - Ping ESP32: `ping [ESP32_IP]`

2. Check firewall:
   - Windows Firewall might block connections
   - Temporarily disable to test

3. Verify IP address:
   - Update `.env` file with correct ESP32 IP
   - Restart backend after changing IP

### Backend API Not Accessible

1. Check if server is running:
   - Look for "ESP32 Controller Backend Started" message
   - Verify no port conflicts (port 3000)

2. Test localhost first:
   - `curl http://localhost:3000/api/health`

3. Check network firewall:
   - Allow Node.js through firewall
   - Test from same computer first

### CORS Errors

1. Backend already includes CORS middleware
2. If issues persist, check browser console for specific errors
3. Verify `Access-Control-Allow-Origin` headers in response

## Learning Resources

### REST API Concepts:
- [REST API Tutorial](https://restfulapi.net/)
- [HTTP Methods](https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods)
- [JSON Format](https://www.json.org/)

### JavaScript Fetch API:
- [MDN Fetch Documentation](https://developer.mozilla.org/en-US/docs/Web/API/Fetch_API)
- [Using Fetch](https://developer.mozilla.org/en-US/docs/Web/API/Fetch_API/Using_Fetch)

### ESP32 Development:
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [WebServer Library](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer)

## Additional Features to Implement

### Beginner Level:
- [ ] Add more LED colors (if using RGB LED)
- [ ] Display uptime on web interface
- [ ] Add button to restart ESP32

### Intermediate Level:
- [ ] Read sensor data and display on web interface
- [ ] Store historical data in database (MongoDB)
- [ ] Add user authentication
- [ ] Create mobile-responsive design

### Advanced Level:
- [ ] WebSocket for real-time updates
- [ ] MQTT protocol integration
- [ ] Multiple ESP32 device management
- [ ] Data visualization with charts
- [ ] Automation rules (if-then scenarios)

## License

This project is for educational purposes demonstrating REST API communication between web interfaces, backend servers, and IoT devices.

## Support

For issues or questions:
1. Check Serial Monitor for ESP32 errors
2. Check backend console for API errors
3. Use browser developer tools (F12) to inspect network requests
4. Verify all IP addresses are correct in configuration files