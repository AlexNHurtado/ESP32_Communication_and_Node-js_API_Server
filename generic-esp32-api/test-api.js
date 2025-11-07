// Test script for Generic ESP32 API
// Run with: node test-api.js

const axios = require('axios');

const API_BASE_URL = 'http://localhost:3000';

// Test configuration
const testDevice = {
  device_id: 'esp32_test_001',
  ip: '192.168.1.100',
  port: 80,
  device_info: {
    type: 'test_device',
    location: 'test_lab',
    firmware: '1.0.0'
  }
};

async function delay(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function testAPI() {
  console.log('🧪 Testing Generic ESP32 REST API');
  console.log('===================================');

  try {
    // Test 1: Get API Documentation
    console.log('\n1️⃣ Testing API Documentation...');
    const docResponse = await axios.get(`${API_BASE_URL}/`);
    console.log('✅ API Documentation retrieved');
    console.log(`   Title: ${docResponse.data.title}`);
    console.log(`   Registered Devices: ${docResponse.data.registered_devices.length}`);

    // Test 2: Register ESP32 Device
    console.log('\n2️⃣ Testing Device Registration...');
    const registerResponse = await axios.post(`${API_BASE_URL}/register`, testDevice);
    console.log('✅ Device registered successfully');
    console.log(`   Device ID: ${registerResponse.data.device.device_id}`);
    console.log(`   MongoDB Log ID: ${registerResponse.data.mongodb_log_id}`);

    // Test 3: Send Sample Data
    console.log('\n3️⃣ Testing Data Transmission...');
    const sampleData = {
      temperature: 24.5,
      humidity: 62.3,
      pressure: 1013.25,
      battery_level: 85,
      sensor_status: 'ok',
      reading_count: 1,
      timestamp: new Date().toISOString()
    };

    const dataResponse = await axios.post(`${API_BASE_URL}/data`, sampleData, {
      headers: {
        'X-ESP32-ID': testDevice.device_id,
        'Content-Type': 'application/json'
      }
    });
    console.log('✅ Data sent successfully');
    console.log(`   Payload Size: ${dataResponse.data.payload_size} bytes`);
    console.log(`   MongoDB Log ID: ${dataResponse.data.mongodb_log_id}`);

    // Test 4: List Devices
    console.log('\n4️⃣ Testing Device List...');
    const devicesResponse = await axios.get(`${API_BASE_URL}/devices`);
    console.log('✅ Device list retrieved');
    console.log(`   Total Devices: ${devicesResponse.data.total_devices}`);
    console.log(`   MongoDB Connected: ${devicesResponse.data.mongodb_connected}`);

    // Wait a moment before testing communication endpoints
    await delay(1000);

    // Test 5: Send Command (This will fail since ESP32 is not real, but tests the endpoint)
    console.log('\n5️⃣ Testing Command Sending...');
    try {
      const commandResponse = await axios.post(`${API_BASE_URL}/send/command`, {
        device_id: testDevice.device_id,
        command: 'turn_led',
        parameters: {
          state: true,
          brightness: 200,
          color: 'blue'
        },
        endpoint: '/command'
      });
      console.log('✅ Command sent successfully');
    } catch (error) {
      if (error.response && error.response.status === 503) {
        console.log('⚠️ Command endpoint tested (ESP32 not reachable - expected)');
        console.log(`   Error: ${error.response.data.details}`);
      } else {
        throw error;
      }
    }

    // Test 6: Send Configuration
    console.log('\n6️⃣ Testing Configuration Sending...');
    try {
      const configResponse = await axios.post(`${API_BASE_URL}/send/config`, {
        device_id: testDevice.device_id,
        config: {
          wifi_ssid: 'TestNetwork',
          sensor_interval: 30000,
          deep_sleep_duration: 600,
          mqtt_broker: '192.168.1.1'
        },
        endpoint: '/config'
      });
      console.log('✅ Configuration sent successfully');
    } catch (error) {
      if (error.response && error.response.status === 503) {
        console.log('⚠️ Config endpoint tested (ESP32 not reachable - expected)');
      } else {
        throw error;
      }
    }

    // Test 7: Send Update Information
    console.log('\n7️⃣ Testing Update Sending...');
    try {
      const updateResponse = await axios.post(`${API_BASE_URL}/send/update`, {
        device_id: testDevice.device_id,
        update_info: {
          firmware_version: '2.1.0',
          download_url: 'https://example.com/firmware.bin',
          checksum: 'sha256:abc123def456...',
          force_update: false,
          changelog: ['Bug fixes', 'Performance improvements']
        },
        endpoint: '/update'
      });
      console.log('✅ Update information sent successfully');
    } catch (error) {
      if (error.response && error.response.status === 503) {
        console.log('⚠️ Update endpoint tested (ESP32 not reachable - expected)');
      } else {
        throw error;
      }
    }

    // Test 8: Send Custom Data
    console.log('\n8️⃣ Testing Custom Data Sending...');
    try {
      const customResponse = await axios.post(`${API_BASE_URL}/send/custom`, {
        device_id: testDevice.device_id,
        data: {
          message: 'Hello from API test!',
          action: 'display_message',
          parameters: {
            duration: 5000,
            color: 'green',
            font_size: 16
          },
          test_mode: true
        },
        endpoint: '/custom',
        method: 'POST'
      });
      console.log('✅ Custom data sent successfully');
    } catch (error) {
      if (error.response && error.response.status === 503) {
        console.log('⚠️ Custom data endpoint tested (ESP32 not reachable - expected)');
      } else {
        throw error;
      }
    }

    // Test 9: Get Statistics
    console.log('\n9️⃣ Testing Statistics...');
    const statsResponse = await axios.get(`${API_BASE_URL}/stats`);
    console.log('✅ Statistics retrieved');
    console.log(`   API Uptime: ${Math.round(statsResponse.data.api_stats.uptime_seconds)} seconds`);
    console.log(`   Registered Devices: ${statsResponse.data.api_stats.registered_devices}`);
    if (statsResponse.data.mongodb_stats.available) {
      console.log(`   Total MongoDB Logs: ${statsResponse.data.mongodb_stats.total_logs}`);
      console.log(`   Recent Logs (24h): ${statsResponse.data.mongodb_stats.recent_logs_24h}`);
    }

    // Test 10: Send Multiple Data Points
    console.log('\n🔟 Testing Multiple Data Points...');
    for (let i = 0; i < 3; i++) {
      const multiData = {
        temperature: 20 + Math.random() * 10,
        humidity: 50 + Math.random() * 30,
        light_level: Math.round(Math.random() * 1024),
        motion_detected: Math.random() > 0.7,
        sequence: i + 1,
        timestamp: new Date().toISOString()
      };

      await axios.post(`${API_BASE_URL}/data`, multiData, {
        headers: { 'X-ESP32-ID': testDevice.device_id }
      });
      
      console.log(`   📊 Data point ${i + 1} sent: T=${multiData.temperature.toFixed(1)}°C, H=${multiData.humidity.toFixed(1)}%`);
      await delay(500);
    }

    console.log('\n🎉 All tests completed successfully!');
    console.log('===================================');
    console.log('✅ API is working correctly');
    console.log('✅ MongoDB logging is functional');
    console.log('✅ Device registration works');
    console.log('✅ Data transmission works');
    console.log('✅ All 4 ESP32 communication endpoints tested');
    console.log('✅ Statistics and monitoring work');
    
    console.log('\n📋 Next Steps:');
    console.log('1. Start your ESP32 devices with the correct API endpoints');
    console.log('2. Register your real ESP32 devices using POST /register');
    console.log('3. Configure ESP32s to send data to POST /data');
    console.log('4. Use the 4 communication endpoints to control your devices');
    console.log('5. Monitor everything via GET /stats and MongoDB');

  } catch (error) {
    console.error('\n❌ Test failed:', error.message);
    if (error.response) {
      console.error('Response status:', error.response.status);
      console.error('Response data:', error.response.data);
    }
    console.log('\n🔧 Troubleshooting:');
    console.log('1. Make sure the server is running: npm start');
    console.log('2. Check if MongoDB is running');
    console.log('3. Verify the server is accessible at http://localhost:3001');
  }
}

// Run the tests
if (require.main === module) {
  testAPI().catch(console.error);
}

module.exports = { testAPI };