#include "game_photo_album.h"
#include "photo_menu.h"
#include "album_context.h"
#include "pixel_presenter.h"
#include "mod_function_hooks.h"
#include "mod_runtime.h"
#include "runtime_bus_bridge.h"
#include "gba_bus.h"
#include <SDL.h>
#include <jni.h>
#include <algorithm>
#include <mutex>

namespace {
constexpr const char* plugin = "metroid-fusion.game-photo-album";
constexpr std::uint32_t pause_entry = 0x080770B4;
// A Java admission failure may complete synchronously on the calling thread.
// Successful disk/decode completions arrive on the album worker thread.
std::recursive_mutex mutex;
fusion::PhotoMenu menu;
fusion::PhotoMenuPresentation presentation;
std::vector<std::uint8_t> composed, canvas, native_menu;
const std::uint8_t* font_rom=nullptr;
std::uint64_t last_frame = 0;
int last_area=-1,last_room=-1,last_x=-1,last_y=-1;

fusion::MenuSnapshot snapshot(gba::GbaBus& bus) {
    const auto* i=bus.iwram_ptr();
    return {static_cast<std::uint16_t>(i[0xBDE]|(i[0xBDF]<<8)),
        i[0xB84],i[0xB85],i[0x14AC],i[0x148B]};
}
struct Activity {
    JNIEnv* env=static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    jobject object=env?static_cast<jobject>(SDL_AndroidGetActivity()):nullptr;
    jclass cls=object?env->GetObjectClass(object):nullptr;
    ~Activity() {
        if(cls) env->DeleteLocalRef(cls);
        if(object) env->DeleteLocalRef(object);
    }
    jmethodID method(const char* name,const char* signature) {
        return cls?env->GetMethodID(cls,name,signature):nullptr;
    }
};
void load_page(int page,std::uint64_t token) {
    Activity a;
    if(auto method=a.method("loadGameAlbum","(IJ)V"))
        a.env->CallVoidMethod(a.object,method,page,static_cast<jlong>(token));
}
void load_photo(const std::string& id,std::uint64_t token) {
    Activity a;
    if(auto method=a.method("loadGamePhoto","(Ljava/lang/String;J)V")) {
        auto name=a.env->NewStringUTF(id.c_str());
        a.env->CallVoidMethod(a.object,method,name,static_cast<jlong>(token));
        a.env->DeleteLocalRef(name);
    }
}
void save_photo(const std::vector<std::uint8_t>& rgb,int w,int h,std::uint64_t token) {
    Activity a;
    if(auto method=a.method("saveGamePhoto","([BIIIIIIJ)V")) {
        auto bytes=a.env->NewByteArray(static_cast<jsize>(rgb.size()));
        if(!bytes) { menu.capture_finished(token,false);return; }
        a.env->SetByteArrayRegion(bytes,0,static_cast<jsize>(rgb.size()),reinterpret_cast<const jbyte*>(rgb.data()));
        a.env->CallVoidMethod(a.object,method,bytes,w,h,last_area,last_room,last_x,last_y,static_cast<jlong>(token));
        a.env->DeleteLocalRef(bytes);
    } else menu.capture_finished(token,false);
}
void reset() {
    std::lock_guard<std::recursive_mutex> guard(mutex);
    menu.reset();presentation.reset();font_rom=nullptr;
    last_frame=0;last_area=last_room=last_x=last_y=-1;
    // This built-in page is available independently of optional mod packages.
    gba_mod_set_function_hook_enabled(plugin,1);
}
int pause_menu(std::uint32_t,int,ArmCpuState* cpu) {
    auto* bus=gbarecomp::active_bus();
    if(!bus || !cpu) return 0;
    std::lock_guard<std::recursive_mutex> guard(mutex);
    const auto s=snapshot(*bus);
    if(font_rom!=bus->rom_ptr()) {
        font_rom=bus->rom_ptr();menu.load_native_font(font_rom,bus->rom_size());
    }
    const auto* i=bus->iwram_ptr();
    const unsigned keys=i[0x11EC]|(i[0x11ED]<<8); // gChangedInput
    const unsigned next=s.screen==0?1:0;
    const bool stable=fusion::album_in_inventory(s);
    if(!menu.update(keys,s.screen,next,stable)) return 0;
    // Replace only the current menu handler. Its caller still updates OAM,
    // the minimap cursor, VBlank and audio; false in r0 means keep paused.
    cpu->R[0]=0;
    cpu->R[15]=cpu->R[14]&~1u;
    return 1;
}
const std::uint8_t* filter(const std::uint8_t* rgb,int w,int h) {
    auto* bus=gbarecomp::active_bus();
    if(!bus || !rgb || w<1 || h<1 || w>1024 || h>1024) return rgb;
    std::lock_guard<std::recursive_mutex> guard(mutex);
    const auto s=snapshot(*bus);
    if(s.mode==1 && bus->iwram_ptr()[0xBE0]==2) {
        menu.remember_scene(rgb,w,h);
        const auto* i=bus->iwram_ptr();
        last_area=i[0x2C];last_room=i[0x2D];
        last_x=i[0x125A]|(i[0x125B]<<8);last_y=i[0x125C]|(i[0x125D]<<8);
    }
    const bool inventory_visible=fusion::album_in_inventory(s);
    if(menu.active() && !inventory_visible) menu.update(0,s.screen,0,false);
    if(!inventory_visible) { presentation.reset();return rgb; }
    const int dw=std::min(w,h*240/160),dh=std::min(h,w*160/240);
    const int ox=(w-dw)/2,oy=(h-dh)/2;
    const std::uint8_t* native=rgb;
    if(w!=240 || h!=160) {
        native_menu.resize(240*160*3);
        for(int y=0;y<160;++y) for(int x=0;x<240;++x) {
            const auto p=((oy+y*dh/160)*w+ox+x*dw/240)*3;
            std::copy_n(rgb+p,3,native_menu.data()+(y*240+x)*3);
        }
        native=native_menu.data();
    }
    if(!presentation.compose(menu,native,last_frame,inventory_visible,canvas)) return rgb;
    if(w==240 && h==160) return canvas.data();
    // Optional view margins remain the live native background, too.
    composed.assign(rgb,rgb+w*h*3);
    for(int y=0;y<dh;++y) for(int x=0;x<dw;++x) {
        const int source=((y*160/dh)*240+x*240/dw)*3;
        std::copy_n(canvas.data()+source,3,composed.data()+((y+oy)*w+x+ox)*3);
    }
    return composed.data();
}
std::vector<std::uint8_t> bytes(JNIEnv* env,jbyteArray data,jsize maximum) {
    if(!data) return {};
    const auto size=env->GetArrayLength(data);
    if(size<=0 || size>maximum) return {};
    std::vector<std::uint8_t> out(size);
    env->GetByteArrayRegion(data,0,size,reinterpret_cast<jbyte*>(out.data()));return out;
}
std::string string(JNIEnv* env,jobjectArray array,jsize index) {
    auto s=static_cast<jstring>(env->GetObjectArrayElement(array,index));
    if(!s) return {};
    const char* utf=env->GetStringUTFChars(s,nullptr);
    std::string result=utf?utf:"";
    if(utf) env->ReleaseStringUTFChars(s,utf);
    env->DeleteLocalRef(s);return result;
}
}
namespace fusion {
void configure_game_photo_album() {
    menu.load_page=&load_page;menu.load_photo=&load_photo;menu.save_photo=&save_photo;
    set_frame_filter(&filter);
    gba_mod_set_function_hook_enabled(plugin,1);
}
void photo_album_frame(std::uint64_t frame) {
    std::lock_guard<std::recursive_mutex> guard(mutex);
    if(frame<last_frame) { menu.reset();presentation.reset(); }
    last_frame=frame;
}
}
GBA_MOD_CONSTRUCTOR(fusion_register_photo_menu) {
    gba_mod_register_function_entry_plugin(plugin,pause_entry,1,&pause_menu);
    gba_mod_register_reset_callback(&reset);
}
extern "C" JNIEXPORT void JNICALL
Java_io_github_genissonemilio_metroidfusion_GameAlbumBridge_nativePage(
        JNIEnv* env,jclass,jlong token,jint page,jint total,
        jobjectArray ids,jobjectArray titles,jbyteArray thumbnails) {
    if(!ids || !titles) return;
    const auto count=env->GetArrayLength(ids);
    if(count>6 || env->GetArrayLength(titles)!=count) return;
    auto rgb=bytes(env,thumbnails,6*60*36*3);
    if(rgb.size()!=static_cast<std::size_t>(count*60*36*3)) return;
    std::vector<fusion::PhotoEntry> entries;
    for(jsize n=0;n<count;++n) {
        fusion::PhotoEntry entry{string(env,ids,n),string(env,titles,n),{}};
        entry.thumbnail.assign(rgb.begin()+n*60*36*3,rgb.begin()+(n+1)*60*36*3);
        entries.push_back(std::move(entry));
    }
    std::lock_guard<std::recursive_mutex> guard(mutex);
    menu.receive_page(static_cast<std::uint64_t>(token),page,total,std::move(entries));
}
extern "C" JNIEXPORT void JNICALL
Java_io_github_genissonemilio_metroidfusion_GameAlbumBridge_nativePhoto(
        JNIEnv* env,jclass,jlong token,jbyteArray image) {
    auto rgb=bytes(env,image,240*160*3);
    std::lock_guard<std::recursive_mutex> guard(mutex);
    menu.receive_photo(static_cast<std::uint64_t>(token),std::move(rgb));
}
extern "C" JNIEXPORT void JNICALL
Java_io_github_genissonemilio_metroidfusion_GameAlbumBridge_nativeSaved(
        JNIEnv*,jclass,jlong token,jboolean success) {
    std::lock_guard<std::recursive_mutex> guard(mutex);
    menu.capture_finished(static_cast<std::uint64_t>(token),success==JNI_TRUE);
}
