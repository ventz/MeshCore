#include <Arduino.h>

#ifdef LILYGO_TPAGER

#include "../../src/MeshCore.h"
#include "../../variants/lilygo_tpager/target.h"
#include "../../src/helpers/BaseChatMesh.h"

class TPagerMesh : public BaseChatMesh {
public:
  TPagerMesh() : BaseChatMesh(board, rtc_clock, radio_driver) {}

protected:
  void onMessage(const mesh::ContactInfo& sender, const char* message) override {
    Serial.printf("Message from %s: %s\n", sender.getName(), message);
    
    #ifdef DISPLAY_CLASS
    // Display message on screen
    display.startFrame();
    display.setColor(DisplayDriver::LIGHT);
    display.setTextSize(1);
    
    display.setCursor(10, 10);
    display.print("From: ");
    display.print(sender.getName());
    
    display.setCursor(10, 40);
    display.print("Msg: ");
    display.print(message);
    
    display.setCursor(10, 180);
    display.print("Press button to reply");
    
    display.endFrame();
    #endif
  }

  void onStatusUpdate(const char* status) override {
    Serial.printf("Status: %s\n", status);
    
    #ifdef DISPLAY_CLASS
    display.startFrame();
    display.setColor(DisplayDriver::GREEN);
    display.setTextSize(1);
    display.setCursor(10, 200);
    display.print(status);
    display.endFrame();
    #endif
  }
};

TPagerMesh mesh;
bool button_pressed = false;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== T-Pager MeshCore Example ===");
  
  // Initialize board
  board.begin();
  Serial.println("Board initialized");
  
  // Initialize display
  #ifdef DISPLAY_CLASS
  if (display.begin()) {
    Serial.println("Display initialized");
    display.turnOn();
    display.clear();
    display.startFrame();
    display.setColor(DisplayDriver::LIGHT);
    display.setTextSize(2);
    display.setCursor(50, 50);
    display.print("T-Pager");
    display.setCursor(50, 80);
    display.print("MeshCore");
    display.setTextSize(1);
    display.setCursor(10, 150);
    display.print("Initializing mesh...");
    display.endFrame();
  }
  #endif
  
  // Initialize radio
  if (radio_init()) {
    Serial.println("Radio initialized");
    
    // Set 915MHz frequency for US
    radio_set_params(915.0, 125.0, 7, 5);
    radio_set_tx_power(14);
    
    Serial.println("Radio configured for 915MHz");
  } else {
    Serial.println("Radio initialization failed");
    return;
  }
  
  // Initialize mesh network
  mesh.begin();
  Serial.println("Mesh network started");
  
  // Set device name
  mesh.setNodeName("T-Pager-001");
  
  #ifdef DISPLAY_CLASS
  display.startFrame();
  display.setColor(DisplayDriver::GREEN);
  display.setTextSize(1);
  display.setCursor(10, 180);
  display.print("Mesh ready! Listening...");
  display.endFrame();
  #endif
  
  // Configure button pin (using rotary encoder center button)
  pinMode(P_ROTARY_C, INPUT_PULLUP);
  
  Serial.println("=== MeshCore Ready ===");
  Serial.println("Press center button to send test message");
}

void loop() {
  // Process mesh network
  mesh.loop();
  
  // Check for button press
  static bool last_button_state = HIGH;
  bool current_button_state = digitalRead(P_ROTARY_C);
  
  if (last_button_state == HIGH && current_button_state == LOW) {
    // Button pressed
    button_pressed = true;
    delay(50); // Debounce
  }
  last_button_state = current_button_state;
  
  if (button_pressed) {
    button_pressed = false;
    
    // Send test message to all nodes
    mesh.sendMessage("Hello from T-Pager!");
    Serial.println("Test message sent");
    
    #ifdef DISPLAY_CLASS
    display.startFrame();
    display.setColor(DisplayDriver::YELLOW);
    display.setTextSize(1);
    display.setCursor(10, 200);
    display.print("Message sent!");
    display.endFrame();
    #endif
  }
  
  // Periodic status update
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 30000) { // Every 30 seconds
    lastStatus = millis();
    
    Serial.printf("Battery: %dmV, Mesh peers: %d\n", 
                  board.getBattMilliVolts(), 
                  mesh.getPeerCount());
    
    #ifdef DISPLAY_CLASS
    display.startFrame();
    display.setColor(DisplayDriver::BLUE);
    display.setTextSize(1);
    display.setCursor(300, 10);
    display.print("Peers: ");
    display.print(mesh.getPeerCount());
    display.setCursor(300, 30);
    display.print("Batt: ");
    display.print(board.getBattMilliVolts());
    display.print("mV");
    display.endFrame();
    #endif
  }
  
  delay(10);
}

#else
void setup() {
  Serial.begin(115200);
  Serial.println("This example is only for LilyGo T-Pager");
}

void loop() {
  delay(1000);
}
#endif
