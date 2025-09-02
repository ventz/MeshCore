#include <Arduino.h>
#include <LilyGoLib.h>
#include <LV_Helper.h>

lv_obj_t *status_label;
lv_obj_t *counter_label;
lv_obj_t *input_label;
lv_obj_t *keyboard_area;
lv_obj_t *typed_text;

String currentText = "";
String lastKeys = "";
uint32_t hardware_result = 0;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== LilyGo T-Pager KEYBOARD TEST ===");
    
    // Initialize the LilyGoLib instance - disable filesystem to prevent hanging
    uint32_t disable_flags = NO_INIT_FATFS;  // Skip USB MSC filesystem initialization
    hardware_result = instance.begin(disable_flags);
    
    Serial.printf("Hardware initialization result: 0x%08X\n", hardware_result);
    Serial.printf("Device name: %s\n", instance.getName());
    
    // Check keyboard specifically
    if (hardware_result & HW_KEYBOARD_ONLINE) {
        Serial.println("✅ TCA8418 Keyboard Online - Ready for input!");
    } else {
        Serial.println("❌ Keyboard NOT detected!");
    }
    
    // Set brightness to maximum for visibility
    instance.setBrightness(DEVICE_MAX_BRIGHTNESS_LEVEL);
    Serial.printf("Display brightness set to: %d/16\n", DEVICE_MAX_BRIGHTNESS_LEVEL);
    
    // Initialize LVGL
    beginLvglHelper(instance);
    
    // Create main title
    lv_obj_t *title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "KEYBOARD TEST");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF0000), 0); // RED
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);
    
    // Create typing area background
    lv_obj_t *typing_bg = lv_obj_create(lv_screen_active());
    lv_obj_set_size(typing_bg, 460, 100);
    lv_obj_set_style_bg_color(typing_bg, lv_color_hex(0x1111), 0);
    lv_obj_set_style_border_width(typing_bg, 2, 0);
    lv_obj_set_style_border_color(typing_bg, lv_color_hex(0x07E0), 0);
    lv_obj_align(typing_bg, LV_ALIGN_TOP_MID, 0, 40);
    
    // Create typed text display
    typed_text = lv_label_create(typing_bg);
    lv_label_set_text(typed_text, "Type on keyboard...");
    lv_obj_set_style_text_color(typed_text, lv_color_hex(0xFFFF00), 0); // Bright Yellow
    lv_obj_set_style_text_font(typed_text, &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(typed_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(typed_text, 450);
    lv_obj_align(typed_text, LV_ALIGN_TOP_LEFT, 5, 5);
    
    // Create last key display
    input_label = lv_label_create(lv_screen_active());
    lv_label_set_text(input_label, "Last Key: None");
    lv_obj_set_style_text_color(input_label, lv_color_hex(0xFFFF00), 0); // Bright Yellow
    lv_obj_set_style_text_font(input_label, &lv_font_montserrat_16, 0);
    lv_obj_align(input_label, LV_ALIGN_TOP_LEFT, 10, 150);
    
    // Create recent keys history
    keyboard_area = lv_label_create(lv_screen_active());
    lv_label_set_text(keyboard_area, "Recent: ");
    lv_obj_set_style_text_color(keyboard_area, lv_color_hex(0xFFFF00), 0); // Bright Yellow
    lv_obj_align(keyboard_area, LV_ALIGN_TOP_LEFT, 10, 175);
    
    // Create counter
    counter_label = lv_label_create(lv_screen_active());
    lv_label_set_text(counter_label, "Count: 0");
    lv_obj_set_style_text_color(counter_label, lv_color_hex(0x888888), 0);
    lv_obj_align(counter_label, LV_ALIGN_BOTTOM_RIGHT, -10, -5);
    
    Serial.println("=== KEYBOARD READY ===");
    Serial.println("Press any key on the T-Pager keyboard!");
    Serial.println("Keys will appear on both display and serial monitor");
}

void loop() {
    static unsigned long lastLvglUpdate = 0;
    static unsigned long lastInputCheck = 0;
    static unsigned long lastKeyTime = 0;
    static char lastKey = 0;
    static int counter = 0;
    static int rotaryCount = 0;
    
    // Rate limit main loop to prevent CPU spinning and improve key responsiveness
    static unsigned long lastLoopTime = 0;
    if (millis() - lastLoopTime < 5) { // Minimum 5ms between full loop iterations
        delayMicroseconds(100);
        return;
    }
    lastLoopTime = millis();
    
    // PRIORITY 1: Check inputs every 10ms instead of continuously
    if (millis() - lastInputCheck > 10) {
        // Check rotary encoder FIRST (most sensitive to timing)
        RotaryMsg_t rotary = instance.getRotary();
        if (rotary.dir != ROTARY_DIR_NONE) {
            rotaryCount++;
            // Fix backwards direction
            Serial.printf("🔄 ROTARY #%d: %s\n", rotaryCount, rotary.dir == ROTARY_DIR_UP ? "DOWN" : "UP");
            lv_label_set_text_fmt(input_label, "Rotary #%d: %s", rotaryCount,
                                  rotary.dir == ROTARY_DIR_UP ? "DOWN" : "UP");
        }
        
        if (rotary.centerBtnPressed) {
            Serial.println("🔘 Center button pressed!");
            lv_label_set_text(input_label, "CENTER BTN");
            
            // Clear text on center button press
            currentText = "";
            lv_label_set_text(typed_text, "Text cleared!");
            lastKeys = "";
            lv_label_set_text(keyboard_area, "Recent: ");
            rotaryCount = 0; // Reset counter
            
            if (hardware_result & HW_DRV_ONLINE) {
                instance.feedback();
            }
        }
        
        // PRIORITY 2: Check keyboard input with debouncing
        char key;
        if (instance.getKeyChar(&key) == 0) {
            unsigned long now = millis();
            
            // Implement debouncing: ignore duplicate keys within 50ms window
            if (key != lastKey || (now - lastKeyTime) > 50) {
                lastKey = key;
                lastKeyTime = now;
                
                // Show ALL key codes to debug space bar and backspace
                Serial.printf("🔤 RAW KEY: 0x%02X (%d) '%c'\n", key, key, (key >= 32 && key <= 126) ? key : '?');
                
                // Handle ALL possible keys - try specific key codes for backspace and space
                if (key == 0x08 || key == 0x7F || key == 0x0E || key == 30) { // Backspace - try TCA8418 code 30
                    if (currentText.length() > 0) {
                        currentText.remove(currentText.length() - 1);
                    }
                    lv_label_set_text(input_label, "⌫ BACKSPACE");
                    Serial.println("✅ BACKSPACE detected!");
                } else if (key == 0x20 || key == 31) { // Space bar - try TCA8418 code 31
                    currentText += " ";
                    lv_label_set_text(input_label, "␣ SPACE");
                    Serial.println("✅ SPACE detected!");
                } else if (key == 0x0A || key == 0x0D) { // Enter (CR or LF)
                    currentText += "\n";
                    lv_label_set_text(input_label, "↵ ENTER");
                    Serial.println("✅ ENTER detected!");
                } else if (key == 0x01) { // Shift
                    lv_label_set_text(input_label, "⇧ SHIFT");
                } else if (key == 0x02) { // Modifier
                    lv_label_set_text(input_label, "⌥ MODIFIER");
                } else if (key >= 97 && key <= 122) { // Lowercase a-z
                    currentText += key;
                    lv_label_set_text_fmt(input_label, "'%c' (lowercase)", key);
                } else if (key >= 65 && key <= 90) { // Uppercase A-Z
                    currentText += key;
                    lv_label_set_text_fmt(input_label, "'%c' (UPPERCASE)", key);
                } else if (key >= 48 && key <= 57) { // Numbers 0-9
                    currentText += key;
                    lv_label_set_text_fmt(input_label, "'%c' (number)", key);
                } else if (key >= 32 && key <= 126) { // Other printable
                    currentText += key;
                    lv_label_set_text_fmt(input_label, "'%c' (symbol)", key);
                } else if (key == 0x1B) { // ESC
                    lv_label_set_text(input_label, "ESC");
                } else if (key == 0x09) { // TAB
                    lv_label_set_text(input_label, "TAB");
                } else {
                    lv_label_set_text_fmt(input_label, "UNKNOWN: 0x%02X", key);
                }
                
                // Update display immediately
                if (currentText.length() == 0) {
                    lv_label_set_text(typed_text, "Type on keyboard...");
                } else {
                    lv_label_set_text(typed_text, currentText.c_str());
                }
                
                // Update recent keys for all keys
                if (key == 0x08 || key == 0x7F || key == 0x0E || key == 30) {
                    lastKeys += "⌫";
                } else if (key == 0x20 || key == 31) {
                    lastKeys += "␣";
                } else if (key == 0x0A || key == 0x0D) {
                    lastKeys += "↵";
                } else if (key >= 32 && key <= 126) {
                    lastKeys += key;
                } else {
                    lastKeys += "?";
                }
                
                if (lastKeys.length() > 10) {
                    lastKeys = lastKeys.substring(lastKeys.length() - 10);
                }
                
                lv_label_set_text_fmt(keyboard_area, "Recent: %s", lastKeys.c_str());
                
                // Haptic feedback for special keys
                if (hardware_result & HW_DRV_ONLINE && (key == 0x08 || key == 0x7F || key == 0x0E || key == 30 || key == 0x20 || key == 31 || key == 0x0A || key == 0x0D)) {
                    instance.feedback();
                }
            }
        }
        
        lastInputCheck = millis();
    }
    
    // PRIORITY 3: Process LVGL only when needed (less frequently)
    if (millis() - lastLvglUpdate > 16) { // ~60fps max
        lv_timer_handler();
        lastLvglUpdate = millis();
        counter++;
        if (counter % 600 == 0) { // Update counter every 10 seconds
            lv_label_set_text_fmt(counter_label, "Beat: %d", counter / 60);
        }
    }
    
    // PRIORITY 4: Background tasks least frequently
    static unsigned long lastBackgroundUpdate = 0;
    if (millis() - lastBackgroundUpdate > 100) { // Every 100ms
        instance.loop();
        lastBackgroundUpdate = millis();
    }
}
