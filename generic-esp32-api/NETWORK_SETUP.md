# Network Access Configuration Guide

## Overview
The Generic ESP32 API server is configured to accept connections from any IP address (external access enabled). This allows ESP32 devices on your local network or even remote networks to connect to your API server.

## Network Configuration Details

### Server Binding
- **Listen Address**: `0.0.0.0` (all network interfaces)
- **Port**: 3001 (configurable via .env PORT variable)
- **Protocol**: HTTP (HTTPS can be configured with reverse proxy)

### Access Methods

#### 1. Local Access (Same Computer)
```
http://localhost:3001
http://127.0.0.1:3001
```

#### 2. Local Network Access
```
http://YOUR-LOCAL-IP:3001
Examples:
http://192.168.1.100:3001
http://10.0.0.50:3001
http://172.16.1.25:3001
```

#### 3. External Network Access (Internet)
```
http://YOUR-PUBLIC-IP:3001
```

## Finding Your IP Addresses

### Windows
```cmd
ipconfig
# Look for IPv4 Address under your active network adapter
```

### macOS/Linux
```bash
ifconfig
# or
ip addr show
```

### From API Server Startup
The server automatically detects and displays available IP addresses when it starts:
```
🌍 External Network Access:
   API: http://192.168.1.100:3001
   Dashboard: http://192.168.1.100:3001/dashboard
```

## ESP32 Configuration Examples

### Local Network (Most Common)
```cpp
const char* apiServer = "192.168.1.100";  // Your computer's local IP
const int apiPort = 3001;
```

### Same Computer (Testing)
```cpp
const char* apiServer = "127.0.0.1";      // Localhost
const int apiPort = 3001;
```

### Remote Access (Advanced)
```cpp
const char* apiServer = "your-domain.com"; // Domain name
// or
const char* apiServer = "203.0.113.100";   // Public IP
const int apiPort = 3001;
```

## Firewall Configuration

### Windows Firewall
1. Open Windows Defender Firewall
2. Click "Allow an app or feature through Windows Defender Firewall"
3. Click "Change Settings" → "Allow another app"
4. Browse to Node.js executable or add port 3001
5. Check both "Private" and "Public" networks (as needed)

### Router/Network Firewall
- **Port Forwarding**: Configure port 3001 to forward to your computer
- **DMZ**: Place server computer in DMZ (less secure)
- **VPN Access**: Use VPN for secure remote access

## Network Testing

### Test Local Access
```bash
curl http://localhost:3001/
```

### Test Network Access
```bash
# From another computer on same network
curl http://192.168.1.100:3001/

# Test specific endpoints
curl http://192.168.1.100:3001/devices
curl http://192.168.1.100:3001/stats
```

### Test from ESP32
```cpp
// In your ESP32 setup() function
HTTPClient http;
http.begin("http://192.168.1.100:3001/");
int responseCode = http.GET();
if (responseCode > 0) {
  Serial.println("✅ API server reachable!");
} else {
  Serial.println("❌ Cannot reach API server");
}
http.end();
```

## Troubleshooting Network Issues

### Common Problems

#### 1. "Connection Refused"
- **Check**: Is the server running?
- **Check**: Is the IP address correct?
- **Check**: Is the port correct (3001)?

#### 2. "No Route to Host"
- **Check**: Are devices on the same network?
- **Check**: Firewall blocking connections?
- **Check**: Router configuration?

#### 3. "Timeout"
- **Check**: Network connectivity between devices
- **Check**: Server overloaded or crashed?
- **Check**: DNS resolution (if using domain names)

### Diagnostic Commands

#### Ping Test
```bash
# Test network connectivity
ping 192.168.1.100
```

#### Port Test
```bash
# Windows
telnet 192.168.1.100 3001

# Linux/macOS
nc -zv 192.168.1.100 3001
```

#### Network Route
```bash
# Check routing to server
tracert 192.168.1.100  # Windows
traceroute 192.168.1.100  # Linux/macOS
```

## Security Recommendations

### Development Environment
- ✅ Allow all network interfaces (current configuration)
- ✅ Use local network IPs only
- ⚠️ Monitor who has network access

### Production Environment
- 🔒 Use reverse proxy (nginx, Apache) with HTTPS
- 🔒 Implement API authentication
- 🔒 Use VPN for remote access
- 🔒 Configure specific IP whitelisting
- 🔒 Enable MongoDB authentication
- 🔒 Use environment-based configuration

### Network Isolation Options
1. **Private Network**: Keep server on isolated network
2. **VPN Access**: Require VPN connection for external access
3. **Proxy Server**: Use authenticated proxy for ESP32 connections
4. **API Gateway**: Implement rate limiting and authentication

## Advanced Network Configurations

### Using nginx Reverse Proxy
```nginx
server {
    listen 80;
    server_name your-domain.com;
    
    location / {
        proxy_pass http://localhost:3001;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }
}
```

### Using PM2 for Production
```bash
# Install PM2
npm install -g pm2

# Start server with PM2
pm2 start server.js --name "esp32-api"

# Configure auto-restart
pm2 startup
pm2 save
```

### Docker Deployment
```dockerfile
FROM node:18-alpine
WORKDIR /app
COPY package*.json ./
RUN npm install --production
COPY . .
EXPOSE 3001
CMD ["node", "server.js"]
```

```bash
# Build and run
docker build -t esp32-api .
docker run -d -p 3001:3001 --name esp32-api esp32-api
```

This configuration allows maximum flexibility for ESP32 devices to connect from various network locations while maintaining security best practices for different deployment scenarios.