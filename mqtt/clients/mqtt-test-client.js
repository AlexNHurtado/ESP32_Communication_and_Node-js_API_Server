#!/usr/bin/env node
/**
 * ESP32 MQTT Test Client (Node.js)
 * Interactive CLI tool for testing MQTT communication with ESP32
 */

const mqtt = require('mqtt');
const readline = require('readline');

// MQTT Configuration
const BROKER_URL = 'mqtt://broker.hivemq.com:1883';

// MQTT Topics
const TOPICS = {
    LED_CONTROL: 'esp32/led/control',
    LED_STATUS: 'esp32/led/status',
    DEVICE_STATUS: 'esp32/device/status',
    DEVICE_COMMAND: 'esp32/device/command'
};

class ESP32MqttClient {
    constructor() {
        this.client = null;
        this.connected = false;
        this.setupReadline();
    }

    setupReadline() {
        this.rl = readline.createInterface({
            input: process.stdin,
            output: process.stdout
        });
    }

    connect() {
        console.log(`Connecting to MQTT broker: ${BROKER_URL}`);
        
        this.client = mqtt.connect(BROKER_URL);

        this.client.on('connect', () => {
            console.log('Connected to MQTT broker');
            console.log('Subscribing to ESP32 topics...');
            
            // Subscribe to response topics
            this.client.subscribe([
                TOPICS.LED_STATUS,
                TOPICS.DEVICE_STATUS
            ], (err) => {
                if (!err) {
                    console.log(`   - ${TOPICS.LED_STATUS}`);
                    console.log(`   - ${TOPICS.DEVICE_STATUS}`);
                    this.connected = true;
                    this.showHelp();
                    this.startCommandLoop();
                } else {
                    console.log('Subscription failed:', err.message);
                }
            });
        });

        this.client.on('message', (topic, message) => {
            this.handleMessage(topic, message);
        });

        this.client.on('error', (error) => {
            console.log('MQTT Error:', error.message);
        });

        this.client.on('close', () => {
            this.connected = false;
            console.log('Disconnected from MQTT broker');
        });
    }

    handleMessage(topic, message) {
        const timestamp = new Date().toLocaleTimeString();
        
        try {
            const data = JSON.parse(message.toString());
            
            if (topic === TOPICS.LED_STATUS) {
                const state = data.led_state;
                const msg = data.message || '';
                console.log(`[${timestamp}] LED Status: ${state} (${msg})`);
                
            } else if (topic === TOPICS.DEVICE_STATUS) {
                console.log(`[${timestamp}] Device Status:`);
                console.log(`   Device ID: ${data.device_id || 'unknown'}`);
                console.log(`   IP Address: ${data.ip || 'unknown'}`);
                console.log(`   Uptime: ${data.uptime || 0}s`);
                console.log(`   Free Heap: ${data.heap || 0} bytes`);
                console.log(`   LED State: ${data.led_state}`);
                console.log(`   MQTT Connected: ${data.mqtt_connected}`);
            }
        } catch (error) {
            // Handle plain text messages
            console.log(`[${timestamp}] ${topic}: ${message.toString()}`);
        }
        
        this.showPrompt();
    }

    controlLED(state) {
        if (!this.connected) {
            console.log('Not connected to MQTT broker');
            return;
        }

        const payload = JSON.stringify({ state: state });
        this.client.publish(TOPICS.LED_CONTROL, payload, (err) => {
            if (!err) {
                const action = state ? 'ON' : 'OFF';
                console.log(`Sent LED command: ${action}`);
            } else {
                console.log('Failed to send LED command:', err.message);
            }
            this.showPrompt();
        });
    }

    requestStatus() {
        if (!this.connected) {
            console.log('Not connected to MQTT broker');
            return;
        }

        this.client.publish(TOPICS.DEVICE_COMMAND, 'status', (err) => {
            if (!err) {
                console.log('Requested device status');
            } else {
                console.log('Failed to request status:', err.message);
            }
            this.showPrompt();
        });
    }

    restartDevice() {
        if (!this.connected) {
            console.log('Not connected to MQTT broker');
            return;
        }

        this.rl.question('Are you sure you want to restart the ESP32? (y/n): ', (answer) => {
            if (answer.toLowerCase() === 'y') {
                this.client.publish(TOPICS.DEVICE_COMMAND, 'restart', (err) => {
                    if (!err) {
                        console.log('Sent restart command');
                    } else {
                        console.log('Failed to send restart command:', err.message);
                    }
                    this.showPrompt();
                });
            } else {
                console.log('Restart cancelled');
                this.showPrompt();
            }
        });
    }

    showHelp() {
        console.log('\nAvailable Commands:');
        console.log('   on      - Turn LED ON');
        console.log('   off     - Turn LED OFF');
        console.log('   status  - Request device status');
        console.log('   restart - Restart ESP32 device');
        console.log('   help    - Show this help');
        console.log('   quit    - Exit program');
        console.log();
    }

    showPrompt() {
        process.stdout.write('Enter command: ');
    }

    startCommandLoop() {
        this.showPrompt();
        
        this.rl.on('line', (input) => {
            const command = input.trim().toLowerCase();
            
            switch (command) {
                case 'on':
                    this.controlLED(true);
                    break;
                    
                case 'off':
                    this.controlLED(false);
                    break;
                    
                case 'status':
                    this.requestStatus();
                    break;
                    
                case 'restart':
                    this.restartDevice();
                    break;
                    
                case 'help':
                    this.showHelp();
                    this.showPrompt();
                    break;
                    
                case 'quit':
                case 'exit':
                case 'q':
                    this.disconnect();
                    break;
                    
                default:
                    if (command) {
                        console.log(`Unknown command: '${command}'. Type 'help' for available commands.`);
                    }
                    this.showPrompt();
                    break;
            }
        });
    }

    disconnect() {
        console.log('Disconnecting...');
        if (this.client) {
            this.client.end();
        }
        this.rl.close();
        console.log('Goodbye!');
        process.exit(0);
    }
}

// Check if mqtt module is available
try {
    require('mqtt');
} catch (error) {
    console.log('Error: mqtt module not found');
    console.log('Install it using: npm install mqtt');
    process.exit(1);
}

// Handle process interruption
process.on('SIGINT', () => {
    console.log('\nInterrupted by user');
    process.exit(0);
});

// Start the application
console.log('ESP32 MQTT Test Client (Node.js)');
console.log('=' .repeat(40));

const client = new ESP32MqttClient();
client.connect();