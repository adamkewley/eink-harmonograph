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
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  
  randomSeed(analogRead(0));
  ArduinoRenderer st;
  Screen screen;
  Harmonograph harmonograph = STARTING_PARAMS;

  
  tick(random(), harmonograph);
  digitalWrite(4, HIGH);  // keep power source on (via a relay)
  render_screen(harmonograph, screen, st);
  st.render_present();  
}

void loop() {
  digitalWrite(4, LOW); // turn power source off (via a relay)
}
