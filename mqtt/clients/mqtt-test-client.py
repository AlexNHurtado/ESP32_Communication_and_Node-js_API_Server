#!/usr/bin/env python3
"""
ESP32 MQTT Test Client
Simple Python script to test MQTT communication with ESP32 device
"""

import paho.mqtt.client as mqtt
import json
import time
import threading
from datetime import datetime

# MQTT Configuration
BROKER = "broker.hivemq.com"
PORT = 1883
KEEPALIVE = 60

# MQTT Topics
TOPIC_LED_CONTROL = "esp32/led/control"
TOPIC_LED_STATUS = "esp32/led/status"
TOPIC_DEVICE_STATUS = "esp32/device/status"
TOPIC_DEVICE_COMMAND = "esp32/device/command"

class ESP32MqttClient:
    def __init__(self):
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        self.connected = False
        
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print(f"Connected to MQTT broker: {BROKER}:{PORT}")
            print("Subscribing to ESP32 topics...")
            
            # Subscribe to ESP32 response topics
            client.subscribe(TOPIC_LED_STATUS)
            client.subscribe(TOPIC_DEVICE_STATUS)
            
            print(f"   - {TOPIC_LED_STATUS}")
            print(f"   - {TOPIC_DEVICE_STATUS}")
            self.connected = True
        else:
            print(f"Failed to connect, return code {rc}")
            
    def on_message(self, client, userdata, msg):
        topic = msg.topic
        try:
            # Try to parse as JSON
            message = json.loads(msg.payload.decode())
            timestamp = datetime.now().strftime("%H:%M:%S")
            
            if topic == TOPIC_LED_STATUS:
                state = message.get('led_state', 'unknown')
                msg_text = message.get('message', '')
                print(f"[{timestamp}] LED Status: {state} ({msg_text})")
                
            elif topic == TOPIC_DEVICE_STATUS:
                device_id = message.get('device_id', 'unknown')
                ip = message.get('ip', 'unknown')
                uptime = message.get('uptime', 0)
                heap = message.get('heap', 0)
                led_state = message.get('led_state', False)
                mqtt_connected = message.get('mqtt_connected', False)
                
                print(f"[{timestamp}] Device Status:")
                print(f"   Device ID: {device_id}")
                print(f"   IP Address: {ip}")
                print(f"   Uptime: {uptime}s")
                print(f"   Free Heap: {heap} bytes")
                print(f"   LED State: {led_state}")
                print(f"   MQTT Connected: {mqtt_connected}")
                
        except json.JSONDecodeError:
            # Handle plain text messages
            message = msg.payload.decode()
            timestamp = datetime.now().strftime("%H:%M:%S")
            print(f"[{timestamp}] {topic}: {message}")
            
    def on_disconnect(self, client, userdata, rc):
        self.connected = False
        if rc != 0:
            print("Unexpected MQTT disconnection. Attempting to reconnect...")
        else:
            print("Disconnected from MQTT broker")
            
    def connect(self):
        try:
            print(f"Connecting to MQTT broker {BROKER}:{PORT}...")
            self.client.connect(BROKER, PORT, KEEPALIVE)
            self.client.loop_start()
        except Exception as e:
            print(f"Connection error: {e}")
            
    def disconnect(self):
        self.client.loop_stop()
        self.client.disconnect()
        
    def control_led(self, state):
        if not self.connected:
            print("Not connected to MQTT broker")
            return False
            
        payload = json.dumps({"state": state})
        result = self.client.publish(TOPIC_LED_CONTROL, payload)
        
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            action = "ON" if state else "OFF"
            print(f"Sent LED command: {action}")
            return True
        else:
            print(f"Failed to send LED command")
            return False
            
    def request_status(self):
        if not self.connected:
            print("Not connected to MQTT broker")
            return False
            
        result = self.client.publish(TOPIC_DEVICE_COMMAND, "status")
        
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            print("Requested device status")
            return True
        else:
            print("Failed to request status")
            return False
            
    def restart_device(self):
        if not self.connected:
            print("Not connected to MQTT broker")
            return False
            
        result = self.client.publish(TOPIC_DEVICE_COMMAND, "restart")
        
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            print("Sent restart command")
            return True
        else:
            print("Failed to send restart command")
            return False

def print_help():
    print("\nAvailable Commands:")
    print("   on      - Turn LED ON")
    print("   off     - Turn LED OFF")
    print("   status  - Request device status")
    print("   restart - Restart ESP32 device")
    print("   help    - Show this help")
    print("   quit    - Exit program")
    print()

def main():
    print("ESP32 MQTT Test Client")
    print("=" * 40)
    
    # Create and connect client
    client = ESP32MqttClient()
    client.connect()
    
    # Wait for connection
    time.sleep(2)
    
    if not client.connected:
        print("Failed to connect. Please check:")
        print("   - Internet connection")
        print("   - MQTT broker availability")
        print("   - Firewall settings")
        return
        
    print_help()
    
    try:
        while True:
            command = input("Enter command: ").lower().strip()
            
            if command == "on":
                client.control_led(True)
                
            elif command == "off":
                client.control_led(False)
                
            elif command == "status":
                client.request_status()
                
            elif command == "restart":
                confirm = input("Are you sure you want to restart the ESP32? (y/n): ")
                if confirm.lower() == 'y':
                    client.restart_device()
                else:
                    print("Restart cancelled")
                    
            elif command == "help":
                print_help()
                
            elif command in ["quit", "exit", "q"]:
                break
                
            else:
                print(f"Unknown command: '{command}'. Type 'help' for available commands.")
                
    except KeyboardInterrupt:
        print("\nInterrupted by user")
        
    finally:
        print("Disconnecting...")
        client.disconnect()
        print("Goodbye!")

if __name__ == "__main__":
    # Check if paho-mqtt is installed
    try:
        import paho.mqtt.client as mqtt
    except ImportError:
        print("Error: paho-mqtt library not found")
        print("Install it using: pip install paho-mqtt")
        exit(1)
        
    main()