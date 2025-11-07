# Quick Start Guide - ESP32 REST API Demo

## Overview
This project demonstrates how a web browser communicates with an ESP32 through a Node.js backend API.

```
Browser (HTML/JS) → Node.js API → ESP32 Device
```

## What You'll Need
- ESP32 development board
- Computer with Node.js installed
- Arduino IDE with ESP32 support
- WiFi network

## Quick Setup (5 Minutes)

### Step 1: Start the Backend Server

```bash
# Install dependencies
npm install

# Update .env file with your ESP32 IP (you'll get this in step 2)
# ESP32_IP=192.168.1.XXX

# Start server
npm start
```

The server will show you all available network addresses:
```
Local: http://localhost:3000
Network access:
  http://192.168.1.78:3000  ← Use this for ESP32 configuration
```

### Step 2: Configure and Upload ESP32 Code

1. Open `ESP32_REST_Demo.cpp` in Arduino IDE

2. Update these three lines:
```cpp
const char* ssid = "YOUR_WIFI_SSID";           // Your WiFi name
const char* password = "YOUR_WIFI_PASSWORD";   // Your WiFi password
const char* BACKEND_API_URL = "http://192.168.1.78:3000";  // From Step 1
```

3. Upload to ESP32

4. Open Serial Monitor (115200 baud) and note the ESP32's IP address:
```
ESP32 IP Address: 192.168.1.100  ← Note this!
```

### Step 3: Update Backend with ESP32 IP

Update `.env` file:
```
ESP32_IP=192.168.1.100
PORT=3000
```

Restart the backend server (Ctrl+C, then `npm start`)

### Step 4: Access the Web Interface

Open browser to: `http://localhost:3000`

You should see:
- System status showing both backend and ESP32 online
- LED control buttons
- API response display
- Code examples

## Two Ways to Access

### Method 1: Backend-Served Interface (Recommended)
```
http://localhost:3000
```
- Full-featured web interface
- Goes through Node.js backend
- Better for production use

### Method 2: ESP32-Served Interface
```
http://[ESP32_IP_ADDRESS]
```
- Served directly from ESP32
- Shows direct vs API communication
- Educational comparison

## Testing the Setup

### Test 1: Check Backend Health
```bash
curl http://localhost:3000/api/health
```

Expected response:
```json
{
  "backend": "online",
  "esp32": "online"
}
```

### Test 2: Control LED
```bash
# Turn LED ON
curl -X POST http://localhost:3000/api/led/on

# Turn LED OFF
curl -X POST http://localhost:3000/api/led/off
```

### Test 3: Get Status
```bash
curl http://localhost:3000/api/status
```

## Interactive Controllers

### Web Interface
- Open browser to `http://localhost:3000`
- Click buttons to control LED
- See real-time API responses

### Command Line
```bash
# Interactive terminal controller
node led-toggle.js

# Simple one-off commands
node led-simple.js on
node led-simple.js off
node led-simple.js test
```

## Understanding the Communication

### When you click "Turn LED ON" in the browser:

1. **Browser JavaScript** sends HTTP POST request:
```javascript
fetch('/api/led/on', { method: 'POST' })
```

2. **Node.js Backend** receives request and forwards to ESP32:
```javascript
axios.post('http://192.168.1.100/led/on')
```

3. **ESP32** receives request and turns on LED:
```cpp
digitalWrite(ledPin, HIGH);
```

4. **ESP32** sends JSON response back:
```json
{"success": true, "led_state": true}
```

5. **Backend** forwards response to browser

6. **Browser** displays result to user

## Troubleshooting

### Problem: Backend can't connect to ESP32
**Solution**: 
- Verify ESP32 IP in `.env` file
- Ping ESP32: `ping [ESP32_IP]`
- Check ESP32 Serial Monitor for connection status

### Problem: Can't access backend from network
**Solution**:
- Check Windows Firewall
- Verify backend is running
- Try `http://localhost:3000` first

### Problem: LED doesn't respond
**Solution**:
- Check ESP32 is powered and running
- Verify WiFi connection in Serial Monitor
- Test direct endpoint: `http://[ESP32_IP]/led/on`

## Next Steps

1. **Explore the web interface** - Click all the buttons and see responses
2. **Try command line tools** - Use `led-toggle.js` for interactive control
3. **Read the full guide** - Check `REST_API_GUIDE.md` for details
4. **Modify the code** - Add sensors, more LEDs, or new features

## API Endpoints Quick Reference

### Backend API (Port 3000)
- `GET /api/health` - System health check
- `GET /api/status` - Get ESP32 status
- `POST /api/led/on` - Turn LED on
- `POST /api/led/off` - Turn LED off
- `POST /api/led/toggle` - Toggle LED
- `POST /api/led/control` - Control with JSON body

### ESP32 Direct (Port 80)
- `GET /` - Interactive demo page
- `GET /led/on` - Turn LED on
- `GET /led/off` - Turn LED off
- `GET /status` - Get device status
- `POST /led` - Control with JSON: `{"state": true}`

## Files Reference

| File | Purpose |
|------|---------|
| `ESP32_REST_Demo.cpp` | ESP32 firmware with REST demo |
| `esp32-controller.js` | Node.js backend server |
| `public/index.html` | Web interface (HTML) |
| `public/app.js` | Web interface (JavaScript) |
| `public/styles.css` | Web interface (CSS) |
| `led-toggle.js` | Interactive CLI controller |
| `led-simple.js` | Simple CLI controller |
| `.env` | Configuration (ESP32 IP, port) |

## Tips

- **Keep Serial Monitor open** to see ESP32 debug messages
- **Use browser DevTools** (F12) to inspect network requests
- **Check backend console** for API call logs
- **Test direct ESP32 access** first before using backend
- **Update IP addresses** whenever network changes

## Getting Help

Check these in order:
1. ESP32 Serial Monitor (115200 baud)
2. Backend console output
3. Browser DevTools → Console tab
4. Browser DevTools → Network tab
5. `REST_API_GUIDE.md` for detailed docs

Happy coding! 🚀