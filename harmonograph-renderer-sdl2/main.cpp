#include <iostream>
#include <math.h>
#include <array>

#include <SDL2/SDL.h>

namespace {
  using pixelcoord_t = uint16_t;
  
  // screen: widths, segments, etc.
  const pixelcoord_t SCREEN_WIDTH = 400;
  const pixelcoord_t SCREEN_HEIGHT = 300;
  const pixelcoord_t SEGMENT_WIDTH = 400;
  const pixelcoord_t SEGMENT_HEIGHT = 300;
  const int SEGMENT_NUM_PIXELS = SEGMENT_WIDTH * SEGMENT_HEIGHT;
  const int SEGMENT_NUM_BYTES = (SEGMENT_NUM_PIXELS/8)+1;
  const int NUM_X_SEGMENTS = SCREEN_WIDTH/SEGMENT_WIDTH;
  const int NUM_Y_SEGMENTS = SCREEN_HEIGHT/SEGMENT_HEIGHT;
  const float START_TIME = 0.0;
  const float TIME_STEP = 0.3;
  const float MAX_TIME = 500.0;

  
  // TYPES

  template<typename T>
  struct Point {
    T x;
    T y;

    Point<T> operator +(const Point<T>& o) {
      return { .x = x + o.x, .y = y + o.y };
    }
  };

  using FunctionPoint = Point<float>;
  using PixelPoint = Point<pixelcoord_t>;

  struct Rect {
    pixelcoord_t x;
    pixelcoord_t y;
    pixelcoord_t w;
    pixelcoord_t h;

    bool contains(const PixelPoint& p) const {
      if (x <= p.x && p.x < x + w &&
	  y <= p.y && p.y < y + h) {
	return true;
      } else {
	return false;
      }
    }
  };

  struct HarmonographPendulum {
    float frequency;
    float phase_y;
    float phase_x;
    float dampening;
    float amplitude_x;
    float amplitude_y;

    FunctionPoint pos(float t) const {
      float dt = expf(-dampening * t);
      
      float x = amplitude_x * sinf(t*frequency + phase_x) * dt;
      float y = amplitude_y * sinf(t*frequency + phase_y) * dt;

      // Rescale function to always be positive (screen coords)
      return {
	.x = x + amplitude_x,
	.y = y + amplitude_y,
      };
    }
  };

  struct Harmonograph {
    HarmonographPendulum a;
    HarmonographPendulum b;
    HarmonographPendulum c;

    FunctionPoint pos(float t) const {
      return a.pos(t) + b.pos(t) + c.pos(t);
    }

    PixelPoint pixel_pos(float t) const {
      FunctionPoint p = pos(t);

      return {
	.x = static_cast<pixelcoord_t>(p.x),
	.y = static_cast<pixelcoord_t>(p.y),
      };
    }
  };

  // `buf` is in a format as used by hardware.
  struct ScreenBuf {
    std::array<uint8_t, SEGMENT_NUM_BYTES> buf;

    void zero() {
      std::fill(std::begin(buf), std::end(buf), 0);
    }

    void paint_pixel(const Rect& bounds, const PixelPoint& p) {
      if (bounds.contains(p)) {
	pixelcoord_t bitbuf_x = p.x - bounds.x;
	pixelcoord_t bitbuf_y = p.y - bounds.y;
    
	size_t  bitidx = bitbuf_y*bounds.w + bitbuf_x;
	size_t byteidx = bitidx / 8;
	size_t bytebitidx = bitidx & 7;
    
	buf[byteidx] |= (0x1 << bytebitidx);
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
      
      pixelcoord_t y = p1.y;

      for (pixelcoord_t x = p1.x; x < p2.x; ++x) {
	PixelPoint p = { .x = x, .y = y };
	paint_pixel(bounds, p);
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
      pixelcoord_t x = p1.x;

      for (pixelcoord_t y = p1.y; y < p2.y; ++y) {
	PixelPoint p = { .x = x, .y = y };
	paint_pixel(bounds, p);
	if (D > 0) {
	  x += xi;
	  D -= 2*dy;
	}
	D += 2*dx;
      }
    }

    void paint_line(
      const Rect& bounds,
      const PixelPoint& p1,
      const PixelPoint& p2) {

      // Using a custom line painting alg. because of the screen
      // segmentation. Line coords might fall out of bounds.

      // Breshenham alg.

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
  };

  const Harmonograph STARTING_PARAMS = {
    .a = {
      .frequency = 1.005,
      .phase_y = 180.0/180.0 * M_PI,
      .phase_x = 95.0/180.0 * M_PI,
      .dampening = 2.0/1000,
      .amplitude_x = 0.483 * (SCREEN_WIDTH/2),
      .amplitude_y = 0.625 * (SCREEN_HEIGHT/2),
    },
    .b = {
      .frequency = 2.990,
      .phase_y = 180.0/180.0 * M_PI,
      .phase_x = 180.0/180.0 * M_PI,
      .dampening = 1.0/1000,
      .amplitude_x = 0.2586 * (SCREEN_WIDTH/2),
      .amplitude_y = 0.2083 * (SCREEN_HEIGHT/2),
    },
    .c = {
      .frequency = 1.000,
      .phase_y = 180.0/180.0 * M_PI,
      .phase_x = 120.0/180.0 * M_PI,
      .dampening = 0.5/1000,
      .amplitude_x = 0.2586 * (SCREEN_WIDTH/2),
      .amplitude_y = 0.1667 * (SCREEN_HEIGHT/2),
    },
  };

  const Harmonograph TRIANGLE = {
    .a = {
      .frequency = 1.99,
      .phase_y = 90.0/180.0 * M_PI,
      .phase_x = 180.0/180.0 * M_PI,
      .dampening = 3.0/1000,
      .amplitude_x = 0.6 * (SCREEN_WIDTH/2),
      .amplitude_y = 0.6 * (SCREEN_HEIGHT/2),
    },
    .b = {
      .frequency = 0.99,
      .phase_y = 90.0/180.0 * M_PI,
      .phase_x = 360.0/180.0 * M_PI,
      .dampening = 3.0/1000,
      .amplitude_x = 0.20 * (SCREEN_WIDTH/2),
      .amplitude_y = 0.20 * (SCREEN_HEIGHT/2),
    },
    .c = {
      .frequency = 1.00,
      .phase_y = 180.0/180.0 * M_PI,
      .phase_x = 270.0/180.0 * M_PI,
      .dampening = 1.0/1000,
      .amplitude_x = 0.20 * (SCREEN_WIDTH/2),
      .amplitude_y = 0.20 * (SCREEN_HEIGHT/2),
    },
  };


  // This struct is designed so that its implementation can be
  // switched out when this code is ported to hardware.
  struct AppState {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Surface* surface;

    AppState() {
      window = SDL_CreateWindow("win",
			       SDL_WINDOWPOS_CENTERED,
			       SDL_WINDOWPOS_CENTERED,
			       SCREEN_WIDTH,
			       SCREEN_HEIGHT,
			       SDL_WINDOW_SHOWN);
      
      if (window == NULL) {
	throw std::runtime_error("Could not create a window: must be ran in a GUI environment");
      }

      surface = SDL_GetWindowSurface(window);

      if (surface == NULL) {
	SDL_DestroyWindow(window);
	throw std::runtime_error("Could not create a surface");
      }

      renderer = SDL_CreateSoftwareRenderer(surface);

      if (renderer == NULL) {
	SDL_DestroyWindow(window);
	throw std::runtime_error("Could not create an SDL renderer");
      }
    }

    ~AppState() {
      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(window);
    }

    void render(const Rect& screen_bounds, const std::array<uint8_t, SEGMENT_NUM_BYTES>& buf) {
      Uint8 bbp = surface->format->BytesPerPixel;
      
      for (size_t x = 0; x < screen_bounds.w; ++x) {
	for (size_t y = 0; y < screen_bounds.h; ++y) {
	  int bitidx = y*screen_bounds.w + x;
	  int byteidx = bitidx >> 3;
	  int bytebitidx = bitidx & 7;
      
	  uint8_t pix = buf[byteidx] & (1 << bytebitidx);
      
	  if (pix) {
	    size_t x_pix = x + screen_bounds.x;
	    size_t y_pix = y + screen_bounds.y;
	    SDL_RenderDrawPoint(renderer, x_pix, y_pix);
	  }
	}
      }
    }
    
    void render_clear() {
      SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
      SDL_RenderClear(renderer);    
      SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    }

    void render_present() {
      SDL_UpdateWindowSurface(window);
    }
  };



  void paint_segment(
    const Harmonograph& params,
    const Rect& segment_bounds,
    ScreenBuf& screenbuf) {
    
    PixelPoint p1 = params.pixel_pos(START_TIME);
  
    for (float t = START_TIME + TIME_STEP; t < MAX_TIME; t += TIME_STEP) {
      PixelPoint p2 = params.pixel_pos(t);
      screenbuf.paint_line(segment_bounds, p1, p2);
      p1 = p2;
    }
  }
  
  // FUNCS
  void render_screen(
    const Harmonograph& params,
    ScreenBuf& screenbuf,
    AppState& st) {

    // The screen needs to be rendered as a bunch of separate
    // non-overlapping segments because an Arduino can't hold an entire
    // screen in memory.
    for (pixelcoord_t xs = 0; xs < NUM_X_SEGMENTS; ++xs) {
      for (pixelcoord_t ys = 0; ys < NUM_Y_SEGMENTS; ++ys) {
	Rect segment = {
	  .x = SEGMENT_WIDTH * xs,
	  .y = SEGMENT_HEIGHT * ys,
	  .w = SEGMENT_WIDTH,
	  .h = SEGMENT_HEIGHT,
	};

	screenbuf.zero();
	paint_segment(params, segment, screenbuf);
	st.render(segment, screenbuf.buf);
      }
    }
  }

  void tick(size_t i, Harmonograph& harmonograph) {
    static float a_dir = 1.0;
    
    harmonograph.b.phase_y += (1/180.0) * M_PI;
    harmonograph.b.phase_x += (2/180.0) * M_PI;

    harmonograph.b.frequency += a_dir*0.0001;

    if (std::abs(harmonograph.a.frequency) > 15) {
      a_dir *= -1.0;
    }
  }

  void run(AppState& st) {
    ScreenBuf screenbuf;
    Harmonograph harmonograph = STARTING_PARAMS;
    
    for (size_t i = 0;; ++i) {
      tick(i, harmonograph);

      st.render_clear();
      render_screen(harmonograph, screenbuf, st);
      st.render_present();
    }
  }
}

int main(int argc, char** argv) {
  try {
    AppState st;
    run(st);
    return 0;
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    return 1;
  }
}
