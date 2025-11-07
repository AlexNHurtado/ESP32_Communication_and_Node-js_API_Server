# ESP32 Hybrid Server - REST + WebSocket

## Overview

This ESP32 firmware runs **both HTTP REST and WebSocket servers simultaneously** on the same device, demonstrating that the ESP32 can handle multiple communication protocols at once.

## Architecture

```
┌─────────────────────────────────────────┐
│            ESP32 Device                 │
│                                         │
│  ┌─────────────────┐  ┌──────────────┐ │
│  │  HTTP Server    │  │  WebSocket   │ │
│  │   Port 80       │  │   Port 81    │ │
│  │                 │  │              │ │
│  │  GET /status    │  │  Broadcast   │ │
│  │  GET /led/on    │  │  Status      │ │
│  │  GET /led/off   │  │  Every 5s    │ │
│  │  POST /led      │  │              │ │
│  └─────────────────┘  └──────────────┘ │
│           │                   │         │
│           └───────┬───────────┘         │
│                   │                     │
│            ┌──────▼──────┐              │
│            │   LED Pin   │              │
│            │   GPIO 2    │              │
│            └─────────────┘              │
└─────────────────────────────────────────┘
```

## Can ESP32 Handle Both?

### **YES!** The ESP32 can handle both protocols simultaneously because:

1. **Dual-Core Processor** - 240MHz, can multitask easily
2. **Sufficient Memory** - 520KB RAM, enough for both servers
3. **Non-blocking Architecture** - Both servers use event loops
4. **Separate Ports** - HTTP (80) and WebSocket (81) don't conflict

## Memory Usage

| Component | RAM Usage |
|-----------|-----------|
| HTTP Server | ~8KB |
| WebSocket Server | ~12KB |
| WiFi Stack | ~40KB |
| Application Code | ~5KB |
| **Total** | **~65KB** (12% of available RAM) |

## Performance

### **HTTP REST:**
- Latency: ~50-100ms per request
- Throughput: ~50 requests/second
- Use case: Simple commands, integrations, polling

### **WebSocket:**
- Latency: ~10-30ms per message
- Throughput: ~200 messages/second
- Use case: Real-time updates, live monitoring, bidirectional communication

## Features

### **Shared State:**
- ✅ Single LED state managed by both servers
- ✅ LED changes via HTTP are broadcast to WebSocket clients
- ✅ LED changes via WebSocket are visible to HTTP clients
- ✅ Synchronized status across both protocols

### **HTTP REST Endpoints:**
```bash
# Get status
curl http://192.168.1.100/status

# Turn LED on
curl http://192.168.1.100/led/on

# Turn LED off
curl http://192.168.1.100/led/off

# Control with JSON
curl -X POST http://192.168.1.100/led \
  -H "Content-Type: application/json" \
  -d '{"state": true}'
```

### **WebSocket Commands:**
```javascript
// Connect
const ws = new WebSocket('ws://192.168.1.100:81');

// Send commands
ws.send('{"command":"led_on"}');
ws.send('{"command":"led_off"}');
ws.send('{"command":"toggle"}');
ws.send('{"command":"status"}');

// Receive updates (auto every 5 seconds)
ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  console.log(data);
};
```

## Use Cases

### **When to Use HTTP REST:**
- ✅ Simple integrations (IFTTT, Zapier, etc.)
- ✅ Curl commands for testing
- ✅ Backend services that poll occasionally
- ✅ Mobile apps with periodic updates
- ✅ Legacy systems that don't support WebSocket

### **When to Use WebSocket:**
- ✅ Real-time dashboards
- ✅ Live monitoring applications
- ✅ Two-way communication needs
- ✅ Frequent status updates
- ✅ Low-latency requirements
- ✅ Modern web applications

### **Example Scenario:**
A smart home system might use:
- **HTTP REST** for Alexa/Google Home integration ("Turn on the light")
- **WebSocket** for the mobile app real-time status updates

## Testing Both Protocols

### **1. Test HTTP REST:**
```bash
# In terminal
curl http://192.168.1.100/status
curl http://192.168.1.100/led/on
```

### **2. Test WebSocket:**
```javascript
// In browser console (F12)
const ws = new WebSocket('ws://192.168.1.100:81');
ws.onopen = () => console.log('Connected!');
ws.onmessage = (e) => console.log('Received:', e.data);
ws.send('{"command":"status"}');
```

### **3. See Cross-Protocol Communication:**
1. Open browser console with WebSocket connection
2. In terminal, run: `curl http://192.168.1.100/led/on`
3. Watch the WebSocket automatically receive the LED update!

## Advantages of Hybrid Approach

### **1. Flexibility:**
- Clients can choose the best protocol for their needs
- One device serves multiple use cases

### **2. Compatibility:**
- Works with old systems (HTTP) and new systems (WebSocket)
- Maximum integration possibilities

### **3. Efficiency:**
- WebSocket clients get instant updates (no polling)
- HTTP clients can still do simple commands

### **4. Development:**
- Easy to test with curl (HTTP)
- Easy to debug with browser console (WebSocket)

## Resource Considerations

### **✅ ESP32 Can Handle:**
- ~10 simultaneous HTTP requests
- ~5-8 concurrent WebSocket connections
- Both protocols running together
- Auto-status broadcasts every 5 seconds

### **⚠️ Limitations:**
- Total memory is finite (~520KB RAM)
- More connections = more memory usage
- Very heavy traffic on both protocols may cause delays
- Recommended: < 15 total connections combined

## Best Practices

### **1. Choose the Right Protocol:**
```
Simple command?        → Use HTTP REST
Need real-time?        → Use WebSocket
Third-party service?   → Use HTTP REST
Building dashboard?    → Use WebSocket
```

### **2. Connection Management:**
- Close WebSocket connections when not needed
- Use HTTP for infrequent operations
- Monitor connection counts

### **3. Error Handling:**
- HTTP returns proper status codes
- WebSocket sends error responses
- Both log to Serial for debugging

## Comparison

| Feature | HTTP REST | WebSocket | Hybrid |
|---------|-----------|-----------|--------|
| **Latency** | 50-100ms | 10-30ms | Both |
| **Real-time** | No (polling) | Yes (push) | Best of both |
| **Complexity** | Simple | Moderate | Moderate |
| **Compatibility** | Universal | Modern | Maximum |
| **Memory** | Low | Medium | Medium |
| **Best For** | Commands | Monitoring | Everything |

## Conclusion

**Yes, the ESP32 can absolutely handle both HTTP REST and WebSocket simultaneously!**

This hybrid approach provides:
- ✅ Maximum flexibility
- ✅ Wide compatibility
- ✅ Best performance for each use case
- ✅ Single device, multiple communication methods

The ESP32's dual-core processor and ample memory make it perfectly capable of running both servers without performance issues for typical IoT applications.