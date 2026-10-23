#include "pixel_presenter.h"
#include <atomic>

namespace minish {
namespace { std::atomic<PixelMode> mode{PixelMode::Smooth}; }
PixelMode pixel_mode() { return mode.load(std::memory_order_relaxed); }
void set_pixel_mode(PixelMode value) { mode.store(value,std::memory_order_relaxed); }

void PixelPresenter::close() {
    if (texture_) SDL_DestroyTexture(texture_);
    texture_=nullptr; width_=height_=0;
}

void PixelPresenter::present(SDL_Renderer* renderer, SDL_Texture* original,
        const std::uint8_t* rgb, int w, int h, const SDL_Rect& destination) {
    // Do not smooth a downscaled image or compound an optional HD raster.
    const bool smooth=pixel_mode()==PixelMode::Smooth && w==240 && h==160 &&
        destination.w>=w*2 && destination.h>=h*2;
    SDL_SetTextureScaleMode(original,SDL_ScaleModeNearest);
    if (smooth) {
        if (!texture_ || width_!=w*2 || height_!=h*2) {
            close();
            texture_=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB24,
                SDL_TEXTUREACCESS_STREAMING,w*2,h*2);
            if (texture_) {
                width_=w*2; height_=h*2;
                SDL_SetTextureBlendMode(texture_,SDL_BLENDMODE_NONE);
                SDL_SetTextureScaleMode(texture_,SDL_ScaleModeNearest);
            }
        }
        if (texture_) {
            scale2x_rgb24(rgb,w,h,scaled_);
            if (SDL_UpdateTexture(texture_,nullptr,scaled_.data(),w*6)==0 &&
                    SDL_RenderCopy(renderer,texture_,nullptr,&destination)==0) return;
        }
    }
    SDL_RenderCopy(renderer,original,nullptr,&destination);
}
}
