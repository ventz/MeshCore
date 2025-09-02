#pragma once

#if defined(LILYGO_TPAGER)

#include <Wire.h>
#include <Arduino.h>
#include "helpers/ESP32Board.h"
#include <driver/rtc_io.h>

// LoRa radio module pins for T-Pager SX1262
#define P_LORA_DIO_0    -1      // NC
#define P_LORA_DIO_1    14      // SX1262 IRQ pin
#define P_LORA_NSS      36      // SX1262 SS pin
#define P_LORA_RESET    47      // SX1262 Reset pin
#define P_LORA_BUSY     48      // SX1262 Busy pin
#define P_LORA_SCLK     35      // SX1262 SCLK pin
#define P_LORA_MISO     33      // SX1262 MISO pin
#define P_LORA_MOSI     34      // SX1262 MOSI pin

// I2C pins - shared by multiple devices
#define PIN_BOARD_SDA   3       // I2C SDA
#define PIN_BOARD_SCL   2       // I2C SCL

// Display pins (ST7796U)
#define P_DISP_MOSI     34      // Share with LoRa MOSI
#define P_DISP_MISO     33      // Share with LoRa MISO
#define P_DISP_SCK      35      // Share with LoRa SCK
#define P_DISP_CS       38      // Display CS
#define P_DISP_DC       37      // Display DC
#define P_DISP_RST      -1      // Display reset (not connected)  
#define P_DISP_BL       42      // Display backlight

// GPS pins (MIA-M10Q)
#define PIN_GPS_RX      4       // GPS RX
#define PIN_GPS_TX      12      // GPS TX
#define PIN_GPS_PPS     13      // GPS PPS

// SD Card
#define P_SD_CS         21      // SD Card CS

// Keyboard and input
#define P_KB_INT        6       // TCA8418 Keyboard interrupt
#define P_KB_BACKLIGHT  46      // Keyboard backlight
#define P_ROTARY_A      40      // Rotary encoder A
#define P_ROTARY_B      41      // Rotary encoder B
#define P_ROTARY_C      7       // Rotary encoder center button

// Interrupt pins
#define P_RTC_INT       1       // PCF85063A RTC interrupt
#define P_NFC_INT       5       // ST25R3916 NFC interrupt
#define P_SENSOR_INT    8       // BHI260AP sensor interrupt
#define P_NFC_CS        39      // NFC CS

// Audio codec pins (ES8311)
#define P_I2S_WS        18      // I2S Word Select
#define P_I2S_SCK       11      // I2S Serial Clock
#define P_I2S_MCLK      10      // I2S Master Clock
#define P_I2S_SDOUT     45      // I2S Serial Data Out
#define P_I2S_SDIN      17      // I2S Serial Data In

// External expansion pins
#define P_EXT_TX        43      // External TX
#define P_EXT_RX        44      // External RX

// XL9555 GPIO expander definitions
#define EXPANDS_DRV_EN      0   // DRV2605 haptic driver enable
#define EXPANDS_AMP_EN      1   // Audio amplifier enable
#define EXPANDS_KB_RST      2   // Keyboard reset
#define EXPANDS_LORA_EN     3   // LoRa power enable
#define EXPANDS_GPS_EN      4   // GPS power enable
#define EXPANDS_NFC_EN      5   // NFC power enable
#define EXPANDS_GPS_RST     7   // GPS reset
#define EXPANDS_KB_EN       8   // Keyboard power enable
#define EXPANDS_GPIO_EN     9   // General GPIO enable
#define EXPANDS_SD_DET      10  // SD card detect
#define EXPANDS_SD_PULLEN   11  // SD pullup enable
#define EXPANDS_SD_EN       12  // SD card power enable

// I2C device addresses
#define I2C_ES8311_ADD      0x18    // Audio codec
#define I2C_XL9555_ADD      0x20    // GPIO expander
#define I2C_BHI260_ADD      0x28    // Smart sensor
#define I2C_TCA8418_ADD     0x34    // Keyboard controller
#define I2C_BQ27220_ADD     0x55    // Battery gauge
#define I2C_RTC_ADD         0x51    // Real-time clock
#define I2C_DRV2605_ADD     0x5A    // Haptic driver
#define I2C_BQ25896_ADD     0x6B    // Battery charger

class TPagerBoard : public ESP32Board {
private:
    enum {
        POWERMANAGE_ONLINE  = _BV(0),
        DISPLAY_ONLINE      = _BV(1),
        RADIO_ONLINE        = _BV(2),
        GPS_ONLINE          = _BV(3),
        PSRAM_ONLINE        = _BV(4),
        SDCARD_ONLINE       = _BV(5),
        KEYBOARD_ONLINE     = _BV(6),
        AUDIO_ONLINE        = _BV(7),
        SENSOR_ONLINE       = _BV(8),
        NFC_ONLINE          = _BV(9),
        RTC_ONLINE          = _BV(10),
        HAPTIC_ONLINE       = _BV(11),
        EXPANDER_ONLINE     = _BV(12),
    };

    bool power_init();
    bool expander_init();

public:
#ifdef MESH_DEBUG
    void scanDevices(TwoWire *w);
#endif

    void begin();

    void onBeforeTransmit() override {
        // Turn on TX indicator if available
    }

    void onAfterTransmit() override {
        // Turn off TX indicator if available
    }

    void enterDeepSleep(uint32_t secs, int pin_wake_btn) {
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

        // Configure LoRa pins for deep sleep
        rtc_gpio_set_direction((gpio_num_t)P_LORA_DIO_1, RTC_GPIO_MODE_INPUT_ONLY);
        rtc_gpio_pulldown_en((gpio_num_t)P_LORA_DIO_1);
        rtc_gpio_hold_en((gpio_num_t)P_LORA_NSS);

        if (pin_wake_btn < 0) {
            esp_sleep_enable_ext1_wakeup((1L << P_LORA_DIO_1), ESP_EXT1_WAKEUP_ANY_HIGH);
        } else {
            esp_sleep_enable_ext1_wakeup((1L << P_LORA_DIO_1) | (1L << pin_wake_btn), ESP_EXT1_WAKEUP_ANY_HIGH);
        }

        if (secs > 0) {
            esp_sleep_enable_timer_wakeup(secs * 1000000);
        }

        esp_deep_sleep_start();
    }

    uint16_t getBattMilliVolts() {
        // TODO: Implement battery voltage reading via BQ27220 gauge
        return 3700; // Default 3.7V
    }

    const char* getManufacturerName() const {
        return "LilyGo T-Pager";
    }

    // T-Pager specific methods
    bool enableLoRaPower(bool enable);
    bool enableGPSPower(bool enable);
    bool enableNFCPower(bool enable);
    bool enableSDPower(bool enable);
    bool enableKeyboardPower(bool enable);
    bool enableAudioAmp(bool enable);
    bool enableHaptic(bool enable);
};

#endif // LILYGO_TPAGER
