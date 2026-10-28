#include "photo_menu.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string_view>

namespace minish {
namespace {
using Color = std::array<std::uint8_t, 3>;
constexpr Color ink{48,48,56}, paper{216,212,212}, edge{144,152,160},
    white{255,255,255}, blue{24,48,176}, orange{255,104,0}, dark{16,16,24};

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
        int right=0,left=8;
        for(int py=0;py<16;++py) for(int px=0;px<8;++px)
            if(font[(ch*16+py)*8+px]) { left=std::min(left,px);right=std::max(right,px); }
        return left==8?4:right-left+2;
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
                int left=8;
                for(int row=0;row<16;++row) for(int col=0;col<8;++col)
                    if(font[(ch*16+row)*8+col]) left=std::min(left,col);
                for(int row=0;row<16;++row) for(int col=0;col<8;++col)
                    if(font[(ch*16+row)*8+col]) pixel(x+col-left,y+row,c);
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
    std::array<unsigned,4096> counts{};
    std::array<Color,4096> colors{};
    for(int y=26;y<136;++y) for(int x=34;x<214;++x) {
        const auto* p=frame+(y*240+x)*3;
        if(p[0]<180 || p[1]<180 || p[2]<180 || p[0]>240 ||
                std::abs(int(p[0])-p[1])>24 || std::abs(int(p[1])-p[2])>24) continue;
        const int key=((p[0]>>4)<<8)|((p[1]>>4)<<4)|(p[2]>>4);
        ++counts[key];
        if(p[0]+p[1]+p[2]>colors[key][0]+colors[key][1]+colors[key][2]) colors[key]={p[0],p[1],p[2]};
    }
    const auto peak=*std::max_element(counts.begin(),counts.end());
    if(!peak) return paper;
    Color chosen{};
    // Empty item slots can be the largest gray patch. Prefer the lighter
    // common stone tone while rejecting tiny white highlights in the frame.
    for(std::size_t i=0;i<counts.size();++i)
        if(counts[i]>=std::max(1u,peak/3) && colors[i][0]+colors[i][1]+colors[i][2]>
                chosen[0]+chosen[1]+chosen[2]) chosen=colors[i];
    return chosen;
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
        // Insert exactly one page between Items (1) and Quest Status (2).
        // Both directions are read from the actual native navigation table.
        if((screen==1 && next==2 && keys==R) || (screen==2 && next==1 && keys==L)) {
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
    // USA font-page metadata: tmc data/const/text.s, gUnk_08109248.
    // The first page contains 8x16 packed-nibble glyphs, 64 bytes each.
    constexpr std::size_t table=0x109248,base=0x692F60;
    if(!rom || size!=0x1000000 || rom[table]!=0x60 || rom[table+1]!=0x2F ||
            rom[table+2]!=0x69 || rom[table+3]!=0x08) return false;
    std::array<std::uint8_t,128*8*16> decoded{};
    unsigned marks=0;
    for(unsigned ch=0;ch<128;++ch) for(unsigned y=0;y<16;++y) for(unsigned x=0;x<8;++x) {
        const unsigned pixel=(rom[base+ch*64+y*4+x/2]>>((x%2)*4))&15;
        // Zero is transparent; F is the font's column/spacing sentinel.
        decoded[(ch*16+y)*8+x]=(pixel!=0 && pixel!=15)?1:0;
        if(ch=='A') marks+=decoded[(ch*16+y)*8+x];
    }
    if(!marks) return false;
    font_=decoded;native_font_=true;return true;
}
bool PhotoMenu::render(std::vector<std::uint8_t>& rgb,const std::uint8_t* native) const {
    if(!active()) return false;
    if(native) rgb.assign(native,native+width*height*3);
    else rgb.assign(width*height*3,216); // source-only preview, never runtime chrome
    Canvas c{rgb,native_font_?font_.data():nullptr};
    if(view_==View::Camera) {
        c.rect(0,0,240,160,ink);c.image(scene_,scene_w_,scene_h_,0,0,240,160);
        c.rect(0,0,240,18,dark);c.frame(1,1,238,16,white);
        c.text(8,1,"Camera de Hyrule",white);
        // Viewfinder corners stay outside the saved clean scene.
        for(int x:{12,216}) for(int y:{23,125}) {
            c.rect(x,y,12,1,white);c.rect(x,y,1,12,white);
        }
        c.rect(15,140,210,18,dark);c.frame(15,140,210,18,white);
        c.text(21,141,message_.empty()?"A: Fotografar   B: Album":message_,white,198);
        if(scene_.empty()) c.text(28,72,"Volte ao jogo para fotografar.",white,190);
        return true;
    }
    if(view_==View::Photo) {
        c.rect(0,0,240,160,ink);c.image(photo_,240,160,0,0,240,160);
        c.rect(15,140,210,18,dark);c.frame(15,140,210,18,white);
        c.text(21,141,"B: Voltar ao album",white,198);
        if(photo_loading_ || photo_.empty()) c.text(40,72,photo_loading_?"Abrindo lembranca...":"Foto indisponivel",white,180);
        return true;
    }
    // Keep the live yellow backdrop, carved border, L/R tabs and A/B badges.
    // Only the original title field and inner content are owned by this page.
    const auto stone=native_paper(native);
    c.rect(34,35,180,101,stone);
    if(native) for(int y=35;y<136;++y) for(int x=34;x<214;++x) {
        const auto* p=native+((119+(y-26)%8)*240+34+(x-34)%8)*3;
        if(std::abs(int(p[0])-stone[0])<12 && std::abs(int(p[1])-stone[1])<12 &&
                std::abs(int(p[2])-stone[2])<12) c.pixel(x,y,{p[0],p[1],p[2]});
    }
    Color cyan{80,208,240};
    if(native) std::copy_n(native+(9*240+9)*3,3,cyan.begin());
    c.rect(4,4,103,17,cyan);
    const int title_x=(102-c.text_width("ALBUM"))/2;
    const auto title=[&](int dx,int dy,Color color) {
        int x=title_x;
        for(const char ch:std::string_view("ALBUM")) {
            c.text(x+dx,2+dy,std::string_view(&ch,1),color);
            x+=c.advance(static_cast<unsigned char>(ch))+2;
        }
    };
    for(int dx=-1;dx<=1;++dx) for(int dy=-1;dy<=1;++dy)
        title(dx,dy,blue);
    title(0,0,white);

    c.camera(43,37);c.text(64,34,"Camera",ink,38);
    if(selected_==0) c.cursor(40,34,21,19);
    char count[24];std::snprintf(count,sizeof(count),"%03d/200",total_);
    c.text(210-c.text_width(count),34,count,ink);
    Color slot=stone,highlight=stone;
    for(auto& v:slot) v=static_cast<std::uint8_t>(std::max(0,int(v)-18));
    for(auto& v:highlight) v=static_cast<std::uint8_t>(std::min(255,int(v)+24));
    for(int i=0;i<page_size;++i) {
        const int x=39+(i%3)*58,y=55+(i/3)*37;
        c.rect(x+2,y,49,33,edge);c.rect(x,y+2,53,29,edge);
        c.rect(x+2,y+1,49,31,slot);c.rect(x+1,y+2,51,29,slot);
        c.rect(x+2,y+32,49,1,highlight);c.rect(x+52,y+3,1,28,highlight);
        if(i<static_cast<int>(entries_.size())) c.image(entries_[i].thumbnail,60,36,x+2,y+2,49,29);
        if(selected_==i+1) c.cursor(x-1,y-1,55,35);
    }
    c.rect(15,140,210,18,dark);c.frame(15,140,210,18,white);
    char page_label[40];std::snprintf(page_label,sizeof(page_label),"A: Camera    %d/%d",page_+1,std::max(1,(total_+5)/6));
    std::string label=selected_>0 && selected_<=static_cast<int>(entries_.size())?
        entries_[selected_-1].title:page_label;
    if(loading_) label="Abrindo lembrancas...";
    c.text(21,141,message_.empty()?label:message_,white,198);
    return true;
}

bool PhotoMenuPresentation::album_content_pixel(int x,int y) {
    return (x>=4 && x<107 && y>=3 && y<21) ||
           (x>=34 && x<214 && y>=34 && y<136) ||
           (x>=15 && x<225 && y>=140 && y<158);
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
} // namespace minish
