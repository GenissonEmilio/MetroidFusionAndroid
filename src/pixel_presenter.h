#pragma once
#include "pixel_filter.h"
#include <SDL.h>

namespace minish {
PixelMode pixel_mode();
void set_pixel_mode(PixelMode mode);
using CaptureSink = void (*)(const std::uint8_t*, int, int);
void set_capture_sink(CaptureSink sink);
bool request_capture();

// Owned by HostWindow's backend; close() runs before SDL destroys the renderer.
class PixelPresenter {
public:
    void present(SDL_Renderer*, SDL_Texture* original, const std::uint8_t* rgb,
                 int w, int h, const SDL_Rect& destination);
    void close();
private:
    SDL_Texture* texture_ = nullptr;
    int width_ = 0, height_ = 0;
    std::vector<std::uint8_t> scaled_;
};
}
