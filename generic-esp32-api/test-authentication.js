#!/usr/bin/env node

/**
 * Comprehensive Authentication System Test Script
 * Tests all authentication features including whitelist, duplicate prevention, and rate limiting
 */

const axios = require('axios');
const colors = require('colors');

// Configuration
const BASE_URL = 'http://localhost:3001';
const TEST_TIMEOUT = 5000;

// Test results tracking
let testResults = {
    passed: 0,
    failed: 0,
    total: 0
};

// Helper functions
function logTest(testName, passed, message = '') {
    testResults.total++;
    if (passed) {
        testResults.passed++;
        console.log(`✅ ${testName}`.green);
    } else {
        testResults.failed++;
        console.log(`❌ ${testName} - ${message}`.red);
    }
}

function logSection(title) {
    console.log(`\n${'='.repeat(50)}`.cyan);
    console.log(`${title}`.cyan.bold);
    console.log(`${'='.repeat(50)}`.cyan);
}

async function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

// Test functions
async function testServerHealth() {
    logSection('🏥 SERVER HEALTH TESTS');
    
    try {
        // Test basic connectivity
        const response = await axios.get(`${BASE_URL}/`, { timeout: TEST_TIMEOUT });
        logTest('Server is responding', response.status === 200);
        
        // Test auth status endpoint
        const authStatus = await axios.get(`${BASE_URL}/auth/status`, { timeout: TEST_TIMEOUT });
        logTest('Authentication system is active', 
               authStatus.status === 200 && authStatus.data.authentication_enabled);
        
        console.log('Auth System Config:'.yellow, JSON.stringify(authStatus.data.statistics.config, null, 2));
        
    } catch (error) {
        logTest('Server connectivity', false, error.message);
        throw new Error('Server is not accessible. Please start the server first.');
    }
}

async function testDeviceRegistration() {
    logSection('📝 DEVICE REGISTRATION TESTS');
    
    // Test 1: Successful registration
    try {
        const response = await axios.post(`${BASE_URL}/register`, {
            device_id: 'test_device_001',
            ip: '192.168.1.100',
            port: 80,
            device_info: {
                type: 'test_sensor',
                location: 'test_lab',
                firmware: '1.0.0'
            }
        }, { timeout: TEST_TIMEOUT });
        
        logTest('New device registration', 
               response.status === 201 && response.data.success === true);
        
        console.log('Registration response:'.yellow, JSON.stringify(response.data, null, 2));
        
    } catch (error) {
        logTest('New device registration', false, error.response?.data?.error || error.message);
    }
    
    // Test 2: Duplicate registration from same IP (should succeed)
    try {
        const response = await axios.post(`${BASE_URL}/register`, {
            device_id: 'test_device_001',
            ip: '192.168.1.100',
            port: 80,
            device_info: {
                type: 'test_sensor',
                location: 'test_lab',
                firmware: '1.0.1'
            }
        }, { timeout: TEST_TIMEOUT });
        
        logTest('Re-registration from same IP', 
               response.status === 200 && response.data.success === true);
        
    } catch (error) {
        logTest('Re-registration from same IP', false, error.response?.data?.error || error.message);
    }
    
    // Test 3: Duplicate registration from different IP (should fail)
    try {
        const response = await axios.post(`${BASE_URL}/register`, {
            device_id: 'test_device_001',
            ip: '192.168.1.200',  // Different IP
            port: 80,
            device_info: {
                type: 'malicious_device',
                location: 'unknown',
                firmware: '0.1.0'
            }
        }, { timeout: TEST_TIMEOUT });
        
        logTest('Duplicate registration from different IP', 
               response.status === 409, 'Should have been blocked');
        
    } catch (error) {
        if (error.response?.status === 409) {
            logTest('Duplicate registration from different IP', true);
            console.log('Correctly blocked:'.green, error.response.data.error);
        } else {
            logTest('Duplicate registration from different IP', false, 
                   'Expected 409 but got: ' + (error.response?.status || error.message));
        }
    }
    
    // Test 4: Register second unique device
    try {
        const response = await axios.post(`${BASE_URL}/register`, {
            device_id: 'test_device_002',
            ip: '192.168.1.101',
            port: 80,
            device_info: {
                type: 'test_actuator',
                location: 'test_lab',
                firmware: '1.0.0'
            }
        }, { timeout: TEST_TIMEOUT });
        
        logTest('Second unique device registration', 
               response.status === 201 && response.data.success === true);
        
    } catch (error) {
        logTest('Second unique device registration', false, error.response?.data?.error || error.message);
    }
}

async function testDataSubmission() {
    logSection('📊 DATA SUBMISSION TESTS');
    
    // Test 1: Data submission with registered device
    try {
        const response = await axios.post(`${BASE_URL}/data`, {
            temperature: 25.5,
            humidity: 60.2,
            test: 'authenticated_submission'
        }, { 
            headers: {
                'X-ESP32-ID': 'test_device_001',
                'Content-Type': 'application/json'
            },
            timeout: TEST_TIMEOUT 
        });
        
        logTest('Data submission from registered device', 
               response.status === 200 && response.data.success === true);
        
        console.log('Submission response:'.yellow, JSON.stringify(response.data, null, 2));
        
    } catch (error) {
        logTest('Data submission from registered device', false, 
               error.response?.data?.error || error.message);
    }
    
    // Test 2: Data submission without header (should fail)
    try {
        const response = await axios.post(`${BASE_URL}/data`, {
            temperature: 23.1,
            test: 'no_header'
        }, { 
            headers: {
                'Content-Type': 'application/json'
            },
            timeout: TEST_TIMEOUT 
        });
        
        logTest('Data submission without X-ESP32-ID header', 
               response.status === 400, 'Should have been rejected');
        
    } catch (error) {
        if (error.response?.status === 400) {
            logTest('Data submission without X-ESP32-ID header', true);
            console.log('Correctly rejected:'.green, error.response.data.error);
        } else {
            logTest('Data submission without X-ESP32-ID header', false,
                   'Expected 400 but got: ' + (error.response?.status || error.message));
        }
    }
    
    // Test 3: Data submission from unregistered device (should fail)
    try {
        const response = await axios.post(`${BASE_URL}/data`, {
            temperature: 99.9,
            test: 'unauthorized'
        }, { 
            headers: {
                'X-ESP32-ID': 'unregistered_device',
                'Content-Type': 'application/json'
            },
            timeout: TEST_TIMEOUT 
        });
        
        logTest('Data submission from unregistered device', 
               response.status === 403, 'Should have been blocked');
        
    } catch (error) {
        if (error.response?.status === 403) {
            logTest('Data submission from unregistered device', true);
            console.log('Correctly blocked:'.green, error.response.data.error);
        } else {
            logTest('Data submission from unregistered device', false,
                   'Expected 403 but got: ' + (error.response?.status || error.message));
        }
    }
    
    // Test 4: Multiple successful submissions to test activity tracking
    for (let i = 0; i < 3; i++) {
        try {
            const response = await axios.post(`${BASE_URL}/data`, {
                temperature: 25 + i,
                humidity: 60 + i,
                test: `activity_tracking_${i}`
            }, { 
                headers: {
                    'X-ESP32-ID': 'test_device_002',
                    'Content-Type': 'application/json'
                },
                timeout: TEST_TIMEOUT 
            });
            
            if (i === 2) {  // Only log the last one
                logTest('Multiple submissions for activity tracking', 
                       response.status === 200 && response.data.submission_count > 1);
                console.log('Final submission count:'.yellow, response.data.submission_count);
            }
            
        } catch (error) {
            if (i === 2) {
                logTest('Multiple submissions for activity tracking', false, 
                       error.response?.data?.error || error.message);
            }
        }
        await sleep(100);  // Small delay between submissions
    }
}

async function testAuthenticationManagement() {
    logSection('⚙️ AUTHENTICATION MANAGEMENT TESTS');
    
    // Test 1: Get authentication status
    try {
        const response = await axios.get(`${BASE_URL}/auth/status`, { timeout: TEST_TIMEOUT });
        logTest('Authentication status retrieval', 
               response.status === 200 && response.data.success === true);
        
        console.log('System Statistics:'.yellow);
        console.log(`- Total Registered: ${response.data.statistics.totalRegistered}`);
        console.log(`- Active Devices: ${response.data.statistics.activeDevices}`);
        console.log(`- Blacklisted IPs: ${response.data.statistics.blacklistedIPs}`);
        
    } catch (error) {
        logTest('Authentication status retrieval', false, 
               error.response?.data?.error || error.message);
    }
    
    // Test 2: Get device list
    try {
        const response = await axios.get(`${BASE_URL}/devices`, { timeout: TEST_TIMEOUT });
        logTest('Device list retrieval', 
               response.status === 200 && Array.isArray(response.data));
        
        console.log('Registered Devices:'.yellow, response.data.length);
        response.data.forEach((device, idx) => {
            console.log(`  ${idx + 1}. ${device.device_id} (${device.ip}:${device.port}) - ${device.status}`);
        });
        
    } catch (error) {
        logTest('Device list retrieval', false, 
               error.response?.data?.error || error.message);
    }
    
    // Test 3: Update authentication configuration
    try {
        const response = await axios.put(`${BASE_URL}/auth/config`, {
            maxRegistrationAttempts: 3,
            enableWhitelist: true
        }, { timeout: TEST_TIMEOUT });
        
        logTest('Authentication configuration update', 
               response.status === 200 && response.data.success === true);
        
    } catch (error) {
        logTest('Authentication configuration update', false, 
               error.response?.data?.error || error.message);
    }
    
    // Test 4: Manual IP blacklisting
    try {
        const response = await axios.post(`${BASE_URL}/auth/blacklist`, {
            ip: '192.168.1.250',
            reason: 'Test blacklist'
        }, { timeout: TEST_TIMEOUT });
        
        logTest('Manual IP blacklisting', 
               response.status === 200 && response.data.success === true);
        
    } catch (error) {
        logTest('Manual IP blacklisting', false, 
               error.response?.data?.error || error.message);
    }
    
    // Test 5: Device unregistration
    try {
        const response = await axios.delete(`${BASE_URL}/devices/test_device_002`, {
            data: { reason: 'Test cleanup' },
            timeout: TEST_TIMEOUT
        });
        
        logTest('Device unregistration', 
               response.status === 200 && response.data.success === true);
        
    } catch (error) {
        logTest('Device unregistration', false, 
               error.response?.data?.error || error.message);
    }
    
    // Test 6: Verify unregistered device cannot submit data
    try {
        const response = await axios.post(`${BASE_URL}/data`, {
            temperature: 50.0,
            test: 'after_unregistration'
        }, { 
            headers: {
                'X-ESP32-ID': 'test_device_002',
                'Content-Type': 'application/json'
            },
            timeout: TEST_TIMEOUT 
        });
        
        logTest('Data submission after unregistration', 
               response.status === 403, 'Should be blocked after unregistration');
        
    } catch (error) {
        if (error.response?.status === 403) {
            logTest('Data submission after unregistration', true);
            console.log('Correctly blocked after unregistration:'.green, error.response.data.error);
        } else {
            logTest('Data submission after unregistration', false,
                   'Expected 403 but got: ' + (error.response?.status || error.message));
        }
    }
}

async function testRateLimiting() {
    logSection('🚦 RATE LIMITING TESTS');
    
    let rateLimitHit = false;
    
    // Attempt multiple registrations with same device ID from different IPs
    for (let i = 0; i < 6; i++) {  // Exceed the default limit of 5
        try {
            const response = await axios.post(`${BASE_URL}/register`, {
                device_id: 'rate_limit_test',
                ip: `192.168.1.${200 + i}`,  // Different IPs
                port: 80,
                device_info: {
                    type: 'rate_test',
                    location: 'test',
                    firmware: '1.0.0'
                }
            }, { timeout: TEST_TIMEOUT });
            
        } catch (error) {
            if (error.response?.status === 409 && i === 0) {
                // First attempt should fail due to duplicate prevention
                continue;
            } else if (error.response?.status === 429) {
                rateLimitHit = true;
                console.log(`Rate limit hit after ${i} attempts`.yellow);
                break;
            }
        }
        await sleep(100);  // Small delay between attempts
    }
    
    logTest('Rate limiting activation', rateLimitHit, 'Rate limit should activate after multiple failed attempts');
}

// Main test execution
async function runAllTests() {
    console.log('🧪 Generic ESP32 API Authentication System Tests'.blue.bold);
    console.log('='.repeat(60).blue);
    
    try {
        await testServerHealth();
        await testDeviceRegistration();
        await testDataSubmission();
        await testAuthenticationManagement();
        await testRateLimiting();
        
        // Final summary
        logSection('📊 TEST SUMMARY');
        console.log(`Total Tests: ${testResults.total}`.white);
        console.log(`Passed: ${testResults.passed}`.green);
        console.log(`Failed: ${testResults.failed}`.red);
        console.log(`Success Rate: ${((testResults.passed / testResults.total) * 100).toFixed(1)}%`.cyan);
        
        if (testResults.failed === 0) {
            console.log('\n🎉 ALL TESTS PASSED! Authentication system is working correctly.'.green.bold);
        } else {
            console.log(`\n⚠️  ${testResults.failed} test(s) failed. Please check the issues above.`.yellow.bold);
        }
        
    } catch (error) {
        console.error('❌ Test suite failed:'.red.bold, error.message);
        process.exit(1);
    }
}

// Check if server is accessible before running tests
async function checkServerAndRun() {
    console.log('Checking server availability...'.yellow);
    
    try {
        await axios.get(`${BASE_URL}/`, { timeout: 3000 });
        console.log('✅ Server is running, starting tests...\n'.green);
        await runAllTests();
    } catch (error) {
        console.error('❌ Server is not accessible at'.red, BASE_URL);
        console.error('Please start the server first with: npm start'.red);
        console.error('Or check if the server is running on a different port.'.red);
        process.exit(1);
    }
}

// Run the tests
if (require.main === module) {
    checkServerAndRun();
}

module.exports = {
    runAllTests,
    testResults
};