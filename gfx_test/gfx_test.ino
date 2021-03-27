#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <gfxfont.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT_Macros.h>

#include <splash.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

double interpolate(double frin, double toin, double frout, double toout, double value) {
    return frout + (toout - frout) * (value - frin) / (toin - frin);  
}

int iterate(double x, double y) {
    double zr = 0;
    double zi = 0;

    int iter = 0;

    double zis = 0;
    double zrs = 0;
    while ((iter < 150) && (zrs + zis < 4)) {
        double zrn = zrs - zis + x;
            double zin = zr * zi;
            zin += zin + y;

            zr = zrn;
            zi = zin;
            zrs = zr * zr;
            zis = zi * zi;
            iter++;
    }
    return iter == 150 ? BLACK : WHITE;
}

void mandelDraw(double x0, double y0, double x1, double y1) {
  for (int row = 0; row < SCREEN_HEIGHT; row++) {
    double y = interpolate(0, SCREEN_HEIGHT, y0, y1, row);
    for (int col = 0; col < SCREEN_WIDTH; col++) {
      double x = interpolate(0, SCREEN_WIDTH, x0, x1, col);
      int color = iterate(x, y);
      display.drawPixel(col, row, color);
    } 
  }
  display.display();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello Mandelbrot.");
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();
}
 
double x01 = -4, x11 = 4, y01 = -2, y11 = 2;
double x02 = -0.118, x12 = 0.122, y02 = 0.982, y12 = 0.981;
double t = 0;
double dt = 0.02;

void loop() {
  Serial.println(t);
  double x0 = t * x01 + (1 - t) * x02;
  double x1 = t * x11 + (1 - t) * x12;
  double y0 = t * y01 + (1 - t) * y02;
  double y1 = t * y11 + (1 - t) * y12;

  mandelDraw(x0, y0, x1, y1);

  t = t + dt; 

  if (t > 1) {
    t = 1;
    dt = -dt;
  }

  if (t < 0) {
    t = 0;
    dt = -dt;
  }
}
