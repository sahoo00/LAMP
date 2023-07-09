#include <PIDController.h>
#include <SoftwareSerial.h>
#include <CommandHandler.h>

SoftwareSerial serial2(12, 13); // RX, TX
// 1. Maximum number of Commands = 10
// 2. Maximum command length = 30
// 3. Maximum number of Variables = 15
// 4. Start of Message = !
// 5. End of Message = \r
CommandHandler<4, 20, 0> SerialCommandHandler(serial2, '!', '\n');

// Mosfet Pin
#define mosfet_pin 9

#define encoder0Press  2
#define encoder0PinA  3
#define encoder0PinB  4
volatile unsigned int encoder0Pos = 0;
volatile unsigned int lastEncoder0Pos = 0;
bool encoderPrevA=true, encoderPrevB = true;

#define __Kp 200 // 500 // 30 // Proportional constant
#define __Ki 1 // 0.1 // 0.7 // Integral Constant
#define __Kd 10 // 100 // 200 // Derivative Constant
uint8_t set_temperature = 60;
float temperature_value_c = 60.0; // stores temperature value
PIDController pid; // Create an instance of the PID controller class, called "pid"
uint8_t tindex = 0;
uint8_t toutput = 0;

#define ThermistorPin 1
#define __R1 100000
#define __R2(x) (__R1 * x /(1024.0 - x))
#define __c1 0.00166736
#define __c2 0.000113695
#define __c3 2.57974e-07

#define MAIN_MENU 1
#define TEMP_MENU 2
#define INFO_MENU 3

uint8_t encoder_btn_count = 0;
unsigned long debounce = 0; // Debounce delay

byte menuState = MAIN_MENU;
byte menuCount = 1;
byte dir = 0;
bool runState = false;
bool enter_info = true;
byte camera_status = 0;
byte _s_ip[4];

void mainMenu() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(10, 0);
  display.println(F("Main"));
  //---------------------------------
  display.setTextSize(1);
  display.setCursor(10, 20);
  display.print(F("Info "));
  display.print(toutput, DEC);
  display.print(F(" "));
  display.print(tindex, DEC);
  
  display.setCursor(10, 30);
  display.print(F("Set Temp: "));
  // display.print(temperature_value_c, 1); // This causes some unwanted points in the display at the bottom right corner
  display.print((int)temperature_value_c);
  display.print(F("."));
  display.print((int) abs((temperature_value_c - (int)temperature_value_c) * 10));
  display.print(F("C"));

  if (runState) {
    display.setCursor(10, 40);
    display.print(F("Stop "));
  }
  else {
    display.setCursor(10, 40);
    display.print(F("Run "));
  }
  display.print(camera_status, DEC);
  display.print(F(" "));
  display.print(_s_ip[0], DEC);
  display.print(F("."));
  display.print(_s_ip[1], DEC);
  display.print(F("."));
  display.print(_s_ip[2], DEC);
  display.print(F("."));
  display.print(_s_ip[3], DEC);

  display.setCursor(2, (menuCount * 10) + 10);
  display.println(F(">"));

  display.display();
}

void tempMenu() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(10, 0);
  display.println(F("Temp C"));
  //---------------------------------
  display.setTextSize(1);
  display.setCursor(10, 20);
  display.print(F("Value:"));
  display.println(set_temperature, 1);

  display.setCursor(10, 30);
  display.println(F("Return"));

  display.setCursor(2, (menuCount * 10) + 10);
  display.println(F(">"));

  display.display();
}

void displayChart(CommandParameter &parameter) {
  const unsigned char *value = (unsigned char *) parameter.NextParameter();
  Serial.println((const char *)value);
  if (value[0] == 'R') {
    camera_status = value[1] - 40;
    _s_ip[0] = value[2]-40;_s_ip[1] = value[3]-40;
    _s_ip[2] = value[4]-40;_s_ip[3] = value[5]-40;
  }
  if (menuState == INFO_MENU) {
    if (value[0] == 'L') {
      display.drawLine(value[1]-40, value[2]-40, value[3]-40, value[4]-40, value[5]-40);
    }
    if (value[0] == 'H') {
      display.drawFastHLine(value[1]-40, value[2]-40, value[3]-40, value[4]-40);
    }
    if (value[0] == 'V') {
      display.drawFastVLine(value[1]-40, value[2]-40, value[3]-40, value[4]-40);
    }
    if (value[0] == 'T') {
      display.setTextSize(1);
      display.setTextColor(WHITE);    
      display.setCursor(value[1]-40, value[2]-40);
      display.write((const char *)(value + 3));
    }
    if (value[0] == 'C') {
      display.fillCircle(value[1]-40, value[2]-40, value[3]-40, value[4]-40);
    }
    if (value[0] == 'X') {
      display.clearDisplay();
    }
    display.display();
  }
}

const char reset_cmd [] PROGMEM = "!R 1\r\n";

void infoMenu() {
  if (enter_info) {
    //resetChart(display);
    display.clearDisplay();
    display.display();
    serial2.print((const __FlashStringHelper *)reset_cmd);
    Serial.print((const __FlashStringHelper *)reset_cmd);
  }
  enter_info = false;
}

void staticMenu() {
  if (menuState == MAIN_MENU) {
    mainMenu();
  }
  if (menuState == TEMP_MENU) {
    tempMenu();
  }
  if (menuState == INFO_MENU) {
    infoMenu();
  }
}

void menuCheck() {
  if (menuState == MAIN_MENU) {
    if (lastEncoder0Pos != encoder0Pos && dir == 0) {
      if ( millis() - debounce > 100) {
        menuCount++;
        debounce = millis();
      }
    }
    if (lastEncoder0Pos != encoder0Pos && dir == 1) {
      if ( millis() - debounce > 100) {
        menuCount--;
        debounce = millis();
      }
    }
    if (menuCount > 3) { menuCount = 1;}
    if (menuCount < 1) { menuCount = 3; }
    if (encoder_btn_count > 0) {
      if (menuCount == 1) { menuState = INFO_MENU; enter_info = true; }
      if (menuCount == 2) { menuState = TEMP_MENU;}
      if (menuCount == 3) { runState = !runState;}
      encoder_btn_count = 0; 
    }
  }
  if (menuState == TEMP_MENU) {
    if (lastEncoder0Pos != encoder0Pos && dir == 1) {
      set_temperature++;
    }
    if (lastEncoder0Pos != encoder0Pos && dir == 0) {
      set_temperature--;
    }
    if (set_temperature < 1 ) set_temperature = 1;
    if (set_temperature > 100 ) set_temperature = 100;

    if (encoder_btn_count > 0) {
      menuState = MAIN_MENU;
      menuCount = 2;
      encoder_btn_count = 0;
      enter_info = true; 
    }
  }
  if (menuState == INFO_MENU) {
    if (encoder_btn_count > 0) {
      menuState = MAIN_MENU;
      menuCount = 1;
      encoder_btn_count = 0;
      enter_info = true;
    }
  }
  lastEncoder0Pos = encoder0Pos;
}

void doEncoder() {
  lastEncoder0Pos = encoder0Pos;
  bool pinA = digitalRead(encoder0PinA);
  bool pinB = digitalRead(encoder0PinB);

  if ( (encoderPrevA == pinA && encoderPrevB == pinB) ) return;  // no change since last time (i.e. reject bounce)

  // same direction (alternating between 0,1 and 1,0 in one direction or 1,1 and 0,0 in the other direction)
         if (encoderPrevA == 1 && encoderPrevB == 0 && pinA == 0 && pinB == 1) {encoder0Pos -= 1; dir = 0;}
    else if (encoderPrevA == 0 && encoderPrevB == 1 && pinA == 1 && pinB == 0) {encoder0Pos -= 1; dir = 0;}
    else if (encoderPrevA == 0 && encoderPrevB == 0 && pinA == 1 && pinB == 1) {encoder0Pos += 1; dir = 1;}
    else if (encoderPrevA == 1 && encoderPrevB == 1 && pinA == 0 && pinB == 0) {encoder0Pos += 1; dir = 1;}

  // change of direction
    else if (encoderPrevA == 1 && encoderPrevB == 0 && pinA == 0 && pinB == 0) {encoder0Pos += 1; dir = 1;}
    else if (encoderPrevA == 0 && encoderPrevB == 1 && pinA == 1 && pinB == 1) {encoder0Pos += 1; dir = 1;}
    else if (encoderPrevA == 0 && encoderPrevB == 0 && pinA == 1 && pinB == 0) {encoder0Pos -= 1; dir = 0;}
    else if (encoderPrevA == 1 && encoderPrevB == 1 && pinA == 0 && pinB == 1) {encoder0Pos -= 1; dir = 0;}

  // update previous readings
    encoderPrevA = pinA;
    encoderPrevB = pinB;
}

void readEncoder() {
  if ( digitalRead(encoder0Press) == LOW)   //If we detect LOW signal, button is pressed
  {
    if ( millis() - debounce > 200) { //debounce delay
      encoder_btn_count++; // Increment the values 
      if (encoder_btn_count != 2) encoder_btn_count = 1;
      debounce = millis(); // update the time variable
    }
  }
}

void calculateTemp() {
    //float logR2, R2, T, Tc, Tf;
    int Vo = analogRead(ThermistorPin);
    //R2 = R1 * Vo /(1024.0 - Vo);
    //logR2 = log(R2);
    //T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
    //Tc = T - 273.15;
    //Tf = (Tc * 9.0)/ 5.0 + 32.0;
    float Tc =  (1.0 / (__c1 + __c2*log(__R2(Vo)) + __c3*log(__R2(Vo))*log(__R2(Vo))*log(__R2(Vo)))) - 273.15;
  
    temperature_value_c = Tc;
    toutput = pid.compute(temperature_value_c);    // Let the PID compute the value, returns the optimal output
    analogWrite(mosfet_pin, toutput);           // Write the output to the output pin
    pid.setpoint(set_temperature); // Use the setpoint methode of the PID library to
    tindex = (tindex + 1) % 100;
}

Task t1(1000, TASK_FOREVER, &calculateTemp);

void initSMenu() {
  _s_ip[0] = 0;_s_ip[1] = 0;_s_ip[2] = 0;_s_ip[3] = 0;
  pinMode(mosfet_pin, OUTPUT); // MOSFET output PIN
  pinMode(encoder0Press, INPUT_PULLUP);
  pinMode(encoder0PinA, INPUT);
  pinMode(encoder0PinB, INPUT);
  pid.begin();          // initialize the PID instance
  pid.setpoint(150);    // The "goal" the PID controller tries to "reach"
  pid.tune(__Kp, __Ki,__Kd);    // Tune the PID, arguments: kP, kI, kD
  pid.limit(0, 255);    // Limit the PID output between 0 and 255, this is important to get rid of integral windup!
  attachInterrupt(digitalPinToInterrupt(encoder0PinA), doEncoder, CHANGE);
  serial2.begin(9600);
  runner.addTask(t1);
  t1.enable();
  SerialCommandHandler.AddCommand(F("A"), displayChart);
}

void loopSerial() {
  while(serial2.available()){
    char c = serial2.read();
    Serial.write(c);
  }
}

void loopSMenu() {
  readEncoder();
  menuCheck();
  staticMenu();
  runner.execute();
  SerialCommandHandler.Process();
  //loopSerial();
  //delay(1000);
}
