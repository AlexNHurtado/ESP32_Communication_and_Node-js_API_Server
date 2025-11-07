// Configuration
const API_BASE_URL = window.location.origin; // Uses the current server's URL

// Activity log
const activityLog = [];

/**
 * Add entry to activity log
 */
function addLogEntry(message, type = 'info') {
  const timestamp = new Date().toLocaleTimeString();
  const entry = {
    timestamp,
    message,
    type
  };
  
  activityLog.unshift(entry);
  if (activityLog.length > 20) activityLog.pop();
  
  updateActivityLog();
}

/**
 * Update activity log display
 */
function updateActivityLog() {
  const logContainer = document.getElementById('activityLog');
  logContainer.innerHTML = activityLog.map(entry => `
    <div class="log-entry ${entry.type}">
      <span class="log-time">[${entry.timestamp}]</span>
      <span class="log-message">${entry.message}</span>
    </div>
  `).join('');
}

/**
 * Display API response
 */
function displayResponse(method, url, status, data, isError = false) {
  document.getElementById('responseMethod').textContent = method;
  document.getElementById('responseMethod').className = `method ${method.toLowerCase()}`;
  document.getElementById('responseUrl').textContent = url;
  
  const statusBadge = document.getElementById('responseStatus');
  statusBadge.textContent = status;
  statusBadge.className = `status-badge ${isError ? 'error' : 'success'}`;
  
  document.getElementById('responseBody').textContent = JSON.stringify(data, null, 2);
}

/**
 * Control LED
 */
async function controlLED(action) {
  const endpoint = `/api/led/${action}`;
  const url = `${API_BASE_URL}${endpoint}`;
  
  addLogEntry(`Sending ${action.toUpperCase()} command...`, 'info');
  
  try {
    const response = await fetch(url, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      }
    });
    
    const data = await response.json();
    displayResponse('POST', endpoint, response.status, data, !response.ok);
    
    if (response.ok) {
      addLogEntry(`LED ${action} successful`, 'success');
      // Update LED status - ESP32 returns "led", not "led_state"
      const ledState = data.led !== undefined ? data.led : data.led_state;
      if (ledState !== undefined) {
        updateLEDStatus(ledState);
      }
      // Refresh status after action
      setTimeout(getStatus, 500);
    } else {
      addLogEntry(`LED ${action} failed: ${data.error || 'Unknown error'}`, 'error');
    }
  } catch (error) {
    addLogEntry(`Error: ${error.message}`, 'error');
    displayResponse('POST', endpoint, 'ERROR', { error: error.message }, true);
  }
}

/**
 * Get device status
 */
async function getStatus() {
  const endpoint = '/api/status';
  const url = `${API_BASE_URL}${endpoint}`;
  
  addLogEntry('Fetching device status...', 'info');
  
  try {
    const response = await fetch(url);
    const data = await response.json();
    
    displayResponse('GET', endpoint, response.status, data, !response.ok);
    
    if (response.ok) {
      addLogEntry('Status retrieved successfully', 'success');
      updateStatusDisplay(data);
    } else {
      addLogEntry(`Failed to get status: ${data.error || 'Unknown error'}`, 'error');
    }
  } catch (error) {
    addLogEntry(`Error getting status: ${error.message}`, 'error');
    displayResponse('GET', endpoint, 'ERROR', { error: error.message }, true);
  }
}

/**
 * Test API endpoint
 */
async function testAPI(method, endpoint, body = null) {
  const url = `${API_BASE_URL}${endpoint}`;
  
  addLogEntry(`Testing ${method} ${endpoint}...`, 'info');
  
  try {
    const options = {
      method: method,
      headers: {
        'Content-Type': 'application/json'
      }
    };
    
    if (body && method !== 'GET') {
      options.body = JSON.stringify(body);
    }
    
    const response = await fetch(url, options);
    const data = await response.json();
    
    displayResponse(method, endpoint, response.status, data, !response.ok);
    
    if (response.ok) {
      addLogEntry(`${method} ${endpoint} - Success`, 'success');
    } else {
      addLogEntry(`${method} ${endpoint} - Failed`, 'error');
    }
  } catch (error) {
    addLogEntry(`Error: ${error.message}`, 'error');
    displayResponse(method, endpoint, 'ERROR', { error: error.message }, true);
  }
}

/**
 * Update status display
 */
function updateStatusDisplay(data) {
  // Check if we have esp32 data nested
  const esp32Data = data.esp32 || data;
  
  // LED Status - ESP32 returns "led", not "led_state"
  const ledState = esp32Data.led !== undefined ? esp32Data.led : esp32Data.led_state;
  if (ledState !== undefined) {
    updateLEDStatus(ledState);
  }
  
  // WiFi Signal - ESP32 returns "rssi", not "wifi_signal"
  const wifiSignal = esp32Data.rssi !== undefined ? esp32Data.rssi : esp32Data.wifi_signal;
  if (wifiSignal !== undefined) {
    const wifiSignalEl = document.getElementById('wifiSignal');
    wifiSignalEl.textContent = `${wifiSignal} dBm`;
    wifiSignalEl.className = 'status-value';
  }
  
  // ESP32 Status
  const esp32Status = document.getElementById('esp32Status');
  if (esp32Data.device || esp32Data.ip) {
    esp32Status.textContent = `Online (${esp32Data.ip || 'N/A'})`;
    esp32Status.className = 'status-value online';
  }
}

/**
 * Update LED status display
 */
function updateLEDStatus(state) {
  const ledStatus = document.getElementById('ledStatus');
  if (state) {
    ledStatus.textContent = 'ON';
    ledStatus.style.color = '#28a745';
  } else {
    ledStatus.textContent = 'OFF';
    ledStatus.style.color = '#dc3545';
  }
}

/**
 * Check backend health
 */
async function checkBackendHealth() {
  try {
    const response = await fetch(`${API_BASE_URL}/api/health`);
    const data = await response.json();
    
    const backendStatus = document.getElementById('backendStatus');
    if (response.ok && data.status === 'healthy') {
      backendStatus.textContent = 'Online';
      backendStatus.className = 'status-value online';
      
      // Update ESP32 status from health check
      const esp32Status = document.getElementById('esp32Status');
      if (data.esp32_reachable === true) {
        esp32Status.textContent = 'Online';
        esp32Status.className = 'status-value online';
      } else {
        esp32Status.textContent = 'Offline';
        esp32Status.className = 'status-value offline';
      }
    } else {
      backendStatus.textContent = 'Error';
      backendStatus.className = 'status-value offline';
      
      // Update ESP32 status as offline if backend has error
      const esp32Status = document.getElementById('esp32Status');
      esp32Status.textContent = 'Offline';
      esp32Status.className = 'status-value offline';
    }
  } catch (error) {
    const backendStatus = document.getElementById('backendStatus');
    backendStatus.textContent = 'Offline';
    backendStatus.className = 'status-value offline';
    
    const esp32Status = document.getElementById('esp32Status');
    esp32Status.textContent = 'Unknown';
    esp32Status.className = 'status-value offline';
    
    addLogEntry('Backend connection failed', 'error');
  }
}

/**
 * Save current status to MongoDB
 */
async function saveToMongoDB() {
  const url = `${API_BASE_URL}/api/status/save`;
  
  addLogEntry('Saving status to MongoDB...', 'info');
  
  try {
    const response = await fetch(url, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      }
    });
    
    const data = await response.json();
    
    displayResponse('POST', '/api/status/save', response.status, data, !response.ok);
    

    if (response.ok) {
      addLogEntry(`Status saved to MongoDB successfully (ID: ${data.mongodb?.document_id || 'unknown'})`, 'success');
    } else {
      if (response.status === 503 && data.error?.includes('MongoDB is not connected')) {
        addLogEntry('MongoDB is not connected. Check database configuration.', 'error');
      } else if (data.error?.includes('ESP32')) {
        addLogEntry('Cannot save - ESP32 device unreachable', 'error');
      } else {
        addLogEntry(`Failed to save to MongoDB: ${data.error || 'Unknown error'}`, 'error');
      }
    }
  } catch (error) {
    addLogEntry(`Error saving to MongoDB: ${error.message}`, 'error');
    displayResponse('POST', '/api/status/save', 'ERROR', { error: error.message }, true);
  }
}

/**
 * Tab switching
 */
function showTab(tabName) {
  // Hide all tabs
  const tabs = document.querySelectorAll('.tab-content');
  tabs.forEach(tab => tab.classList.remove('active'));
  
  // Remove active class from all buttons
  const buttons = document.querySelectorAll('.tab-btn');
  buttons.forEach(btn => btn.classList.remove('active'));
  
  // Show selected tab
  document.getElementById(tabName).classList.add('active');
  
  // Add active class to clicked button
  event.target.classList.add('active');
}

/**
 * Initialize page
 */
async function init() {
  addLogEntry('Application started', 'info');
  
  // Check backend health
  await checkBackendHealth();
  
  // Get initial status
  await getStatus();
  
  // Set up periodic status updates (every 10 seconds)
  setInterval(async () => {
    await checkBackendHealth();
  }, 10000);
}

// Initialize when page loads
document.addEventListener('DOMContentLoaded', init);