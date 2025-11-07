let ws = null;
let reconnectTimer = null;
let isConnected = false;

/**
 * Connect to backend WebSocket
 */
function connectWebSocket() {
  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
  const wsUrl = `${protocol}//${window.location.host}`;
  
  addLog(`Connecting to ${wsUrl}...`, 'info');
  
  ws = new WebSocket(wsUrl);
  
  ws.onopen = () => {
    isConnected = true;
    updateConnectionStatus(true, false);
    addLog('Connected to backend', 'success');
    enableButtons(true);
    
    if (reconnectTimer) {
      clearInterval(reconnectTimer);
      reconnectTimer = null;
    }
  };
  
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      handleMessage(data);
    } catch (err) {
      console.error('Parse error:', err);
      addLog(`Parse error: ${err.message}`, 'error');
    }
  };
  
  ws.onerror = (error) => {
    console.error('WebSocket error:', error);
    addLog('Connection error', 'error');
  };
  
  ws.onclose = () => {
    isConnected = false;
    updateConnectionStatus(false, false);
    addLog('Disconnected from backend', 'error');
    enableButtons(false);
    
    // Auto-reconnect
    if (!reconnectTimer) {
      addLog('Reconnecting in 3 seconds...', 'info');
      reconnectTimer = setInterval(() => {
        connectWebSocket();
      }, 3000);
    }
  };
}

/**
 * Handle incoming WebSocket messages
 */
function handleMessage(data) {
  console.log('Received:', data);
  
  switch(data.type) {
    case 'status':
      updateStatus(data);
      addLog('Status received', 'info');
      break;
      
    case 'response':
      if (data.success) {
        addLog(`✓ ${data.message}`, 'success');
        updateLedState(data.led);
      } else {
        addLog(`✗ ${data.message}`, 'error');
      }
      break;
      
    case 'connection':
      updateConnectionStatus(true, data.esp32Connected);
      addLog(data.message, data.esp32Connected ? 'success' : 'error');
      break;
      
    case 'error':
      addLog(`Error: ${data.message}`, 'error');
      break;
      
    default:
      console.log('Unknown message type:', data.type);
  }
}

/**
 * Send command to ESP32 via WebSocket
 */
function sendCommand(command) {
  if (!ws || ws.readyState !== WebSocket.OPEN) {
    addLog('Not connected to backend', 'error');
    return;
  }
  
  const message = { command: command };
  addLog(`→ Sending: ${command}`, 'info');
  ws.send(JSON.stringify(message));
}

/**
 * Update connection status display
 */
function updateConnectionStatus(backendConnected, esp32Connected) {
  const backendCard = document.getElementById('backendStatus');
  const backendText = document.getElementById('backendStatusText');
  const esp32Card = document.getElementById('esp32Status');
  const esp32Text = document.getElementById('esp32StatusText');
  
  // Backend status
  if (backendConnected) {
    backendCard.classList.add('connected');
    backendCard.classList.remove('disconnected');
    backendText.textContent = 'Connected';
  } else {
    backendCard.classList.remove('connected');
    backendCard.classList.add('disconnected');
    backendText.textContent = 'Disconnected';
  }
  
  // ESP32 status
  if (esp32Connected) {
    esp32Card.classList.add('connected');
    esp32Card.classList.remove('disconnected');
    esp32Text.textContent = 'Connected';
  } else {
    esp32Card.classList.remove('connected');
    esp32Card.classList.add('disconnected');
    esp32Text.textContent = 'Disconnected';
  }
}

/**
 * Update device status display
 */
function updateStatus(data) {
  updateLedState(data.led);
  
  document.getElementById('ipAddress').textContent = data.ip || '-';
  document.getElementById('wifiSsid').textContent = data.ssid || '-';
  
  if (data.rssi) {
    document.getElementById('wifiSignal').textContent = `${data.rssi} dBm`;
  }
  
  if (data.uptime) {
    const hours = Math.floor(data.uptime / 3600);
    const minutes = Math.floor((data.uptime % 3600) / 60);
    const seconds = data.uptime % 60;
    document.getElementById('uptime').textContent = 
      `${hours}h ${minutes}m ${seconds}s`;
  }
  
  if (data.heap) {
    document.getElementById('freeHeap').textContent = 
      `${(data.heap / 1024).toFixed(1)} KB`;
  }
}

/**
 * Update LED state display
 */
function updateLedState(state) {
  const ledStateEl = document.getElementById('ledState');
  if (state) {
    ledStateEl.textContent = 'ON';
    ledStateEl.classList.add('on');
    ledStateEl.classList.remove('off');
  } else {
    ledStateEl.textContent = 'OFF';
    ledStateEl.classList.add('off');
    ledStateEl.classList.remove('on');
  }
}

/**
 * Add entry to activity log
 */
function addLog(message, type = 'info') {
  const logContainer = document.getElementById('logContainer');
  const timestamp = new Date().toLocaleTimeString();
  const entry = document.createElement('div');
  entry.className = `log-entry ${type}`;
  entry.textContent = `[${timestamp}] ${message}`;
  
  // Add to top of log
  if (logContainer.firstChild) {
    logContainer.insertBefore(entry, logContainer.firstChild);
  } else {
    logContainer.appendChild(entry);
  }
  
  // Limit log entries to 50
  while (logContainer.children.length > 50) {
    logContainer.removeChild(logContainer.lastChild);
  }
}

/**
 * Clear activity log
 */
function clearLog() {
  const logContainer = document.getElementById('logContainer');
  logContainer.innerHTML = '<div class="log-entry">Log cleared</div>';
}

/**
 * Enable/disable control buttons
 */
function enableButtons(enabled) {
  document.getElementById('btnOn').disabled = !enabled;
  document.getElementById('btnOff').disabled = !enabled;
  document.getElementById('btnToggle').disabled = !enabled;
  document.getElementById('btnRefresh').disabled = !enabled;
}

/**
 * Switch between code tabs
 */
function showTab(tabName) {
  // Hide all tabs
  const contents = document.querySelectorAll('.tab-content');
  contents.forEach(content => content.classList.remove('active'));
  
  const buttons = document.querySelectorAll('.tab-btn');
  buttons.forEach(btn => btn.classList.remove('active'));
  
  // Show selected tab
  document.getElementById(`tab-${tabName}`).classList.add('active');
  event.target.classList.add('active');
}

/**
 * Initialize application
 */
function init() {
  addLog('Application started', 'info');
  enableButtons(false);
  connectWebSocket();
}

// Start when page loads
window.addEventListener('load', init);

// Reconnect when page becomes visible
document.addEventListener('visibilitychange', () => {
  if (!document.hidden && (!ws || ws.readyState !== WebSocket.OPEN)) {
    addLog('Page visible - reconnecting...', 'info');
    connectWebSocket();
  }
});
