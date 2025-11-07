require('dotenv').config();
const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const path = require('path');
const os = require('os');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Configuration
const PORT = process.env.PORT || 3001;
const ESP32_IP = process.env.ESP32_IP || '192.168.1.100';
const ESP32_WS_PORT = process.env.ESP32_WS_PORT || 81;

// Store ESP32 WebSocket connection
let esp32Socket = null;
let reconnectTimer = null;
let lastStatus = null;

// Middleware
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

/**
 * Get local network IP addresses
 */
function getNetworkIPs() {
  const interfaces = os.networkInterfaces();
  const addresses = [];
  
  for (const name of Object.keys(interfaces)) {
    for (const iface of interfaces[name]) {
      if (iface.family === 'IPv4' && !iface.internal) {
        addresses.push({ name, address: iface.address });
      }
    }
  }
  
  return addresses;
}

/**
 * Connect to ESP32 WebSocket
 */
function connectToESP32() {
  const wsUrl = `ws://${ESP32_IP}:${ESP32_WS_PORT}`;
  console.log(`\n[ESP32] Connecting to ${wsUrl}...`);
  
  try {
    esp32Socket = new WebSocket(wsUrl);
    
    esp32Socket.on('open', () => {
      console.log('[ESP32] ✓ Connected successfully');
      if (reconnectTimer) {
        clearInterval(reconnectTimer);
        reconnectTimer = null;
      }
      
      // Request initial status
      esp32Socket.send(JSON.stringify({ command: 'status' }));
    });
    
    esp32Socket.on('message', (data) => {
      try {
        const message = JSON.parse(data.toString());
        console.log('[ESP32] ←', message.type || 'message');
        
        // Store latest status
        if (message.type === 'status') {
          lastStatus = message;
        }
        
        // Broadcast to all browser clients
        wss.clients.forEach(client => {
          if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify(message));
          }
        });
      } catch (err) {
        console.error('[ESP32] Parse error:', err.message);
      }
    });
    
    esp32Socket.on('error', (error) => {
      console.error('[ESP32] ✗ Connection error:', error.message);
    });
    
    esp32Socket.on('close', () => {
      console.log('[ESP32] ✗ Connection closed');
      esp32Socket = null;
      
      // Auto-reconnect every 5 seconds
      if (!reconnectTimer) {
        console.log('[ESP32] Will retry connection in 5 seconds...');
        reconnectTimer = setInterval(() => {
          connectToESP32();
        }, 5000);
      }
    });
    
  } catch (err) {
    console.error('[ESP32] Connection failed:', err.message);
  }
}

/**
 * Send command to ESP32
 */
function sendToESP32(command) {
  return new Promise((resolve, reject) => {
    if (!esp32Socket || esp32Socket.readyState !== WebSocket.OPEN) {
      reject(new Error('ESP32 not connected'));
      return;
    }
    
    try {
      const message = JSON.stringify(command);
      console.log('[ESP32] →', command.command);
      esp32Socket.send(message);
      
      // Wait for response (with timeout)
      const timeout = setTimeout(() => {
        reject(new Error('ESP32 response timeout'));
      }, 5000);
      
      const handleResponse = (data) => {
        try {
          const response = JSON.parse(data.toString());
          if (response.type === 'response') {
            clearTimeout(timeout);
            esp32Socket.removeListener('message', handleResponse);
            resolve(response);
          }
        } catch (err) {
          // Ignore parse errors, keep waiting
        }
      };
      
      esp32Socket.on('message', handleResponse);
      
    } catch (err) {
      reject(err);
    }
  });
}

/**
 * Handle browser WebSocket connections
 */
wss.on('connection', (ws, req) => {
  const clientIP = req.socket.remoteAddress;
  console.log(`[Browser] ✓ Client connected from ${clientIP}`);
  
  // Send current ESP32 status if available
  if (lastStatus) {
    ws.send(JSON.stringify(lastStatus));
  }
  
  // Send connection status
  ws.send(JSON.stringify({
    type: 'connection',
    esp32Connected: esp32Socket && esp32Socket.readyState === WebSocket.OPEN,
    message: esp32Socket ? 'ESP32 connected' : 'ESP32 disconnected'
  }));
  
  ws.on('message', async (data) => {
    try {
      const command = JSON.parse(data.toString());
      console.log('[Browser] ←', command.command);
      
      // Forward command to ESP32
      try {
        const response = await sendToESP32(command);
        ws.send(JSON.stringify(response));
      } catch (err) {
        ws.send(JSON.stringify({
          type: 'error',
          success: false,
          message: err.message
        }));
      }
      
    } catch (err) {
      console.error('[Browser] Parse error:', err.message);
      ws.send(JSON.stringify({
        type: 'error',
        success: false,
        message: 'Invalid command format'
      }));
    }
  });
  
  ws.on('close', () => {
    console.log(`[Browser] ✗ Client disconnected`);
  });
  
  ws.on('error', (error) => {
    console.error('[Browser] Error:', error.message);
  });
});

/**
 * REST API endpoints (for compatibility/testing)
 */
app.get('/api/health', (req, res) => {
  res.json({
    status: 'ok',
    esp32Connected: esp32Socket && esp32Socket.readyState === WebSocket.OPEN,
    timestamp: Date.now()
  });
});

app.get('/api/status', async (req, res) => {
  if (lastStatus) {
    res.json(lastStatus);
  } else {
    res.status(503).json({
      success: false,
      message: 'ESP32 not connected or no status available'
    });
  }
});

app.post('/api/led/on', async (req, res) => {
  try {
    const response = await sendToESP32({ command: 'led_on' });
    res.json(response);
  } catch (err) {
    res.status(503).json({
      success: false,
      message: err.message
    });
  }
});

app.post('/api/led/off', async (req, res) => {
  try {
    const response = await sendToESP32({ command: 'led_off' });
    res.json(response);
  } catch (err) {
    res.status(503).json({
      success: false,
      message: err.message
    });
  }
});

app.post('/api/led/toggle', async (req, res) => {
  try {
    const response = await sendToESP32({ command: 'toggle' });
    res.json(response);
  } catch (err) {
    res.status(503).json({
      success: false,
      message: err.message
    });
  }
});

/**
 * Start server
 */
server.listen(PORT, '0.0.0.0', () => {
  console.clear();
  console.log('\n╔════════════════════════════════════════╗');
  console.log('║   ESP32 WebSocket Controller v1.0     ║');
  console.log('╚════════════════════════════════════════╝\n');
  
  console.log('🌐 Server Information:');
  console.log(`   Local:    http://localhost:${PORT}`);
  
  const ips = getNetworkIPs();
  ips.forEach(({ name, address }) => {
    console.log(`   Network:  http://${address}:${PORT} (${name})`);
  });
  
  console.log(`\n🎯 ESP32 Target: ws://${ESP32_IP}:${ESP32_WS_PORT}`);
  console.log(`📁 Serving files from: ${path.join(__dirname, 'public')}\n`);
  console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n');
  
  // Connect to ESP32
  connectToESP32();
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\n\n[Server] Shutting down gracefully...');
  
  if (esp32Socket) {
    esp32Socket.close();
  }
  
  wss.clients.forEach(client => {
    client.close();
  });
  
  server.close(() => {
    console.log('[Server] Shutdown complete');
    process.exit(0);
  });
});
