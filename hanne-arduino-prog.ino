/**
 *  @filename   :   epd4in2-demo.ino
 *  @brief      :   4.2inch e-paper display demo
 *  @author     :   Yehui from Waveshare
 *
 *  Copyright (C) Waveshare     August 4 2017
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <SPI.h>
#include "epd4in2.h"
#include "imagedata.h"
#include "epdpaint.h"


#define COLORED     0
#define UNCOLORED   1

#define X_PIXELS 128
#define Y_PIXELS 64


// frequencies
const float f1 = 0.25;//1.005;
const float f2 = 0.75;//2.99;
const float f3 = 0.25;//1;

// dampenings
const float d1 = 2.0;
const float d2 = 1.0;
const float d3 = 0.5;

// phase shifts
const int p1x = (PI/180) * 340;
const int p2x = (PI/180) * 180;
const int p3x = (PI/180) * 270;
const int p1y = (PI/180) * 180;
const int p2y = (PI/180) * 180;
const int p3y = (PI/180) * 180;

// weightings
const float A1x = 2.8;
const float A2x = 1.5;
const float A3x = 1.5;
const float A1y = 3;
const float A2y = 1;
const float A3y = 0.8;

// Answers from Hanne's eqn's are in a range that needs to be rescaled for
// pixel painting.
const float X_RESCALE = X_PIXELS/(2*(A1x + A2x + A3x));
const float Y_RESCALE = Y_PIXELS/(2*(A1y + A2y + A3y));


Epd epd;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  if (epd.Init() != 0) {
    Serial.print("e-Paper init failed");
    return;
  }
}

Paint init_paint(unsigned char* buf) {  
  Paint paint(buf, X_PIXELS, Y_PIXELS); // width should be multiple of 8
  paint.SetWidth(X_PIXELS);
  paint.SetHeight(Y_PIXELS);
  return paint;
}

void clear_screen() {
  epd.ClearFrame();
  epd.DisplayFrame();
}

void calc_math_coordinates(long t, float* xout, float* yout) {
  float x1 = A1x * sin(t*f1 + p1x) * exp(-(d1/1000)*t);
  float x2 = A2x * sin(t*f2 + p2x) * exp(-(d2/1000)*t);
  float x3 = A3x * sin(t*f3 + p3x) * exp(-(d3/1000)*t);
  
  *xout = (x1 + x2 + x3) + (A1x + A2x + A3x);

  float y1 = A1y * sin(t*f1 + p1y) * exp(-(d1/1000)*t);
  float y2 = A2y * sin(t*f2 + p2y) * exp(-(d2/1000)*t);
  float y3 = A3y * sin(t*f3 + p3y) * exp(-(d3/1000)*t);
  
  *yout = (y1 + y2 + y3) + (A1y + A2y + A3y);
}

void calc_pixel_coordinates(long t, long* xout, long* yout) {
  float x;
  float y;
  calc_math_coordinates(t, &x, &y);
  
  *xout = (int)(x * X_RESCALE);
  *yout = (int)(y * Y_RESCALE);
}

void draw_line(Paint& p, int x1, int y1, int x2, int y2) {
  p.DrawLine(x1, y1, x2, y2, COLORED);
}

void paint_eqn(Paint& p) {
  long x1;
  long y1;
  calc_pixel_coordinates(0, &x1, &y1);
  
  for (long i = 1; i < 100; ++i) {
    long t = i * 1000;
      long x2;
      long y2;
      calc_pixel_coordinates(t, &x2, &y2);     
      draw_line(p, x1, y1, x2, y2);
      x1 = x2;
      y1 = y2;
  }
}

void flush_painted_eqn_to_screen(Paint& p) {
  Serial.print("flushing");
  epd.SetPartialWindow(p.GetImage(), 100, 40, p.GetWidth(), p.GetHeight());
  epd.DisplayFrame();
}

void loop() {
  clear_screen();

  unsigned char image[1024];
  Paint p = init_paint(image);

  paint_eqn(p);

  flush_painted_eqn_to_screen(p);
  
  /* This displays an image */
  //epd.DisplayFrame(IMAGE_BUTTERFLY);

  delay(10000);
}
