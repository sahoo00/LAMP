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

class SChart: public Adafruit_SSD1306 {
  private:
    double _previous_x_coordinate[2], _previous_y_coordinate[2]; //Previous point coordinates
    double _x_lower_left_coordinate, _y_lower_left_coordinate;   //Chart lower left coordinates
    double _chart_width, _chart_height;                          //Chart width and height
    double _y_min_values[2], _y_max_values[2];                   //Y axis Min and max values
    double _x_min_value, _x_max_value;                           //X axis Min and max values
    double _x_inc;                                               //X coordinate increment between values
    double _actual_x_coordinate;                                 //Actual point x coordinate
    double _xinc_div, _yinc_div;                                 //X and Y axis distance between division
    double _dig;
    char _mode;               //Plot mode: single or double
    char _point_geometry[2];  //Point geometry
    bool _y_labels_visible;   //Determines if the y labels should be shown
    char *_y_min_label[2];    //Labels of the lower y value
    char *_y_max_label[2];    //Labels of the higher y value
    double _x_drawing_offset; //Used to draw the char after the labels are applied
    bool _mid_line_visible;   //Determines if the mid line should be shown in Double plot mode
    char _lines_thickness[2]; //Line thickness
    void _drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color, uint8_t thickness);
    
  public:
  SChart() { 
        _mode = SINGLE_PLOT_MODE;
        _point_geometry[0] = POINT_GEOMETRY_NONE;
        _point_geometry[1] = POINT_GEOMETRY_NONE;
        _y_labels_visible = false;
        _mid_line_visible = true;
        _lines_thickness[0] = NORMAL_LINE;
        _lines_thickness[1] = NORMAL_LINE;
        _y_min_label[0] = "";
        _y_min_label[1] = "";
        _y_max_label[0] = "";
        _y_max_label[1] = "";
  }

  SChart(uint8_t w, uint8_t h, TwoWire *twi, int8_t rst_pin = -1,
                       uint32_t clkDuring = 400000UL, uint32_t clkAfter = 100000UL) : Adafruit_SSD1306(w, h, twi, rst_pin, clkDuring, clkAfter)
    {
        _mode = SINGLE_PLOT_MODE;
        _point_geometry[0] = POINT_GEOMETRY_NONE;
        _point_geometry[1] = POINT_GEOMETRY_NONE;
        _y_labels_visible = false;
        _mid_line_visible = true;
        _lines_thickness[0] = NORMAL_LINE;
        _lines_thickness[1] = NORMAL_LINE;
        _y_min_label[0] = "";
        _y_min_label[1] = "";
        _y_max_label[0] = "";
        _y_max_label[1] = "";
    }

    SChart(uint8_t w, uint8_t h, int8_t mosi_pin, int8_t sclk_pin,
                       int8_t dc_pin, int8_t rst_pin, int8_t cs_pin) : Adafruit_SSD1306(w, h, mosi_pin, sclk_pin, dc_pin, rst_pin, cs_pin)
    {
        _mode = SINGLE_PLOT_MODE;
        _point_geometry[0] = POINT_GEOMETRY_NONE;
        _point_geometry[1] = POINT_GEOMETRY_NONE;
        _y_labels_visible = false;
        _mid_line_visible = true;
        _lines_thickness[0] = NORMAL_LINE;
        _lines_thickness[1] = NORMAL_LINE;
        _y_min_label[0] = "";
        _y_min_label[1] = "";
        _y_max_label[0] = "";
        _y_max_label[1] = "";
    }

    SChart(uint8_t w, uint8_t h, SPIClass *spi,
                       int8_t dc_pin, int8_t rst_pin, int8_t cs_pin, uint32_t bitrate = 8000000UL) : Adafruit_SSD1306(w, h, spi, dc_pin, rst_pin, cs_pin, bitrate)
    {
        _mode = SINGLE_PLOT_MODE;
        _point_geometry[0] = POINT_GEOMETRY_NONE;
        _point_geometry[1] = POINT_GEOMETRY_NONE;
        _y_labels_visible = false;
        _mid_line_visible = true;
        _lines_thickness[0] = NORMAL_LINE;
        _lines_thickness[1] = NORMAL_LINE;
        _y_min_label[0] = "";
        _y_min_label[1] = "";
        _y_max_label[0] = "";
        _y_max_label[1] = "";
    }
    
    void setPlotMode(char mode);
    void setChartCoordinates(double x, double y);
    void setChartWidthAndHeight(double w, double h);
    void setXIncrement(double xinc);
    double getXIncrement() { return _x_inc; }
    void setAxisDivisionsInc(double xinc, double yinc);
    void setMidLineVisible(bool lineVisible);
    void setYLabelsVisible(bool yLabelsVisible);
    void setYLimits(double ylo, double yhi, uint8_t chart = 0);
    double getYMin(uint8_t chart = 0) { return _y_min_values[chart]; }
    double getYMax(uint8_t chart = 0) { return _y_max_values[chart]; }
    void setYLimitLabels(char *loLabel, char *hiLabel, uint8_t chart = 0);
    void setXLimits(double xlo, double xhi);
    double getXMin() { return _x_min_value; }
    double getXMax() { return _x_max_value; }
    void setPointGeometry(char pointGeometry, uint8_t chart = 0);
    void setLineThickness(char thickness, uint8_t chart = 0);

    void drawChart();
    bool updateChart(double firstValue, double secondValue = 0);

/*
    void setTextSize(int i) { _display.setTextSize(i);}
    void setTextColor(uint16_t c) { _display.setTextColor(c);}
    void getTextBounds(const char *string, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
      _display.getTextBounds(string, x, y, x1, y1, w, h);
      }
    void setCursor(int16_t x, int16_t y) { _display.setCursor(x, y); }
    void write(char * s) { _display.write(s);}
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) { _display.drawFastVLine(x, y, h, color);}
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) { _display.drawFastHLine(x, y, w, color);}
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) { _display.drawLine(x0, y0, x1, y1, color);}
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) { _display.fillCircle(x0, y0, r, color);}
    void clearDisplay() { _display.clearDisplay(); }
    */
};

/*!
    @brief  Updates the internal buffer to draw the cartesian graph
    @note   Call the object's begin() function before use -- buffer allocation is performed there!
            Call the object's configureChart() function before use -- params are updated there
*/
void SChart::drawChart()
{
    double i, temp;
    _dig = 0;
    _previous_x_coordinate[0] = _x_lower_left_coordinate;
    _previous_x_coordinate[1] = _x_lower_left_coordinate;
    _previous_y_coordinate[0] = _y_lower_left_coordinate;
    _previous_y_coordinate[1] = _y_lower_left_coordinate - _chart_height / 2;
    _actual_x_coordinate = _x_lower_left_coordinate;
    _x_drawing_offset = 0;

    if (_y_labels_visible)
    {
        if (_mode == SINGLE_PLOT_MODE || _mode == ALL_PLOT_MODE)
        {
            int16_t x, y;
            uint16_t w, h;

            setTextSize(1);
            setTextColor(WHITE);

            getTextBounds(_y_max_label[0], _x_lower_left_coordinate + 5, _y_lower_left_coordinate + 5 - _chart_height, &x, &y, &w, &h);
            _x_drawing_offset = w;

            // high label
            setCursor(_x_lower_left_coordinate, _y_lower_left_coordinate - _chart_height);
            write(_y_max_label[0]);

            getTextBounds(_y_min_label[0], _x_lower_left_coordinate + 5, _y_lower_left_coordinate - 5, &x, &y, &w, &h);

            // low label
            setCursor(_x_lower_left_coordinate, _y_lower_left_coordinate - (h / 2));
            write(_y_min_label[0]);

            if (w > _x_drawing_offset)
            {
                _x_drawing_offset = w;
            }

            // compensation for the y axis tick lines
            _x_drawing_offset += 4;
        }
        else if (_mode == DOUBLE_PLOT_MODE)
        {
            int16_t x, y;
            uint16_t w, h;

            setTextSize(1);
            setTextColor(WHITE);
            //Chart 1
            getTextBounds(_y_max_label[1], _x_lower_left_coordinate + 5, _y_lower_left_coordinate + 5 - _chart_height, &x, &y, &w, &h);
            _x_drawing_offset = w;

            // high label
            setCursor(_x_lower_left_coordinate, _y_lower_left_coordinate - _chart_height);
            write(_y_max_label[1]);

            getTextBounds(_y_min_label[1], _x_lower_left_coordinate + 5, _y_lower_left_coordinate - 5, &x, &y, &w, &h);

            // low label
            setCursor(_x_lower_left_coordinate, _y_lower_left_coordinate - _chart_height / 2 - (h / 2));
            write(_y_min_label[1]);

            if (w > _x_drawing_offset)
            {
                _x_drawing_offset = w;
            }

            //Chart 0
            getTextBounds(_y_max_label[0], _x_lower_left_coordinate + 5, _y_lower_left_coordinate + 5 - _chart_height, &x, &y, &w, &h);
            if (w > _x_drawing_offset)
            {
                _x_drawing_offset = w;
            }

            // high label
            setCursor(_x_lower_left_coordinate, _y_lower_left_coordinate - _chart_height / 2 + (h / 2));
            write(_y_max_label[0]);

            getTextBounds(_y_min_label[0], _x_lower_left_coordinate + 5, _y_lower_left_coordinate - 5, &x, &y, &w, &h);

            // low label
            setCursor(_x_lower_left_coordinate, _y_lower_left_coordinate - (h / 2));
            write(_y_min_label[0]);

            if (w > _x_drawing_offset)
            {
                _x_drawing_offset = w;
            }

            // compensation for the y axis tick lines
            _x_drawing_offset += 4;
        }
    }
    // draw y divisions
    for (i = _y_lower_left_coordinate; i <= _y_lower_left_coordinate + _chart_height; i += _yinc_div)
    {
        temp = (i - _y_lower_left_coordinate) * (_y_lower_left_coordinate - _chart_height - _y_lower_left_coordinate) / (_chart_height) + _y_lower_left_coordinate;
        if (i == _y_lower_left_coordinate)
        {
            drawFastHLine(_x_lower_left_coordinate - 3 + _x_drawing_offset, temp, _chart_width + 3 - _x_drawing_offset, WHITE);
        }
        else
        {
            drawFastHLine(_x_lower_left_coordinate - 3 + _x_drawing_offset, temp, 3, WHITE);
        }
    }
    // draw x divisions
    for (i = 0; i <= _chart_width - _x_drawing_offset; i += _xinc_div)
    {
        temp = (i) + _x_lower_left_coordinate + _x_drawing_offset;
        if (i == 0)
        {
            drawFastVLine(temp, _y_lower_left_coordinate - _chart_height, _chart_height + 3, WHITE);
        }
        else
        {
            drawFastVLine(temp, _y_lower_left_coordinate, 3, WHITE);
        }
    }
    if (_mid_line_visible && _mode == DOUBLE_PLOT_MODE)
    {
        drawFastHLine(_x_lower_left_coordinate + _x_drawing_offset, _y_lower_left_coordinate - _chart_height / 2, _chart_width - _x_drawing_offset, WHITE);
        for (i = 0; i <= _chart_width - _x_drawing_offset; i += _xinc_div)
        {
            drawFastVLine(i + _x_lower_left_coordinate + _x_drawing_offset, _y_lower_left_coordinate - _chart_height / 2, 3, WHITE);
        }
    }
}

void SChart::setPlotMode(char mode)
{
    if (mode == SINGLE_PLOT_MODE || mode == DOUBLE_PLOT_MODE || mode == ALL_PLOT_MODE)
        _mode = mode;
}

void SChart::setYLimits(double ylo, double yhi, uint8_t chart)
{
    if (chart == 0 || chart == 1)
    {
        _y_min_values[chart] = ylo;
        _y_max_values[chart] = yhi;
    }
}

void SChart::setXLimits(double xlo, double xhi)
{
        _x_min_value = xlo;
        _x_max_value = xhi;
}

void SChart::setLineThickness(char thickness, uint8_t chart)
{
    if (chart == 0 || chart == 1)
    {
        _lines_thickness[chart] = thickness;
    }
}

void SChart::setYLimitLabels(char *loLabel, char *hiLabel, uint8_t chart)
{
    if (chart == 0 || chart == 1)
    {
        _y_min_label[chart] = loLabel;
        _y_max_label[chart] = hiLabel;
    }
}

void SChart::setYLabelsVisible(bool yLabelsVisible)
{
    _y_labels_visible = yLabelsVisible;
}

void SChart::setMidLineVisible(bool lineVisible)
{
    _mid_line_visible = lineVisible;
}

void SChart::setPointGeometry(char pointGeometry, uint8_t chart)
{
    if (chart == 0 || chart == 1)
        _point_geometry[chart] = pointGeometry;
}

void SChart::setChartCoordinates(double x, double y)
{
    _x_lower_left_coordinate = x;
    _y_lower_left_coordinate = y;
}

void SChart::setChartWidthAndHeight(double w, double h)
{
    _chart_width = w;
    _chart_height = h;
}

void SChart::setAxisDivisionsInc(double xinc, double yinc)
{
    _xinc_div = xinc;
    _yinc_div = yinc;
}

void SChart::setXIncrement(double xinc)
{
    _x_inc = xinc;
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

bool SChart::updateChart(double firstValue, double secondValue)
{
    if (_actual_x_coordinate >= _x_lower_left_coordinate + _chart_width - _x_drawing_offset)
        return false;

    _actual_x_coordinate += _x_inc;

    if (firstValue < _y_min_values[0])
        firstValue = _y_min_values[0];

    if (firstValue > _y_max_values[0])
        firstValue = _y_max_values[0];

    if (_mode == SINGLE_PLOT_MODE)
    {
        double y = (firstValue - _y_min_values[0]) * (_y_lower_left_coordinate - _chart_height - _y_lower_left_coordinate) / (_y_max_values[0] - _y_min_values[0]) + _y_lower_left_coordinate;
        _drawLine(_previous_x_coordinate[0] + _x_drawing_offset, _previous_y_coordinate[0], _actual_x_coordinate + _x_drawing_offset, y, WHITE, _lines_thickness[0]);

        _previous_x_coordinate[0] = _actual_x_coordinate;
        _previous_y_coordinate[0] = y;

        if (_point_geometry[0] == POINT_GEOMETRY_CIRCLE)
            fillCircle(_previous_x_coordinate[0] + _x_drawing_offset, _previous_y_coordinate[0], 2, WHITE);

        display();
        return true;
    }
    else if (_mode == DOUBLE_PLOT_MODE)
    {
        if (secondValue < _y_min_values[1])
            secondValue = _y_min_values[1];

        if (secondValue > _y_max_values[1])
            secondValue = _y_max_values[1];
        auto semiHeight = _chart_height / 2;
        double y = (firstValue - _y_min_values[0]) * (-semiHeight) / (_y_max_values[0] - _y_min_values[0]) + _y_lower_left_coordinate;
        double secondY = (secondValue - _y_min_values[1]) * (-semiHeight) / (_y_max_values[0] - _y_min_values[0]) + _y_lower_left_coordinate - semiHeight;

        _drawLine(_previous_x_coordinate[0] + _x_drawing_offset, _previous_y_coordinate[0], _actual_x_coordinate + _x_drawing_offset, y, WHITE, _lines_thickness[0]);
        _drawLine(_previous_x_coordinate[1] + _x_drawing_offset, _previous_y_coordinate[1], _actual_x_coordinate + _x_drawing_offset, secondY, WHITE, _lines_thickness[1]);

        _previous_x_coordinate[0] = _actual_x_coordinate;
        _previous_y_coordinate[0] = y;
        _previous_x_coordinate[1] = _actual_x_coordinate;
        _previous_y_coordinate[1] = secondY;

        if (_point_geometry[0] == POINT_GEOMETRY_CIRCLE)
            fillCircle(_previous_x_coordinate[0] + _x_drawing_offset, _previous_y_coordinate[0], 2, WHITE);
        if (_point_geometry[1] == POINT_GEOMETRY_CIRCLE)
            fillCircle(_previous_x_coordinate[1] + _x_drawing_offset, _previous_y_coordinate[1], 2, WHITE);

        display();
        return true;
    }
    else if (_mode == ALL_PLOT_MODE)
    {
        if (secondValue < _y_min_values[0])
            secondValue = _y_min_values[0];

        if (secondValue > _y_max_values[0])
            secondValue = _y_max_values[0];
        double y = (firstValue - _y_min_values[0]) * (_y_lower_left_coordinate - _chart_height - _y_lower_left_coordinate) / (_y_max_values[0] - _y_min_values[0]) + _y_lower_left_coordinate;
        double secondY = (secondValue - _y_min_values[0]) * (_y_lower_left_coordinate - _chart_height - _y_lower_left_coordinate) / (_y_max_values[0] - _y_min_values[0]) + _y_lower_left_coordinate;

        _drawLine(_previous_x_coordinate[0] + _x_drawing_offset, _previous_y_coordinate[0], _actual_x_coordinate + _x_drawing_offset, y, WHITE, _lines_thickness[0]);
        _drawLine(_previous_x_coordinate[1] + _x_drawing_offset, _previous_y_coordinate[1], _actual_x_coordinate + _x_drawing_offset, secondY, WHITE, _lines_thickness[1]);

        _previous_x_coordinate[0] = _actual_x_coordinate;
        _previous_y_coordinate[0] = y;
        _previous_x_coordinate[1] = _actual_x_coordinate;
        _previous_y_coordinate[1] = secondY;

        if (_point_geometry[0] == POINT_GEOMETRY_CIRCLE)
            fillCircle(_previous_x_coordinate[0] + _x_drawing_offset, _previous_y_coordinate[0], 2, WHITE);
        if (_point_geometry[1] == POINT_GEOMETRY_CIRCLE)
            fillCircle(_previous_x_coordinate[1] + _x_drawing_offset, _previous_y_coordinate[1], 2, WHITE);

        display();
        return true;
    }
}

int start_chart = 0;
char actualThickness;

void init_chart(SChart display) {
  if (start_chart == 1) {
    return;
  }
  start_chart = 1;
  display.clearDisplay();
  display.setChartCoordinates(0, 60);      //Chart lower left coordinates (X, Y)
  display.setChartWidthAndHeight(123, 55); //Chart width = 123 and height = 60
  display.setXIncrement(5);                //Distance between Y points will be 5px
  display.setYLimits(0, 100);             //Ymin = 0 and Ymax = 100
  display.setYLimitLabels("0", "100");    //Setting Y axis labels
  display.setYLimits(0, 100, 1);             //Ymin = 0 and Ymax = 100
  display.setYLimitLabels("0", "100", 1);    //Setting Y axis labels
  display.setXLimits(0, 24);             //Xmin = 0 and Xmax = 24
  display.setYLabelsVisible(true);
  display.setAxisDivisionsInc(12, 6);    //Each 12 px a division will be painted in X axis and each 6px in Y axis
  display.setPlotMode(ALL_PLOT_MODE); //Set single plot mode
  display.setPointGeometry(POINT_GEOMETRY_CIRCLE, 1);
  display.setLineThickness(NORMAL_LINE, 0);
  display.setLineThickness(LIGHT_LINE, 1);
  display.drawChart(); //Update the buffer to draw the cartesian chart
  display.display();
}

double history[2];
int indexHist = 0;

void updateChart(SChart display, double value, double value2 = 0) {
  if (indexHist > 1) {
    return;
  }
  history[indexHist] = value;
  indexHist += 1;
  init_chart();
  //Value between Ymin and Ymax will be added to chart
  if (!display.updateChart(value, value2)) {
    display.clearDisplay(); //If chart is full, it is drawn again
    display.setXIncrement(display.getXIncrement() * 2 / 3);
    display.setXLimits(0, display.getXMax() * 3 / 2);
    display.drawChart();
    for (int i = 0; i < indexHist; i++) {
      display.updateChart(history[i], history[i] - 40);
    }
  }
}

