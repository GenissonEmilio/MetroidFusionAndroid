#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

namespace minish {
enum class PixelMode { Perfect = 0, Smooth = 1 };

struct PixelLayout { int x, y, width, height, integer_scale; };

// Whole source pixels (or whole Scale2x subpixels) occupy equal screen areas.
// Only displays smaller than the source itself require a proportional fit.
inline PixelLayout pixel_layout(int dw, int dh, int sw, int sh, PixelMode mode) {
    if (dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0) return {};
    int scale = std::min(dw / sw, dh / sh);
    if (mode == PixelMode::Smooth && scale >= 2) scale -= scale % 2;
    int w, h;
    if (scale > 0) { w = sw * scale; h = sh * scale; }
    else if (std::int64_t(dw) * sh <= std::int64_t(dh) * sw) {
        w = dw; h = std::max(1, int(std::int64_t(dw) * sh / sw));
    } else {
        h = dh; w = std::max(1, int(std::int64_t(dh) * sw / sh));
    }
    return {(dw-w)/2, (dh-h)/2, w, h, scale};
}

// Independent RGB24 implementation of the published Scale2x rules:
// https://www.scale2x.it/algorithm . No library code or image assets copied.
// Border samples are clamped; colors are selected, never averaged.
inline void scale2x_rgb24(const std::uint8_t* source, int w, int h,
                         std::vector<std::uint8_t>& result) {
    if (!source || w <= 0 || h <= 0 || w > 1024 || h > 1024) {
        result.clear(); return;
    }
    result.resize(std::size_t(w) * h * 12);
    const auto at = [&](int x, int y) {
        return source + (std::size_t(y) * w + x) * 3;
    };
    const auto equal = [](const auto* a, const auto* b) {
        return a[0]==b[0] && a[1]==b[1] && a[2]==b[2];
    };
    for (int y=0; y<h; ++y) for (int x=0; x<w; ++x) {
        const auto *e=at(x,y), *b=at(x,std::max(0,y-1)),
            *d=at(std::max(0,x-1),y), *f=at(std::min(w-1,x+1),y),
            *bottom=at(x,std::min(h-1,y+1));
        const bool edge=!equal(b,bottom) && !equal(d,f);
        const auto *p0=edge && equal(d,b)?d:e, *p1=edge && equal(b,f)?f:e,
            *p2=edge && equal(d,bottom)?d:e, *p3=edge && equal(bottom,f)?f:e;
        auto* out=result.data()+(std::size_t(y*2)*w*2+x*2)*3;
        std::memcpy(out,p0,3); std::memcpy(out+3,p1,3);
        std::memcpy(out+w*6,p2,3); std::memcpy(out+w*6+3,p3,3);
    }
}
} // namespace minish
