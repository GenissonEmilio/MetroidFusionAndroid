#include "pixel_filter.h"
#include "album_context.h"
#include <array>
#include <iostream>
#include <stdexcept>
#if defined(FUSION_TEST_SDL)
#include "pixel_presenter.h"
#endif

void check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
using RGB=std::array<std::uint8_t,3>;

void check_filter() {
    // Exhaust every binary 3x3 neighborhood, including edge clamping, against
    // the unoptimized four-rule specification (not the implementation shortcut).
    const RGB palette[]={{15,81,230},{15,82,231}};
    for (int bits=0;bits<512;++bits) {
        std::vector<std::uint8_t> input;
        for (int p=0;p<9;++p) {
            const auto color=palette[(bits>>p)&1];
            input.insert(input.end(),color.begin(),color.end());
        }
        const auto before=input;
        std::vector<std::uint8_t> result;
        fusion::scale2x_rgb24(input.data(),3,3,result);
        check(result.size()==108,"Wrong Scale2x size");
        check(input==before,"Filter modified the guest framebuffer");
        const auto at=[&](int x,int y) {
            x=std::clamp(x,0,2); y=std::clamp(y,0,2);
            return RGB{input[(y*3+x)*3],input[(y*3+x)*3+1],input[(y*3+x)*3+2]};
        };
        for (int y=0;y<3;++y) for (int x=0;x<3;++x) {
            auto b=at(x,y-1), d=at(x-1,y), e=at(x,y), f=at(x+1,y), h=at(x,y+1);
            RGB expected[]={d==b && b!=f && d!=h?d:e,
                b==f && b!=d && f!=h?f:e, d==h && d!=b && h!=f?d:e,
                h==f && d!=h && b!=f?f:e};
            for (int n=0;n<4;++n) {
                const int p=((y*2+n/2)*6+x*2+n%2)*3;
                check(RGB{result[p],result[p+1],result[p+2]}==expected[n],
                    "Scale2x differs from the specification");
            }
        }
    }
    for (const auto dims : {std::array<int,2>{1,1},{1,7},{7,1},{240,160}}) {
        std::vector<std::uint8_t> input(dims[0]*dims[1]*3), output;
        for (std::size_t i=0;i<input.size();i+=3) {
            input[i]=23; input[i+1]=143; input[i+2]=226;
        }
        fusion::scale2x_rgb24(input.data(),dims[0],dims[1],output);
        check(output.size()==input.size()*4,"Thin or native image size mismatch");
        for (std::size_t i=0;i<output.size();++i)
            check(output[i]==input[i%3],"Solid color/border was changed");
    }
}

void check_layout() {
    for (auto mode : {fusion::PixelMode::Perfect,fusion::PixelMode::Smooth}) {
        for (int w : {240,640,720,1080,1280,1920,2400,2772})
            for (int h : {160,360,480,720,1080,1280}) {
                const auto r=fusion::pixel_layout(w,h,240,160,mode);
                check(r.width<=w && r.height<=h && r.x>=0 && r.y>=0,"Layout clips");
                check(r.width==240*r.integer_scale && r.height==160*r.integer_scale,
                    "Pixels have fractional or unequal scale");
                check(r.x==(w-r.width)/2 && r.y==(h-r.height)/2,"Layout not centered");
                if(mode==fusion::PixelMode::Smooth && r.integer_scale>=2)
                    check(r.integer_scale%2==0,"Scale2x subpixels have unequal sizes");
            }
    }
    const auto poco=fusion::pixel_layout(2772,1280,240,160,fusion::PixelMode::Smooth);
    check(poco.x==426 && poco.y==0 && poco.width==1920 && poco.height==1280,
        "POCO geometry changed");
    const auto invalid=fusion::pixel_layout(0,0,240,160,fusion::PixelMode::Perfect);
    check(invalid.width==0 && invalid.height==0,"Invalid layout not empty");
    const auto tiny=fusion::pixel_layout(120,80,240,160,fusion::PixelMode::Perfect);
    check(tiny.width==120 && tiny.height==80 && tiny.integer_scale==0,"Small screen fit");
}

#if defined(FUSION_TEST_SDL)
std::vector<std::uint8_t> captured;
int capture_width=0,capture_height=0;
void capture_sink(const std::uint8_t* rgb,int w,int h) {
    captured.assign(rgb,rgb+w*h*3);capture_width=w;capture_height=h;
}
std::vector<std::uint8_t> menu_pixels(240*160*3,61);
const std::uint8_t* menu_filter(const std::uint8_t*,int,int) { return menu_pixels.data(); }
void check_renderer() {
    check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    SDL_Surface* surface=SDL_CreateRGBSurfaceWithFormat(0,960,640,24,SDL_PIXELFORMAT_RGB24);
    check(surface!=nullptr,SDL_GetError());
    SDL_Renderer* renderer=SDL_CreateSoftwareRenderer(surface);
    check(renderer!=nullptr,SDL_GetError());
    SDL_Texture* original=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,240,160);
    check(original!=nullptr,SDL_GetError());
    std::vector<std::uint8_t> input(240*160*3), scaled;
    for (int y=0;y<160;++y) for (int x=0;x<240;++x) {
        const RGB color=x>y?RGB{23,143,226}:RGB{231,193,55};
        std::copy(color.begin(),color.end(),input.begin()+(y*240+x)*3);
    }
    check(SDL_UpdateTexture(original,nullptr,input.data(),240*3)==0,SDL_GetError());
    fusion::scale2x_rgb24(input.data(),240,160,scaled);
    fusion::PixelPresenter presenter;
    fusion::set_capture_sink(capture_sink);
    for (int trial=0;trial<6;++trial) {
        const bool smooth=trial%2;
        fusion::set_pixel_mode(smooth?fusion::PixelMode::Smooth:fusion::PixelMode::Perfect);
        check(fusion::request_capture(),"Capture request was rejected");
        check(!fusion::request_capture(),"Repeated requests not coalesced");
        presenter.present(renderer,original,input.data(),240,160,{0,0,960,640});
        check(capture_width==(smooth?480:240) && capture_height==(smooth?320:160),"Capture dimensions");
        check(captured==(smooth?scaled:input),"Capture is not the clean displayed game image");
        std::vector<std::uint8_t> actual(960*640*3);
        check(SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_RGB24,actual.data(),960*3)==0,
            SDL_GetError());
        for (int y=0;y<640;++y) for (int x=0;x<960;++x) for (int c=0;c<3;++c) {
            const auto expected=smooth?scaled[((y/2)*480+x/2)*3+c]:input[((y/4)*240+x/4)*3+c];
            check(actual[(y*960+x)*3+c]==expected,"SDL output has blur, bad colors or stale mode");
        }
    }
    const auto before=input;
    fusion::set_frame_filter(menu_filter);
    fusion::set_pixel_mode(fusion::PixelMode::Perfect);
    presenter.present(renderer,original,input.data(),240,160,{0,0,960,640});
    std::vector<std::uint8_t> page(960*640*3);
    check(SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_RGB24,page.data(),960*3)==0,SDL_GetError());
    check(std::all_of(page.begin(),page.end(),[](std::uint8_t c) { return c==61; }),
        "In-game page did not reach the actual SDL renderer");
    check(input==before,"In-game page modified the source GBA framebuffer");
    fusion::set_frame_filter(nullptr);
    // HostWindow uploads the new guest image before each present.
    check(SDL_UpdateTexture(original,nullptr,input.data(),240*3)==0,SDL_GetError());
    presenter.present(renderer,original,input.data(),240,160,{0,0,960,640});
    check(SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_RGB24,page.data(),960*3)==0,SDL_GetError());
    check(page[0]==input[0] && page[1]==input[1] && page[2]==input[2],
        "Closing the page left stale menu pixels on screen");
    presenter.close(); presenter.close();
    fusion::set_capture_sink(nullptr);
    SDL_DestroyTexture(original); SDL_DestroyRenderer(renderer); SDL_FreeSurface(surface);
    SDL_Quit();
}
#endif

int main() {
    check_filter(); check_layout();
    check(fusion::album_in_inventory({3,2,2,0,0}),"Fusion manual pause not recognized");
    check(!fusion::album_in_inventory({3,1,2,3,2}),"Album appears in ADAM conversation");
#if defined(FUSION_TEST_SDL)
    check_renderer();
#endif
    std::cout<<"Pixel filter, borders, palette, layout and rendering checks passed\n";
}
