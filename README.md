# ESP32 Controller Backend

A Node.js backend server that provides HTTP API endpoints to control an ESP32 microcontroller with LED functionality.

## 🚀 Features

- **RESTful API** to control ESP32 LED
- **Health monitoring** for ESP32 connectivity
- **JSON responses** with detailed status information
- **Error handling** with meaningful messages
- **Configurable ESP32 IP** address
- **CORS support** for web applications
- **Automatic ESP32 connectivity testing**

## 📋 API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/` | API documentation |
| GET | `/api/health` | Check ESP32 connectivity |
| GET | `/api/status` | Get complete device status |
| GET | `/api/led/status` | Get LED status only |
| POST | `/api/led/on` | Turn LED on |
| POST | `/api/led/off` | Turn LED off |
| POST | `/api/led/toggle` | Toggle LED state |
| POST | `/api/led/control` | Control LED with JSON `{"state": true/false}` |
| PUT | `/api/config` | Update ESP32 IP configuration |

## 🛠️ Setup Instructions

### Prerequisites
- Node.js (v14 or higher)
- ESP32 with the provided WiFi endpoint code running
- Both devices on the same network

### 1. Install Dependencies
```bash
npm install
```

### 2. Configure ESP32 IP Address
Edit the `.env` file and update the ESP32 IP address:
```env
ESP32_IP=192.168.1.100  # Replace with your ESP32's actual IP
ESP32_PORT=80
PORT=3000
```

### 3. Start the Server
```bash
# Production mode
npm start

# Development mode (with auto-restart)
npm run dev
```

### 4. Test the API
```bash
# Run automated tests
npm test

# Run interactive demo
node test-api.js --interactive
```

## 📱 Usage Examples

### Using curl:
```bash
# Check if ESP32 is reachable
curl http://localhost:3000/api/health

# Turn LED on
curl -X POST http://localhost:3000/api/led/on

# Turn LED off
curl -X POST http://localhost:3000/api/led/off

# Toggle LED
curl -X POST http://localhost:3000/api/led/toggle

# Control with JSON
curl -X POST http://localhost:3000/api/led/control \
  -H "Content-Type: application/json" \
  -d '{"state": true}'

# Get device status
curl http://localhost:3000/api/status
```

### Using JavaScript (fetch):
```javascript
// Turn LED on
fetch('http://localhost:3000/api/led/on', {method: 'POST'})
  .then(response => response.json())
  .then(data => console.log(data));

// Get status
fetch('http://localhost:3000/api/status')
  .then(response => response.json())
  .then(data => console.log(data));

// Control with JSON
fetch('http://localhost:3000/api/led/control', {
  method: 'POST',
  headers: {'Content-Type': 'application/json'},
  body: JSON.stringify({state: true})
});
```

### Using Python (requests):
```python
import requests

BASE_URL = "http://localhost:3000"

# Turn LED on
response = requests.post(f"{BASE_URL}/api/led/on")
print(response.json())

# Get status
response = requests.get(f"{BASE_URL}/api/status")
print(response.json())

# Control with JSON
response = requests.post(f"{BASE_URL}/api/led/control", 
                        json={"state": True})
print(response.json())
```

## 🔧 Configuration

### Environment Variables (.env file):
```env
# ESP32 Configuration
ESP32_IP=192.168.1.100    # Your ESP32's IP address
ESP32_PORT=80             # ESP32 web server port

# Backend Server Configuration  
PORT=3000                 # Node.js server port
```

### Dynamic IP Update:
You can update the ESP32 IP address without restarting the server:
```bash
curl -X PUT http://localhost:3000/api/config \
  -H "Content-Type: application/json" \
  -d '{"ip": "192.168.1.150", "port": 80}'
```

## 📊 Response Examples

### Successful LED Control:
```json
{
  "success": true,
  "action": "LED turned ON",
  "esp32_response": {
    "success": true,
    "led_state": true,
    "timestamp": 12345
  },
  "timestamp": "2025-10-30T10:30:00.000Z"
}
```

### ESP32 Status:
```json
{
  "success": true,
  "backend_server": {
    "port": 3000,
    "uptime": 123.45
  },
  "esp32": {
    "device": "ESP32",
    "ip": "192.168.1.100",
    "wifi_signal": -45,
    "led_state": true,
    "uptime_seconds": 3600,
    "free_memory": 123456
  }
}
```

### Error Response:
```json
{
  "success": false,
  "error": "ESP32 device is not reachable",
  "error_code": "CONNECTION_REFUSED",
  "timestamp": "2025-10-30T10:30:00.000Z"
}
```

## 🐛 Troubleshooting

### ESP32 Not Reachable:
1. Check if ESP32 is powered on
2. Verify ESP32 is connected to WiFi
3. Confirm ESP32 IP address in `.env` file
4. Test ESP32 directly: `http://[ESP32_IP]/status`
5. Check firewall settings

### Connection Timeout:
1. Verify both devices are on same network
2. Check network connectivity: `ping [ESP32_IP]`
3. Ensure ESP32 web server is running
4. Try increasing timeout in `esp32-controller.js`

### Port Already in Use:
```bash
# Change port in .env file or use different port
PORT=3001 npm start
```

## 🔍 Development

### Project Structure:
```
├── esp32-controller.js    # Main server file
├── package.json          # Dependencies and scripts
├── .env                  # Configuration
├── test-api.js          # API testing script
├── ESP_WIFI_Endpoint.cpp # ESP32 code
└── README.md            # Documentation
```

### Adding New Features:
1. Add new endpoint in `esp32-controller.js`
2. Update API documentation in root endpoint
3. Add tests in `test-api.js`
4. Update README.md

## 📝 License

MIT License - Feel free to use and modify for your projects!

---

**Happy coding! 🎉**