# ESP32 HTTP REST API Controller

Traditional HTTP REST API communication between browser, backend, and ESP32.

## 🏗️ Architecture

```
┌─────────────────┐         ┌─────────────────┐         ┌─────────────────┐
│   Web Browser   │────────►│   Backend API   │────────►│     ESP32       │
│                 │         │   (Node.js)     │         │   (Hardware)    │
│  - Fetch API    │         │  - Express.js   │         │  - WebServer    │
│  - HTML/CSS/JS  │         │  - HTTP Proxy   │         │  - LED Control  │
│  - Polling      │◄────────│  - REST Routes  │◄────────│  - REST API     │
└─────────────────┘         └─────────────────┘         └─────────────────┘
   http://localhost:3000         axios requests              http://ESP32_IP:80
```

## 📦 Project Structure

```
http-REST/
├── ESP32_REST_Minimal.cpp         # ESP32 firmware (production-ready)
├── esp32-controller.js            # Node.js Express backend
├── package.json                   # Node.js dependencies
├── .env                          # Configuration
├── public/
│   ├── index.html                # Web interface
│   ├── app.js                    # Frontend JavaScript
│   └── styles.css                # Styling
└── README.md                     # This file
```

## 🚀 Quick Start

### 1. ESP32 Setup

**Requirements:**
- ESP32 board
- Arduino IDE
- WiFi.h and WebServer.h libraries (built-in)

**Steps:**
1. Open `ESP32_REST_Minimal.cpp` in Arduino IDE
2. Upload to ESP32
3. Open Serial Monitor (115200 baud)
4. Enter WiFi credentials when prompted
5. Note the IP address displayed

### 2. Backend Setup

**Requirements:**
- Node.js 14+ installed

**Steps:**
```bash
# Navigate to http-REST directory
cd http-REST

# Install dependencies
npm install

# Configure .env file
# Edit ESP32_IP to match your ESP32's IP address

# Start server
npm start
```

### 3. Access Web Interface

Open browser to: `http://localhost:3000`

## 🔧 Configuration

Edit `.env` file:

```env
PORT=3000                    # Backend server port
ESP32_IP=192.168.1.100      # Your ESP32 IP address
```

## 📡 REST API Endpoints

### ESP32 Endpoints (Direct Access)

**GET `/status`** - Get device status
```bash
curl http://192.168.1.100/status
```

Response:
```json
{
  "device": "ESP32",
  "ip": "192.168.1.100",
  "ssid": "MyWiFi",
  "rssi": -45,
  "led": true,
  "uptime": 3600,
  "heap": 295432,
  "timestamp": 123456
}
```

**GET `/led/on`** - Turn LED on
```bash
curl http://192.168.1.100/led/on
```

**GET `/led/off`** - Turn LED off
```bash
curl http://192.168.1.100/led/off
```

**POST `/led`** - Control LED with JSON
```bash
curl -X POST http://192.168.1.100/led \
  -H "Content-Type: application/json" \
  -d '{"state": true}'
```

### Backend Proxy Endpoints

**GET `/api/health`** - Backend health check
```bash
curl http://localhost:3000/api/health
```

**GET `/api/status`** - Get ESP32 status (proxied)
```bash
curl http://localhost:3000/api/status
```

**POST `/api/led/on`** - Turn LED on (proxied)
```bash
curl -X POST http://localhost:3000/api/led/on
```

**POST `/api/led/off`** - Turn LED off (proxied)
```bash
curl -X POST http://localhost:3000/api/led/off
```

**POST `/api/led/toggle`** - Toggle LED (proxied)
```bash
curl -X POST http://localhost:3000/api/led/toggle
```

## ✨ Key Features

### HTTP REST Architecture
- **Request/Response** - Traditional HTTP model
- **Stateless** - Each request is independent
- **Cache-friendly** - Responses can be cached
- **Polling** - Frontend polls for updates (every 5 seconds)
- **Standard HTTP methods** - GET, POST

### Production Ready
- ✅ Error handling
- ✅ CORS enabled
- ✅ Input validation
- ✅ Connection monitoring
- ✅ Serial WiFi configuration
- ✅ Auto-reconnection

## 🔍 Troubleshooting

### ESP32 Won't Connect
1. Check Serial Monitor for errors
2. Verify WiFi credentials
3. Ensure 2.4GHz network
4. Check WiFi signal strength

### Backend Can't Connect to ESP32
1. Verify ESP32 IP in `.env`
2. Ping ESP32: `ping [ESP32_IP]`
3. Check firewall settings
4. Ensure same network

### Browser Can't Connect
1. Check backend is running
2. Verify URL: `http://localhost:3000`
3. Check browser console for errors
4. Ensure port 3000 is not in use

## 📊 Performance Metrics

| Metric | Value |
|--------|-------|
| **Request Latency** | ~100-500ms |
| **Connection Time** | New per request |
| **Status Update Rate** | Manual/Polling |
| **Reconnect Delay** | Immediate retry |
| **Memory Usage** | ~2KB (ESP32) |
| **Concurrent Requests** | 1 at a time |

## 🆚 REST vs WebSocket

| Feature | REST (This Project) | WebSocket |
|---------|---------------------|-----------|
| **Communication** | Request/Response | Bidirectional |
| **Connection** | New per request | Persistent |
| **Real-time Updates** | Polling required | Push notifications |
| **Latency** | Higher | Lower |
| **Complexity** | Simpler | More complex |
| **Browser Support** | Universal | Modern browsers |

## 📚 Technology Stack

### ESP32
- **Language**: C++ (Arduino)
- **Libraries**: WiFi.h, WebServer.h
- **Port**: 80 (HTTP)
- **Protocol**: HTTP/1.1

### Backend
- **Runtime**: Node.js
- **Framework**: Express.js
- **HTTP Client**: Axios
- **Middleware**: CORS, dotenv
- **Port**: 3000

### Frontend
- **Languages**: HTML5, CSS3, JavaScript (ES6+)
- **API**: Fetch API
- **Pattern**: Polling (every 5 seconds)
- **UI**: Responsive design

## 🎯 Use Cases

### When to Use REST:
✅ Simple control operations
✅ Infrequent updates
✅ Firewall-friendly deployment
✅ Standard HTTP infrastructure
✅ Caching beneficial
✅ Stateless preferred

### When NOT to Use REST:
❌ Real-time monitoring needed
❌ Low latency required
❌ Server push notifications
❌ Frequent status updates
❌ Persistent connection beneficial

## 📖 Testing

### Test ESP32 Directly
```bash
# Set ESP32 IP
$ESP32_IP = "192.168.1.100"

# Get status
curl http://$ESP32_IP/status

# Turn LED on
curl http://$ESP32_IP/led/on

# Turn LED off
curl http://$ESP32_IP/led/off

# Control with JSON
curl -X POST http://$ESP32_IP/led `
  -H "Content-Type: application/json" `
  -d '{"state":true}'
```

### Test Backend API
```bash
# Health check
curl http://localhost:3000/api/health

# Get status (proxied)
curl http://localhost:3000/api/status

# Control LED (proxied)
curl -X POST http://localhost:3000/api/led/on
curl -X POST http://localhost:3000/api/led/off
curl -X POST http://localhost:3000/api/led/toggle
```

## 🔐 Security Considerations

**Current Implementation:**
- ❌ No authentication
- ❌ No HTTPS (HTTP only)
- ❌ Open to local network

**Production Recommendations:**
- ✅ Use HTTPS with TLS certificates
- ✅ Implement API authentication (JWT, API keys)
- ✅ Add rate limiting
- ✅ Validate all inputs
- ✅ Use environment variables for secrets

## 🚀 Advanced Features

### Add Authentication
```javascript
// Backend - verify API key
app.use((req, res, next) => {
  const apiKey = req.headers['x-api-key'];
  if (apiKey !== process.env.API_KEY) {
    return res.status(401).json({ error: 'Unauthorized' });
  }
  next();
});
```

### Add Request Logging
```javascript
// Backend - log all requests
app.use((req, res, next) => {
  console.log(`${new Date().toISOString()} ${req.method} ${req.path}`);
  next();
});
```

### Add Response Caching
```javascript
// Backend - cache status responses
const cache = new Map();
app.get('/api/status', async (req, res) => {
  const cached = cache.get('status');
  if (cached && Date.now() - cached.time < 5000) {
    return res.json(cached.data);
  }
  // Fetch and cache new data
});
```

## 📝 Related Projects

- **WebSocket Version**: `../websocket/` - Real-time bidirectional communication
- **REST Demo**: `../ESP32_REST_Demo.cpp` - Extended version with embedded HTML

## 📧 Support

Issues? Check:
1. Serial Monitor output (ESP32)
2. Browser console (Frontend)
3. Terminal logs (Backend)
4. Network connectivity

---

**Built for reliable HTTP REST API communication with ESP32**
