/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-cam-take-photo-display-web-server/
  
  IMPORTANT!!! 
   - Select Board "AI Thinker ESP32-CAM"
   - GPIO 0 must be connected to GND to upload a sketch
   - After connecting GPIO 0 to GND, press the ESP32-CAM on-board RESET button to put your board in flashing mode
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include "WiFi.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <WebServer.h>
#include <Ticker.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <CommandHandler.h>

// define the number of bytes you want to access
#define EEPROM_SIZE 1

#define DEBUG
#include "DebugMacros.h"

#include <Wire.h>

// Replace with your network credentials
//const char* ssid = "DSSP-Home";
//const char* password = "260604072013";
const char* ssid = "Pixel_8154";
const char* password = "Shanvi123!";

const char* apssid = "ActiveGene";
const char* appassword = "BooleanLab";

const char* jquery = "/jquery-3.1.0.min.js";

class SWebServer: public WebServer {
  protected:
  void _addMHeader(String &header, String hdr) {
    header = header.substring(0, header.length()-3) + hdr + "\r\n";
  }
  void _addMHeader(String &header, String t, String val) {
    _addMHeader(header, t + ": " + val + "\r\n");
  }
  
  public:
  SWebServer(int port): WebServer(port) {}
  void send(int code, const char* content_type, const String& content) {
    WebServer::send(code, content_type, content);
  }
  
  void send(int code, char* content_type, String hdr, const char * buf, size_t len) {
    String header;
    _prepareHeader(header, code, content_type, len);
    //header = header.substring(0, header.length()-3) + hdr + "\r\n";
    _addMHeader(header, hdr);
    _currentClient.write(header.c_str(), header.length());
    if(len > 0) {
      const char * footer = "\r\n";
      if(_chunked) {
        char * chunkSize = (char *)malloc(11);
        if(chunkSize){
          sprintf(chunkSize, "%x%s", len, footer);
          _currentClient.write(chunkSize, strlen(chunkSize));
          free(chunkSize);
        }
      }
      _currentClient.write(buf, len);
      if(_chunked){
        _currentClient.write(footer, 2);
      }
    }
  }

  void send(FS &fs, const String& path, char * contentType, bool download) {
    int code = 200;  
    File file = fs.open(path, "r");
    int len = file.size();
    
    String header;
    _prepareHeader(header, code, contentType, len);

    int filenameStart = path.lastIndexOf('/') + 1;
    char buf[26+path.length()-filenameStart];
    char* filename = (char*)path.c_str() + filenameStart;
    if(download) {
      // set filename and force download
      snprintf(buf, sizeof (buf), "attachment; filename=\"%s\"", filename);
    } else {
      // set filename and force rendering
      snprintf(buf, sizeof (buf), "inline; filename=\"%s\"", filename);
    }
    _addMHeader(header, "Content-Disposition", String(buf));
    _currentClient.write(header.c_str(), header.length());
  
    if(len > 0) {
      _currentClient.write(file);
    }
  }
};

// Create WebServer object on port 80
SWebServer server(80);
File root;
bool opened = false;

int pictureNumber = 0;
int recordPictures = 0;
int sdcard_present = 1;
Ticker scheduler;
#define LED_PIN 12
#define RXD2 13
#define TXD2 4
CommandHandler<4, 20, 0> SerialCommandHandler(Serial2, '!', '\n');
IPAddress _s_ip;                    // the IP address of your shield

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { text-align:center; }
    .vert { margin-bottom: 10%; }
    .hori{ margin-bottom: 0%; }
  </style>
  <script src="https://code.jquery.com/jquery-3.1.0.min.js" type="text/javascript"></script>
  <script src="jquery" type="text/javascript"></script>
  <script type="text/javascript">
  function sendLED() {
    var url = "led?num=" + $("#photoNum").val();
    $("#results").load(url);
    return false;
  }
  function updateImage() {
    var url = "capture-photo";
    $("#photo").attr("src", url + `?v=${new Date().getTime()}`);
    return false;
  }
  function sendSave() {
    var url = "save-photo?num=" + $("#photoNum").val();
    $("#results").load(url);
    return false;
  }
  function sendRecord() {
    var url = "record-start";
    $("#results").load(url);
    return false;
  }
  function sendStop() {
    var url = "record-stop";
    $("#results").load(url);
    return false;
  }
  function sendReset() {
    var url = "reset?num=" + $("#photoNum").val();
    $("#results").load(url);
    return false;
  }
  function getInfo() {
    var url = "get-info?num=" + $("#photoNum").val();
    $("#results").load(url);
    return false;
  }
  function getPhoto() {
    var url = "get-photo?num=" + $("#photoNum").val();
    $("#photo").removeAttr("src").attr("src", url);
    return false;
  }
  </script>
</head>
<body>
  <div id="container">
    <h2>ESP32-CAM Photo Capture</h2>
    <p>
      <button onclick="sendLED();">LED</button>
      <button onclick="updateImage();">Capture</button>
      <button onclick="sendSave();">Save</button>
      <button onclick="sendRecord();">Record</button>
      <button onclick="sendStop();">Stop</button>
      <button onclick="sendReset();">Reset</button>
      <button onclick="getInfo();">GetInfo</button>
      <button onclick="getPhoto();">Get</button>
      Num:<input type="text" size="4" id="photoNum" name="num" value="0"/>
    </p>
  </div>
  <div id="results"></div>
  <div><img src="capture-photo" id="photo" width="70%"></div>
  <br/>
<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>
<input type='file' name='update'>
<input type='submit' value='Upload'>
</form>
<div id='prg'>progress: 0%</div>
<script>
$('form').submit(function(e){
    e.preventDefault();
    var form = $('#upload_form')[0];
    var data = new FormData(form);
    $.ajax({
          url: '/update',
          type: 'POST',
          data: data,
          contentType: false,
          processData:false,
          xhr: function() {
              var xhr = new window.XMLHttpRequest();
              xhr.upload.addEventListener('progress', function(evt) {
                  if (evt.lengthComputable) {
                      var per = evt.loaded / evt.total;
                      $('#prg').html('progress: ' + Math.round(per*100) + '%');
                  }
             }, false);
             return xhr;
          },
          success:function(d, s) {
              console.log('success!')
          },
          error: function (a, b, c) {
          }
    });
});
</script>
</body>
</html>)rawliteral";

void savePicture(int num) {
  camera_fb_t * fb = NULL;

  if (sdcard_present == 0) {
    return;
  }
  
  // Take Picture with Camera
  fb = esp_camera_fb_get();  
  if(!fb) {
    DPRINTLN("Camera capture failed");
    return;
  }

  // Path where new picture will be saved in SD Card
  String path = "/picture-" + String(num) +".jpg";

  fs::FS &fs = SD_MMC;
#ifdef DEBUG
  Serial.printf("Picture file name: %s\n", path.c_str());
#endif

  File file = fs.open(path.c_str(), FILE_WRITE);
  if(!file){
    DPRINTLN("Failed to open file in writing mode");
  } 
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
#ifdef DEBUG
    Serial.printf("Saved file to path: %s\n", path.c_str());
#endif
  }
  file.close();
  esp_camera_fb_return(fb); 
}

int setLED(int num) {
    if (num > 0) {
      num = 1;
      digitalWrite(LED_PIN,HIGH);
    }
    else {
      num = 0;
      digitalWrite(LED_PIN,LOW);
    }
    return num;
}

camera_fb_t * readImageFile(String path) {
  if (sdcard_present == 0) {
    return NULL;
  }
  
    fs::FS &fs = SD_MMC;          // sd card file system
    File file = fs.open(path, FILE_READ);
    if (file) {
      size_t fbLen = file.size();
      uint8_t *jpg_buffer = (uint8_t *)ps_malloc(fbLen);
      if (jpg_buffer) {
        file.read(jpg_buffer, fbLen);
        file.close();
        camera_fb_t * fb = (camera_fb_t*) ps_malloc(sizeof(camera_fb_t));
        fb->buf = jpg_buffer;
        fb->len = fbLen;
        fb->width = 320;
        fb->height = 240;
        fb->format = PIXFORMAT_JPEG;
        return fb;
      }
    }
    return NULL;
}

int getIntensity(uint8_t * rgb_buffer, int w, int c1, int c2, int r1, int r2) {
  int intensity = 0;
  for (int col = c1; col < c2; ++col) {
    for (int row = r1; row < r2; ++row) {
      intensity += rgb_buffer[3*w*row+3*col+0];
      intensity += rgb_buffer[3*w*row+3*col+1];
      intensity += rgb_buffer[3*w*row+3*col+2];
    }
  }
  return intensity;
}

void getIntensities(camera_fb_t * fb, int res[6]) {
  if (!fb) {
    return;
  }
  int width = 320;
  int height = 240;
  int len = width*height*3;
  uint8_t * rgb_buffer = (uint8_t *)ps_malloc(len);
  fmt2rgb888(fb->buf,fb->len,fb->format,rgb_buffer);
  res[0] = getIntensity(rgb_buffer, width, 130, 150, 75, 95);
  res[1] = getIntensity(rgb_buffer, width, 205, 225, 75, 95);
  res[2] = getIntensity(rgb_buffer, width, 130, 150, 142, 162);
  res[3] = getIntensity(rgb_buffer, width, 205, 225, 142, 162);
  res[4] = getIntensity(rgb_buffer, width, 130, 150, 209, 229);
  res[5] = getIntensity(rgb_buffer, width, 205, 225, 209, 229);
  free(rgb_buffer);
}

void drawDelay() {
  delay(100);
}

void drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color) {
  char buf[20];
  sprintf(buf, "!A L00000\r\n");
  buf[4] = x1 + 40; buf[5] = y1 + 40;
  buf[6] = x2 + 40;
  buf[7] = y2 + 40;
  buf[8] = color + 40;
  Serial2.print(buf);
  Serial.println("Sent " + String(buf) + " " + String(x1) + " " + String(y1) + " " + String(x2) + " " + String(y2) + " " + String(color));
  drawDelay();
}

void drawText(int16_t x, int16_t y, const char * s) {
  char buf[20];
  sprintf(buf, "!A T00%s\r\n", s);
  buf[4] = x + 40; buf[5] = y + 40;
  Serial2.print(buf);
  Serial.println("Sent " + String(buf) + " " + String(x) + " " + String(y));
  drawDelay();
}

void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  char buf[20];
  sprintf(buf, "!A V0000\r\n");
  buf[4] = x + 40; buf[5] = y + 40;
  buf[6] = h + 40;
  buf[7] = color + 40;
  Serial2.print(buf);
  Serial.println("Sent " + String(buf) + " " + String(x) + " " + String(y) + " " + String(h) + " " + String(color));
  drawDelay();
}

void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  char buf[20];
  sprintf(buf, "!A H0000\r\n");
  buf[4] = x + 40; buf[5] = y + 40;
  buf[6] = w + 40;
  buf[7] = color + 40;
  Serial2.print(buf);
  Serial.println("Sent " + String(buf) + " " + String(x) + " " + String(y) + " " + String(w) + " " + String(color));
  drawDelay();
}

void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {  
  char buf[20];
  sprintf(buf, "!A C0000\r\n");
  buf[4] = x0 + 40; buf[5] = y0 + 40;
  buf[6] = r + 40;
  buf[7] = color + 40;
  Serial2.print(buf);
  Serial.println("Sent " + String(buf) + " " + String(x0) + " " + String(y0) + " " + String(r) + " " + String(color));
  drawDelay();
}

void clearDisplay() {  
  char buf[20];
  sprintf(buf, "!A X0000\r\n");
  Serial2.print(buf);
  Serial.println("Sent " + String(buf));
  drawDelay();
}

void sendCameraStatus(byte s, byte ip0, byte ip1, byte ip2, byte ip3) {
  char buf[20];
  sprintf(buf, "!A R00000\r\n");
  buf[4] = s + 40;
  buf[5] = ip0 + 40; buf[6] = ip1 + 40;
  buf[7] = ip2 + 40; buf[8] = ip3 + 40;
  Serial2.print(buf);
  Serial.println("Sent " + String(buf));
  drawDelay();
}

#define x_lower_left_coordinate 0
#define y_lower_left_coordinate 55
#define chart_width 123
#define chart_height 55
#define x_drawing_offset 4

double _previous_x_coordinate, _previous_y_coordinate[6];    //Previous point coordinates
double _y_min_value, _y_max_value;                           //Y axis Min and max values
double _x_min_value, _x_max_value;                           //X axis Min and max values
double _x_inc;
int _x_index;

void initValues() {
  _previous_x_coordinate = x_lower_left_coordinate;
  for (int i=0; i < 6; i++)
    _previous_y_coordinate[i] = y_lower_left_coordinate;
  _y_min_value = 0; _y_max_value = 100;
  _x_min_value = 0; _x_max_value = 24;
  _x_index = 0; _x_inc = (chart_width - x_drawing_offset) / 10.0;
}

void resetValues() {
  _previous_x_coordinate = x_lower_left_coordinate;
  for (int i=0; i < 6; i++)
    _previous_y_coordinate[i] = y_lower_left_coordinate;
  _x_index = 0; 
}

void drawChartAxis() {
  int i = 0;

  int16_t x, y;
  uint16_t w, h;
  if (_y_max_value > 500000) {
    _y_max_value = 500000;
    Serial.println("!!Max Reached 1!!");
  }
  // high label
  drawText(x_lower_left_coordinate + 10, y_lower_left_coordinate - chart_height, String(_y_max_value, 1).c_str());

  float yinc_div = chart_height / 10;
  float xinc_div = (chart_width - x_drawing_offset) / 10;

  // draw x divisions
  for (i = 0; i <= chart_width - x_drawing_offset; i += xinc_div)
  {
      float temp = (i) + x_lower_left_coordinate + x_drawing_offset;
      if (i == 0)
      {
          drawFastVLine(temp, y_lower_left_coordinate - chart_height, chart_height + 3, WHITE);
      }
      else
      {
          drawFastVLine(temp, y_lower_left_coordinate, 3, WHITE);
      }
  }
  // draw y divisions
  for (i = y_lower_left_coordinate; i <= y_lower_left_coordinate + chart_height; i += yinc_div)
  {
      float temp = (i - y_lower_left_coordinate) * (y_lower_left_coordinate - chart_height - y_lower_left_coordinate) / (chart_height) + y_lower_left_coordinate;
      if (i == y_lower_left_coordinate)
      {
          drawFastHLine(x_lower_left_coordinate - 3 + x_drawing_offset, temp, chart_width + 3 - x_drawing_offset, WHITE);
      }
      else
      {
          drawFastHLine(x_lower_left_coordinate - 3 + x_drawing_offset, temp, 3, WHITE);
      }
  }
}

float getActualX() {
  return x_lower_left_coordinate + _x_index * _x_inc;
}

bool chartFull() {
  float actual_x_coordinate = getActualX();
  if (actual_x_coordinate >= x_lower_left_coordinate + chart_width - x_drawing_offset) {
      return true;
  }
  return false;  
}

void printIntensity(int value, int index) {
  float actual_x_coordinate = getActualX();
  if (chartFull()) {
      return;
  }
  float firstValue = value;
  if (firstValue < _y_min_value)
    firstValue = _y_min_value;
  if (firstValue > _y_max_value)
    firstValue = _y_max_value;
  double y = (firstValue - _y_min_value) * (y_lower_left_coordinate - chart_height - y_lower_left_coordinate) / (_y_max_value - _y_min_value) + y_lower_left_coordinate;
  drawLine(_previous_x_coordinate + x_drawing_offset, _previous_y_coordinate[index], actual_x_coordinate + x_drawing_offset, y, WHITE);

  if (index == 1) {
    fillCircle(actual_x_coordinate + x_drawing_offset, y, 2, WHITE);
  }
  _previous_y_coordinate[index] = y;
}

void saveIntensities(int res[6]) {
  if (sdcard_present == 0) {
    return;
  }
  String path = "/results-1.txt";
  fs::FS &fs = SD_MMC;
  File file = fs.open(path.c_str(), FILE_APPEND);
  if(!file){
    DPRINTLN("Failed to open file in appending mode");
  } 
  else {
    for (int i = 0; i < 6; i++) {
      if (i > 0) { file.write('\t'); }
      file.print(res[i]);
    }
    file.println();
  }
  file.close();
}

int getLineNo() {
  if (sdcard_present == 0) {
    return 0;
  }
  String path = "/results-1.txt";
  fs::FS &fs = SD_MMC;
  int num = 0;
  File file = fs.open(path.c_str(), FILE_READ);
  if(!file){
    return num;
  } 
  else {
    while (file.available()) {
      int ch = file.read();
      if (ch == '\n') {
        num += 1;
      }
    }
  }
  file.close();
  return num;
}

int getMaxResults() {
  int num = 0;
  if (sdcard_present == 0) {
    return num;
  }
  String path = "/results-1.txt";
  fs::FS &fs = SD_MMC;
  File file = fs.open(path.c_str(), FILE_READ);
  if(!file){
    return num;
  } 
  else {
    while (file.available()) {
      int v = file.parseInt();
      num = max(num, v);
    }
  }
  file.close();
  return num;
}

void resetResultFile() {
  if (sdcard_present == 0) {
    return;
  }
  String path = "/results-1.txt";
  fs::FS &fs = SD_MMC;
  if (fs.exists(path)) {
    fs.remove(path);
  }
  File file = fs.open(path.c_str(), FILE_WRITE);
  file.close();
}

void printIntensities(int res[6]) {
  float actual_x_coordinate = getActualX();
  for (int i =0; i < 6; i++) {
    printIntensity(res[i], i);
  }
  _previous_x_coordinate = actual_x_coordinate;
}

void printHistory(double max_y_val = 100) {
  clearDisplay();
  initValues();
  int line_num = getLineNo();
  int maxVal = getMaxResults();
  if (maxVal > 500000) {
    maxVal = 500000;
    Serial.println("!!Max Reached 2!!");
  }
  _y_max_value = max(max_y_val, (double)maxVal) * 3/2.0;
  _x_max_value = max(line_num, 24) * 3/2.0;
  _x_inc = (chart_width - x_drawing_offset) / _x_max_value;
  drawChartAxis();
  if (line_num <= 0) {
    return;
  }
  String path = "/results-1.txt";
  fs::FS &fs = SD_MMC;
  int num = 0;
  int numInc = (int) (line_num/10.0 + 1.5);
  int indexSample = 0;
  File file = fs.open(path.c_str(), FILE_READ);
  if(file){
    while (file.available()) {
      int v[6];
      for (int i = 0; i < 6; i++) {
        if (file.available()) {
          v[i] = file.parseInt();
        }
        else {
          v[0] = -1;
          break;
        }
      }
      if (v[0] == -1) {
        break;
      }
      if (num == indexSample) {
        printIntensities(v);
        _x_index += numInc;
        indexSample += numInc;
      }
      num += 1;
    }
  }
  file.close();
  _x_index = line_num;
}

void updateFileChart(int res[6]) {
  int maxVal = res[0];
  for (int i = 0; i < 6; i++) {
    maxVal = max(res[i],maxVal);
  }
  if (chartFull() || maxVal > _y_max_value) {
    printHistory(maxVal);
  }
  saveIntensities(res);
  printIntensities(res);
  _x_index += 1;
}

void resetChart(CommandParameter &parameter) {
  const char *value = parameter.NextParameter();
  Serial.println(value);
  printHistory();
}

bool recordData(int num) {
  String path = "/picture-" + String(num) +".jpg";
  camera_fb_t * fb = readImageFile(path);
  if (fb) {
    int res[6];
    getIntensities(fb, res);
    updateFileChart(res);
    free(fb->buf);
    free(fb);
    return true;
  }
  return false; 
}

void deleteDirs(File dir, int numTabs) {
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      deleteDirs(entry, numTabs + 1);
      SD_MMC.rmdir(entry.name());
    }
    else {
      Serial.println("");
      if(strcmp(jquery, entry.name())!=0) {
        SD_MMC.remove(entry.name());
      }
    }
    entry.close();
  }
}

void deleteAllFiles() {
  Serial.println("Deleting all files");
  if (sdcard_present == 0) {
    Serial.println("no SD card");
    return;
  }
  fs::FS &fs = SD_MMC;
  deleteDirs(fs.open("/"), 0);
}

void loopRecord() {
  if (recordPictures == 1) {
    setLED(1);
    savePicture(pictureNumber);
    savePicture(pictureNumber);
    setLED(0);
    recordData(pictureNumber);
    pictureNumber++;
  }
}

void setup() {
  
#ifdef DEBUG
  // Serial port for debugging purposes
  Serial.begin(115200);
#endif

  // Connect to Wi-Fi as Client
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    DPRINTLN("Connecting to WiFi...");
  }

  _s_ip = WiFi.localIP();

  // Connect to Wi-Fi as AccessPoint
  //WiFi.softAP(apssid, appassword);
  //_s_ip = WiFi.softAPIP();

  // Print ESP32 Local IP Address
  DPRINT("IP Address: http://");
  DPRINTLN(_s_ip);
  
  // Turn-off the 'brownout detector'
  // SD card working depends on this line
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  pinMode(15, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(4, OUTPUT);
  pinMode(13, OUTPUT);
  digitalWrite(4, HIGH);
  digitalWrite(13, HIGH);
  
  if(!SD_MMC.begin("/sdcard", true)){ // Turns off LED Flash
    DPRINTLN("SD Card Mount Failed");
    sdcard_present = 0;
  }
  
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    DPRINTLN("No SD Card attached");
    sdcard_present = 0;
  }

  // OV2640 camera module
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
#ifdef DEBUG
    Serial.printf("Camera init failed with error 0x%x", err);
#endif
    ESP.restart();
  }

  // Route for root / web page
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", index_html);
  });

  // Route for root / web page
  server.on("/led", HTTP_GET, []() {
    int paramsNr = server.args();
    int num = 0;
    for(int i=0;i<paramsNr;i++){
        String name1 = server.argName(i);
        if (name1 == "num") {
          num = server.arg(i).toInt();
        }
    }
    num = setLED(num);
    String msg = "LED " + String(num);
    server.send(200, "text/plain", msg);
  });

  server.on("/capture-photo", HTTP_GET, []() {
    camera_fb_t * fb = NULL; // pointer
    fb = esp_camera_fb_get();
    if (!fb) {
      DPRINTLN("Camera capture failed");
      return;
    }
    DPRINTLN("Camera captured: " + String(fb->len));
    String hdr = "Content-Disposition: inline; filename=capture.jpg\r\n";
    server.send(200, "image/jpeg", hdr, (char*) fb->buf, fb->len);
    esp_camera_fb_return(fb);
  });

  // Route for root / web page
  server.on("/save-photo", HTTP_GET, []() {
    int paramsNr = server.args();
    int num = 0;
    for(int i=0;i<paramsNr;i++){
        String name1 = server.argName(i);
        if (name1 == "num") {
          num = server.arg(i).toInt();
        }
    }
    setLED(1);
    savePicture(num);
    savePicture(num);
    setLED(0);
    if (sdcard_present == 1) {
      String msg = "Saved pictureNumber " + String(num);
      server.send(200, "text/plain", msg);
    }
    else {
      String msg = "No SD Card pictureNumber " + String(num);
      server.send(200, "text/plain", msg);      
    }
  });

  // Route for root / web page
  server.on("/record-start", HTTP_GET, []() {
    resetResultFile();
    recordPictures = 1;
    String msg = "Record start " + String(pictureNumber);
    server.send(200, "text/plain", msg);
  });

  // Route for root / web page
  server.on("/record-stop", HTTP_GET, []() {
    recordPictures = 0;
    String msg = "Record stop " + String(pictureNumber);
    server.send(200, "text/plain", msg);
  });

  // Route for root / web page
  server.on("/reset", HTTP_GET, []() {
    int paramsNr = server.args();
    int num = 0;
    for(int i=0;i<paramsNr;i++){
        String name1 = server.argName(i);
        if (name1 == "num") {
          num = server.arg(i).toInt();
        }
    }
    pictureNumber = num;
    String msg = "Reset pictureNumber " + String(pictureNumber);
    server.send(200, "text/plain", msg);
  });

  // Route for root / web page
  server.on("/get-info", HTTP_GET, []() {
    int paramsNr = server.args();
    int num = 0;
    for(int i=0;i<paramsNr;i++){
        String name1 = server.argName(i);
        if (name1 == "num") {
          num = server.arg(i).toInt();
        }
    }
    String msg = "pictureNumber: " + String(pictureNumber) + "\n";
    msg += "recordPictures: " + String(recordPictures) + "\n";
    msg += "SD Card: " + String(sdcard_present) + "\n";
    if (num >= 0) {
      String path = "/picture-" + String(num) +".jpg";
      camera_fb_t * fb = readImageFile(path);
      if (fb) {
        int res[6];
        getIntensities(fb, res);
        msg += "intensity: " + String(res[0]) + "\n";
        free(fb->buf);
        free(fb);
      }
      else {
        msg += "File not found\n";
     }
    }
    server.send(200, "text/plain", msg);
  });

  server.on("/get-photo", HTTP_GET, []() {
    int paramsNr = server.args();
    DPRINTLN(paramsNr);
    int num = -1;
    for(int i=0;i<paramsNr;i++){
        String name1 = server.argName(i);
        String value = server.arg(i);
        DPRINT("Param name: ");
        DPRINTLN(name1);
        DPRINT("Param value: ");
        DPRINTLN(value);
        DPRINTLN("------");
        if (name1 == "num") {
          num = value.toInt();
        }
    }
    if (num >= 0) {
      String path = "/picture-" + String(num) +".jpg";
      server.send(SD_MMC, path, "image/jpeg", false);
      /*
      camera_fb_t * fb = readImageFile(path);
      if (fb) {
        DPRINTLN("Path: " + path + " Size:" + String(fb->len));
        String hdr = "Content-Disposition: inline; filename=" + path + "\r\n";
        server.send(200, "image/jpeg", hdr, (char*) fb->buf, fb->len);
        free(fb->buf);
        free(fb);
      }
      */
    }
    else {
      String path = "/picture-" + String(num) +".jpg";
      server.send(500, "text/plain", String(path + " not found"));
    }
  });
  
  server.on("/download", HTTP_GET, []() {
    int paramsNr = server.args();
    DPRINTLN(paramsNr);
    int num = -1;
    String filename = "";
    for(int i=0;i<paramsNr;i++){
        String name1 = server.argName(i);
        String value = server.arg(i);
        DPRINT("Param name: ");
        DPRINTLN(name1);
        DPRINT("Param value: ");
        DPRINTLN(value);
        DPRINTLN("------");
        if (name1 == "num") {
          num = value.toInt();
        }
        if (name1 == "file") {
          filename = value;
        }
    }
    if (num >= 0) {
      String path = "/picture-" + String(num) +".jpg";
      server.send(SD_MMC, path, "image/jpeg", true);
    }
    else {
      String path = "/" + filename;
      server.send(SD_MMC, path, "text/plain", true);
    }
  });
  
  server.on("/plot-data", HTTP_GET, []() {
    int paramsNr = server.args();
    DPRINTLN(paramsNr);
    int start_num = 0;
    int end_num = pictureNumber - 1;
    String filename = "";
    for(int i=0;i<paramsNr;i++){
        String name1 = server.argName(i);
        String value = server.arg(i);
        DPRINT("Param name: ");
        DPRINTLN(name1);
        DPRINT("Param value: ");
        DPRINTLN(value);
        DPRINTLN("------");
        if (name1 == "start") {
          start_num = value.toInt();
        }
        if (name1 == "end") {
          end_num = value.toInt();
        }
    }
    resetResultFile();
    clearDisplay();
    initValues();
    drawChartAxis();
    for (int i = start_num; i <= end_num; i++) {
       recordData(i);
    }
    server.send(200, "text/plain", "Done");
  });

  server.on("/delete-all-data", HTTP_GET, []() {
    deleteAllFiles();
    server.send(200, "text/plain", "Done");
  });

  server.on("/jquery", HTTP_GET, []() {
    if (sdcard_present) {
      server.send(SD_MMC, jquery, "text/plain", true);
    }
    else {
      String msg = "No SD Card";
      DPRINTLN(msg);
      server.send(404, "text/plain", msg);
    }
  });

  /*handling uploading file */
  server.on("/update", HTTP_POST, [](){
    server.sendHeader("Connection", "close");
  },[](){
    HTTPUpload& upload = server.upload();
    if(opened == false){
      opened = true;
      fs::FS &fs = SD_MMC;
      root = fs.open((String("/") + upload.filename).c_str(), FILE_WRITE);  
      if(!root){
        Serial.println("- failed to open file for writing");
        return;
      }
    } 
    if(upload.status == UPLOAD_FILE_WRITE){
      if(root.write(upload.buf, upload.currentSize) != upload.currentSize){
        Serial.println("- failed to write");
        return;
      }
    } else if(upload.status == UPLOAD_FILE_END){
      root.close();
      Serial.println("UPLOAD_FILE_END");
      opened = false;
    }
  });
  
  // Start server
  server.begin();

  pinMode(LED_PIN, OUTPUT); //LED
  setLED(0);
  
  pinMode(TXD2, INPUT_PULLUP);
  pinMode(RXD2, INPUT_PULLUP);
    
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  scheduler.attach(10, loopRecord);
  //resetResultFile(); // reset result file when record start
  clearDisplay();
  initValues();
  drawChartAxis();
  if (sdcard_present == 1) {
    sendCameraStatus(1, _s_ip[0], _s_ip[1], _s_ip[2], _s_ip[3]);
  }
  else {
    sendCameraStatus(2, _s_ip[0], _s_ip[1], _s_ip[2], _s_ip[3]);
  }
  SerialCommandHandler.AddCommand(F("R"), resetChart);
}

void loopSerial() {
  while(Serial2.available()){
    size_t l = Serial2.available();
    uint8_t b[l];
    l = Serial2.read(b, l);
    Serial.write(b, l);
  }
}
  
void loop(void) {
  server.handleClient();
  delay(2);//allow the cpu to switch to other tasks
  //loopSerial();
  SerialCommandHandler.Process();
}
