#include "ShanviLogo.h"
//#include "SChart.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TaskScheduler.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
//SChart display;
Adafruit_SSD1306 display;
Scheduler runner;

#include "SMenu.h"

void init_display() {
  //display = SChart(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
  //display.setRotation(2); //Rotate the Display
  //display.display(); //Show initial display buffer contents on the screen -- the library initializes this with an Adafruit splash screen.
  display.clearDisplay();
  display.drawBitmap(0, 0, slogo_bitmap_logo_1, 128, 64, WHITE);
  display.display(); // Update the Display
  Serial.println(F("SSD1306 display successful"));
  delay(2000);
}

void setup() {
  Serial.begin(115200);
  init_display();
  initSMenu();
}

void loop() {
  loopSMenu();
}
