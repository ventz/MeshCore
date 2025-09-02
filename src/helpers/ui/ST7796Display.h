#pragma once

#include "DisplayDriver.h"
#include <Wire.h>
#include <SPI.h>
#include <TFT_eSPI.h>

class ST7796Display : public DisplayDriver {
  TFT_eSPI* display;
  bool _isOn;
  uint16_t _color;
  int _x=0, _y=0;
  int _cs_pin, _dc_pin, _rst_pin, _bl_pin;
  int _textSize;

  bool i2c_probe(TwoWire& wire, uint8_t addr);
public:
  ST7796Display(int cs_pin = 38, int dc_pin = 37, int rst_pin = -1, int bl_pin = 42) 
    : DisplayDriver(480, 222), _cs_pin(cs_pin), _dc_pin(dc_pin), _rst_pin(rst_pin), _bl_pin(bl_pin), _textSize(1) {
    _isOn = false;
    display = nullptr;
  }

  ~ST7796Display() {
    if (display) {
      delete display;
    }
  }

  bool begin();

  bool isOn() override { return _isOn; }
  void turnOn() override;
  void turnOff() override;
  void clear() override;
  void startFrame(Color bkg = DARK) override;
  void setTextSize(int sz) override;
  void setColor(Color c) override;
  void setCursor(int x, int y) override;
  void print(const char* str) override;
  void fillRect(int x, int y, int w, int h) override;
  void drawRect(int x, int y, int w, int h) override;
  void drawXbm(int x, int y, const uint8_t* bits, int w, int h) override;
  uint16_t getTextWidth(const char* str) override;
  void endFrame() override;
  
  // T-Pager specific methods
  void setBacklight(uint8_t brightness);
  void setRotation(uint8_t rotation);
};
