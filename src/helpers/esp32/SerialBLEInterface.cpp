#include "SerialBLEInterface.h"

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define ADVERT_RESTART_DELAY  1000   // millis
#define MAX_CONNECTION_FAILURES  5   // Max consecutive failures before extended recovery
#define RECOVERY_DELAY_BASE     5000   // Base recovery delay in milliseconds  
#define RECOVERY_DELAY_MAX     30000   // Maximum recovery delay
#define HEARTBEAT_INTERVAL     10000   // Connection health check interval
#define CONNECTION_TIMEOUT     45000   // Consider connection dead after this time

void SerialBLEInterface::begin(const char* device_name, uint32_t pin_code) {
  _pin_code = pin_code;

  // Create the BLE Device
  BLEDevice::init(device_name);
  BLEDevice::setSecurityCallbacks(this);
  BLEDevice::setMTU(MAX_FRAME_SIZE);

  BLESecurity  sec;
  sec.setStaticPIN(pin_code);
  sec.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);

  //BLEDevice::setPower(ESP_PWR_LVL_N8);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(this);

  // Create the BLE Service
  pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM);
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setAccessPermissions(ESP_GATT_PERM_WRITE_ENC_MITM);
  pRxCharacteristic->setCallbacks(this);

  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
}

// -------- BLESecurityCallbacks methods

uint32_t SerialBLEInterface::onPassKeyRequest() {
  BLE_DEBUG_PRINTLN("onPassKeyRequest()");
  return _pin_code;
}

void SerialBLEInterface::onPassKeyNotify(uint32_t pass_key) {
  BLE_DEBUG_PRINTLN("onPassKeyNotify(%u)", pass_key);
}

bool SerialBLEInterface::onConfirmPIN(uint32_t pass_key) {
  BLE_DEBUG_PRINTLN("onConfirmPIN(%u)", pass_key);
  return true;
}

bool SerialBLEInterface::onSecurityRequest() {
  BLE_DEBUG_PRINTLN("onSecurityRequest()");
  return true;  // allow
}

void SerialBLEInterface::onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) {
  if (cmpl.success) {
    BLE_DEBUG_PRINTLN(" - SecurityCallback - Authentication Success");
    //deviceConnected = true;
  } else {
    BLE_DEBUG_PRINTLN(" - SecurityCallback - Authentication Failure*");

    //pServer->removePeerDevice(pServer->getConnId(), true);
    pServer->disconnect(pServer->getConnId());
    adv_restart_time = millis() + ADVERT_RESTART_DELAY;
  }
}

// -------- BLEServerCallbacks methods

void SerialBLEInterface::onConnect(BLEServer* pServer) {
}

void SerialBLEInterface::onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
  BLE_DEBUG_PRINTLN("onConnect(), conn_id=%d, mtu=%d", param->connect.conn_id, pServer->getPeerMTU(param->connect.conn_id));
  
  // Reset recovery state on successful connection
  consecutive_failures = 0;
  connection_recovery_active = false;
  last_connection_time = millis();
  last_heartbeat_time = millis();
  total_connections++;
  
  BLE_DEBUG_PRINTLN("BLE Connection established. Total connections: %u", total_connections);
}

void SerialBLEInterface::onMtuChanged(BLEServer* pServer, esp_ble_gatts_cb_param_t* param) {
  BLE_DEBUG_PRINTLN("onMtuChanged(), mtu=%d", pServer->getPeerMTU(param->mtu.conn_id));

  deviceConnected = true;
  last_heartbeat_time = millis();
}

void SerialBLEInterface::onDisconnect(BLEServer* pServer) {
  BLE_DEBUG_PRINTLN("onDisconnect()");
  
  last_disconnect_time = millis();
  total_disconnections++;
  
  // Calculate connection duration for stability metrics
  if (last_connection_time > 0) {
    unsigned long connection_duration = last_disconnect_time - last_connection_time;
    BLE_DEBUG_PRINTLN("Connection lasted %lu ms. Total disconnections: %u", connection_duration, total_disconnections);
    
    // If connection was very short, consider it a failure
    if (connection_duration < 5000) {  // Less than 5 seconds
      consecutive_failures++;
      connection_failures++;
      BLE_DEBUG_PRINTLN("Short connection detected. Consecutive failures: %u", consecutive_failures);
    }
  }
  
  if (_isEnabled) {
    // Use exponential backoff for restart delay if we have consecutive failures
    unsigned long restart_delay = ADVERT_RESTART_DELAY;
    if (consecutive_failures >= MAX_CONNECTION_FAILURES) {
      restart_delay = min(RECOVERY_DELAY_BASE * (1 << consecutive_failures), RECOVERY_DELAY_MAX);
      connection_recovery_active = true;
      recovery_start_time = millis();
      BLE_DEBUG_PRINTLN("Entering recovery mode with %lu ms delay", restart_delay);
    }
    
    adv_restart_time = millis() + restart_delay;
  }
}

// -------- BLECharacteristicCallbacks methods

void SerialBLEInterface::onWrite(BLECharacteristic* pCharacteristic, esp_ble_gatts_cb_param_t* param) {
  uint8_t* rxValue = pCharacteristic->getData();
  int len = pCharacteristic->getLength();

  if (len > MAX_FRAME_SIZE) {
    BLE_DEBUG_PRINTLN("ERROR: onWrite(), frame too big, len=%d", len);
  } else if (recv_queue_len >= FRAME_QUEUE_SIZE) {
    BLE_DEBUG_PRINTLN("ERROR: onWrite(), recv_queue is full!");
  } else {
    recv_queue[recv_queue_len].len = len;
    memcpy(recv_queue[recv_queue_len].buf, rxValue, len);
    recv_queue_len++;
  }
}

// ---------- public methods

void SerialBLEInterface::enable() { 
  if (_isEnabled) return;

  _isEnabled = true;
  clearBuffers();

  // Start the service
  pService->start();

  // Start advertising

  //pServer->getAdvertising()->setMinInterval(500);
  //pServer->getAdvertising()->setMaxInterval(1000);

  pServer->getAdvertising()->start();
  adv_restart_time = 0;
}

void SerialBLEInterface::disable() {
  _isEnabled = false;

  BLE_DEBUG_PRINTLN("SerialBLEInterface::disable");

  pServer->getAdvertising()->stop();
  pService->stop();
  oldDeviceConnected = deviceConnected = false;
  adv_restart_time = 0;
}

size_t SerialBLEInterface::writeFrame(const uint8_t src[], size_t len) {
  if (len > MAX_FRAME_SIZE) {
    BLE_DEBUG_PRINTLN("writeFrame(), frame too big, len=%d", len);
    return 0;
  }

  if (deviceConnected && len > 0) {
    if (send_queue_len >= FRAME_QUEUE_SIZE) {
      BLE_DEBUG_PRINTLN("writeFrame(), send_queue is full!");
      return 0;
    }

    send_queue[send_queue_len].len = len;  // add to send queue
    memcpy(send_queue[send_queue_len].buf, src, len);
    send_queue_len++;

    return len;
  }
  return 0;
}

#define  BLE_WRITE_MIN_INTERVAL   60

bool SerialBLEInterface::isWriteBusy() const {
  return millis() < _last_write + BLE_WRITE_MIN_INTERVAL;   // still too soon to start another write?
}

size_t SerialBLEInterface::checkRecvFrame(uint8_t dest[]) {
  unsigned long now = millis();
  
  // Enhanced connection health monitoring
  if (deviceConnected) {
    // Update heartbeat when we successfully process data
    if (send_queue_len > 0 || recv_queue_len > 0) {
      last_heartbeat_time = now;
    }
    
    // Check for connection timeout (stale connection)
    if (now - last_heartbeat_time > CONNECTION_TIMEOUT) {
      BLE_DEBUG_PRINTLN("Connection timeout detected, forcing disconnect");
      pServer->disconnect(pServer->getConnId());
      deviceConnected = false;
      consecutive_failures++;
      return 0;
    }
  }
  
  if (send_queue_len > 0   // first, check send queue
    && now >= _last_write + BLE_WRITE_MIN_INTERVAL    // space the writes apart
  ) {
    _last_write = now;
    
    // Enhanced error handling for notifications
    try {
      pTxCharacteristic->setValue(send_queue[0].buf, send_queue[0].len);
      pTxCharacteristic->notify();
      last_heartbeat_time = now;  // Update heartbeat on successful write
      
      BLE_DEBUG_PRINTLN("writeBytes: sz=%d, hdr=%d", (uint32_t)send_queue[0].len, (uint32_t) send_queue[0].buf[0]);
    } catch (...) {
      BLE_DEBUG_PRINTLN("BLE notify failed, connection may be stale");
      // Don't remove from queue if notify failed - we'll retry
      return 0;
    }

    send_queue_len--;
    for (int i = 0; i < send_queue_len; i++) {   // delete top item from queue
      send_queue[i] = send_queue[i + 1];
    }
  }

  if (recv_queue_len > 0) {   // check recv queue
    size_t len = recv_queue[0].len;   // take from top of queue
    memcpy(dest, recv_queue[0].buf, len);
    last_heartbeat_time = now;  // Update heartbeat on successful read

    BLE_DEBUG_PRINTLN("readBytes: sz=%d, hdr=%d", len, (uint32_t) dest[0]);

    recv_queue_len--;
    for (int i = 0; i < recv_queue_len; i++) {   // delete top item from queue
      recv_queue[i] = recv_queue[i + 1];
    }
    return len;
  }

  // Enhanced connection state management
  bool current_connected = (pServer->getConnectedCount() > 0);
  if (current_connected != deviceConnected) {
    deviceConnected = current_connected;
  }

  if (deviceConnected != oldDeviceConnected) {
    if (!deviceConnected) {    // disconnecting
      clearBuffers();

      BLE_DEBUG_PRINTLN("SerialBLEInterface -> disconnecting...");

      // Enhanced restart logic with exponential backoff
      unsigned long restart_delay = ADVERT_RESTART_DELAY;
      if (consecutive_failures >= MAX_CONNECTION_FAILURES) {
        restart_delay = min(RECOVERY_DELAY_BASE * (1 << (consecutive_failures - MAX_CONNECTION_FAILURES)), 
                           (unsigned long)RECOVERY_DELAY_MAX);
        connection_recovery_active = true;
        recovery_start_time = now;
        BLE_DEBUG_PRINTLN("Enhanced recovery mode: %lu ms delay, failures: %u", restart_delay, consecutive_failures);
      }
      
      adv_restart_time = now + restart_delay;
    } else {
      BLE_DEBUG_PRINTLN("SerialBLEInterface -> stopping advertising");
      BLE_DEBUG_PRINTLN("SerialBLEInterface -> connecting...");
      // connecting - reset failure state
      consecutive_failures = 0;
      connection_recovery_active = false;
      pServer->getAdvertising()->stop();
      adv_restart_time = 0;
    }
    oldDeviceConnected = deviceConnected;
  }

  // Enhanced advertising restart with recovery mode support
  if (adv_restart_time && now >= adv_restart_time) {
    if (pServer->getConnectedCount() == 0) {
      if (connection_recovery_active) {
        BLE_DEBUG_PRINTLN("SerialBLEInterface -> recovery restart after %lu ms", now - recovery_start_time);
      } else {
        BLE_DEBUG_PRINTLN("SerialBLEInterface -> normal restart");
      }
      
      // Restart BLE stack if we've had too many failures
      if (consecutive_failures >= MAX_CONNECTION_FAILURES * 2) {
        BLE_DEBUG_PRINTLN("Too many failures, reinitializing BLE stack");
        pServer->getAdvertising()->stop();
        delay(1000);  // Give BLE stack time to reset
        consecutive_failures = MAX_CONNECTION_FAILURES;  // Reset but keep some failures recorded
      }
      
      pServer->getAdvertising()->start();  // re-Start advertising
    }
    adv_restart_time = 0;
  }
  
  return 0;
}

bool SerialBLEInterface::isConnected() const {
  return deviceConnected;  //pServer != NULL && pServer->getConnectedCount() > 0;
}
