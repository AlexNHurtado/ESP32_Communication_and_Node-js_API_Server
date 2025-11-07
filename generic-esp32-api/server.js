const express = require('express');
const cors = require('cors');
const axios = require('axios');
const os = require('os');
const { MongoClient, ObjectId } = require('mongodb');
const deviceAuthManager = require('./deviceAuth');
require('dotenv').config();

const app = express();
const PORT = process.env.PORT || 3001;

// Middleware
app.use(express.json({ limit: process.env.MAX_PAYLOAD_SIZE || '10mb' }));
app.use(express.urlencoded({ extended: true }));
app.use(cors({
  origin: process.env.CORS_ORIGIN || '*',
  methods: ['GET', 'POST', 'PUT', 'DELETE', 'OPTIONS'],
  allowedHeaders: ['Content-Type', 'Authorization', 'X-API-Key', 'X-ESP32-ID']
}));

// Serve static files (dashboard)
app.use('/dashboard', express.static('public'));

// MongoDB Configuration
const MONGODB_CONFIG = {
  uri: process.env.MONGODB_URI || 'mongodb://localhost:27017',
  database: process.env.MONGODB_DATABASE || 'generic_esp32_iot',
  collection: process.env.MONGODB_COLLECTION || 'device_data',
  username: process.env.MONGODB_USERNAME || '',
  password: process.env.MONGODB_PASSWORD || ''
};

// MongoDB Client
let mongoClient = null;
let isMongoConnected = false;

// In-memory ESP32 registry
const esp32Registry = new Map(); // deviceId -> { ip, port, lastSeen, info }

// Request logging middleware
app.use((req, res, next) => {
  const timestamp = new Date().toISOString();
  const deviceId = req.headers['x-esp32-id'] || 'unknown';
  const clientIP = req.ip || req.connection.remoteAddress;
  
  if (process.env.LOG_REQUESTS === 'true') {
    console.log(`[${timestamp}] ${req.method} ${req.path} - Device: ${deviceId} - IP: ${clientIP}`);
  }
  
  next();
});

// ============================================
// MongoDB Functions
// ============================================

async function initMongoDB() {
  try {
    const connectionOptions = {
      useUnifiedTopology: true,
    };

    if (MONGODB_CONFIG.username && MONGODB_CONFIG.password) {
      connectionOptions.auth = {
        username: MONGODB_CONFIG.username,
        password: MONGODB_CONFIG.password
      };
    }

    mongoClient = new MongoClient(MONGODB_CONFIG.uri, connectionOptions);
    await mongoClient.connect();
    
    // Test the connection
    await mongoClient.db(MONGODB_CONFIG.database).admin().ping();
    isMongoConnected = true;
    console.log('✅ MongoDB connected successfully');
    console.log(`   Database: ${MONGODB_CONFIG.database}`);
    console.log(`   Collection: ${MONGODB_CONFIG.collection}`);
  } catch (error) {
    console.error('❌ MongoDB connection failed:', error.message);
    console.log('   Data logging will be disabled');
    isMongoConnected = false;
  }
}

async function logToMongoDB(data) {
  if (!isMongoConnected || !mongoClient) {
    console.warn('MongoDB not connected - data not logged');
    return null;
  }

  try {
    const db = mongoClient.db(MONGODB_CONFIG.database);
    const collection = db.collection(MONGODB_CONFIG.collection);

    const document = {
      ...data,
      logged_at: new Date(),
      logged_timestamp: Date.now()
    };

    const result = await collection.insertOne(document);
    return result.insertedId;
  } catch (error) {
    console.error('MongoDB logging error:', error.message);
    return null;
  }
}

// ============================================
// Utility Functions
// ============================================

function getNetworkIPs() {
  const interfaces = os.networkInterfaces();
  const ips = [];
  
  for (const name of Object.keys(interfaces)) {
    for (const iface of interfaces[name]) {
      // Skip internal and non-IPv4 addresses
      if (iface.family === 'IPv4' && !iface.internal) {
        ips.push(iface.address);
      }
    }
  }
  
  return ips;
}

function registerESP32Device(deviceId, ip, port = 80, additionalInfo = {}) {
  esp32Registry.set(deviceId, {
    ip,
    port,
    lastSeen: new Date(),
    registeredAt: esp32Registry.get(deviceId)?.registeredAt || new Date(),
    info: additionalInfo
  });
  console.log(`📱 ESP32 device registered: ${deviceId} at ${ip}:${port}`);
}

async function sendToESP32(deviceId, endpoint, data, method = 'POST') {
  const device = esp32Registry.get(deviceId);
  
  if (!device) {
    throw new Error(`Device ${deviceId} not found in registry`);
  }

  const url = `http://${device.ip}:${device.port}${endpoint}`;
  
  try {
    const config = {
      method,
      url,
      timeout: parseInt(process.env.REQUEST_TIMEOUT) || 30000,
      headers: {
        'Content-Type': 'application/json',
        'User-Agent': 'Generic-ESP32-API/1.0'
      }
    };

    if (data && (method === 'POST' || method === 'PUT')) {
      config.data = data;
    }

    const response = await axios(config);
    
    // Update last seen
    device.lastSeen = new Date();
    
    return {
      success: true,
      status: response.status,
      data: response.data,
      device: deviceId
    };
  } catch (error) {
    console.error(`ESP32 communication error (${deviceId}):`, error.message);
    
    return {
      success: false,
      error: error.message,
      code: error.code,
      device: deviceId
    };
  }
}

// ============================================
// API Routes
// ============================================

/**
 * GET / - API Documentation
 */
app.get('/', (req, res) => {
  const networkIPs = getNetworkIPs();
  const serverAccess = {
    local: `http://localhost:${PORT}`,
    external: networkIPs.map(ip => `http://${ip}:${PORT}`),
    dashboard_local: `http://localhost:${PORT}/dashboard`,
    dashboard_external: networkIPs.map(ip => `http://${ip}:${PORT}/dashboard`)
  };

  res.json({
    title: 'Generic ESP32 REST API',
    version: '1.0.0',
    description: 'Universal REST API for any ESP32 device with MongoDB logging',
    server_access: serverAccess,
    listening_on: '0.0.0.0 (all network interfaces)',
    mongodb: {
      connected: isMongoConnected,
      database: MONGODB_CONFIG.database,
      collection: MONGODB_CONFIG.collection
    },
    registered_devices: Array.from(esp32Registry.entries()).map(([id, info]) => ({
      device_id: id,
      ip: info.ip,
      port: info.port,
      last_seen: info.lastSeen,
      registered_at: info.registeredAt
    })),
    endpoints: {
      'POST /register': 'Register ESP32 device with the API (whitelist enabled)',
      'POST /data': 'Send data from registered ESP32 devices only',
      'GET /devices': 'List all registered ESP32 devices',
      'DELETE /devices/:deviceId': 'Unregister device from whitelist',
      'POST /send/command': 'Send command to specific ESP32 device',
      'POST /send/config': 'Send configuration to specific ESP32 device',
      'POST /send/update': 'Send update/firmware info to specific ESP32 device',
      'POST /send/custom': 'Send custom data to specific ESP32 device',
      'GET /auth/status': 'Get authentication system status',
      'PUT /auth/config': 'Update authentication configuration',
      'POST /auth/blacklist': 'Blacklist an IP address',
      'DELETE /auth/blacklist/:ip': 'Remove IP from blacklist'
    },
    usage: {
      register_device: 'POST /register with {"device_id": "esp32_001", "ip": "192.168.1.100", "port": 80}',
      send_data: {
        description: 'POST /data with any JSON payload + X-ESP32-ID header',
        example: {
          headers: {
            'Content-Type': 'application/json',
            'X-ESP32-ID': 'esp32_sensor_01'
          },
          body: {
            temperature: 25.6,
            humidity: 60.2,
            light_level: 450,
            motion_detected: false,
            battery_level: 85,
            timestamp: '2024-01-15T10:30:00Z'
          }
        }
      },
      send_to_device: 'POST /send/command with {"device_id": "esp32_001", "endpoint": "/command", "data": {...}}'
    },
    authentication: {
      whitelist_enabled: deviceAuthManager.getStats().config.enableWhitelist,
      duplicate_prevention: true,
      rate_limiting: true,
      ip_blacklisting: true,
      note: 'Only registered devices can submit data. Register first, then use X-ESP32-ID header.'
    }
  });
});

/**
 * POST /register - Register ESP32 device (with whitelist and duplicate prevention)
 */
app.post('/register', async (req, res) => {
  try {
    const { device_id, ip, port = 80, device_info = {} } = req.body;
    const clientIP = req.ip || req.connection.remoteAddress;
    
    if (!device_id || !ip) {
      return res.status(400).json({
        success: false,
        error: 'device_id and ip are required',
        example: {
          device_id: 'esp32_sensor_01',
          ip: '192.168.1.100',
          port: 80,
          device_info: {
            type: 'sensor',
            location: 'kitchen',
            firmware: '1.0.0'
          }
        }
      });
    }

    // Use authentication manager for registration
    const registrationResult = deviceAuthManager.registerDevice(
      device_id, 
      { ip, port, device_info }, 
      clientIP
    );

    if (!registrationResult.success) {
      const status = registrationResult.retryAfter ? 429 : 409; // Too Many Requests or Conflict
      const response = {
        success: false,
        error: registrationResult.message,
        device_id
      };
      
      if (registrationResult.retryAfter) {
        response.retry_after = registrationResult.retryAfter;
        res.set('Retry-After', registrationResult.retryAfter);
      }
      
      return res.status(status).json(response);
    }

    // Also register in legacy registry for compatibility
    registerESP32Device(device_id, ip, port, device_info);
    
    // Log registration to MongoDB
    const logData = {
      event_type: 'device_registration',
      device_id,
      ip,
      port,
      device_info,
      client_ip: clientIP,
      user_agent: req.headers['user-agent'],
      auth_token: registrationResult.authToken,
      registration_method: 'whitelist_enabled'
    };
    
    const logId = await logToMongoDB(logData);
    
    const response = {
      success: true,
      message: registrationResult.message,
      device: registrationResult.deviceInfo,
      auth_token: registrationResult.authToken,
      mongodb_log_id: logId,
      total_devices: deviceAuthManager.getRegisteredDevices().length
    };

    if (registrationResult.warnings.length > 0) {
      response.warnings = registrationResult.warnings;
    }

    res.json(response);

  } catch (error) {
    console.error('Registration error:', error);
    res.status(500).json({
      success: false,
      error: 'Registration failed',
      details: error.message
    });
  }
});

/**
 * POST /data - Receive and log data from registered ESP32 devices only (whitelist enforced)
 */
app.post('/data', async (req, res) => {
  try {
    const deviceId = req.headers['x-esp32-id'];
    const clientIP = req.ip || req.connection.remoteAddress;
    const payload = req.body;
    
    // Validate device authorization
    if (!deviceId) {
      return res.status(400).json({
        success: false,
        error: 'Missing X-ESP32-ID header',
        required_header: 'X-ESP32-ID',
        message: 'Device identification is required for data submission'
      });
    }

    const validation = deviceAuthManager.validateDataSubmission(deviceId, clientIP, req.headers);
    
    if (!validation.allowed) {
      return res.status(403).json({
        success: false,
        error: 'Device not authorized',
        device_id: deviceId,
        reason: validation.reason,
        message: 'Only registered devices can submit data. Please register first at POST /register'
      });
    }

    // Log authorized data to MongoDB
    const logData = {
      event_type: 'data_received',
      device_id: deviceId,
      client_ip: clientIP,
      payload: payload,
      headers: {
        'content-type': req.headers['content-type'],
        'user-agent': req.headers['user-agent'],
        'content-length': req.headers['content-length']
      },
      timestamp: new Date(),
      payload_size: JSON.stringify(payload).length,
      device_info: validation.deviceInfo,
      authorization_status: 'authorized'
    };
    
    const logId = await logToMongoDB(logData);
    
    // Update legacy device registry for compatibility
    if (payload.ip) {
      registerESP32Device(deviceId, payload.ip, payload.port || 80, payload.device_info || {});
    }
    
    res.json({
      success: true,
      message: 'Data received and logged successfully',
      received_at: new Date().toISOString(),
      device_id: deviceId,
      device_status: validation.deviceInfo.status,
      submission_count: validation.deviceInfo.dataCount,
      mongodb_log_id: logId,
      payload_size: logData.payload_size
    });

  } catch (error) {
    console.error('Data processing error:', error);
    res.status(500).json({
      success: false,
      error: 'Failed to process data',
      details: error.message
    });
  }
});

/**
 * GET /devices - List registered ESP32 devices (from authentication manager)
 */
app.get('/devices', (req, res) => {
  const authorizedDevices = deviceAuthManager.getRegisteredDevices();
  const authStats = deviceAuthManager.getStats();
  
  // Add online status calculation
  const devicesWithStatus = authorizedDevices.map(device => {
    const lastSeenTime = new Date(device.lastSeen).getTime();
    const now = Date.now();
    const offlineDuration = now - lastSeenTime;
    
    return {
      ...device,
      online_status: offlineDuration < 300000 ? 'online' : 'offline', // 5 minutes threshold
      offline_duration_seconds: Math.floor(offlineDuration / 1000),
      has_auth_token: device.deviceId && deviceAuthManager.authTokens && deviceAuthManager.authTokens.has(device.deviceId)
    };
  });
  
  res.json({
    success: true,
    total_devices: authorizedDevices.length,
    active_devices: authStats.activeDevices,
    devices: devicesWithStatus,
    authentication_stats: authStats,
    mongodb_connected: isMongoConnected
  });
});

// ============================================
// ESP32 Communication Endpoints (4 endpoints as requested)
// ============================================

/**
 * POST /send/command - Send command to ESP32 device
 */
app.post('/send/command', async (req, res) => {
  try {
    const { device_id, command, parameters = {}, endpoint = '/command' } = req.body;
    
    if (!device_id || !command) {
      return res.status(400).json({
        success: false,
        error: 'device_id and command are required',
        example: {
          device_id: 'esp32_001',
          command: 'turn_led',
          parameters: { state: true, brightness: 255 },
          endpoint: '/command'
        }
      });
    }

    const commandData = {
      command,
      parameters,
      timestamp: new Date().toISOString(),
      sent_from: 'generic-esp32-api'
    };

    const result = await sendToESP32(device_id, endpoint, commandData);
    
    // Log the command to MongoDB
    await logToMongoDB({
      event_type: 'command_sent',
      device_id,
      command,
      parameters,
      endpoint,
      result: result.success,
      response: result.data || result.error
    });

    if (result.success) {
      res.json({
        success: true,
        message: 'Command sent successfully',
        device_id,
        command,
        response: result.data,
        sent_at: new Date().toISOString()
      });
    } else {
      res.status(503).json({
        success: false,
        error: 'Failed to send command to device',
        device_id,
        details: result.error
      });
    }

  } catch (error) {
    res.status(500).json({
      success: false,
      error: 'Command sending failed',
      details: error.message
    });
  }
});

/**
 * POST /send/config - Send configuration to ESP32 device
 */
app.post('/send/config', async (req, res) => {
  try {
    const { device_id, config, endpoint = '/config' } = req.body;
    
    if (!device_id || !config) {
      return res.status(400).json({
        success: false,
        error: 'device_id and config are required',
        example: {
          device_id: 'esp32_001',
          config: {
            wifi_ssid: 'NewNetwork',
            sensor_interval: 30000,
            mqtt_broker: '192.168.1.1'
          },
          endpoint: '/config'
        }
      });
    }

    const configData = {
      config,
      config_version: Date.now(),
      sent_at: new Date().toISOString(),
      config_source: 'generic-esp32-api'
    };

    const result = await sendToESP32(device_id, endpoint, configData);
    
    // Log configuration change
    await logToMongoDB({
      event_type: 'config_sent',
      device_id,
      config,
      endpoint,
      result: result.success,
      response: result.data || result.error
    });

    if (result.success) {
      res.json({
        success: true,
        message: 'Configuration sent successfully',
        device_id,
        config_items: Object.keys(config).length,
        response: result.data,
        sent_at: configData.sent_at
      });
    } else {
      res.status(503).json({
        success: false,
        error: 'Failed to send configuration to device',
        device_id,
        details: result.error
      });
    }

  } catch (error) {
    res.status(500).json({
      success: false,
      error: 'Configuration sending failed',
      details: error.message
    });
  }
});

/**
 * POST /send/update - Send update/firmware information to ESP32 device
 */
app.post('/send/update', async (req, res) => {
  try {
    const { device_id, update_info, endpoint = '/update' } = req.body;
    
    if (!device_id || !update_info) {
      return res.status(400).json({
        success: false,
        error: 'device_id and update_info are required',
        example: {
          device_id: 'esp32_001',
          update_info: {
            firmware_version: '2.1.0',
            download_url: 'https://example.com/firmware.bin',
            checksum: 'sha256:abc123...',
            force_update: false
          },
          endpoint: '/update'
        }
      });
    }

    const updateData = {
      ...update_info,
      update_id: new ObjectId().toString(),
      initiated_at: new Date().toISOString(),
      update_source: 'generic-esp32-api'
    };

    const result = await sendToESP32(device_id, endpoint, updateData);
    
    // Log update attempt
    await logToMongoDB({
      event_type: 'update_sent',
      device_id,
      update_info,
      endpoint,
      update_id: updateData.update_id,
      result: result.success,
      response: result.data || result.error
    });

    if (result.success) {
      res.json({
        success: true,
        message: 'Update information sent successfully',
        device_id,
        update_id: updateData.update_id,
        response: result.data,
        sent_at: updateData.initiated_at
      });
    } else {
      res.status(503).json({
        success: false,
        error: 'Failed to send update information to device',
        device_id,
        details: result.error
      });
    }

  } catch (error) {
    res.status(500).json({
      success: false,
      error: 'Update sending failed',
      details: error.message
    });
  }
});

/**
 * POST /send/custom - Send custom data to ESP32 device
 */
app.post('/send/custom', async (req, res) => {
  try {
    const { device_id, data, endpoint = '/custom', method = 'POST' } = req.body;
    
    if (!device_id || !data) {
      return res.status(400).json({
        success: false,
        error: 'device_id and data are required',
        example: {
          device_id: 'esp32_001',
          data: {
            message: 'Hello ESP32',
            action: 'display_text',
            parameters: { color: 'blue', duration: 5000 }
          },
          endpoint: '/custom',
          method: 'POST'
        }
      });
    }

    const customData = {
      ...data,
      custom_id: new ObjectId().toString(),
      sent_at: new Date().toISOString(),
      sent_from: 'generic-esp32-api'
    };

    const result = await sendToESP32(device_id, endpoint, customData, method);
    
    // Log custom data transmission
    await logToMongoDB({
      event_type: 'custom_data_sent',
      device_id,
      custom_data: data,
      endpoint,
      method,
      custom_id: customData.custom_id,
      result: result.success,
      response: result.data || result.error
    });

    if (result.success) {
      res.json({
        success: true,
        message: 'Custom data sent successfully',
        device_id,
        custom_id: customData.custom_id,
        method,
        endpoint,
        response: result.data,
        sent_at: customData.sent_at
      });
    } else {
      res.status(503).json({
        success: false,
        error: 'Failed to send custom data to device',
        device_id,
        details: result.error
      });
    }

  } catch (error) {
    res.status(500).json({
      success: false,
      error: 'Custom data sending failed',
      details: error.message
    });
  }
});

// ============================================
// Device Authentication Management
// ============================================

/**
 * DELETE /devices/:deviceId - Unregister a device from whitelist
 */
app.delete('/devices/:deviceId', (req, res) => {
  try {
    const { deviceId } = req.params;
    const { reason } = req.body;
    
    const success = deviceAuthManager.unregisterDevice(deviceId, reason || 'API request');
    
    if (success) {
      // Also remove from legacy registry
      esp32Registry.delete(deviceId);
      
      res.json({
        success: true,
        message: `Device ${deviceId} unregistered successfully`,
        device_id: deviceId,
        remaining_devices: deviceAuthManager.getRegisteredDevices().length
      });
    } else {
      res.status(404).json({
        success: false,
        error: 'Device not found',
        device_id: deviceId
      });
    }
  } catch (error) {
    res.status(500).json({
      success: false,
      error: 'Failed to unregister device',
      details: error.message
    });
  }
});

/**
 * POST /auth/blacklist - Blacklist an IP address
 */
app.post('/auth/blacklist', (req, res) => {
  try {
    const { ip, reason } = req.body;
    
    if (!ip) {
      return res.status(400).json({
        success: false,
        error: 'IP address is required',
        example: { ip: '192.168.1.100', reason: 'Suspicious activity' }
      });
    }

    deviceAuthManager.blacklistIP(ip, reason || 'Manual blacklist');
    
    res.json({
      success: true,
      message: `IP ${ip} blacklisted successfully`,
      ip: ip,
      reason: reason || 'Manual blacklist'
    });
  } catch (error) {
    res.status(500).json({
      success: false,
      error: 'Failed to blacklist IP',
      details: error.message
    });
  }
});

/**
 * DELETE /auth/blacklist/:ip - Remove IP from blacklist
 */
app.delete('/auth/blacklist/:ip', (req, res) => {
  try {
    const { ip } = req.params;
    
    const success = deviceAuthManager.unblacklistIP(ip);
    
    if (success) {
      res.json({
        success: true,
        message: `IP ${ip} removed from blacklist`,
        ip: ip
      });
    } else {
      res.status(404).json({
        success: false,
        error: 'IP not found in blacklist',
        ip: ip
      });
    }
  } catch (error) {
    res.status(500).json({
      success: false,
      error: 'Failed to remove IP from blacklist',
      details: error.message
    });
  }
});

/**
 * PUT /auth/config - Update authentication configuration
 */
app.put('/auth/config', (req, res) => {
  try {
    const allowedFields = [
      'maxRegistrationAttempts',
      'registrationCooldown', 
      'tokenExpiryTime',
      'requireUniqueIPs',
      'enableWhitelist'
    ];
    
    const updates = {};
    for (const [key, value] of Object.entries(req.body)) {
      if (allowedFields.includes(key)) {
        updates[key] = value;
      }
    }
    
    if (Object.keys(updates).length === 0) {
      return res.status(400).json({
        success: false,
        error: 'No valid configuration fields provided',
        allowed_fields: allowedFields
      });
    }

    deviceAuthManager.updateConfig(updates);
    
    res.json({
      success: true,
      message: 'Authentication configuration updated',
      updated_fields: Object.keys(updates),
      new_config: deviceAuthManager.getStats().config
    });
  } catch (error) {
    res.status(500).json({
      success: false,
      error: 'Failed to update configuration',
      details: error.message
    });
  }
});

/**
 * GET /auth/status - Get authentication system status
 */
app.get('/auth/status', (req, res) => {
  try {
    const stats = deviceAuthManager.getStats();
    const registeredDevices = deviceAuthManager.getRegisteredDevices();
    
    res.json({
      success: true,
      authentication_enabled: stats.config.enableWhitelist,
      statistics: stats,
      recent_devices: registeredDevices.slice(-5), // Last 5 registered devices
      system_health: {
        whitelist_active: stats.config.enableWhitelist,
        devices_registered: stats.totalRegistered,
        active_devices: stats.activeDevices,
        blacklisted_ips: stats.blacklistedIPs,
        pending_tokens: stats.pendingTokens
      }
    });
  } catch (error) {
    res.status(500).json({
      success: false,
      error: 'Failed to get authentication status',
      details: error.message
    });
  }
});

// ============================================
// Analytics and Monitoring
// ============================================

/**
 * GET /stats - Get API usage statistics
 */
app.get('/stats', async (req, res) => {
  try {
    let mongoStats = { available: false };
    
    if (isMongoConnected && mongoClient) {
      const db = mongoClient.db(MONGODB_CONFIG.database);
      const collection = db.collection(MONGODB_CONFIG.collection);
      
      const totalLogs = await collection.countDocuments();
      const recentLogs = await collection.countDocuments({
        logged_timestamp: { $gte: Date.now() - (24 * 60 * 60 * 1000) } // Last 24 hours
      });
      
      const eventTypes = await collection.aggregate([
        { $group: { _id: '$event_type', count: { $sum: 1 } } },
        { $sort: { count: -1 } }
      ]).toArray();
      
      mongoStats = {
        available: true,
        total_logs: totalLogs,
        recent_logs_24h: recentLogs,
        event_types: eventTypes
      };
    }
    
    res.json({
      success: true,
      api_stats: {
        uptime_seconds: process.uptime(),
        memory_usage: process.memoryUsage(),
        registered_devices: esp32Registry.size,
        mongodb_connected: isMongoConnected
      },
      mongodb_stats: mongoStats,
      devices: Array.from(esp32Registry.entries()).map(([id, info]) => ({
        device_id: id,
        last_seen_ago: new Date() - info.lastSeen,
        is_online: (new Date() - info.lastSeen) < 300000 // 5 minutes
      }))
    });

  } catch (error) {
    res.status(500).json({
      success: false,
      error: 'Failed to get statistics',
      details: error.message
    });
  }
});

// ============================================
// Error Handling
// ============================================

app.use((err, req, res, next) => {
  console.error('Unhandled error:', err);
  res.status(500).json({
    success: false,
    error: 'Internal server error',
    message: err.message,
    timestamp: new Date().toISOString()
  });
});

app.use('*', (req, res) => {
  res.status(404).json({
    success: false,
    error: 'Endpoint not found',
    requested_path: req.originalUrl,
    method: req.method,
    available_endpoints: [
      'GET /',
      'POST /register',
      'POST /data',
      'GET /devices',
      'POST /send/command',
      'POST /send/config',
      'POST /send/update',
      'POST /send/custom',
      'GET /stats'
    ]
  });
});

// ============================================
// Server Startup
// ============================================

async function startServer() {
  try {
    // Initialize MongoDB
    await initMongoDB();
    
    // Start Express server
    app.listen(PORT, '0.0.0.0', () => {
      const networkIPs = getNetworkIPs();
      
      console.log('\n==========================================');
      console.log('🚀 Generic ESP32 REST API Server Started');
      console.log('==========================================');
      console.log(`🌐 Local Access: http://localhost:${PORT}`);
      console.log(`🎮 Dashboard: http://localhost:${PORT}/dashboard`);
      
      if (networkIPs.length > 0) {
        console.log('\n🌍 External Network Access:');
        networkIPs.forEach(ip => {
          console.log(`   API: http://${ip}:${PORT}`);
          console.log(`   Dashboard: http://${ip}:${PORT}/dashboard`);
        });
      } else {
        console.log('⚠️  No external network interfaces found');
      }
      
      console.log(`\n📊 MongoDB: ${isMongoConnected ? '✅ Connected' : '❌ Disconnected'}`);
      console.log(`📁 Database: ${MONGODB_CONFIG.database}`);
      console.log(`📋 Collection: ${MONGODB_CONFIG.collection}`);
      console.log('==========================================');
      console.log('📡 Available Endpoints:');
      console.log('  POST /register - Register ESP32 device');
      console.log('  POST /data - Receive data from ESP32');
      console.log('  GET /devices - List registered devices');
      console.log('  POST /send/command - Send commands');
      console.log('  POST /send/config - Send configuration');
      console.log('  POST /send/update - Send updates');
      console.log('  POST /send/custom - Send custom data');
      console.log('  GET /stats - API statistics');
      console.log('  GET /dashboard - Web interface');
      console.log('==========================================');
      console.log('✨ Ready to accept connections from any ESP32!');
    });

  } catch (error) {
    console.error('❌ Failed to start server:', error.message);
    process.exit(1);
  }
}

// Handle graceful shutdown
process.on('SIGINT', async () => {
  console.log('\n🛑 Shutting down server...');
  
  if (mongoClient) {
    await mongoClient.close();
    console.log('📁 MongoDB connection closed');
  }
  
  console.log('✅ Server shutdown complete');
  process.exit(0);
});

// Start the server
startServer();

module.exports = app;