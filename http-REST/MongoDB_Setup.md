# MongoDB Integration Setup Guide

## Overview
This ESP32 REST API project now includes MongoDB integration to save device status information to a local MongoDB database.

## Prerequisites
1. **MongoDB installed locally** (default: `localhost:27017`)
2. **Node.js dependencies** installed (`npm install`)
3. **ESP32 device** running the REST API server

## Configuration

### 1. Environment Variables (.env file)
The MongoDB configuration is stored in the `.env` file:

```env
# MongoDB Configuration
MONGODB_URI=mongodb://localhost:27017
MONGODB_DATABASE=esp32_iot
MONGODB_COLLECTION=device_status
MONGODB_USERNAME=
MONGODB_PASSWORD=
```

### 2. MongoDB Database Structure
When you save ESP32 status, the following document structure is created:

```json
{
  "_id": "ObjectId(...)",
  "device_info": {
    "ip": "10.100.1.67",
    "port": 80,
    "base_url": "http://10.100.1.67:80"
  },
  "esp32_status": {
    "led": true,
    "wifi_ssid": "YourWiFi",
    "rssi": -45,
    "uptime": 123456,
    "free_heap": 234567
  },
  "server_info": {
    "backend_server": "DESKTOP-ABC123:3000",
    "platform": "win32",
    "arch": "x64",
    "uptime_seconds": 456.789,
    "memory_usage": { ... }
  },
  "saved_at": "2024-12-19T10:30:00.000Z",
  "saved_timestamp": 1734602200000
}
```

## Installation Steps

### 1. Install MongoDB (if not already installed)
- **Windows**: Download from [MongoDB Community Server](https://www.mongodb.com/try/download/community)
- **macOS**: `brew install mongodb-community`
- **Linux**: Follow [official installation guide](https://docs.mongodb.com/manual/installation/)

### 2. Start MongoDB Service
```bash
# Windows (as Administrator)
net start MongoDB

# macOS/Linux
brew services start mongodb-community
# or
sudo systemctl start mongod
```

### 3. Install Node.js Dependencies
```bash
cd http-REST
npm install mongodb
```

## Usage

### Web Interface
1. Start your ESP32 device with the REST API server
2. Start the Node.js backend: `node esp32-controller.js`
3. Open `http://localhost:3000` in your browser
4. Click the **"Save to MongoDB"** button in the LED Control Panel

### API Endpoint
**POST** `/api/status/save`

**Response (Success):**
```json
{
  "success": true,
  "message": "ESP32 status saved to MongoDB successfully",
  "mongodb": {
    "database": "esp32_iot",
    "collection": "device_status",
    "document_id": "ObjectId(...)"
  },
  "saved_data": { ... },
  "timestamp": "2024-12-19T10:30:00.000Z"
}
```

**Response (Error):**
```json
{
  "success": false,
  "error": "MongoDB is not connected",
  "details": "Database service unavailable. Check MongoDB connection and credentials."
}
```

## Troubleshooting

### MongoDB Connection Issues
1. **Check if MongoDB is running:**
   ```bash
   # Windows
   sc query MongoDB
   
   # macOS/Linux
   brew services list | grep mongodb
   # or
   systemctl status mongod
   ```

2. **Check MongoDB logs:**
   - Windows: `C:\Program Files\MongoDB\Server\7.0\log\mongod.log`
   - macOS: `/usr/local/var/log/mongodb/mongo.log`
   - Linux: `/var/log/mongodb/mongod.log`

3. **Test MongoDB connection:**
   ```bash
   mongosh
   > use esp32_iot
   > db.device_status.find().limit(5)
   ```

### Common Error Messages
- **"MongoDB is not connected"**: MongoDB service not running or wrong URI
- **"ESP32 device unreachable"**: ESP32 not responding (check WiFi/IP)
- **"Failed to save status to MongoDB"**: Database permission or connection issues

## Database Management

### View Saved Data
```bash
mongosh
> use esp32_iot
> db.device_status.find().pretty()
```

### Count Documents
```bash
> db.device_status.countDocuments()
```

### Clear Collection (Optional)
```bash
> db.device_status.deleteMany({})
```

## Security Notes
- Default configuration uses no authentication (localhost only)
- For production, configure MongoDB authentication
- Update `.env` file with username/password if needed
- Consider network security when deploying

## Features
- ✅ Automatic MongoDB connection on server startup
- ✅ Error handling and graceful degradation
- ✅ Real-time status capture with metadata
- ✅ Web interface integration
- ✅ Comprehensive logging and feedback
- ✅ Configurable database and collection names