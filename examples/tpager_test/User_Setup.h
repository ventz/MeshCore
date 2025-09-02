// T-Pager specific TFT_eSPI configuration

#define ST7796_DRIVER

#define TFT_WIDTH  480
#define TFT_HEIGHT 222

// T-Pager pin definitions
#define TFT_MOSI 34
#define TFT_SCLK 35
#define TFT_CS   38
#define TFT_DC   37
#define TFT_RST  -1  // Not connected

// Fonts to be available
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.

// Optional reduced SPI port control to 6 control lines
#define TFT_SDA_READ

// ST7796 specific commands
#define TFT_INIT_DELAY 0x80

// SPI frequency
#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
