#if defined(LILYGO_TPAGER)

#include "TPagerBoard.h"

TPagerBoard board;

// XL9555 GPIO expander I2C functions
bool writeExpanderPin(uint8_t pin, bool state) {
  Wire.beginTransmission(I2C_XL9555_ADD);
  Wire.write(0x02 + (pin >= 8 ? 1 : 0)); // Output port 0 or 1
  Wire.write(state ? (1 << (pin % 8)) : 0);
  return Wire.endTransmission() == 0;
}

bool configExpanderPin(uint8_t pin, bool output) {
  Wire.beginTransmission(I2C_XL9555_ADD);
  Wire.write(0x06 + (pin >= 8 ? 1 : 0)); // Config port 0 or 1
  Wire.write(output ? 0 : (1 << (pin % 8)));
  return Wire.endTransmission() == 0;
}

bool TPagerBoard::power_init() {
  // Initialize I2C for power management
  Wire.begin(PIN_BOARD_SDA, PIN_BOARD_SCL);
  Wire.setClock(100000); // Reduced from 400kHz to 100kHz for TCA8418 stability

  // Initialize GPIO expander
  return expander_init();
}

bool TPagerBoard::expander_init() {
  // Check if XL9555 GPIO expander is present
  Wire.beginTransmission(I2C_XL9555_ADD);
  if (Wire.endTransmission() != 0) {
    #ifdef MESH_DEBUG
    Serial.println("XL9555 GPIO expander not found");
    #endif
    return false;
  }

  // Configure expander pins as outputs for power control
  configExpanderPin(EXPANDS_LORA_EN, true);
  configExpanderPin(EXPANDS_GPS_EN, true);
  configExpanderPin(EXPANDS_NFC_EN, true);
  configExpanderPin(EXPANDS_SD_EN, true);
  configExpanderPin(EXPANDS_KB_EN, true);
  configExpanderPin(EXPANDS_AMP_EN, true);
  configExpanderPin(EXPANDS_DRV_EN, true);

  // Enable essential peripherals by default
  enableLoRaPower(true);
  
  #ifdef MESH_DEBUG
  Serial.println("XL9555 GPIO expander initialized");
  #endif
  
  return true;
}

void TPagerBoard::begin() {
  ESP32Board::begin();

  // Initialize power management
  if (power_init()) {
    _boardStatus |= POWERMANAGE_ONLINE;
    _boardStatus |= EXPANDER_ONLINE;
  }

  // Initialize SPI for LoRa and display
  SPI.begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI);

  // Check for PSRAM
  if (psramFound()) {
    #ifdef MESH_DEBUG
    Serial.printf("PSRAM found: %d bytes\n", ESP.getPsramSize());
    #endif
    _boardStatus |= PSRAM_ONLINE;
  }

  #ifdef MESH_DEBUG
  Serial.println("T-Pager board initialization complete");
  printPMU();
  scanDevices(&Wire);
  #endif
}

bool TPagerBoard::enableLoRaPower(bool enable) {
  return writeExpanderPin(EXPANDS_LORA_EN, enable);
}

bool TPagerBoard::enableGPSPower(bool enable) {
  return writeExpanderPin(EXPANDS_GPS_EN, enable);
}

bool TPagerBoard::enableNFCPower(bool enable) {
  return writeExpanderPin(EXPANDS_NFC_EN, enable);
}

bool TPagerBoard::enableSDPower(bool enable) {
  return writeExpanderPin(EXPANDS_SD_EN, enable);
}

bool TPagerBoard::enableKeyboardPower(bool enable) {
  return writeExpanderPin(EXPANDS_KB_EN, enable);
}

bool TPagerBoard::enableAudioAmp(bool enable) {
  return writeExpanderPin(EXPANDS_AMP_EN, enable);
}

bool TPagerBoard::enableHaptic(bool enable) {
  return writeExpanderPin(EXPANDS_DRV_EN, enable);
}

#ifdef MESH_DEBUG
void TPagerBoard::printPMU() {
  Serial.println("=== T-Pager Power Management Status ===");
  Serial.printf("Board Status: 0x%04X\n", _boardStatus);
  Serial.printf("PSRAM Online: %s\n", (_boardStatus & PSRAM_ONLINE) ? "Yes" : "No");
  Serial.printf("Power Mgmt Online: %s\n", (_boardStatus & POWERMANAGE_ONLINE) ? "Yes" : "No");
  Serial.printf("Expander Online: %s\n", (_boardStatus & EXPANDER_ONLINE) ? "Yes" : "No");
}

void TPagerBoard::scanDevices(TwoWire *w) {
  Serial.println("=== I2C Device Scan ===");
  for (uint8_t addr = 1; addr < 127; addr++) {
    w->beginTransmission(addr);
    if (w->endTransmission() == 0) {
      Serial.printf("I2C device found at address 0x%02X\n", addr);
      
      // Identify known devices
      switch (addr) {
        case I2C_ES8311_ADD:
          Serial.println("  -> ES8311 Audio Codec");
          break;
        case I2C_XL9555_ADD:
          Serial.println("  -> XL9555 GPIO Expander");
          break;
        case I2C_BHI260_ADD:
          Serial.println("  -> BHI260AP Smart Sensor");
          break;
        case I2C_TCA8418_ADD:
          Serial.println("  -> TCA8418 Keyboard Controller");
          break;
        case I2C_BQ27220_ADD:
          Serial.println("  -> BQ27220 Battery Gauge");
          break;
        case I2C_RTC_ADD:
          Serial.println("  -> PCF85063A Real-Time Clock");
          break;
        case I2C_DRV2605_ADD:
          Serial.println("  -> DRV2605 Haptic Driver");
          break;
        case I2C_BQ25896_ADD:
          Serial.println("  -> BQ25896 Battery Charger");
          break;
        default:
          Serial.println("  -> Unknown device");
          break;
      }
    }
  }
  Serial.println("=== I2C Scan Complete ===");
}
#endif

#endif // LILYGO_TPAGER
