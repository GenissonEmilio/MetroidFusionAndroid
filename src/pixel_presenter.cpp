#include "pixel_presenter.h"
#include <atomic>

namespace fusion {
namespace {
std::atomic<PixelMode> mode{PixelMode::Smooth};
CaptureSink capture_sink=nullptr;
FrameFilter frame_filter=nullptr;
std::atomic<bool> capture_pending{false};
void capture(const std::uint8_t* rgb,int w,int h) {
    if (capture_pending.exchange(false) && capture_sink) capture_sink(rgb,w,h);
}
}
PixelMode pixel_mode() { return mode.load(std::memory_order_relaxed); }
void set_pixel_mode(PixelMode value) { mode.store(value,std::memory_order_relaxed); }
void set_capture_sink(CaptureSink sink) { capture_sink=sink; capture_pending=false; }
void set_frame_filter(FrameFilter filter) { frame_filter=filter; }
bool request_capture() {
    return capture_sink && !capture_pending.exchange(true);
}

void PixelPresenter::close() {
    if (texture_) SDL_DestroyTexture(texture_);
    texture_=nullptr; width_=height_=0;
}

void PixelPresenter::present(SDL_Renderer* renderer, SDL_Texture* original,
        const std::uint8_t* rgb, int w, int h, const SDL_Rect& destination) {
    const auto* source=rgb;
    if(frame_filter) {
        if(const auto* filtered=frame_filter(rgb,w,h)) rgb=filtered;
        if(rgb!=source) SDL_UpdateTexture(original,nullptr,rgb,w*3);
    }
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
                    SDL_RenderCopy(renderer,texture_,nullptr,&destination)==0) {
                capture(scaled_.data(),w*2,h*2); return;
            }
        }
    }
    SDL_RenderCopy(renderer,original,nullptr,&destination);
    capture(rgb,w,h);
}
}
