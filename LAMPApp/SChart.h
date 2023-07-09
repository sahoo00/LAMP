#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SINGLE_PLOT_MODE 0
#define DOUBLE_PLOT_MODE 1
#define ALL_PLOT_MODE 2
#define POINT_GEOMETRY_NONE 3
#define POINT_GEOMETRY_CIRCLE 4
#define POINT_GEOMETRY_DOT 5
#define POINT_GEOMETRY_SQUARE 6
#define POINT_GEOMETRY_TRIANGLE 7
#define POINT_GEOMETRY_DIAMOND 8
#define LIGHT_LINE 9
#define NORMAL_LINE 10

#define x_lower_left_coordinate 0
#define y_lower_left_coordinate 55
#define chart_width 123
#define chart_height 55
#define x_drawing_offset 1

class SChart: public Adafruit_SSD1306 {
  private:
    double _previous_x_coordinate, _previous_y_coordinate[6];    //Previous point coordinates
    double _y_min_value, _y_max_value;                           //Y axis Min and max values
    double _x_min_value, _x_max_value;                           //X axis Min and max values
    double _x_inc;
    int _x_index;
    void _drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color, uint8_t thickness);
    
  public:
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

  SChart() { initValues(); }

  SChart(uint8_t w, uint8_t h, TwoWire *twi, int8_t rst_pin = -1,
                       uint32_t clkDuring = 400000UL, uint32_t clkAfter = 100000UL) : Adafruit_SSD1306(w, h, twi, rst_pin, clkDuring, clkAfter)
    { initValues(); }

    SChart(uint8_t w, uint8_t h, int8_t mosi_pin, int8_t sclk_pin,
                       int8_t dc_pin, int8_t rst_pin, int8_t cs_pin) : Adafruit_SSD1306(w, h, mosi_pin, sclk_pin, dc_pin, rst_pin, cs_pin)
    { initValues(); }

    SChart(uint8_t w, uint8_t h, SPIClass *spi,
                       int8_t dc_pin, int8_t rst_pin, int8_t cs_pin, uint32_t bitrate = 8000000UL) : Adafruit_SSD1306(w, h, spi, dc_pin, rst_pin, cs_pin, bitrate)
    { initValues(); }
    
    void setYLimits(double ylo, double yhi) { _y_min_value = ylo; _y_max_value = yhi; }
    double getYMin() { return _y_min_value; }
    double getYMax() { return _y_max_value; }
    void setXLimits(double xlo, double xhi)  { _x_min_value = xlo; _x_max_value = xhi; }
    double getXMin() { return _x_min_value; }
    double getXMax() { return _x_max_value; }
    double getXInc() { return _x_inc; }
    void setXInc(double x) { _x_inc = x; }
    double getCurrentCORD() {  return x_lower_left_coordinate + _x_index * _x_inc;}
    double getXMaxCORD() { return x_lower_left_coordinate + chart_width - x_drawing_offset; }
    void setXIndex(int x) { _x_index = x; }
    int getXIndex() { return _x_index;}

    void drawChart();
    bool updateChart(double vals[6]);

};

/*!
    @brief  Updates the internal buffer to draw the cartesian graph
    @note   Call the object's begin() function before use -- buffer allocation is performed there!
            Call the object's configureChart() function before use -- params are updated there
*/
void SChart::drawChart()
{
    int i = 0;

    int16_t x, y;
    uint16_t w, h;

    setTextSize(1);
    setTextColor(WHITE);

    // high label
    setCursor(x_lower_left_coordinate + 10, y_lower_left_coordinate - chart_height);
    //sprintf(buffer, "%3f", _y_max_value);
    //write(buffer);
    print(_y_max_value, 1);

    float yinc_div = chart_height / 10;
    float xinc_div = (chart_width - x_drawing_offset) / 10;

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
}

void SChart::_drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color, uint8_t thickness)
{
    char linesToDraw = 0;
    if (thickness == LIGHT_LINE)
    {
        linesToDraw = 2;
        for (size_t i = 0; i < linesToDraw; i++)
            drawLine(x0, y0 - i, x1, y1, color);
    }
    else if (thickness == NORMAL_LINE)
    {
        linesToDraw = 5;
        for (size_t i = 0; i < linesToDraw; i++)
            drawLine(x0, y0 + 2 - i, x1, y1, color);
    }
}

bool SChart::updateChart(double vals[6])
{
  float actual_x_coordinate = x_lower_left_coordinate + _x_index * _x_inc;
  if (actual_x_coordinate >= x_lower_left_coordinate + chart_width - x_drawing_offset) {
      return false;
  }

  _x_index += 1;

  for (int i =0; i < 2; i++) {
    float firstValue = vals[i];
    if (firstValue < _y_min_value)
      firstValue = _y_min_value;
    if (firstValue > _y_max_value)
      firstValue = _y_max_value;
    double y = (firstValue - _y_min_value) * (y_lower_left_coordinate - chart_height - y_lower_left_coordinate) / (_y_max_value - _y_min_value) + y_lower_left_coordinate;
    _drawLine(_previous_x_coordinate + x_drawing_offset, _previous_y_coordinate[i], actual_x_coordinate + x_drawing_offset, y, WHITE, LIGHT_LINE);

    if (i == 1) {
      fillCircle(_previous_x_coordinate + x_drawing_offset, _previous_y_coordinate[i], 2, WHITE);
    }
    _previous_y_coordinate[i] = y;
  }
  _previous_x_coordinate = actual_x_coordinate;
  display();
  return true;
}

int start_chart = 0;
char actualThickness;
int indexHist = 0;

void init_chart(SChart &display) {
  display.setYLimits(0, 100);             //Ymin = 0 and Ymax = 100
  display.setXLimits(0, 24);             //Xmin = 0 and Xmax = 24
  display.drawChart(); //Update the buffer to draw the cartesian chart
  display.display();
}

void init_chart_once(SChart &display) {
  if (start_chart == 1) {
    return;
  }
  start_chart = 1;
  init_chart(display);
}

void resetChart(SChart &display) {
    double vals[6];
    display.clearDisplay(); //If chart is full, it is drawn again
    display.resetValues();
    display.drawChart();
    float xinc = display.getXInc();
    display.setXInc(xinc * indexHist/20);
    for (int i = 0; i < 20; i++) {
      int value = random(50) + 50;
      vals[0] = value;
      vals[1] = value - 40;
      display.updateChart(vals);
    }
    display.setXInc(xinc);
    display.setXIndex(indexHist);
}

void updateChart(SChart &display, double vals[6]) {
  if (indexHist > 2000) {
    return;
  }
  indexHist += 1;
  init_chart_once(display);
  //Value between Ymin and Ymax will be added to chart
  if (!display.updateChart(vals)) {
    display.setXInc(display.getXInc() * 2/3);
    display.setXLimits(0, display.getXMax() * 3 / 2);
    resetChart(display);
  }
}

