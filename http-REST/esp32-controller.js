const express = require('express');
const axios = require('axios');
const cors = require('cors');
const os = require('os');
const { MongoClient } = require('mongodb');
require('dotenv').config();

const app = express();
const PORT = process.env.PORT || 3000;

// Middleware
app.use(express.json());
app.use(cors());
app.use(express.static('public')); // Serve static files

// ESP32 Configuration
const ESP32_CONFIG = {
  ip: process.env.ESP32_IP || '10.100.1.67', // Update with your ESP32 IP
  port: process.env.ESP32_PORT || 80,
  timeout: 5000
};

// MongoDB Configuration
const MONGODB_CONFIG = {
  uri: process.env.MONGODB_URI || 'mongodb://localhost:27017',
  database: process.env.MONGODB_DATABASE || 'esp32_iot',
  collection: process.env.MONGODB_COLLECTION || 'device_status',
  username: process.env.MONGODB_USERNAME || '',
  password: process.env.MONGODB_PASSWORD || ''
};

// MongoDB Client
let mongoClient = null;
let isMongoConnected = false;

// Initialize MongoDB connection
async function initMongoDB() {
  try {
    const connectionOptions = {
      useUnifiedTopology: true,
    };

    // Add authentication if credentials provided
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
    console.log(' MongoDB connected successfully');
    console.log(`   Database: ${MONGODB_CONFIG.database}`);
    console.log(`   Collection: ${MONGODB_CONFIG.collection}`);
  } catch (error) {
    console.error(' MongoDB connection failed:', error.message);
    console.log('   MongoDB features will be disabled');
    isMongoConnected = false;
  }
}

// Save status to MongoDB
async function saveStatusToMongoDB(statusData) {
  if (!isMongoConnected || !mongoClient) {
    throw new Error('MongoDB is not connected');
  }

  const db = mongoClient.db(MONGODB_CONFIG.database);
  const collection = db.collection(MONGODB_CONFIG.collection);

  // Add metadata
  const document = {
    ...statusData,
    saved_at: new Date(),
    saved_timestamp: Date.now()
  };

  const result = await collection.insertOne(document);
  return result;
}

// Utility function to get ESP32 base URL
const getESP32BaseURL = () => `http://${ESP32_CONFIG.ip}:${ESP32_CONFIG.port}`;

// Utility function to handle ESP32 requests
async function callESP32(endpoint, method = 'GET', data = null) {
  try {
    const config = {
      method,
      url: `${getESP32BaseURL()}${endpoint}`,
      timeout: ESP32_CONFIG.timeout,
      headers: {
        'Content-Type': 'application/json'
      }
    };

    if (data && method !== 'GET') {
      config.data = data;
    }

    console.log(`[${new Date().toISOString()}] ${method} ${config.url}`);
    const response = await axios(config);
    
    return {
      success: true,
      data: response.data,
      status: response.status
    };
  } catch (error) {
    console.error(`ESP32 Error:`, error.message);
    
    if (error.code === 'ECONNREFUSED') {
      return {
        success: false,
        error: 'ESP32 device is not reachable. Check if it\'s powered on and connected to WiFi.',
        code: 'CONNECTION_REFUSED'
      };
    } else if (error.code === 'ETIMEDOUT') {
      return {
        success: false,
        error: 'ESP32 device did not respond in time.',
        code: 'TIMEOUT'
      };
    } else {
      return {
        success: false,
        error: error.response?.data || error.message,
        code: error.code || 'UNKNOWN_ERROR'
      };
    }
  }
}

// ============================================
// API Routes
// ============================================

/**
 * GET / - API Documentation
 */
app.get('/', (req, res) => {
  const apiDocs = {
    title: 'ESP32 Controller API',
    version: '1.0.0',
    description: 'Node.js backend to control ESP32 LED via HTTP endpoints',
    esp32_config: {
      ip: ESP32_CONFIG.ip,
      port: ESP32_CONFIG.port,
      base_url: getESP32BaseURL()
    },
    endpoints: {
      'GET /': 'API documentation (this page)',
      'GET /api/status': 'Get ESP32 device status',
      'GET /api/led/status': 'Get LED status only',
      'POST /api/led/on': 'Turn LED on',
      'POST /api/led/off': 'Turn LED off',
      'POST /api/led/toggle': 'Toggle LED state',
      'POST /api/led/control': 'Control LED with JSON payload {"state": true/false}',
      'POST /api/status/save': 'Save current ESP32 status to MongoDB',
      'GET /api/health': 'Check if ESP32 is reachable',
      'PUT /api/config': 'Update ESP32 IP configuration'
    },
    mongodb_config: {
      database: MONGODB_CONFIG.database,
      collection: MONGODB_CONFIG.collection,
      connected: isMongoConnected
    },
    example_requests: {
      'Turn LED ON': 'POST /api/led/on',
      'Turn LED OFF': 'POST /api/led/off',
      'Toggle LED': 'POST /api/led/toggle',
      'Control with JSON': 'POST /api/led/control with {"state": true}',
      'Save Status to MongoDB': 'POST /api/status/save'
    }
  };

  res.json(apiDocs);
});

/**
 * GET /api/health - Check ESP32 connectivity
 */
app.get('/api/health', async (req, res) => {
  const result = await callESP32('/status');
  
  if (result.success) {
    res.json({
      status: 'healthy',
      esp32_reachable: true,
      esp32_data: result.data,
      timestamp: new Date().toISOString()
    });
  } else {
    res.status(503).json({
      status: 'unhealthy',
      esp32_reachable: false,
      error: result.error,
      error_code: result.code,
      timestamp: new Date().toISOString()
    });
  }
});

/**
 * GET /api/status - Get complete ESP32 status
 */
app.get('/api/status', async (req, res) => {
  const result = await callESP32('/status');
  
  if (result.success) {
    res.json({
      success: true,
      backend_server: {
        port: PORT,
        uptime: process.uptime(),
        memory_usage: process.memoryUsage()
      },
      esp32: result.data
    });
  } else {
    res.status(503).json({
      success: false,
      error: 'Could not retrieve ESP32 status',
      details: result.error,
      error_code: result.code
    });
  }
});

/**
 * GET /api/led/status - Get LED status only
 */
app.get('/api/led/status', async (req, res) => {
  const result = await callESP32('/status');
  
  if (result.success && result.data.led_state !== undefined) {
    res.json({
      success: true,
      led_state: result.data.led_state,
      timestamp: new Date().toISOString()
    });
  } else {
    res.status(503).json({
      success: false,
      error: 'Could not retrieve LED status',
      details: result.error
    });
  }
});

/**
 * POST /api/led/on - Turn LED on
 */
app.post('/api/led/on', async (req, res) => {
  const result = await callESP32('/led/on');
  
  if (result.success) {
    res.json({
      success: true,
      action: 'LED turned ON',
      esp32_response: result.data,
      timestamp: new Date().toISOString()
    });
  } else {
    res.status(503).json({
      success: false,
      error: 'Failed to turn LED on',
      details: result.error,
      error_code: result.code
    });
  }
});

/**
 * POST /api/led/off - Turn LED off
 */
app.post('/api/led/off', async (req, res) => {
  const result = await callESP32('/led/off');
  
  if (result.success) {
    res.json({
      success: true,
      action: 'LED turned OFF',
      esp32_response: result.data,
      timestamp: new Date().toISOString()
    });
  } else {
    res.status(503).json({
      success: false,
      error: 'Failed to turn LED off',
      details: result.error,
      error_code: result.code
    });
  }
});

/**
 * POST /api/led/toggle - Toggle LED state
 */
app.post('/api/led/toggle', async (req, res) => {
  try {
    // First get current LED status
    const statusResult = await callESP32('/status');
    
    if (!statusResult.success) {
      return res.status(503).json({
        success: false,
        error: 'Could not get current LED status',
        details: statusResult.error
      });
    }

    // Toggle the LED state - ESP32 returns "led" not "led_state"
    const currentState = statusResult.data.led !== undefined ? statusResult.data.led : statusResult.data.led_state;
    const newEndpoint = currentState ? '/led/off' : '/led/on';
    
    const toggleResult = await callESP32(newEndpoint);
    
    if (toggleResult.success) {
      res.json({
        success: true,
        action: `LED toggled from ${currentState ? 'ON' : 'OFF'} to ${currentState ? 'OFF' : 'ON'}`,
        previous_state: currentState,
        new_state: !currentState,
        esp32_response: toggleResult.data,
        timestamp: new Date().toISOString()
      });
    } else {
      res.status(503).json({
        success: false,
        error: 'Failed to toggle LED',
        details: toggleResult.error
      });
    }
  } catch (error) {
    res.status(500).json({
      success: false,
      error: 'Internal server error during LED toggle',
      details: error.message
    });
  }
});

/**
 * POST /api/led/control - Control LED with JSON payload
 */
app.post('/api/led/control', async (req, res) => {
  const { state } = req.body;
  
  if (typeof state !== 'boolean') {
    return res.status(400).json({
      success: false,
      error: 'Invalid request body. Expected {"state": true/false}',
      received: req.body
    });
  }

  const result = await callESP32('/led', 'POST', { state });
  
  if (result.success) {
    res.json({
      success: true,
      action: `LED controlled via JSON - ${state ? 'ON' : 'OFF'}`,
      esp32_response: result.data,
      timestamp: new Date().toISOString()
    });
  } else {
    res.status(503).json({
      success: false,
      error: 'Failed to control LED via JSON',
      details: result.error,
      error_code: result.code
    });
  }
});

/**
 * POST /api/status/save - Save current ESP32 status to MongoDB
 */
app.post('/api/status/save', async (req, res) => {
  try {
    if (!isMongoConnected) {
      return res.status(503).json({
        success: false,
        error: 'MongoDB is not connected',
        details: 'Database service unavailable. Check MongoDB connection and credentials.'
      });
    }

    // Get current ESP32 status first
    console.log('Fetching ESP32 status for MongoDB save...');
    const statusResult = await callESP32('/status');
    
    if (!statusResult.success) {
      return res.status(503).json({
        success: false,
        error: 'Could not fetch ESP32 status',
        details: statusResult.error,
        error_code: statusResult.code
      });
    }

    // Prepare data for MongoDB
    const statusData = {
      device_info: {
        ip: ESP32_CONFIG.ip,
        port: ESP32_CONFIG.port,
        base_url: getESP32BaseURL()
      },
      esp32_status: statusResult.data,
      server_info: {
        backend_server: `${os.hostname()}:${PORT}`,
        platform: os.platform(),
        arch: os.arch(),
        uptime_seconds: process.uptime(),
        memory_usage: process.memoryUsage()
      }
    };

    // Save to MongoDB
    const result = await saveStatusToMongoDB(statusData);
    
    console.log(`✅ Status saved to MongoDB with ID: ${result.insertedId}`);

    res.json({
      success: true,
      message: 'ESP32 status saved to MongoDB successfully',
      mongodb: {
        database: MONGODB_CONFIG.database,
        collection: MONGODB_CONFIG.collection,
        document_id: result.insertedId
      },
      saved_data: statusData,
      timestamp: new Date().toISOString()
    });

  } catch (error) {
    console.error('MongoDB save error:', error.message);
    
    res.status(500).json({
      success: false,
      error: 'Failed to save status to MongoDB',
      details: error.message,
      mongodb_connected: isMongoConnected
    });
  }
});

/**
 * PUT /api/config - Update ESP32 IP configuration
 */
app.put('/api/config', (req, res) => {
  const { ip, port } = req.body;
  
  if (!ip) {
    return res.status(400).json({
      success: false,
      error: 'IP address is required',
      current_config: ESP32_CONFIG
    });
  }

  // Update configuration
  ESP32_CONFIG.ip = ip;
  if (port) ESP32_CONFIG.port = port;

  res.json({
    success: true,
    message: 'ESP32 configuration updated',
    new_config: ESP32_CONFIG,
    new_base_url: getESP32BaseURL()
  });
});

// ============================================
// Error Handling Middleware
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

// Handle 404
app.use('*', (req, res) => {
  res.status(404).json({
    success: false,
    error: 'Endpoint not found',
    requested_path: req.originalUrl,
    available_endpoints: [
      'GET /',
      'GET /api/health',
      'GET /api/status',
      'GET /api/led/status',
      'POST /api/led/on',
      'POST /api/led/off',
      'POST /api/led/toggle',
      'POST /api/led/control',
      'POST /api/status/save',
      'PUT /api/config'
    ]
  });
});

// ============================================
// Server Startup
// ============================================

// Get local network IP addresses
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

app.listen(PORT, '0.0.0.0', async () => {
  const networkIPs = getNetworkIPs();
  
  console.log('\n=================================');
  console.log('ESP32 Controller Backend Started');
  console.log('=================================');
  console.log(`Local: http://localhost:${PORT}`);
  
  if (networkIPs.length > 0) {
    console.log('Network access:');
    networkIPs.forEach(ip => {
      console.log(`  http://${ip}:${PORT}`);
    });
  }
  
  console.log(`ESP32 device at: ${getESP32BaseURL()}`);
  console.log('=================================');
  console.log('Available API endpoints:');
  console.log('  GET  / - API documentation');
  console.log('  GET  /api/health - Check ESP32 connectivity');
  console.log('  GET  /api/status - Get device status');
  console.log('  POST /api/led/on - Turn LED on');
  console.log('  POST /api/led/off - Turn LED off');
  console.log('  POST /api/led/toggle - Toggle LED');
  console.log('  POST /api/led/control - Control with JSON');
  console.log('  POST /api/status/save - Save status to MongoDB');
  console.log('=================================');
  
  // Initialize MongoDB connection
  console.log('Initializing MongoDB connection...');
  await initMongoDB();
  
  // Test ESP32 connectivity on startup
  setTimeout(async () => {
    console.log('Testing ESP32 connectivity...');
    const result = await callESP32('/status');
    if (result.success) {
      console.log('✓ ESP32 is reachable and responding');
    } else {
      console.log('✗ ESP32 is not reachable:', result.error);
      console.log('  Make sure your ESP32 is powered on and connected');
      console.log(`  Update ESP32_IP in .env file or via PUT /api/config`);
    }
  }, 2000);
});

module.exports = app;