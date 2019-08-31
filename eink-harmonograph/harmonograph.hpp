template<typename T>
struct Point {
  T x;
  T y;

  Point<T> operator+(const Point<T>& o) {
    return { x + o.x, y + o.y };
  }
};

template<typename T>
struct Dimensions {
  T width;
  T height;

  constexpr Dimensions<T> operator/(Dimensions<T> o) const {
    return { width/o.width, height/o.height };
  }
};

template<typename T>
constexpr T area(Dimensions<T> d) {
  return d.width * d.height;
}

template<typename T>
struct RectT {
  T x;
  T y;
  T w;
  T h;
};

template<typename T>
bool contains(const RectT<T>& r, const Point<T>& p) {
  if (r.x <= p.x && p.x < r.x + r.w &&
      r.y <= p.y && p.y < r.y + r.h) {
    return true;
  } else {
    return false;
  }
}

template<typename T>
struct LineT {
  Point<T> p1;
  Point<T> p2;
};

template<size_t N>
struct bitset {
  uint8_t buf[N];

  void set_all() {
    memset(buf, 0xff, N);
  }

  void unset(size_t idx) {
    buf[idx/8] &= ~(0x80 >> (idx & 7));
  }
};

struct Pendulum {
  float frequency;
  float phase_y;
  float phase_x;
  float dampening;
  float amplitude_x;
  float amplitude_y;
};

using FunctionPoint = Point<float>;

FunctionPoint pos(const Pendulum& p, float t) {
  float dt = expf(-p.dampening * t);
      
  float x = p.amplitude_x * sinf(t*p.frequency + p.phase_x) * dt;
  float y = p.amplitude_y * sinf(t*p.frequency + p.phase_y) * dt;

  // Rescale function to always be positive (screen coords)
  return { x + p.amplitude_x, y + p.amplitude_y };
}

struct Harmonograph {
  Pendulum a;
  Pendulum b;
  Pendulum c;
};

FunctionPoint pos(const Harmonograph& h, float t) {
  return pos(h.a, t) + pos(h.b, t) + pos(h.c, t);
}

using pixelcoord_t = int16_t;
using PixelPoint = Point<pixelcoord_t>;

PixelPoint pixel_pos(const Harmonograph& h, float t) {
  FunctionPoint p = pos(h, t);
  return { static_cast<pixelcoord_t>(p.x), static_cast<pixelcoord_t>(p.y) };
}
  
// screen: widths, segments, etc.
constexpr Dimensions<pixelcoord_t> screendims{400, 300};
constexpr Dimensions<pixelcoord_t> segment_dims{400, 30};
constexpr auto segment_num_pixels = area(segment_dims);
constexpr auto segment_num_bytes = (segment_num_pixels/8)+1;
constexpr auto segment_grid = screendims/segment_dims;
const float START_TIME = 0.0;
const float TIME_STEP = 0.1;
const float MAX_TIME = 600.0;
  
// TYPES
using Rect = RectT<pixelcoord_t>;

struct Screen {
  bitset<segment_num_bytes> bs;

  void clear() {
    bs.set_all();
  }

  void paint(const Rect& bounds, const PixelPoint& p) {
    if (contains(bounds, p)) {
      pixelcoord_t bitbuf_x = p.x - bounds.x;
      pixelcoord_t bitbuf_y = p.y - bounds.y;

      auto rowstart = bitbuf_y*bounds.w;
      auto bitidx = rowstart + bitbuf_x;
      auto byteidx = bitidx / 8;

      bs.unset(bitidx);
    }
  }
};

using Line = LineT<pixelcoord_t>;

void paint_line_breshenham_low(Screen& s,
                               const Rect& bounds,
                               const Line& l) {
        
  const PixelPoint& p1 = l.p1;
  const PixelPoint& p2 = l.p2;
        
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
    s.paint(bounds, p);
    if (D > 0) {
      y += yi;
      D -= 2*dx;
    }
    D += 2*dy;
  }
}

void paint_line_breshenham_high(Screen& s,
                                const Rect& bounds,
                                const Line& l) {
  const PixelPoint& p1 = l.p1;
  const PixelPoint& p2 = l.p2;
        
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
    s.paint(bounds, p);
    if (D > 0) {
      x += xi;
      D -= 2*dy;
    }
    D += 2*dx;
  }
}

void paint_line(Screen& s,
                const Rect& bounds,
                const Line& l) {

  const PixelPoint& p1 = l.p1;
  const PixelPoint& p2 = l.p2;

  // Using a custom line painting alg. because of the screen
  // segmentation. Line coords might fall out of bounds.

  // Breshenham alg.

  // covers wide & tall representations with [-1,1] gradients: base
  // alg only covers wide representations with positive gradient
      
  if (abs(p2.y - p1.y) < abs (p2.x - p1.x)) {
    // wider than it is tall (not steep)
    if (p1.x > p2.x) {
      paint_line_breshenham_low(s, bounds, {p2, p1});
    } else {
      paint_line_breshenham_low(s, bounds, {p1, p2});
    }
  } else {
    // taller than it is wide (steep)
    if (p1.y > p2.y) {
      paint_line_breshenham_high(s, bounds, {p2, p1});
    } else {
      paint_line_breshenham_high(s, bounds, {p1, p2});
    }
  }
}

const Harmonograph STARTING_PARAMS =
  {
   .a = {
         .frequency = 1.005,
         .phase_y = 180.0/180.0 * M_PI,
         .phase_x = 95.0/180.0 * M_PI,
         .dampening = 2.0/1000,
         .amplitude_x = 0.483 * (screendims.width/2),
         .amplitude_y = 0.625 * (screendims.height/2),
         },
   .b = {
         .frequency = 0.990,
         .phase_y = 180.0/180.0 * M_PI,
         .phase_x = 180.0/180.0 * M_PI,
         .dampening = 1.0/1000,
         .amplitude_x = 0.2586 * (screendims.width/2),
         .amplitude_y = 0.2083 * (screendims.height/2),
         },
   .c = {
         .frequency = 1.000,
         .phase_y = 180.0/180.0 * M_PI,
         .phase_x = 120.0/180.0 * M_PI,
         .dampening = 0.5/1000,
         .amplitude_x = 0.2586 * (screendims.width/2),
         .amplitude_y = 0.1667 * (screendims.height/2),
         },
  };

void paint_segment(const Harmonograph& params,
                   const Rect& segment_bounds,
                   Screen& screen) {
    
  PixelPoint p1 = pixel_pos(params, START_TIME);
  
  for (float t = START_TIME + TIME_STEP; t < MAX_TIME; t += TIME_STEP) {
    PixelPoint p2 = pixel_pos(params, t);
    paint_line(screen, segment_bounds, {p1, p2});
    p1 = p2;
  }
}

void tick(size_t i, Harmonograph& harmonograph) {
  static const float fq_lut[] = {
                                 0.995,
                                 1.005,
                                 1.5,
                                 2.010,
                                 2.503,
                                 3.000,
  };

  harmonograph.b.phase_x = (((i % 8)*30)/180.0) * M_PI;
  harmonograph.b.phase_y = (((i % 8)*30)/180.0) * M_PI;
  harmonograph.b.frequency = fq_lut[(i+1) % 6];
  harmonograph.c.frequency = fq_lut[i % 6];        
}

// public API:


// There are two renderers possible: Arduino or SDL2. Both receive a
// sequence of render calls (draw into backbuffer), followed by a
// render_present (show), followed by a render_clear (clear
// backbuffer), rinse and repeat.
class Renderer {
public:
  virtual void render(const Rect& screen_bounds, uint8_t* buf) = 0;
  virtual void render_clear() = 0;
  virtual void render_present() = 0;
  virtual ~Renderer() noexcept {}
};

// FUNCS
void render_screen(const Harmonograph& params,
                   Screen& screen,
                   Renderer& st) {

  // The screen needs to be rendered as a bunch of separate
  // non-overlapping segments because an Arduino can't hold an entire
  // screen in memory.
  for (pixelcoord_t xs = 0; xs < segment_grid.width; ++xs) {
    for (pixelcoord_t ys = 0; ys < segment_grid.height; ++ys) {
      Rect segment = {
                      .x = segment_dims.width * xs,
                      .y = segment_dims.height * ys,
                      .w = segment_dims.width,
                      .h = segment_dims.height,
      };

      screen.clear();
      paint_segment(params, segment, screen);
      st.render(segment, screen.bs.buf);
    }
  }
}
