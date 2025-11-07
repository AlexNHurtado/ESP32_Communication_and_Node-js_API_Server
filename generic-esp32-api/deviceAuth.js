/**
 * Device Authentication and Whitelist Module
 * 
 * This module provides:
 * 1. Device registration with duplicate prevention
 * 2. Whitelist-based access control for data endpoints
 * 3. Device verification and validation
 * 4. Registration status tracking
 */

const crypto = require('crypto');

class DeviceAuthManager {
  constructor() {
    this.registeredDevices = new Map(); // deviceId -> device info
    this.authTokens = new Map(); // deviceId -> auth token
    this.registrationAttempts = new Map(); // ip -> attempt count
    this.blacklistedIPs = new Set(); // blocked IP addresses
    
    // Configuration
    this.config = {
      maxRegistrationAttempts: 5,
      registrationCooldown: 300000, // 5 minutes in ms
      tokenExpiryTime: 86400000, // 24 hours in ms
      requireUniqueIPs: false, // allow multiple devices from same IP
      enableWhitelist: true // enable whitelist checking
    };

    console.log('🔐 Device Authentication Manager initialized');
  }

  /**
   * Generate a unique authentication token for a device
   */
  generateAuthToken(deviceId) {
    const timestamp = Date.now();
    const randomBytes = crypto.randomBytes(16).toString('hex');
    const token = crypto.createHash('sha256')
      .update(`${deviceId}-${timestamp}-${randomBytes}`)
      .digest('hex');
    
    this.authTokens.set(deviceId, {
      token,
      createdAt: timestamp,
      expiresAt: timestamp + this.config.tokenExpiryTime
    });

    return token;
  }

  /**
   * Check if a device is registered and authorized
   */
  isDeviceAuthorized(deviceId) {
    if (!this.config.enableWhitelist) {
      return true; // Whitelist disabled, allow all
    }

    return this.registeredDevices.has(deviceId);
  }

  /**
   * Validate device authentication token
   */
  validateAuthToken(deviceId, providedToken) {
    const tokenInfo = this.authTokens.get(deviceId);
    
    if (!tokenInfo) {
      return { valid: false, reason: 'No auth token found' };
    }

    if (Date.now() > tokenInfo.expiresAt) {
      this.authTokens.delete(deviceId);
      return { valid: false, reason: 'Token expired' };
    }

    if (tokenInfo.token !== providedToken) {
      return { valid: false, reason: 'Invalid token' };
    }

    return { valid: true };
  }

  /**
   * Check if IP is allowed to register (rate limiting)
   */
  canRegisterFromIP(clientIP) {
    if (this.blacklistedIPs.has(clientIP)) {
      return { allowed: false, reason: 'IP blacklisted' };
    }

    const attempts = this.registrationAttempts.get(clientIP) || { count: 0, lastAttempt: 0 };
    const now = Date.now();
    
    // Reset attempts if cooldown period has passed
    if (now - attempts.lastAttempt > this.config.registrationCooldown) {
      attempts.count = 0;
    }

    if (attempts.count >= this.config.maxRegistrationAttempts) {
      return { 
        allowed: false, 
        reason: 'Too many registration attempts',
        retryAfter: this.config.registrationCooldown - (now - attempts.lastAttempt)
      };
    }

    return { allowed: true };
  }

  /**
   * Register a new device (with duplicate prevention)
   */
  registerDevice(deviceId, deviceInfo, clientIP) {
    const registrationResult = {
      success: false,
      message: '',
      deviceInfo: null,
      authToken: null,
      warnings: []
    };

    // Check IP registration limits
    const ipCheck = this.canRegisterFromIP(clientIP);
    if (!ipCheck.allowed) {
      this.incrementRegistrationAttempts(clientIP);
      registrationResult.message = ipCheck.reason;
      if (ipCheck.retryAfter) {
        registrationResult.retryAfter = Math.ceil(ipCheck.retryAfter / 1000); // seconds
      }
      return registrationResult;
    }

    // Check for duplicate device ID
    if (this.registeredDevices.has(deviceId)) {
      const existingDevice = this.registeredDevices.get(deviceId);
      
      // Check if it's the same device (same IP) trying to re-register
      if (existingDevice.ip === deviceInfo.ip) {
        // Allow re-registration from same IP (device restart scenario)
        registrationResult.warnings.push('Device re-registered from same IP');
        this.updateDeviceInfo(deviceId, deviceInfo);
      } else {
        // Different IP trying to use same device ID - reject
        this.incrementRegistrationAttempts(clientIP);
        registrationResult.message = `Device ID '${deviceId}' is already registered from different IP (${existingDevice.ip})`;
        return registrationResult;
      }
    } else {
      // New device registration
      this.addNewDevice(deviceId, deviceInfo, clientIP);
    }

    // Generate auth token
    const authToken = this.generateAuthToken(deviceId);

    // Log successful registration
    const device = this.registeredDevices.get(deviceId);
    console.log(`✅ Device registered: ${deviceId} from ${clientIP}`);

    registrationResult.success = true;
    registrationResult.message = 'Device registered successfully';
    registrationResult.deviceInfo = device;
    registrationResult.authToken = authToken;

    return registrationResult;
  }

  /**
   * Add a new device to the registry
   */
  addNewDevice(deviceId, deviceInfo, clientIP) {
    const deviceRecord = {
      deviceId,
      ip: deviceInfo.ip,
      port: deviceInfo.port || 80,
      registeredAt: new Date(),
      registeredFromIP: clientIP,
      lastSeen: new Date(),
      info: deviceInfo.device_info || {},
      status: 'active',
      dataCount: 0, // Track number of data submissions
      lastDataSubmission: null
    };

    this.registeredDevices.set(deviceId, deviceRecord);
  }

  /**
   * Update existing device information
   */
  updateDeviceInfo(deviceId, deviceInfo) {
    const existingDevice = this.registeredDevices.get(deviceId);
    if (existingDevice) {
      existingDevice.ip = deviceInfo.ip;
      existingDevice.port = deviceInfo.port || existingDevice.port;
      existingDevice.lastSeen = new Date();
      existingDevice.info = { ...existingDevice.info, ...(deviceInfo.device_info || {}) };
      existingDevice.status = 'active';
    }
  }

  /**
   * Increment registration attempts for an IP
   */
  incrementRegistrationAttempts(clientIP) {
    const attempts = this.registrationAttempts.get(clientIP) || { count: 0, lastAttempt: 0 };
    attempts.count += 1;
    attempts.lastAttempt = Date.now();
    this.registrationAttempts.set(clientIP, attempts);

    // Auto-blacklist after excessive attempts
    if (attempts.count >= this.config.maxRegistrationAttempts * 2) {
      this.blacklistedIPs.add(clientIP);
      console.warn(`🚫 IP blacklisted for excessive registration attempts: ${clientIP}`);
    }
  }

  /**
   * Validate data submission from device
   */
  validateDataSubmission(deviceId, clientIP, headers) {
    const validation = {
      allowed: false,
      reason: '',
      deviceInfo: null
    };

    // Check if device is registered (whitelist)
    if (!this.isDeviceAuthorized(deviceId)) {
      validation.reason = `Device '${deviceId}' is not registered or authorized`;
      return validation;
    }

    const device = this.registeredDevices.get(deviceId);
    
    // Optional: Verify IP matches registration (strict mode)
    if (this.config.requireUniqueIPs && device.ip !== clientIP) {
      validation.reason = `Data submission from unauthorized IP. Expected: ${device.ip}, Got: ${clientIP}`;
      return validation;
    }

    // Update device activity
    device.lastSeen = new Date();
    device.dataCount += 1;
    device.lastDataSubmission = new Date();

    validation.allowed = true;
    validation.deviceInfo = device;

    return validation;
  }

  /**
   * Get all registered devices
   */
  getRegisteredDevices() {
    return Array.from(this.registeredDevices.entries()).map(([id, info]) => ({
      device_id: id,
      ...info
    }));
  }

  /**
   * Remove a device from registry
   */
  unregisterDevice(deviceId, reason = 'Manual removal') {
    const device = this.registeredDevices.get(deviceId);
    if (device) {
      this.registeredDevices.delete(deviceId);
      this.authTokens.delete(deviceId);
      console.log(`🗑️ Device unregistered: ${deviceId} (${reason})`);
      return true;
    }
    return false;
  }

  /**
   * Blacklist an IP address
   */
  blacklistIP(ip, reason = 'Manual blacklist') {
    this.blacklistedIPs.add(ip);
    console.warn(`🚫 IP blacklisted: ${ip} (${reason})`);
  }

  /**
   * Remove IP from blacklist
   */
  unblacklistIP(ip) {
    const removed = this.blacklistedIPs.delete(ip);
    if (removed) {
      console.log(`✅ IP removed from blacklist: ${ip}`);
    }
    return removed;
  }

  /**
   * Get system statistics
   */
  getStats() {
    const now = Date.now();
    const activeDevices = Array.from(this.registeredDevices.values())
      .filter(device => now - device.lastSeen.getTime() < 300000); // Active in last 5 minutes

    return {
      totalRegistered: this.registeredDevices.size,
      activeDevices: activeDevices.length,
      blacklistedIPs: this.blacklistedIPs.size,
      pendingTokens: this.authTokens.size,
      registrationAttempts: this.registrationAttempts.size,
      config: this.config
    };
  }

  /**
   * Clean up expired tokens and old registration attempts
   */
  cleanup() {
    const now = Date.now();
    let cleaned = 0;

    // Clean expired auth tokens
    for (const [deviceId, tokenInfo] of this.authTokens.entries()) {
      if (now > tokenInfo.expiresAt) {
        this.authTokens.delete(deviceId);
        cleaned++;
      }
    }

    // Clean old registration attempts
    for (const [ip, attempts] of this.registrationAttempts.entries()) {
      if (now - attempts.lastAttempt > this.config.registrationCooldown * 2) {
        this.registrationAttempts.delete(ip);
      }
    }

    if (cleaned > 0) {
      console.log(`🧹 Cleaned up ${cleaned} expired auth tokens`);
    }
  }

  /**
   * Update configuration
   */
  updateConfig(newConfig) {
    this.config = { ...this.config, ...newConfig };
    console.log('⚙️ Device auth configuration updated:', newConfig);
  }
}

// Export singleton instance
const deviceAuthManager = new DeviceAuthManager();

// Set up periodic cleanup (every hour)
setInterval(() => {
  deviceAuthManager.cleanup();
}, 3600000);

module.exports = deviceAuthManager;