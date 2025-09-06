#pragma once

#include "../BaseSerialInterface.h"
#include <bluefruit.h>

class SerialBLEInterface : public BaseSerialInterface {
  BLEUart bleuart;
  bool _isEnabled;
  bool _isDeviceConnected;
  unsigned long _last_write;
  
  // Enhanced stability and recovery for nRF52
  unsigned long last_connection_time;
  unsigned long last_disconnect_time;
  uint8_t connection_failures;
  uint8_t consecutive_failures;
  bool connection_recovery_active;
  unsigned long recovery_start_time;
  unsigned long last_heartbeat_time;
  uint32_t total_connections;
  uint32_t total_disconnections;
  unsigned long adv_restart_time;

  struct Frame {
    uint8_t len;
    uint8_t buf[MAX_FRAME_SIZE];
  };

  #define FRAME_QUEUE_SIZE  4
  int send_queue_len;
  Frame send_queue[FRAME_QUEUE_SIZE];

  void clearBuffers() { send_queue_len = 0; }
  static void onConnect(uint16_t connection_handle);
  static void onDisconnect(uint16_t connection_handle, uint8_t reason);

public:
  SerialBLEInterface() {
    _isEnabled = false;
    _isDeviceConnected = false;
    _last_write = 0;
    send_queue_len = 0;
    
    // Initialize stability tracking
    last_connection_time = 0;
    last_disconnect_time = 0;
    connection_failures = 0;
    consecutive_failures = 0;
    connection_recovery_active = false;
    recovery_start_time = 0;
    last_heartbeat_time = 0;
    total_connections = 0;
    total_disconnections = 0;
    adv_restart_time = 0;
  }

  void startAdv();
  void stopAdv();
  void begin(const char* device_name, uint32_t pin_code);

  // BaseSerialInterface methods
  void enable() override;
  void disable() override;
  bool isEnabled() const override { return _isEnabled; }

  bool isConnected() const override;

  bool isWriteBusy() const override;
  size_t writeFrame(const uint8_t src[], size_t len) override;
  size_t checkRecvFrame(uint8_t dest[]) override;
};

#if BLE_DEBUG_LOGGING && ARDUINO
  #include <Arduino.h>
  #define BLE_DEBUG_PRINT(F, ...) Serial.printf("BLE: " F, ##__VA_ARGS__)
  #define BLE_DEBUG_PRINTLN(F, ...) Serial.printf("BLE: " F "\n", ##__VA_ARGS__)
#else
  #define BLE_DEBUG_PRINT(...) {}
  #define BLE_DEBUG_PRINTLN(...) {}
#endif
