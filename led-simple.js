const axios = require('axios');

// Configuration - Update this with your ESP32's IP address
const ESP32_IP = '192.168.1.100'; // Change this to your ESP32's actual IP
const ESP32_PORT = 80;
const ESP32_URL = `http://${ESP32_IP}:${ESP32_PORT}`;

/**
 * Simple LED toggle function
 * @param {boolean} state - true for ON, false for OFF
 */
async function toggleLED(state) {
  try {
    console.log(`🔄 Turning LED ${state ? 'ON' : 'OFF'}...`);
    
    const response = await axios.post(`${ESP32_URL}/led`, 
      { state: state },
      {
        headers: { 'Content-Type': 'application/json' },
        timeout: 5000
      }
    );

    console.log('✅ Success!', response.data);
    return response.data;
    
  } catch (error) {
    console.error('❌ Error:', error.message);
    if (error.response) {
      console.error('Response:', error.response.data);
    }
    return null;
  }
}

/**
 * Quick test function - toggles LED a few times
 */
async function quickTest() {
  console.log('🚀 Starting LED toggle test...\n');
  
  // Turn ON
  await toggleLED(true);
  await sleep(2000);
  
  // Turn OFF  
  await toggleLED(false);
  await sleep(2000);
  
  // Turn ON again
  await toggleLED(true);
  await sleep(2000);
  
  // Turn OFF again
  await toggleLED(false);
  
  console.log('\n🎉 Test completed!');
}

/**
 * Sleep function for delays
 */
function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

// Get command line arguments
const args = process.argv.slice(2);
const command = args[0];

// Handle different commands
switch (command) {
  case 'on':
  case '1':
    toggleLED(true);
    break;
    
  case 'off':
  case '0':
    toggleLED(false);
    break;
    
  case 'test':
    quickTest();
    break;
    
  default:
    console.log('💡 ESP32 LED Toggle Script');
    console.log('========================');
    console.log('Usage:');
    console.log('  node led-simple.js on    - Turn LED ON');
    console.log('  node led-simple.js off   - Turn LED OFF'); 
    console.log('  node led-simple.js test  - Run toggle test');
    console.log('  node led-simple.js 1     - Turn LED ON');
    console.log('  node led-simple.js 0     - Turn LED OFF');
    console.log('\nMake sure to update ESP32_IP in the script!');
}

module.exports = { toggleLED };