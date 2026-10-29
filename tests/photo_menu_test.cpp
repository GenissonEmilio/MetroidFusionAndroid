#include "photo_menu.h"
#include "album_context.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <iterator>

namespace {
using Menu=fusion::PhotoMenu;
void check(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
struct Harness {
    Menu menu;
    int page=-1, loads=0, saves=0;
    std::uint64_t page_token=0,photo_token=0,save_token=0;
    std::string photo_id;
    std::vector<std::uint8_t> saved;
    Harness() {
        menu.load_page=[&](int p,std::uint64_t t) { page=p;page_token=t;++loads; };
        menu.load_photo=[&](const std::string& id,std::uint64_t t) { photo_id=id;photo_token=t; };
        menu.save_photo=[&](const std::vector<std::uint8_t>& rgb,int w,int h,std::uint64_t t) {
            check(w==240 && h==160,"Capture dimensions changed");saved=rgb;save_token=t;++saves;
        };
    }
    void open() { check(menu.update(Menu::R,0,1,true),"Map -> album was not consumed"); }
    void receive(int count,int total) {
        std::vector<fusion::PhotoEntry> entries;
        for(int n=0;n<count;++n) entries.push_back({std::to_string(n),"LOCAL MARCADO",std::vector<std::uint8_t>(60*36*3,128)});
        check(menu.receive_page(page_token,page,total,std::move(entries)),"Valid page rejected");
    }
    void key(unsigned key) { check(menu.update(key,0,0,true),"Album input leaked to original menu"); }
};
void navigation() {
    check(fusion::album_in_inventory({3,2,2,0,0}),"Manual Map rejected");
    check(fusion::album_in_inventory({3,2,2,1,0}),"Manual Status rejected");
    for(unsigned flag : {0u,1u,3u,4u,5u,6u})
        check(!fusion::album_in_inventory({3,static_cast<std::uint8_t>(flag),2,0,0}),"Story/navigation pause intercepted");
    check(!fusion::album_in_inventory({3,2,1,0,0}),"Fade intercepted");
    check(!fusion::album_in_inventory({3,2,2,6,0}),"Status transition intercepted");
    check(!fusion::album_in_inventory({1,2,2,0,0}),"Gameplay intercepted");
    Harness h;
    check(!h.menu.update(Menu::R,0,1,false),"Story/fade gate bypassed");
    check(!h.menu.update(Menu::R,7,2,true),"Unrelated native page intercepted");
    check(!h.menu.update(Menu::R|Menu::A,0,1,true),"Combined native input intercepted");
    h.open();check(h.menu.active() && h.loads==1,"Album did not load");
    h.key(Menu::A);check(h.menu.view()==Menu::View::Camera,"Camera did not open");
    h.key(Menu::B);check(h.menu.view()==Menu::View::Album,"Camera back lost album");
    h.key(Menu::L);check(!h.menu.active(),"Left did not return to Map");
    h.open();check(!h.menu.update(Menu::R,0,1,true),"Right did not delegate native transition");
    check(!h.menu.active(),"Cross-page album remained visible");
    check(h.menu.update(Menu::L,1,0,true),"Status -> album not inserted");
    check(h.menu.update(Menu::R,1,0,true),"Return to parked Status should consume R");
    check(!h.menu.active(),"Return to Status left album visible");
    check(h.menu.update(Menu::L,1,0,true),"Reverse insertion failed");
    check(!h.menu.update(Menu::L,1,0,true),"Cross to Map did not delegate native L");
    h.open();check(!h.menu.update(Menu::Start,0,0,true),"START did not reach native close");
    h.open();check(!h.menu.update(0,0,0,false) && !h.menu.active(),"External lifecycle change retained UI");
}
void capture() {
    Harness h;h.open();h.receive(0,0);h.key(Menu::A);h.key(Menu::A);
    check(h.saves==0,"Camera invented a scene before gameplay");
    std::vector<std::uint8_t> original(240*160*3);
    for(std::size_t n=0;n<original.size();++n) original[n]=static_cast<std::uint8_t>(n*17);
    h.menu.remember_scene(original.data(),240,160);
    std::vector<std::uint8_t> rendered;
    check(h.menu.render(rendered) && rendered.size()==original.size(),"Camera rendering invalid");
    check(rendered!=original,"Viewfinder did not render");
    h.key(Menu::A);h.key(Menu::A);
    check(h.saves==1 && h.menu.saving(),"Repeated A duplicated an in-flight capture");
    check(h.saved==original,"Menu/viewfinder contaminated saved image");
    h.menu.capture_finished(h.save_token+1,true);check(h.menu.saving(),"Stale save completion accepted");
    h.menu.capture_finished(h.save_token,true);check(!h.menu.saving() && h.loads==2,"Save did not reload album");
    check(h.saved==original,"Capture source mutated after save");
    h.menu.reset();check(!h.menu.active(),"Reset retained page");
    h.open();h.receive(0,200);h.key(Menu::A);h.key(Menu::A);
    check(h.saves==1,"Album limit ignored or stale scene survived reset");
}
void paging() {
    Harness h;h.open();const auto stale=h.page_token;h.receive(6,14);
    h.key(Menu::Down);h.key(Menu::Right);h.key(Menu::Right);h.key(Menu::Right);
    check(h.menu.page()==1 && h.loads==2,"Next photo page unavailable");
    check(!h.menu.receive_page(stale,0,0,{}),"Stale page response replaced current selection");
    h.receive(6,14);h.key(Menu::A);check(h.photo_id=="0","Wrong photo requested");
    check(h.menu.receive_photo(h.photo_token,std::vector<std::uint8_t>(240*160*3,90)),"Full photo rejected");
    h.key(Menu::B);check(!h.menu.receive_photo(h.photo_token,{}),"Stale photo replaced grid");
    h.key(Menu::Left);check(h.menu.page()==0,"Previous photo page unavailable");
    h.receive(6,14);h.key(Menu::Down);h.key(Menu::Down);
    check(h.menu.selected()==4,"Grid row navigation failed");
    h.key(Menu::Up);h.key(Menu::Up);check(h.menu.selected()==0,"Camera tile unreachable");
    const auto old=h.page_token;h.menu.reset();
    check(!h.menu.receive_page(old,0,0,{}),"Closed/reset menu accepted page callback");
}
void presentation() {
    Harness h;fusion::PhotoMenuPresentation display;
    std::vector<std::uint8_t> native(240*160*3,214),out;
    // An animated backdrop must survive each intermediate transition frame.
    const auto original=native;
    check(!display.compose(h.menu,native.data(),99,true,out),"Closed menu replaced native frame");
    h.open();h.receive(0,0);
    for(std::uint64_t frame=100;frame<108;++frame) {
        native[0]=static_cast<std::uint8_t>(frame);
        check(display.compose(h.menu,native.data(),frame,true,out),"Opening page did not compose");
        for(int y=0;y<160;++y) for(int x=0;x<240;++x) {
            if(fusion::PhotoMenuPresentation::album_content_pixel(x,y)) continue;
            const int p=(y*240+x)*3;
            check(std::equal(out.begin()+p,out.begin()+p+3,native.begin()+p),
                "Backdrop, border or original L/R controls changed");
        }
        auto same=out;
        display.compose(h.menu,native.data(),frame,true,out);
        check(out==same,"Presentation timing depended on host redraw count");
    }
    check(out!=native,"Album content did not render");
    h.key(Menu::L);
    check(display.compose(h.menu,native.data(),108,true,out),"Closing page snapped without transition");
    display.compose(h.menu,native.data(),115,true,out);
    check(out==native,"Closing transition left stale photo pixels");
    check(!display.compose(h.menu,native.data(),116,true,out),"Closed compositor remained active");
    h.open();display.compose(h.menu,native.data(),117,true,out);
    check(!display.compose(h.menu,native.data(),118,false,out),"Album leaked into gameplay/restore");
    check(std::equal(native.begin()+1,native.end(),original.begin()+1),"Compositor modified guest pixels");
    std::vector<std::uint8_t> fake_rom(0x800000);
    check(!h.menu.load_native_font(fake_rom.data(),fake_rom.size()),"Unknown font layout accepted");
    const unsigned char pointer[]={0xAC,0x2F,0x68,0x08};
    std::copy(std::begin(pointer),std::end(pointer),fake_rom.begin()+0x791D8);
    fake_rom[0x576234+0x81]=8;
    fake_rom[0x682FAC+0x81*32+3*4+1]=0x20;
    check(h.menu.load_native_font(fake_rom.data(),fake_rom.size()),"Bounded native font unavailable");
    check(!h.menu.load_native_font(fake_rom.data(),1024),"Truncated ROM accepted for font");
}
void preview(const char* path,const char* frame_path,const char* rom_path) {
    Harness h;h.open();h.receive(0,0);
    std::vector<std::uint8_t> image,native;
    if(frame_path) {
        std::ifstream in(frame_path,std::ios::binary);
        std::string kind;int w=0,height=0,maximum=0;
        in>>kind>>w>>height>>maximum;in.get();
        check(kind=="P6" && w==240 && height==160 && maximum==255,"Wrong reference frame format");
        native.resize(240*160*3);in.read(reinterpret_cast<char*>(native.data()),native.size());
        check(static_cast<bool>(in),"Reference frame read failed");
    }
    if(rom_path) {
        std::ifstream in(rom_path,std::ios::binary);
        std::vector<std::uint8_t> rom((std::istreambuf_iterator<char>(in)),{});
        check(h.menu.load_native_font(rom.data(),rom.size()),"User ROM native font decode failed");
    }
    check(h.menu.render(image,native.empty()?nullptr:native.data()),"Album did not render");
    std::ofstream out(path,std::ios::binary);
    out<<"P6\n240 160\n255\n";
    out.write(reinterpret_cast<const char*>(image.data()),static_cast<std::streamsize>(image.size()));
    check(static_cast<bool>(out),"Preview write failed");
}
}
int main(int argc,char** argv) {
    navigation();capture();paging();presentation();
    if(argc>1) preview(argv[1],argc>2?argv[2]:nullptr,argc>3?argv[3]:nullptr);
    std::cout<<"Photo menu navigation, capture, lifecycle, native chrome and transitions passed\n";
}
