# LilyGo T-Pager Support for MeshCore

This document describes the implementation of LilyGo T-Pager support in MeshCore.

## Overview

The T-Pager is a sophisticated ESP32-S3 based device with:
- **Display**: 480x222 IPS display (ST7796U driver)
- **Radio**: Semtech SX1262 LoRa transceiver (915MHz)
- **MCU**: ESP32-S3 with 16MB Flash, 8MB PSRAM
- **Power**: Integrated battery management with BQ25896 charger and BQ27220 gauge
- **Sensors**: BHI260AP motion sensor, PCF85063A RTC
- **Audio**: ES8311 codec with haptic feedback (DRV2605)
- **Connectivity**: NFC (ST25R3916), GPS (MIA-M10Q)
- **I/O**: TCA8418 keyboard controller, XL9555 GPIO expander

## Hardware Features

| Component | Model | Function |
|-----------|--------|----------|
| Display | ST7796U | 480x222 IPS with 16-level backlight |
| LoRa Radio | SX1262 | Sub-1GHz transceiver |
| GPS | MIA-M10Q | Global positioning |
| Audio Codec | ES8311 | Audio processing |
| Battery Gauge | BQ27220 | Battery monitoring |
| Battery Charger | BQ25896 | USB-C charging |
| RTC | PCF85063A | Real-time clock |
| Haptic Driver | DRV2605 | Tactile feedback |
| GPIO Expander | XL9555 | Additional I/O pins |
| Keyboard Controller | TCA8418 | Button matrix |
| Smart Sensor | BHI260AP | Motion/environmental |
| NFC | ST25R3916 | Near-field communication |

## Pin Configuration

### LoRa Radio (SX1262)
- NSS: GPIO36
- DIO1: GPIO14
- RESET: GPIO47
- BUSY: GPIO48
- SCK: GPIO35
- MISO: GPIO33
- MOSI: GPIO34

### Display (ST7796U)
- CS: GPIO38
- DC: GPIO37
- RST: Not connected
- Backlight: GPIO42
- Shares SPI bus with radio

### I2C Bus (400kHz)
- SDA: GPIO3
- SCL: GPIO2

### Input Controls
- Rotary Encoder: GPIO40 (A), GPIO41 (B), GPIO7 (Center)
- Keyboard Interrupt: GPIO6

### Power Management
All peripheral power is controlled via XL9555 GPIO expander:
- LoRa: Expander GPIO3
- GPS: Expander GPIO4
- NFC: Expander GPIO5
- SD Card: Expander GPIO14
- Keyboard: Expander GPIO8
- Audio Amp: Expander GPIO1
- Haptic: Expander GPIO0

## Usage

### PlatformIO Configuration

Add to your `platformio.ini`:

```ini
[env:lilygo_tpager]
extends = common:lilygo_tpager
src_filter = +<examples/tpager_test/>
```

Or for MeshCore integration:

```ini
[env:lilygo_tpager_mesh]
extends = common:lilygo_tpager
src_filter = +<examples/tpager_meshcore/>
```

### Basic Example

See `examples/tpager_test/main.cpp` for a basic hardware test that:
- Initializes the board and display
- Configures the LoRa radio for 915MHz
- Shows status information on screen
- Provides battery voltage monitoring

### MeshCore Integration

See `examples/tpager_meshcore/main.cpp` for a complete mesh networking example that:
- Creates a mesh network using the T-Pager hardware
- Displays incoming messages on screen
- Allows sending messages via button press
- Shows network status and peer count

## Key Classes

### TPagerBoard
Extends `ESP32Board` with T-Pager specific functionality:
- Power management via XL9555 expander
- I2C device scanning and identification
- Battery voltage reading (via BQ27220 gauge)
- Deep sleep configuration

### ST7796Display
Implements `DisplayDriver` for the T-Pager's 480x222 IPS display:
- Hardware SPI communication
- Backlight control (16-level via AW9364)
- Color support and text rendering
- Bitmap drawing capabilities

### Radio Configuration
Uses `CustomSX1262Wrapper` with T-Pager specific pins:
- Supports 915MHz operation
- Hardware SPI communication
- Interrupt-driven receive
- Power-optimized operation

## Development Notes

### TFT_eSPI Configuration
The display uses TFT_eSPI library. Ensure your TFT_eSPI configuration matches the T-Pager pinout:
- Driver: ST7796
- SPI pins as defined in TPagerBoard.h
- Resolution: 480x222

### Power Management
The T-Pager uses an XL9555 I2C GPIO expander for peripheral power control. Always enable the appropriate power domains before using peripherals:

```cpp
board.enableLoRaPower(true);  // Enable LoRa
board.enableGPSPower(true);   // Enable GPS
board.enableSDPower(true);    // Enable SD card
```

### I2C Device Addresses
- XL9555 GPIO Expander: 0x20
- ES8311 Audio Codec: 0x18
- BHI260AP Sensor: 0x28
- TCA8418 Keyboard: 0x34
- BQ27220 Battery Gauge: 0x55
- PCF85063A RTC: 0x51
- DRV2605 Haptic: 0x5A
- BQ25896 Charger: 0x6B

### Frequency Configuration
The T-Pager supports various frequency bands based on the radio module:
- SX1262: Sub-1GHz (433/868/915MHz)
- Default configuration is 915MHz for US use

### Display Orientation
The display is configured in landscape mode (rotation 1) for optimal viewing:
- Width: 480 pixels
- Height: 222 pixels
- Color depth: 16-bit (65K colors)

## Troubleshooting

### Display Issues
- Ensure TFT_eSPI is properly configured
- Check backlight enable (GPIO42)
- Verify SPI connections

### Radio Issues
- Confirm LoRa power is enabled
- Check antenna connection
- Verify frequency settings match local regulations

### Power Issues
- Check XL9555 expander communication
- Verify I2C bus operation
- Monitor battery voltage via gauge

### Build Issues
- Ensure all required libraries are installed
- Check board definition is properly copied
- Verify variant path is correct

## Future Enhancements

Potential improvements for T-Pager support:
- GPS integration for location services
- NFC functionality for device pairing
- Audio codec support for notifications
- Haptic feedback integration
- Advanced power management
- Keyboard matrix support
- SD card storage integration

## Board Files Required

The implementation requires:
- `boards/lilygo-t-lora-pager.json` - PlatformIO board definition
- `src/helpers/esp32/TPagerBoard.h/cpp` - Board support class
- `src/helpers/ui/ST7796Display.h/cpp` - Display driver
- `variants/lilygo_tpager/` - Variant-specific configuration

## Getting Started

1. Copy the board definition from LilyGoLib-PlatformIO
2. Use the provided variant configuration
3. Start with the basic test example
4. Progress to MeshCore integration
5. Customize for your specific application

The T-Pager provides an excellent platform for mesh networking applications with its integrated display, long-range radio, and comprehensive sensor suite.
