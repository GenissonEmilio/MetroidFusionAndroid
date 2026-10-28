#include "photo_menu.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using Menu=minish::PhotoMenu;
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
    void open() { check(menu.update(Menu::R,1,2,true),"Items -> album was not consumed"); }
    void receive(int count,int total) {
        std::vector<minish::PhotoEntry> entries;
        for(int n=0;n<count;++n) entries.push_back({std::to_string(n),"LOCAL MARCADO",std::vector<std::uint8_t>(60*36*3,128)});
        check(menu.receive_page(page_token,page,total,std::move(entries)),"Valid page rejected");
    }
    void key(unsigned key) { check(menu.update(key,1,0,true),"Album input leaked to original menu"); }
};
void navigation() {
    Harness h;
    check(!h.menu.update(Menu::R,1,2,false),"Story/fade gate bypassed");
    check(!h.menu.update(Menu::R,7,2,true),"Unrelated native page intercepted");
    check(!h.menu.update(Menu::R|Menu::A,1,2,true),"Combined native input intercepted");
    h.open();check(h.menu.active() && h.loads==1,"Album did not load");
    h.key(Menu::A);check(h.menu.view()==Menu::View::Camera,"Camera did not open");
    h.key(Menu::B);check(h.menu.view()==Menu::View::Album,"Camera back lost album");
    h.key(Menu::L);check(!h.menu.active(),"Left did not return to Items");
    h.open();check(!h.menu.update(Menu::R,1,2,true),"Right did not delegate native transition");
    check(!h.menu.active(),"Cross-page album remained visible");
    check(h.menu.update(Menu::L,2,1,true),"Quest -> album not inserted");
    check(h.menu.update(Menu::R,2,0,true),"Return to parked Quest should consume R");
    check(!h.menu.active(),"Return to Quest left album visible");
    check(h.menu.update(Menu::L,2,1,true),"Reverse insertion failed");
    check(!h.menu.update(Menu::L,2,1,true),"Cross to Items did not delegate native L");
    h.open();check(!h.menu.update(Menu::Start,1,0,true),"START did not reach native close");
    h.open();check(!h.menu.update(0,1,0,false) && !h.menu.active(),"External lifecycle change retained UI");
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
void preview(const char* path) {
    Harness h;h.open();h.receive(0,0);
    std::vector<std::uint8_t> image;
    check(h.menu.render(image),"Album did not render");
    std::ofstream out(path,std::ios::binary);
    out<<"P6\n240 160\n255\n";
    out.write(reinterpret_cast<const char*>(image.data()),static_cast<std::streamsize>(image.size()));
    check(static_cast<bool>(out),"Preview write failed");
}
}
int main(int argc,char** argv) {
    navigation();capture();paging();
    if(argc>1) preview(argv[1]);
    std::cout<<"Photo menu navigation, ownership, capture, async lifecycle and pagination passed\n";
}
