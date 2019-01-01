#include <SPI.h>
#include <math.h>

#include "epd4in2.h"


// CONSTS

// screen: widths, segments, etc.
const int SCREEN_WIDTH = 400;
const int SCREEN_HEIGHT = 300;
const int SEGMENT_WIDTH = 400;
const int SEGMENT_HEIGHT = 30;
const int SEGMENT_NUM_PIXELS = SEGMENT_WIDTH * SEGMENT_HEIGHT;
const int SEGMENT_NUM_BYTES = (SEGMENT_NUM_PIXELS/8)+1;  // it's a bitbuffer
const int NUM_X_SEGMENTS = SCREEN_WIDTH/SEGMENT_WIDTH;
const int NUM_Y_SEGMENTS = SCREEN_HEIGHT/SEGMENT_HEIGHT;

// pendulum: frequencies
const float f1 = 1.005;
const float f2 = 2.990;
const float f3 = 1.000;

// pendulum: dampenings
const float d1 = 2.0/1000;
const float d2 = 1.0/1000;
const float d3 = 0.5/1000;

// pendulum: phase shifts (radians)
const float p1x = 95.0/180.0 * M_PI;
const float p2x = 180.0/180.0 * M_PI;
const float p3x = 120.0/180.0 * M_PI;
const float p1y = 180.0/180.0 * M_PI;
const float p2y = 180.0/180.0 * M_PI;
const float p3y = 180.0/180.0 * M_PI;

// pendulum: weightings
const float w1x = 2.8;
const float w2x = 1.5;
const float w3x = 1.5;
const float w1y = 3.0;
const float w2y = 1.0;
const float w3y = 0.8;

// screen: coordinate mappings
const float RESULT_X_MAX = 2 * (w1x + w2x + w3x);
const float RESULT_Y_MAX = 2 * (w1y + w2y + w3y);
const float X_RESCALE = SCREEN_WIDTH/RESULT_X_MAX;
const float Y_RESCALE = SCREEN_HEIGHT/RESULT_Y_MAX;


// TYPES

struct Point {
  float x;
  float y;
};

struct PixelPoint {
  int x;
  int y;
};

struct Rect {
  int x;
  int y;
  int w;
  int h;
};

struct AppState {
  Epd* epd;
  uint8_t bitbuf[SEGMENT_NUM_BYTES];
};


// GLOBALS

AppState* appstate = nullptr;


// FUNCS

Point calc_math_coordinates(float t, float p2y) {
  float d1t = exp(-d1*t);
  float d2t = exp(-d2*t);
  float d3t = exp(-d3*t);
  
  float x1 = w1x * sin(t*f1 + p1x) * d1t;
  float x2 = w2x * sin(t*f2 + p2x) * d2t;
  float x3 = w3x * sin(t*f3 + p3x) * d3t;

  float y1 = w1y * sin(t*f1 + p1y) * d1t;
  float y2 = w2y * sin(t*f2 + p2y) * d2t;
  float y3 = w3y * sin(t*f3 + p3y) * d3t;

  return {
    .x = x1 + x2 + x3,
    .y = y1 + y2 + y3,
  };
}

PixelPoint calc_pixel_coordinates(float t, float p2y) {
  Point p = calc_math_coordinates(t, p2y);

  // pixel coordinates must be positive
  p.x += (w1x + w2x + w3x);
  p.y += (w1y + w2y + w3y);

  return {
    .x = (int)(p.x * X_RESCALE),
    .y = (int)(p.y * Y_RESCALE), 
  };
}

bool is_in(const Rect& bounds, int x, int y) {
  if (bounds.x <= x && x < bounds.x + bounds.w &&
      bounds.y <= y && y < bounds.y + bounds.h) {
    return true;
  } else {
    return false;
  }
}

void paint_pixel_into_bitbuf(const Rect& bounds, int x, int y) {
  if (is_in(bounds, x, y)) {
    x -= bounds.x;
    y -= bounds.y;
    int bitidx = x + y*bounds.w;
        
    appstate->bitbuf[bitidx/8] &= ~(0x80 >> (bitidx & 7));
  }
}

void paint_line_breshenham_low(const Rect& bounds, const PixelPoint& p1, const PixelPoint& p2) {
  // covers gradients [-1, 1] (base alg. only covers gradient of > 0)
  int dx = p2.x - p1.x;
  int dy = p2.y - p1.y;
  int yi = 1;
  if (dy < 0) {
    yi = -1;
    dy = -dy;
  }
  int D = 2*dy - dx;
  int y = p1.y;

  for (int x = p1.x; x < p2.x; ++x) {
    paint_pixel_into_bitbuf(bounds, x, y);
    if (D > 0) {
      y += yi;
      D -= 2*dx;
    }
    D += 2*dy;
  }
}

void paint_line_breshenham_high(const Rect& bounds, const PixelPoint& p1, const PixelPoint& p2) {
  // handles "steep" gradients
  int dx = p2.x - p1.x;
  int dy = p2.y - p1.y;
  int xi = 1;
  if (dx < 0) {
    xi = -1;
    dx = -dx;
  }
  int D = 2*dx - dy;
  int x = p1.x;

  for (int y = p1.y; y < p2.y; ++y) {
    paint_pixel_into_bitbuf(bounds, x, y);
    if (D > 0) {
      x += xi;
      D -= 2*dy;
    }
    D += 2*dx;
  }
}

void paint_line_bresenham(const Rect& bounds, const PixelPoint& p1, const PixelPoint& p2) {
  // covers wide & tall representations with [-1,1] gradients: base
  // alg only covers wide representations with positive gradient
  
  if (abs(p2.y - p1.y) < abs (p2.x - p1.x)) {
    // wider than it is tall (not steep)
    if (p1.x > p2.x) {
      paint_line_breshenham_low(bounds, p2, p1);
    } else {
      paint_line_breshenham_low(bounds, p1, p2);
    }
  } else {
    // taller than it is wide (steep)
    if (p1.y > p2.y) {
      paint_line_breshenham_high(bounds, p2, p1);
    } else {
      paint_line_breshenham_high(bounds, p1, p2);
    }
  }
}

void render_line_into_bitbuf(
  const Rect& bounds,
  const PixelPoint& p1,
  const PixelPoint& p2) {
  
  // Using a custom line painting alg. because of the screen
  // segmentation. Line coords might fall out of bounds.
  paint_line_bresenham(bounds, p1, p2);
}

void zero_bitbuf() {
  for (size_t i = 0; i < SEGMENT_NUM_BYTES; ++i) {
    appstate->bitbuf[i] = 0xff;
  }
}

void render_bitbuf(const Rect& screen_bounds) {
  appstate->epd->SetPartialWindow(
    appstate->bitbuf,
    screen_bounds.x,
    screen_bounds.y,
    screen_bounds.w,
    screen_bounds.h);
}

static void render_segment(float p2y, const Rect& segment_bounds) {
  zero_bitbuf();
  
  PixelPoint p1 = calc_pixel_coordinates(0.0, p2y);
  
  for (float t = 0.15; t < 500; t += 0.15) {
    PixelPoint p2 = calc_pixel_coordinates(t, p2y);
    render_line_into_bitbuf(segment_bounds, p1, p2);
    p1 = p2;
  }

  render_bitbuf(segment_bounds);
}

static void render_screen(float p2y) {  
  for (int xs = NUM_X_SEGMENTS-1; xs >= 0; --xs) {
    for (int ys = NUM_Y_SEGMENTS-1; ys >= 0; --ys) {
      Rect segment = {
        .x = SEGMENT_WIDTH * xs,
        .y = SEGMENT_HEIGHT * ys,
        .w = SEGMENT_WIDTH,
        .h = SEGMENT_HEIGHT,
      };

      render_segment(p2y, segment);
    }
  }
}

static void render_clear() {
  appstate->epd->ClearFrame();
}

static void present_screen() {
  appstate->epd->DisplayFrame();
}

static void run(AppState* st) {
  appstate = st;

  render_clear();
  present_screen();

  for (size_t i = 0; ; i += 20) {    
    float p2y = (i/180.0) * M_PI;
    render_screen(p2y);
    present_screen();
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  AppState st = {0};
  Epd epd;

  if (epd.Init() != 0) {
    Serial.print("e-Paper init failed");
    return;
  }
  
  st.epd = &epd;
  
  run(&st);
}

void loop() {
}
