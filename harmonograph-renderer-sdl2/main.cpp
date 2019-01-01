#include <iostream>
#include <math.h>

#include <SDL2/SDL.h>


// CONSTS

// screen: widths, segments, etc.
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int SEGMENT_WIDTH = 50;
const int SEGMENT_HEIGHT = 50;
const int SEGMENT_NUM_PIXELS = SEGMENT_WIDTH * SEGMENT_HEIGHT;
const int SEGMENT_NUM_BYTES = (SEGMENT_NUM_PIXELS/8)+1;
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
  SDL_Window* window;
  SDL_Renderer* renderer;
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
    int bitbuf_x = x - bounds.x;
    int bitbuf_y = y - bounds.y;
    
    int bitidx = bitbuf_y*bounds.w + bitbuf_x;
    int byteidx = bitidx / 8;
    int bytebitidx = bitidx & 7;
    
    appstate->bitbuf[byteidx] |= (0x1 << bytebitidx);
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
    appstate->bitbuf[i] = 0x00;
  }
}

void render_bitbuf(const Rect& screen_bounds) {
  for (size_t x = 0; x < screen_bounds.w; ++x) {
    for (size_t y = 0; y < screen_bounds.h; ++y) {
      int bitidx = y*screen_bounds.w + x;
      int byteidx = bitidx >> 3;
      int bytebitidx = bitidx & 7;
      
      uint8_t pix = appstate->bitbuf[byteidx] & (1 << bytebitidx);
      
      if (pix) {
	SDL_SetRenderDrawColor(appstate->renderer, 0x00, 0x00, 0x00, 0x00);
      } else {
	SDL_SetRenderDrawColor(appstate->renderer, 0xff, 0xff, 0xff, 0xff);	
      }
      SDL_RenderDrawPoint(appstate->renderer, x + screen_bounds.x, y + screen_bounds.y);
    }
  }
}

static void render_segment(float p2y, const Rect& segment_bounds) {
  zero_bitbuf();
  
  PixelPoint p1 = calc_pixel_coordinates(0.0, p2y);
  
  for (float t = 0.3; t < 500; t += 0.3) {
    PixelPoint p2 = calc_pixel_coordinates(t, p2y);
    render_line_into_bitbuf(segment_bounds, p1, p2);
    p1 = p2;
  }

  render_bitbuf(segment_bounds);
}

static void render_screen(float p2y) {
  for (int xs = 0; xs < NUM_X_SEGMENTS; ++xs) {
    for (int ys = 0; ys < NUM_Y_SEGMENTS; ++ys) {
      Rect segment = {
	.x = SEGMENT_WIDTH * xs,
	.y = SEGMENT_WIDTH * ys,
	.w = SEGMENT_WIDTH,
	.h = SEGMENT_HEIGHT,
      };

      render_segment(p2y, segment);
    }
  }
}

static void render_clear() {
  SDL_SetRenderDrawColor(appstate->renderer, 0xff, 0xff, 0xff, 0xff);
  SDL_RenderClear(appstate->renderer);    
  SDL_SetRenderDrawColor(appstate->renderer, 0x00, 0x00, 0x00, 0x00);
}

static void present_screen() {
  SDL_RenderPresent(appstate->renderer);
}

static void run(AppState* st) {
  appstate = st;

  render_clear();
  present_screen();

  for (size_t i = 0; i < 360000; ++i) {    
    float p2y = (i/180.0) * M_PI;
    render_screen(p2y);
    present_screen();
  }
}

int main(int argc, char** argv) {
  AppState st = {0};
  
  st.window = SDL_CreateWindow(
    "win",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    SDL_WINDOW_SHOWN);
  
  if (st.window == NULL) {
    std::cerr << "Could not create a window: must be ran in a GUI environment" << std::endl;
    return 1;
  }

  st.renderer = SDL_CreateRenderer(
    st.window,
    -1,
    SDL_RENDERER_ACCELERATED);

  if (st.renderer == NULL) {
    std::cerr << "Could not create an SDL renderer" << std::endl;
    return 1;
  }

  run(&st);
  
  SDL_DestroyRenderer(st.renderer);
  SDL_DestroyWindow(st.window);
}
