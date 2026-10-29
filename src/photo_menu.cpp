#include "photo_menu.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string_view>

namespace fusion {
namespace {
using Color = std::array<std::uint8_t, 3>;
constexpr Color ink{176,232,248}, paper{16,24,56}, edge{56,144,200},
    white{255,255,255}, blue{24,40,96}, orange{248,224,64}, dark{8,16,40};

// Small original 5x7 bitmap alphabet. UI text does not depend on Android fonts.
constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .:/-<>!?";
constexpr std::array<std::array<unsigned,7>,45> glyphs{{
 {{14,17,17,31,17,17,17}},{{30,17,17,30,17,17,30}},{{14,17,16,16,16,17,14}},
 {{30,17,17,17,17,17,30}},{{31,16,16,30,16,16,31}},{{31,16,16,30,16,16,16}},
 {{14,17,16,23,17,17,15}},{{17,17,17,31,17,17,17}},{{14,4,4,4,4,4,14}},
 {{7,2,2,2,2,18,12}},{{17,18,20,24,20,18,17}},{{16,16,16,16,16,16,31}},
 {{17,27,21,21,17,17,17}},{{17,25,25,21,19,19,17}},{{14,17,17,17,17,17,14}},
 {{30,17,17,30,16,16,16}},{{14,17,17,17,21,18,13}},{{30,17,17,30,20,18,17}},
 {{15,16,16,14,1,1,30}},{{31,4,4,4,4,4,4}},{{17,17,17,17,17,17,14}},
 {{17,17,17,17,17,10,4}},{{17,17,17,21,21,21,10}},{{17,17,10,4,10,17,17}},
 {{17,17,10,4,4,4,4}},{{31,1,2,4,8,16,31}},
 {{14,17,19,21,25,17,14}},{{4,12,4,4,4,4,14}},{{14,17,1,2,4,8,31}},
 {{30,1,1,14,1,1,30}},{{2,6,10,18,31,2,2}},{{31,16,16,30,1,1,30}},
 {{14,16,16,30,17,17,14}},{{31,1,2,4,8,8,8}},{{14,17,17,14,17,17,14}},
 {{14,17,17,15,1,1,14}},{{0,0,0,0,0,0,0}},{{0,0,0,0,0,12,12}},
 {{0,12,12,0,12,12,0}},{{1,2,2,4,8,8,16}},{{0,0,0,31,0,0,0}},
 {{2,4,8,16,8,4,2}},{{8,4,2,1,2,4,8}},{{4,4,4,4,4,0,4}},
 {{14,17,1,2,4,0,4}}
}};

struct Canvas {
    std::vector<std::uint8_t>& rgb;
    const std::uint8_t* font=nullptr;
    bool native_letter(unsigned ch) const {
        return font && ((ch>='0' && ch<='9') || (ch>='A' && ch<='Z') || (ch>='a' && ch<='z'));
    }
    void pixel(int x,int y,Color c) {
        if (x<0 || y<0 || x>=240 || y>=160) return;
        std::copy(c.begin(),c.end(),rgb.begin()+(y*240+x)*3);
    }
    void rect(int x,int y,int w,int h,Color c) {
        for(int py=std::max(0,y);py<std::min(160,y+h);++py)
            for(int px=std::max(0,x);px<std::min(240,x+w);++px) pixel(px,py,c);
    }
    void frame(int x,int y,int w,int h,Color c) {
        rect(x,y,w,1,c);rect(x,y+h-1,w,1,c);rect(x,y,1,h,c);rect(x+w-1,y,1,h,c);
    }
    int advance(unsigned ch) const {
        if(ch==' ') return 4;
        if(ch=='.' || ch==':') return 3;
        if(ch=='/' || ch=='-') return 7;
        if(!native_letter(ch)) return 6;
        int right=0,left=16;
        for(int py=0;py<16;++py) for(int px=0;px<16;++px)
            if(font[(ch*16+py)*16+px]) { left=std::min(left,px);right=std::max(right,px); }
        return left==16?4:right-left+2;
    }
    int text_width(std::string_view s) const {
        int result=0;for(unsigned char ch:s) result+=advance(ch);return result;
    }
    void text(int x,int y,std::string_view s,Color c=ink,int max_width=240) {
        const int end=std::min(240,x+max_width);
        for(unsigned char ch:s) {
            const int step=advance(ch);if(x+step>end) break;
            if(ch=='.' || ch==':') {
                rect(x,y+12,2,2,c);
                if(ch==':') rect(x,y+6,2,2,c);
            } else if(ch=='/') {
                for(int py=3;py<14;++py) pixel(x+5-(py-3)/2,y+py,c);
            } else if(ch=='-') rect(x,y+9,6,1,c);
            else if(native_letter(ch)) {
                int left=16;
                for(int row=0;row<16;++row) for(int col=0;col<16;++col)
                    if(font[(ch*16+row)*16+col]) left=std::min(left,col);
                for(int row=0;row<16;++row) for(int col=0;col<16;++col)
                    if(const auto value=font[(ch*16+row)*16+col])
                        pixel(x+col-left,y+row,value==1?c:blue);
            } else {
                if(ch>='a' && ch<='z') ch-=32;
                const auto i=alphabet.find(static_cast<char>(ch));
                if(i!=std::string_view::npos) for(int row=0;row<7;++row)
                    for(int col=0;col<5;++col)
                        if(glyphs[i][row]&(16>>col)) pixel(x+col,y+row+4,c);
            }
            x+=step;
        }
    }
    void image(const std::vector<std::uint8_t>& src,int sw,int sh,int x,int y,int w,int h) {
        if(sw<=0 || sh<=0 || src.size()!=static_cast<std::size_t>(sw*sh*3)) return;
        // Aspect-fit with nearest-neighbour sampling; no stretching or blur.
        const int dw=std::min(w,h*sw/sh), dh=std::min(h,w*sh/sw);
        x+=(w-dw)/2;y+=(h-dh)/2;
        for(int py=0;py<dh;++py) for(int px=0;px<dw;++px) {
            const int p=((py*sh/dh)*sw+px*sw/dw)*3;
            pixel(x+px,y+py,{src[p],src[p+1],src[p+2]});
        }
    }
    void camera(int x,int y) {
        rect(x+4,y,6,2,ink);rect(x,y+2,15,11,ink);
        rect(x+1,y+3,13,9,{196,164,96});rect(x+1,y+3,13,2,{248,216,136});
        rect(x+2,y+5,2,2,white);rect(x+7,y+4,6,7,blue);
        rect(x+8,y+5,4,5,{88,160,216});rect(x+9,y+6,2,2,white);
    }
    void cursor(int x,int y,int w,int h) {
        for(int sx:{0,1}) for(int sy:{0,1}) {
            const int px=sx?x+w-5:x,py=sy?y+h-5:y;
            rect(px,py,5,5,white);
            rect(px+(sx?0:2),py+(sy?0:2),3,3,paper);
            rect(px+1,py+1,3,3,orange);
            rect(px+(sx?0:3),py+(sy?0:3),2,2,paper);
        }
    }
};

Color native_paper(const std::uint8_t* frame) {
    if(!frame) return paper;
    std::array<unsigned,4096> count{};
    std::array<Color,4096> value{};
    for(int y=8;y<152;++y) for(int x=8;x<232;++x) {
        const auto* p=frame+(y*240+x)*3;
        if(p[0]+p[1]+p[2]>210 || p[2]<p[0]) continue;
        const unsigned key=((p[0]>>4)<<8)|((p[1]>>4)<<4)|(p[2]>>4);
        ++count[key];value[key]={p[0],p[1],p[2]};
    }
    const auto index=std::max_element(count.begin(),count.end())-count.begin();
    return count[index]?value[index]:paper;
}
}

void PhotoMenu::reset() {
    view_=View::Closed;parked_screen_=0;selected_=page_=total_=0;
    loading_=saving_=photo_loading_=false;
    ++token_;++photo_token_;++capture_token_;
    entries_.clear();photo_.clear();scene_.clear();scene_w_=scene_h_=0;message_.clear();
}
void PhotoMenu::remember_scene(const std::uint8_t* rgb,int w,int h) {
    if(!rgb || w<1 || h<1 || w>1024 || h>1024) return;
    scene_.assign(rgb,rgb+w*h*3);scene_w_=w;scene_h_=h;
}
void PhotoMenu::reload() {
    loading_=true;entries_.clear();++token_;
    if(load_page) load_page(page_,token_);
    else { loading_=false;message_="ALBUM INDISPONIVEL"; }
}
void PhotoMenu::open(unsigned screen,bool forward) {
    parked_screen_=screen;entered_forward_=forward;view_=View::Album;
    selected_=page_=0;message_.clear();reload();
}
void PhotoMenu::move(int dx,int dy) {
    message_.clear();
    if(selected_==0) { if(dy>0 && !entries_.empty()) selected_=1;return; }
    const int index=selected_-1,col=index%3,row=index/3;
    if(dy<0 && row==0) { selected_=0;return; }
    int target=index;
    if(dy) target+=dy*3;
    if(dx) {
        if((dx<0 && col==0 && page_>0) ||
           (dx>0 && col==2 && (page_+1)*page_size<total_)) {
            page_+=dx;selected_=1;reload();return;
        }
        if(col+dx<0 || col+dx>2) return;
        target+=dx;
    }
    if(target>=0 && target<static_cast<int>(entries_.size())) selected_=target+1;
}
bool PhotoMenu::update(unsigned keys,unsigned screen,unsigned next,bool stable) {
    if(!stable || (active() && screen!=parked_screen_)) {
        view_=View::Closed;++token_;++photo_token_;return false;
    }
    if(!active()) {
        // Insert the album between Fusion Map (0) and Status (1).
        // Native Map L still opens Sleep; native Status R is unchanged.
        if((screen==0 && next==1 && keys==R) || (screen==1 && next==0 && keys==L)) {
            open(screen,keys==R);return true;
        }
        return false;
    }
    if(keys&Start) { view_=View::Closed;++token_;++photo_token_;return false; }
    if(view_==View::Camera) {
        if(keys&B) { view_=View::Album;message_.clear(); }
        else if((keys&A) && !saving_) {
            if(scene_.empty()) message_="VOLTE AO JOGO PARA FOTOGRAFAR";
            else if(total_>=200) message_="ALBUM CHEIO: 200 FOTOS";
            else if(save_photo) {
                saving_=true;message_="GUARDANDO LEMBRANCA...";++capture_token_;
                save_photo(scene_,scene_w_,scene_h_,capture_token_);
            } else message_="CAMERA INDISPONIVEL";
        }
        return true;
    }
    if(view_==View::Photo) {
        if(keys&B) { view_=View::Album;++photo_token_; }
        return true;
    }
    if(keys&(L|R|B)) {
        const bool cross=(entered_forward_ && keys==R) || (!entered_forward_ && keys==L);
        view_=View::Closed;++token_;++photo_token_;
        // Returning to the parked page consumes this input. Crossing the new
        // page lets the original L/R handler perform its own fade and routing.
        return !cross;
    }
    if(keys&Up) move(0,-1);
    else if(keys&Down) move(0,1);
    else if(keys&Left) move(-1,0);
    else if(keys&Right) move(1,0);
    else if(keys&A) {
        if(selected_==0) { view_=View::Camera;message_.clear(); }
        else if(!loading_ && selected_<=static_cast<int>(entries_.size())) {
            view_=View::Photo;photo_loading_=true;photo_.clear();++photo_token_;
            if(load_photo) load_photo(entries_[selected_-1].id,photo_token_);
            else photo_loading_=false;
        }
    }
    return true;
}
bool PhotoMenu::receive_page(std::uint64_t token,int page,int total,std::vector<PhotoEntry> entries) {
    if(token!=token_ || page!=page_ || !active() || total<0 || total>200 || entries.size()>page_size)
        return false;
    for(const auto& e:entries) if(e.thumbnail.size()!=60*36*3) return false;
    entries_=std::move(entries);total_=total;loading_=false;
    selected_=std::min(selected_,static_cast<int>(entries_.size()));return true;
}
bool PhotoMenu::receive_photo(std::uint64_t token,std::vector<std::uint8_t> rgb) {
    if(token!=photo_token_ || view_!=View::Photo || (rgb.size()!=width*height*3 && !rgb.empty())) return false;
    photo_=std::move(rgb);photo_loading_=false;return true;
}
void PhotoMenu::capture_finished(std::uint64_t token,bool success) {
    if(token!=capture_token_ || !saving_) return;
    saving_=false;message_=success?"LEMBRANCA GUARDADA!":"FALHA AO GUARDAR. TENTE NOVAMENTE.";
    if(success && active()) { page_=0;selected_=0;reload(); }
}
bool PhotoMenu::load_native_font(const std::uint8_t* rom,std::size_t size) {
    native_font_=false; font_.fill(0);
    // Fusion USA: database.json text_characters.gfx + TextDrawCharacter's
    // PC-relative source literal (0x080791D8). Tiles are in 32-glyph rows;
    // the bottom half is 0x400 bytes after each glyph's top tile.
    constexpr std::size_t base=0x682FAC,widths=0x576234,literal=0x791D8;
    if(!rom || size!=0x800000 || rom[literal]!=0xAC || rom[literal+1]!=0x2F ||
            rom[literal+2]!=0x68 || rom[literal+3]!=0x08) return false;
    unsigned marks=0;
    for(unsigned ch=32;ch<127;++ch) {
        const unsigned id=ch>='a' && ch<='z'?ch+0x60:
            ch>='A' && ch<='Z'?ch+0x40:ch+0x20;
        const unsigned glyph_width=std::min(16u,unsigned(rom[widths+id]));
        for(unsigned y=0;y<16;++y) for(unsigned x=0;x<glyph_width;++x) {
            const unsigned offset=base+id*32+(y/8)*0x400+(x/8)*32+(y%8)*4+(x%8)/2;
            const unsigned pixel=(rom[offset]>>((x%2)*4))&15;
            // 2 is the letter; 3 is its separate one-pixel outline. Flattening
            // them into the same color makes Fusion's thin font look bold.
            font_[(ch*16+y)*16+x]=pixel==2?1:pixel==3?2:0;
            if(ch=='A' && pixel==2) ++marks;
        }
    }
    native_font_=marks>0;return native_font_;
}
bool PhotoMenu::render(std::vector<std::uint8_t>& rgb,const std::uint8_t* native) const {
    if(!active()) return false;
    if(native) rgb.assign(native,native+width*height*3);
    else rgb.assign(width*height*3,16); // source-only preview, never runtime chrome
    Canvas c{rgb,native_font_?font_.data():nullptr};
    if(view_==View::Camera) {
        c.rect(0,0,240,160,ink);c.image(scene_,scene_w_,scene_h_,0,0,240,160);
        c.rect(0,0,240,18,dark);c.frame(1,1,238,16,white);
        c.text(8,1,"CAMERA BSL",white);
        // Viewfinder corners stay outside the saved clean scene.
        for(int x:{12,216}) for(int y:{23,125}) {
            c.rect(x,y,12,1,white);c.rect(x,y,1,12,white);
        }
        c.rect(15,140,210,18,dark);c.frame(15,140,210,18,white);
        c.text(21,141,message_.empty()?"A FOTO   B ALBUM":message_,white,198);
        if(scene_.empty()) c.text(28,72,"VOLTE AO JOGO PARA FOTOGRAFAR",white,190);
        return true;
    }
    if(view_==View::Photo) {
        c.rect(0,0,240,160,ink);c.image(photo_,240,160,0,0,240,160);
        c.rect(15,140,210,18,dark);c.frame(15,140,210,18,white);
        c.text(21,141,"B VOLTAR AO ALBUM",white,198);
        if(photo_loading_ || photo_.empty()) c.text(40,72,photo_loading_?"ABRINDO FOTO...":"FOTO INDISPONIVEL",white,180);
        return true;
    }
    // The outer eight pixels remain the live Fusion menu raster. The inner
    // panel uses its actual dark background tone, with Fusion cyan/yellow trim.
    const auto background=native_paper(native);
    c.rect(8,8,224,144,background);
    c.frame(8,8,224,144,edge);c.frame(10,10,220,140,blue);
    c.rect(12,12,216,21,blue);c.rect(12,32,216,1,edge);
    c.text(18,13,"MAP",edge);c.text(166,13,"STATUS",edge);
    c.rect(61,13,90,18,{32,72,112});c.text(79,13,"ALBUM",white,70);
    c.text(18,34,"CAMERA",selected_==0?orange:ink,82);
    if(selected_==0) c.frame(14,35,87,17,orange);
    char count[24];std::snprintf(count,sizeof(count),"%03d/200",total_);
    c.text(224-c.text_width(count),34,count,ink);
    for(int i=0;i<page_size;++i) {
        const int x=16+(i%3)*70,y=55+(i/3)*39;
        c.rect(x,y,66,35,dark);c.frame(x,y,66,35,edge);
        c.rect(x+2,y+2,62,1,blue);
        if(i<static_cast<int>(entries_.size())) c.image(entries_[i].thumbnail,60,36,x+3,y+3,60,29);
        else { char number[8];std::snprintf(number,sizeof(number),"%02d",page_*6+i+1);
            c.text(x+(66-c.text_width(number))/2,y+9,number,edge); }
        if(selected_==i+1) { c.frame(x-1,y-1,68,37,orange);c.frame(x,y,66,35,white); }
    }
    c.rect(12,132,216,16,blue);
    char page_label[48];std::snprintf(page_label,sizeof(page_label),"A FOTO   L/R ABAS   %d/%d",page_+1,std::max(1,(total_+5)/6));
    std::string label=selected_>0 && selected_<=static_cast<int>(entries_.size())?
        entries_[selected_-1].title:page_label;
    if(loading_) label="ABRINDO ALBUM...";
    c.text(17,131,message_.empty()?label:message_,white,207);
    return true;
}

bool PhotoMenuPresentation::album_content_pixel(int x,int y) {
    return x>=8 && x<232 && y>=8 && y<152;
}
void PhotoMenuPresentation::reset() {
    view_=PhotoMenu::View::Closed;transition_=full_frame_=false;
    start_=0;from_.clear();previous_.clear();
}
bool PhotoMenuPresentation::compose(const PhotoMenu& menu,const std::uint8_t* native,
        std::uint64_t frame,bool inventory_visible,std::vector<std::uint8_t>& out) {
    constexpr std::size_t size=240*160*3;
    if(!native || !inventory_visible) { reset();return false; }
    const auto next=menu.view();
    if(next!=view_) {
        from_=previous_.size()==size?previous_:std::vector<std::uint8_t>(native,native+size);
        full_frame_=(view_==PhotoMenu::View::Camera || view_==PhotoMenu::View::Photo ||
                     next==PhotoMenu::View::Camera || next==PhotoMenu::View::Photo);
        start_=frame;transition_=true;view_=next;
    }
    if(!menu.render(out,native)) out.assign(native,native+size);
    if(transition_) {
        const unsigned step=frame>=start_?static_cast<unsigned>(std::min<std::uint64_t>(
            transition_frames,frame-start_+1)):transition_frames;
        for(int y=0;y<160;++y) for(int x=0;x<240;++x) {
            if(!full_frame_ && !album_content_pixel(x,y)) continue;
            const auto p=static_cast<std::size_t>((y*240+x)*3);
            for(int ch=0;ch<3;++ch) out[p+ch]=static_cast<std::uint8_t>(
                (from_[p+ch]*(transition_frames-step)+out[p+ch]*step)/transition_frames);
        }
        if(step==transition_frames) transition_=false;
    }
    previous_=out;
    return menu.active() || transition_ || next!=PhotoMenu::View::Closed;
}
} // namespace fusion
