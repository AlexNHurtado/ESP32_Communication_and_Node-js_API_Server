const axios = require('axios');

// Configuration
const BASE_URL = 'http://localhost:3000';

// Test functions
async function testAPI() {
  console.log('🧪 Testing ESP32 Controller API');
  console.log('================================\n');

  try {
    // Test 1: Get API documentation
    console.log('1. Getting API documentation...');
    const docs = await axios.get(`${BASE_URL}/`);
    console.log('✓ API Documentation:', docs.data.title);
    console.log(`  ESP32 configured at: ${docs.data.esp32_config.base_url}\n`);

    // Test 2: Check health
    console.log('2. Checking ESP32 health...');
    try {
      const health = await axios.get(`${BASE_URL}/api/health`);
      console.log('✓ ESP32 Health:', health.data.status);
      console.log(`  ESP32 reachable: ${health.data.esp32_reachable}\n`);
    } catch (error) {
      console.log('✗ ESP32 Health check failed:', error.response?.data?.error || error.message);
      console.log('  Make sure your ESP32 is running and reachable\n');
    }

    // Test 3: Get status
    console.log('3. Getting device status...');
    try {
      const status = await axios.get(`${BASE_URL}/api/status`);
      console.log('✓ Device Status received');
      console.log(`  LED State: ${status.data.esp32?.led_state ? 'ON' : 'OFF'}`);
      console.log(`  ESP32 IP: ${status.data.esp32?.ip}`);
      console.log(`  WiFi Signal: ${status.data.esp32?.wifi_signal} dBm\n`);
    } catch (error) {
      console.log('✗ Status check failed:', error.response?.data?.error || error.message, '\n');
    }

    // Test 4: Turn LED ON
    console.log('4. Turning LED ON...');
    try {
      const ledOn = await axios.post(`${BASE_URL}/api/led/on`);
      console.log('✓ LED turned ON');
      console.log(`  Response: ${ledOn.data.action}\n`);
      await sleep(2000); // Wait 2 seconds
    } catch (error) {
      console.log('✗ LED ON failed:', error.response?.data?.error || error.message, '\n');
    }

    // Test 5: Turn LED OFF
    console.log('5. Turning LED OFF...');
    try {
      const ledOff = await axios.post(`${BASE_URL}/api/led/off`);
      console.log('✓ LED turned OFF');
      console.log(`  Response: ${ledOff.data.action}\n`);
      await sleep(2000); // Wait 2 seconds
    } catch (error) {
      console.log('✗ LED OFF failed:', error.response?.data?.error || error.message, '\n');
    }

    // Test 6: Toggle LED
    console.log('6. Toggling LED...');
    try {
      const toggle = await axios.post(`${BASE_URL}/api/led/toggle`);
      console.log('✓ LED toggled');
      console.log(`  Action: ${toggle.data.action}`);
      console.log(`  New state: ${toggle.data.new_state ? 'ON' : 'OFF'}\n`);
      await sleep(2000); // Wait 2 seconds
    } catch (error) {
      console.log('✗ LED toggle failed:', error.response?.data?.error || error.message, '\n');
    }

    // Test 7: Control LED with JSON
    console.log('7. Controlling LED with JSON payload...');
    try {
      const jsonControl = await axios.post(`${BASE_URL}/api/led/control`, {
        state: true
      });
      console.log('✓ LED controlled via JSON');
      console.log(`  Response: ${jsonControl.data.action}\n`);
    } catch (error) {
      console.log('✗ JSON control failed:', error.response?.data?.error || error.message, '\n');
    }

    // Test 8: Get LED status only
    console.log('8. Getting LED status only...');
    try {
      const ledStatus = await axios.get(`${BASE_URL}/api/led/status`);
      console.log('✓ LED Status retrieved');
      console.log(`  Current LED state: ${ledStatus.data.led_state ? 'ON' : 'OFF'}\n`);
    } catch (error) {
      console.log('✗ LED status failed:', error.response?.data?.error || error.message, '\n');
    }

    console.log('🎉 API testing completed!');

  } catch (error) {
    console.error('❌ Test failed:', error.message);
    console.log('\nMake sure:');
    console.log('1. The Node.js server is running (npm start)');
    console.log('2. Your ESP32 is powered on and connected to WiFi');
    console.log('3. The ESP32_IP in .env file is correct');
  }
}

// Helper function
function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

// Interactive demo
async function interactiveDemo() {
  console.log('\n🎮 Interactive LED Control Demo');
  console.log('================================');
  console.log('Commands:');
  console.log('  on     - Turn LED ON');
  console.log('  off    - Turn LED OFF');
  console.log('  toggle - Toggle LED state');
  console.log('  status - Get LED status');
  console.log('  quit   - Exit demo\n');

  const readline = require('readline');
  const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
  });

  const askCommand = () => {
    rl.question('Enter command: ', async (command) => {
      try {
        switch (command.toLowerCase().trim()) {
          case 'on':
            const onResult = await axios.post(`${BASE_URL}/api/led/on`);
            console.log('✓ LED turned ON\n');
            break;
          
          case 'off':
            const offResult = await axios.post(`${BASE_URL}/api/led/off`);
            console.log('✓ LED turned OFF\n');
            break;
          
          case 'toggle':
            const toggleResult = await axios.post(`${BASE_URL}/api/led/toggle`);
            console.log(`✓ ${toggleResult.data.action}\n`);
            break;
          
          case 'status':
            const statusResult = await axios.get(`${BASE_URL}/api/led/status`);
            console.log(`✓ LED is currently ${statusResult.data.led_state ? 'ON' : 'OFF'}\n`);
            break;
          
          case 'quit':
          case 'exit':
            console.log('👋 Goodbye!');
            rl.close();
            return;
          
          default:
            console.log('❌ Unknown command. Try: on, off, toggle, status, quit\n');
        }
      } catch (error) {
        console.log('❌ Command failed:', error.response?.data?.error || error.message, '\n');
      }
      
      askCommand(); // Ask for next command
    });
  };

  askCommand();
}

// Main execution
if (require.main === module) {
  const args = process.argv.slice(2);
  
  if (args.includes('--interactive') || args.includes('-i')) {
    interactiveDemo();
  } else {
    testAPI();
  }
}