#include <math.h>
#include <SPI.h>

// this is installed by installing the 4-2inch_e-paper-module.zip as a
// library (e.g. unzip to ~/Arduino)
#include <epd4in2.h>

// This code is shared between the Arduino and SDL
// implementations. The SDL implementation is easier to debug
// interactively.
#include "harmonograph.hpp"

namespace {  
  class ArduinoRenderer final : public Renderer {
  public:        
    ArduinoRenderer() {
      epd.Init();
    }

    void render(const Rect& screen_bounds, uint8_t* buf) override {
      epd.SetPartialWindow(buf, 
                           screen_bounds.x,
                           screen_bounds.y, 
                           screen_bounds.w, 
                           screen_bounds.h);
    }
    
    void render_clear() override {
      epd.ClearFrame();
    }

    void render_present() override {
      epd.DisplayFrame();
    }

  private:
    Epd epd;
  };
}

void setup() {
  randomSeed(analogRead(0));
  ArduinoRenderer st;
  Screen screen;
  Harmonograph harmonograph = STARTING_PARAMS;
    
  for (size_t i = random();; ++i) {
    tick(i, harmonograph);

    st.render_clear();
    render_screen(harmonograph, screen, st);
    st.render_present();
  }
}

void loop() {
  // Setup code runs this
}
