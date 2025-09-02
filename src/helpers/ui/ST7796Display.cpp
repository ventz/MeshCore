#ifdef LILYGO_TPAGER

#include "ST7796Display.h"

// T-Pager ST7796 display pin definitions
#define P_DISP_CS    38
#define P_DISP_DC    37
#define P_DISP_RST   -1  // Not connected
#define P_DISP_BL    42

// Color definitions for ST7796
#define ST7796_BLACK    0x0000
#define ST7796_WHITE    0xFFFF
#define ST7796_RED      0xF800
#define ST7796_GREEN    0x07E0
#define ST7796_BLUE     0x001F
#define ST7796_YELLOW   0xFFE0
#define ST7796_ORANGE   0xFC00
#define ST7796_MAGENTA  0xF81F
#define ST7796_CYAN     0x07FF

bool ST7796Display::begin() {
  if(!_isOn) {
    // Initialize display instance
    display = new TFT_eSPI();

    // Configure backlight pin
    if (_bl_pin != -1) {
      pinMode(_bl_pin, OUTPUT);
      digitalWrite(_bl_pin, LOW); // Start with backlight off
    }

    // Initialize the display
    display->init();
    display->setRotation(1); // Landscape mode (480x222)
    display->fillScreen(ST7796_BLACK);
    
    setCursor(0, 0);
    setColor(DisplayDriver::LIGHT);
    
    _isOn = true;
  }
  return true;
}

void ST7796Display::turnOn() {
  if (!_isOn) {
    if (display) {
      display->init();
      display->setRotation(1);
    }
    
    if (_bl_pin != -1) {
      digitalWrite(_bl_pin, HIGH); // Turn on backlight
    }
    
    _isOn = true;
  }
}

void ST7796Display::turnOff() {
  if (_bl_pin != -1) {
    digitalWrite(_bl_pin, LOW); // Turn off backlight
  }
  _isOn = false;
}

void ST7796Display::clear() {
  if (display) {
    display->fillScreen(ST7796_BLACK);
  }
}

void ST7796Display::startFrame(Color bkg) {
  if (display) {
    uint16_t bgColor = ST7796_BLACK;
    switch (bkg) {
      case DisplayDriver::LIGHT:
        bgColor = ST7796_WHITE;
        break;
      case DisplayDriver::DARK:
      default:
        bgColor = ST7796_BLACK;
        break;
    }
    display->fillScreen(bgColor);
  }
}

void ST7796Display::setTextSize(int sz) {
  _textSize = sz;
  if (display) {
    display->setTextSize(_textSize);
  }
}

void ST7796Display::setColor(Color c) {
  switch (c) {
    case DisplayDriver::DARK:
      _color = ST7796_BLACK;
      break;
    case DisplayDriver::LIGHT:
      _color = ST7796_WHITE;
      break;
    case DisplayDriver::RED:
      _color = ST7796_RED;
      break;
    case DisplayDriver::GREEN:
      _color = ST7796_GREEN;
      break;
    case DisplayDriver::BLUE:
      _color = ST7796_BLUE;
      break;
    case DisplayDriver::YELLOW:
      _color = ST7796_YELLOW;
      break;
    case DisplayDriver::ORANGE:
      _color = ST7796_ORANGE;
      break;
    default:
      _color = ST7796_WHITE;
      break;
  }
  if (display) {
    display->setTextColor(_color);
  }
}

void ST7796Display::setCursor(int x, int y) {
  _x = x;
  _y = y;
  if (display) {
    display->setCursor(_x, _y);
  }
}

void ST7796Display::print(const char* str) {
  if (display) {
    display->setCursor(_x, _y);
    display->setTextColor(_color);
    display->setTextSize(_textSize);
    display->print(str);
  }
}

void ST7796Display::fillRect(int x, int y, int w, int h) {
  if (display) {
    display->fillRect(x, y, w, h, _color);
  }
}

void ST7796Display::drawRect(int x, int y, int w, int h) {
  if (display) {
    display->drawRect(x, y, w, h, _color);
  }
}

void ST7796Display::drawXbm(int x, int y, const uint8_t* bits, int w, int h) {
  if (!display) return;

  // Width in bytes for bitmap processing
  uint16_t widthInBytes = (w + 7) / 8;
  
  // Process the bitmap row by row
  for (uint16_t by = 0; by < h; by++) {
    for (uint16_t bx = 0; bx < w; bx++) {
      // Get the current bit
      uint16_t byteOffset = (by * widthInBytes) + (bx / 8);
      uint8_t bitMask = 0x80 >> (bx & 7);
      bool bitSet = pgm_read_byte(bits + byteOffset) & bitMask;
      
      // If the bit is set, draw a pixel
      if (bitSet) {
        display->drawPixel(x + bx, y + by, _color);
      }
    }
  }
}

uint16_t ST7796Display::getTextWidth(const char* str) {
  if (display) {
    display->setTextSize(_textSize);
    return display->textWidth(str);
  }
  return 0;
}

void ST7796Display::endFrame() {
  // TFT_eSPI updates immediately, no need to explicitly update
}

void ST7796Display::setBacklight(uint8_t brightness) {
  if (_bl_pin != -1) {
    // For simple on/off control
    if (brightness > 128) {
      digitalWrite(_bl_pin, HIGH);
    } else {
      digitalWrite(_bl_pin, LOW);
    }
    // TODO: Implement PWM control for AW9364 16-level LED driver
  }
}

void ST7796Display::setRotation(uint8_t rotation) {
  if (display) {
    display->setRotation(rotation);
  }
}

#endif // LILYGO_TPAGER
