#include <iostream>
#include <math.h>
#include <array>

#include <SDL2/SDL.h>

#include "eink-harmonograph/harmonograph.hpp"

namespace {

    class SDLRenderer final : public Renderer {
    public:
        SDLRenderer() {
            window = SDL_CreateWindow("harmonograph",
                                      SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED,
                                      screendims.width,
                                      screendims.height,
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

        ~SDLRenderer() override {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
        }

        void render(const Rect& screen_bounds, uint8_t* buf) override {      
            for (auto y = 0U; y < screen_bounds.h; ++y) {
                auto rowstart = screen_bounds.w * y;
                for (auto x = 0U; x < screen_bounds.w; ++x) {
                    auto bitidx = rowstart + x;
      
                    uint8_t pix = buf[bitidx/8] & (0x80 >> (bitidx & 7));
      
                    if (pix == 0) {
                        SDL_RenderDrawPoint(renderer,
                                            screen_bounds.x + x,
                                            screen_bounds.y + y);
                    }
                }
            }
        }
    
        void render_clear() override {
            SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
            SDL_RenderClear(renderer);    
            SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        }

        void render_present() override {
            SDL_UpdateWindowSurface(window);
        }

    private:
        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_Surface* surface;
    };

    void run(Renderer& st) {
        Screen screen;
        Harmonograph harmonograph = STARTING_PARAMS;
        SDL_Event e;
    
        for (size_t i = random();; ++i) {
            tick(i, harmonograph);

            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT ||
                    e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                    return;
                }
            }

            st.render_clear();
            render_screen(harmonograph, screen, st);
            st.render_present();
            SDL_Delay(1000);
        }
    }
}

int main(int argc, char** argv) {
    try {
        SDLRenderer st;
        run(st);
        return 0;
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        return 1;
    }
}
