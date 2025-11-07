const axios = require('axios');
const readline = require('readline');

// Configuration - Update this with your ESP32's IP address
const ESP32_IP = '10.100.1.67'; // Change this to your ESP32's actual IP
const ESP32_PORT = 80;
const ESP32_BASE_URL = `http://${ESP32_IP}:${ESP32_PORT}`;

// LED state tracker
let currentLedState = false;

// Create readline interface for interactive commands
const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

/**
 * Send POST request to toggle LED
 * @param {boolean} state - true for ON, false for OFF
 */
async function toggleLED(state) {
  try {
    console.log(`\nSending request to turn LED ${state ? 'ON' : 'OFF'}...`);
    
    const response = await axios.post(`${ESP32_BASE_URL}/led`, {
      state: state
    }, {
      headers: {
        'Content-Type': 'application/json'
      },
      timeout: 5000
    });

    console.log('Success!');
    console.log(`Response:`, response.data);
    
    // Update our local state tracker
    currentLedState = response.data.led_state;
    
    return response.data;
  } catch (error) {
    console.error('Error controlling LED:');
    if (error.response) {
      console.error(`   Status: ${error.response.status}`);
      console.error(`   Data:`, error.response.data);
    } else if (error.request) {
      console.error('   No response received - check ESP32 connection');
      console.error(`   Trying to connect to: ${ESP32_BASE_URL}`);
    } else {
      console.error(`   ${error.message}`);
    }
    return null;
  }
}

/**
 * Get current LED status from ESP32
 */
async function getLEDStatus() {
  try {
    console.log('\nGetting LED status...');
    
    const response = await axios.get(`${ESP32_BASE_URL}/status`, {
      timeout: 5000
    });

    console.log('Status retrieved!');
    console.log(`Response:`, response.data);
    
    // Update our local state tracker
    currentLedState = response.data.led_state;
    
    return response.data;
  } catch (error) {
    console.error('Error getting status:');
    if (error.response) {
      console.error(`   Status: ${error.response.status}`);
      console.error(`   Data:`, error.response.data);
    } else if (error.request) {
      console.error('   No response received - check ESP32 connection');
    } else {
      console.error(`   ${error.message}`);
    }
    return null;
  }
}

/**
 * Toggle LED to opposite state
 */
async function toggleLEDState() {
  // First get current status to ensure we have the right state
  const status = await getLEDStatus();
  if (status) {
    const newState = !status.led_state;
    await toggleLED(newState);
  } else {
    console.log('Could not get current status, toggling based on local state...');
    await toggleLED(!currentLedState);
  }
}

/**
 * Display help menu
 */
function showMenu() {
  console.log('\n' + '='.repeat(50));
  console.log('ESP32 LED Controller');
  console.log('='.repeat(50));
  console.log(`ESP32 Address: ${ESP32_BASE_URL}`);
  console.log(`Current LED State: ${currentLedState ? 'ON' : 'OFF'}`);
  console.log('\nCommands:');
  console.log('  1 or on    - Turn LED ON');
  console.log('  0 or off   - Turn LED OFF');
  console.log('  t or toggle - Toggle LED state');
  console.log('  s or status - Get LED status');
  console.log('  h or help   - Show this menu');
  console.log('  q or quit   - Exit program');
  console.log('='.repeat(50));
}

/**
 * Process user commands
 */
function processCommand(input) {
  const command = input.toLowerCase().trim();
  
  switch (command) {
    case '1':
    case 'on':
      toggleLED(true);
      break;
      
    case '0':
    case 'off':
      toggleLED(false);
      break;
      
    case 't':
    case 'toggle':
      toggleLEDState();
      break;
      
    case 's':
    case 'status':
      getLEDStatus();
      break;
      
    case 'h':
    case 'help':
      showMenu();
      break;
      
    case 'q':
    case 'quit':
    case 'exit':
      console.log('\n👋 Goodbye!');
      rl.close();
      return;
      
    default:
      console.log('❓ Unknown command. Type "help" for available commands.');
  }
  
  // Prompt for next command
  setTimeout(() => {
    rl.question('\n💻 Enter command: ', processCommand);
  }, 100);
}

/**
 * Auto-toggle demo function
 */
async function autoToggleDemo(times = 5, interval = 2000) {
  console.log(`\n🎮 Starting auto-toggle demo (${times} toggles, ${interval}ms interval)...`);
  
  for (let i = 0; i < times; i++) {
    console.log(`\nDemo ${i + 1}/${times}:`);
    await toggleLEDState();
    
    if (i < times - 1) {
      console.log(`⏳ Waiting ${interval}ms...`);
      await new Promise(resolve => setTimeout(resolve, interval));
    }
  }
  
  console.log('\n🎉 Auto-toggle demo completed!');
  showMenu();
  rl.question('\n💻 Enter command: ', processCommand);
}

/**
 * Main function
 */
async function main() {
  console.log('🚀 ESP32 LED Controller Starting...');
  console.log(`🔍 Checking connection to ESP32 at ${ESP32_BASE_URL}...`);
  
  // Test initial connection and get status
  const initialStatus = await getLEDStatus();
  
  if (initialStatus) {
    console.log('✅ Successfully connected to ESP32!');
    showMenu();
    
    // Start interactive mode
    rl.question('\n💻 Enter command (or "demo" for auto-toggle): ', (input) => {
      if (input.toLowerCase().trim() === 'demo') {
        autoToggleDemo();
      } else {
        processCommand(input);
      }
    });
  } else {
    console.log('\n❌ Could not connect to ESP32.');
    console.log('📝 Please check:');
    console.log(`   1. ESP32 is powered on and connected to WiFi`);
    console.log(`   2. IP address is correct: ${ESP32_IP}`);
    console.log(`   3. ESP32 web server is running`);
    console.log(`   4. You can ping the ESP32: ping ${ESP32_IP}`);
    console.log('\n🔧 To change IP address, edit the ESP32_IP variable in this script.');
    
    rl.question('\n🔄 Try again? (y/n): ', (answer) => {
      if (answer.toLowerCase().startsWith('y')) {
        main();
      } else {
        console.log('👋 Goodbye!');
        rl.close();
      }
    });
  }
}

// Handle graceful shutdown
process.on('SIGINT', () => {
  console.log('\n\n👋 Shutting down gracefully...');
  rl.close();
  process.exit(0);
});

// Start the program
if (require.main === module) {
  main();
}

// Export functions for use in other modules
module.exports = {
  toggleLED,
  getLEDStatus,
  toggleLEDState,
  ESP32_BASE_URL
};