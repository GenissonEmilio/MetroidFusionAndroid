#include "photo_menu.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string_view>

namespace minish {
namespace {
using Color = std::array<std::uint8_t, 3>;
constexpr Color ink{31,45,46}, paper{214,211,184}, edge{107,116,105},
    white{255,250,210}, teal{71,170,174}, gold{244,207,92}, dark{28,68,68};

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
    void text(int x,int y,std::string_view s,Color c=ink,int max_chars=38) {
        int n=0;
        for(unsigned char ch:s) {
            if(n++>=max_chars) break;
            if(ch>='a' && ch<='z') ch-=32;
            const auto i=alphabet.find(static_cast<char>(ch));
            if(i!=std::string_view::npos) for(int row=0;row<7;++row)
                for(int col=0;col<5;++col)
                    if(glyphs[i][row]&(16>>col)) pixel(x+col,y+row,c);
            x+=6;
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
        rect(x+4,y,9,3,dark);rect(x,y+3,21,14,dark);
        frame(x+1,y+4,19,12,white);rect(x+3,y+6,3,2,gold);
        rect(x+9,y+6,8,8,teal);frame(x+10,y+7,6,6,white);
        rect(x+12,y+9,2,2,dark);
    }
};
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
bool PhotoMenu::render(std::vector<std::uint8_t>& rgb) const {
    if(!active()) return false;
    rgb.resize(width*height*3);Canvas c{rgb};
    if(view_==View::Camera) {
        c.rect(0,0,240,160,ink);c.image(scene_,scene_w_,scene_h_,0,0,240,160);
        c.rect(0,0,240,14,dark);c.text(7,4,"CAMERA DE HYRULE",white);
        // Viewfinder corners stay outside the saved clean scene.
        for(int x:{12,216}) for(int y:{23,125}) {
            c.rect(x,y,12,1,white);c.rect(x,y,1,12,white);
        }
        c.rect(0,143,240,17,dark);
        c.text(5,148,message_.empty()?"A FOTOGRAFAR    B ALBUM":message_,white);
        if(scene_.empty()) { c.rect(15,65,210,24,paper);c.text(22,73,"VOLTE AO JOGO PARA FOTOGRAFAR"); }
        return true;
    }
    if(view_==View::Photo) {
        c.rect(0,0,240,160,ink);c.image(photo_,240,160,0,0,240,160);
        c.rect(0,146,240,14,dark);c.text(7,150,"B VOLTAR AO ALBUM",white);
        if(photo_loading_ || photo_.empty()) c.text(48,73,photo_loading_?"ABRINDO LEMBRANCA...":"FOTO INDISPONIVEL",white);
        return true;
    }
    c.rect(0,0,240,160,{167,183,123});
    for(int y=0;y<160;++y) for(int x=0;x<240;++x) {
        const unsigned n=static_cast<unsigned>(x*13+y*29+(x/11)*(y/9)*7);
        if(n%47<3) c.pixel(x,y,{180,193,139});
    }
    c.rect(17,19,207,120,edge);c.rect(19,21,203,116,paper);
    c.frame(20,22,201,114,white);
    // Subtle stone grain, an original texture rather than extracted ROM art.
    for(int y=24;y<135;++y) for(int x=22;x<219;++x)
        if((x*17+y*11)%109==0) c.pixel(x,y,{188,189,166});
    c.rect(3,3,135,23,edge);c.rect(5,5,131,19,white);c.rect(7,7,127,15,teal);
    c.text(30,11,"ALBUM DE HYRULE",dark);
    c.rect(151,5,72,24,selected_==0?gold:paper);c.frame(151,5,72,24,edge);
    c.camera(155,8);c.text(181,14,"FOTO",dark);
    c.rect(2,65,13,23,teal);c.frame(2,65,13,23,white);c.text(5,73,"<",dark);
    c.rect(225,65,13,23,teal);c.frame(225,65,13,23,white);c.text(229,73,">",dark);
    char count[40];std::snprintf(count,sizeof(count),"%03d/200  PAG %02d",total_,page_+1);
    c.text(25,30,count,edge);
    for(int i=0;i<page_size;++i) {
        const int x=23+(i%3)*66,y=42+(i/3)*46;
        c.rect(x-1,y-1,64,42,edge);c.rect(x,y,62,40,white);
        c.rect(x+1,y+1,60,36,dark);
        if(i<static_cast<int>(entries_.size())) c.image(entries_[i].thumbnail,60,36,x+1,y+1,60,36);
        if(selected_==i+1) { c.frame(x-2,y-2,66,44,gold);c.frame(x-3,y-3,68,46,dark); }
    }
    if(loading_) { c.rect(42,70,156,18,paper);c.text(51,76,"ABRINDO LEMBRANCAS..."); }
    else if(entries_.empty()) {
        c.rect(27,60,185,47,paper);c.text(42,67,"SUA AVENTURA EM IMAGENS");
        c.text(42,82,"ESCOLHA FOTO E APERTE A");c.text(54,96,"PARA GUARDAR UM LOCAL");
    }
    c.rect(15,140,210,17,ink);c.frame(15,140,210,17,white);
    const std::string label=selected_>0 && selected_<=static_cast<int>(entries_.size())?
        entries_[selected_-1].title:"A CAMERA  L/R ABAS  B VOLTAR";
    c.text(21,145,message_.empty()?label:message_,white,33);
    return true;
}
} // namespace minish
