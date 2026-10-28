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
constexpr const char* plugin = "minish-cap.game-photo-album";
constexpr std::uint32_t pause_entry = 0x080A4EA0;
// A Java admission failure may complete synchronously on the calling thread.
// Successful disk/decode completions arrive on the album worker thread.
std::recursive_mutex mutex;
minish::PhotoMenu menu;
std::vector<std::uint8_t> composed, canvas;
std::uint64_t last_frame = 0;
int last_area=-1,last_room=-1,last_x=-1,last_y=-1;

minish::MenuSnapshot snapshot(gba::GbaBus& bus) {
    const auto* i=bus.iwram_ptr();const auto* e=bus.ewram_ptr();
    return {i[0x1002],i[0x1003],i[0x1004],e[0x32EC0],e[0x32EC2],e[0x344A1],e[0x34491]};
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
    menu.reset();last_frame=0;last_area=last_room=last_x=last_y=-1;
    // This built-in page is available independently of optional mod packages.
    gba_mod_set_function_hook_enabled(plugin,1);
}
int pause_menu(std::uint32_t,int,ArmCpuState* cpu) {
    auto* bus=gbarecomp::active_bus();
    if(!bus || !cpu) return 0;
    std::lock_guard<std::recursive_mutex> guard(mutex);
    const auto s=snapshot(*bus);
    const auto* i=bus->iwram_ptr();const auto* e=bus->ewram_ptr();
    const unsigned keys=i[0xFF2]|(i[0xFF3]<<8); // gInput.newKeys (USA metadata)
    const std::uint32_t table=e[0x8C]|(e[0x8D]<<8)|(e[0x8E]<<16)|(e[0x8F]<<24);
    unsigned next=0;
    const bool table_valid=table>=0x08000000 && table+4<=0x08000000+bus->rom_size();
    if(table_valid) {
        const auto* navigation=bus->rom_ptr()+(table-0x08000000);
        if(keys==minish::PhotoMenu::L) next=navigation[1];
        if(keys==minish::PhotoMenu::R) next=navigation[2];
    }
    const bool stable=minish::album_in_inventory(s) && i[0xFD0]==0 && table_valid;
    if(!menu.update(keys,s.screen,next,stable)) return 0;
    // Return from the complete pause subtask so underlying item/map actions
    // cannot see gallery controls. VBlank/BIOS execution continues normally.
    cpu->R[15]=cpu->R[14]&~1u;
    return 1;
}
const std::uint8_t* filter(const std::uint8_t* rgb,int w,int h) {
    auto* bus=gbarecomp::active_bus();
    if(!bus || !rgb || w<1 || h<1 || w>1024 || h>1024) return rgb;
    std::lock_guard<std::recursive_mutex> guard(mutex);
    const auto s=snapshot(*bus);
    if(s.task==2 && s.state==2 && s.substate!=7) {
        menu.remember_scene(rgb,w,h);
        const auto* i=bus->iwram_ptr();
        last_area=i[0xBF4];last_room=i[0xBF5];
        last_x=i[0x118E]|(i[0x118F]<<8);last_y=i[0x1192]|(i[0x1193]<<8);
    }
    if(menu.active() && !minish::album_in_inventory(s))
        menu.update(0,s.screen,0,false);
    if(!menu.render(canvas)) return rgb;
    if(w==240 && h==160) return canvas.data();
    // Preserve the presentation contract if an optional wider view is active.
    composed.assign(static_cast<std::size_t>(w*h*3),0);
    const int dw=std::min(w,h*240/160),dh=std::min(h,w*160/240);
    const int ox=(w-dw)/2,oy=(h-dh)/2;
    for(int y=0;y<dh;++y) for(int x=0;x<dw;++x) {
        const int source=((y*160/dh)*240+x*240/dw)*3;
        std::copy_n(canvas.data()+source,3,composed.data()+((y+oy)*w+x+ox)*3);
    }
    return composed.data();
}
std::vector<std::uint8_t> bytes(JNIEnv* env,jbyteArray data,jsize maximum) {
    if(!data) return {};
    const auto size=env->GetArrayLength(data);
    if(size<0 || size>maximum) return {};
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
namespace minish {
void configure_game_photo_album() {
    menu.load_page=&load_page;menu.load_photo=&load_photo;menu.save_photo=&save_photo;
    set_frame_filter(&filter);
}
void photo_album_frame(std::uint64_t frame) {
    std::lock_guard<std::recursive_mutex> guard(mutex);
    if(frame<last_frame) menu.reset(); // rewind/load must discard transient UI
    last_frame=frame;
}
}
GBA_MOD_CONSTRUCTOR(minish_register_photo_menu) {
    gba_mod_register_function_entry_plugin(plugin,pause_entry,1,&pause_menu);
    gba_mod_register_reset_callback(&reset);
}
extern "C" JNIEXPORT void JNICALL
Java_io_github_genissonemilio_minishcap_GameAlbumBridge_nativePage(
        JNIEnv* env,jclass,jlong token,jint page,jint total,
        jobjectArray ids,jobjectArray titles,jbyteArray thumbnails) {
    if(!ids || !titles) return;
    const auto count=env->GetArrayLength(ids);
    if(count>6 || env->GetArrayLength(titles)!=count) return;
    auto rgb=bytes(env,thumbnails,6*60*36*3);
    if(rgb.size()!=static_cast<std::size_t>(count*60*36*3)) return;
    std::vector<minish::PhotoEntry> entries;
    for(jsize n=0;n<count;++n) {
        minish::PhotoEntry entry{string(env,ids,n),string(env,titles,n),{}};
        entry.thumbnail.assign(rgb.begin()+n*60*36*3,rgb.begin()+(n+1)*60*36*3);
        entries.push_back(std::move(entry));
    }
    std::lock_guard<std::recursive_mutex> guard(mutex);
    menu.receive_page(static_cast<std::uint64_t>(token),page,total,std::move(entries));
}
extern "C" JNIEXPORT void JNICALL
Java_io_github_genissonemilio_minishcap_GameAlbumBridge_nativePhoto(
        JNIEnv* env,jclass,jlong token,jbyteArray image) {
    auto rgb=bytes(env,image,240*160*3);
    std::lock_guard<std::recursive_mutex> guard(mutex);
    menu.receive_photo(static_cast<std::uint64_t>(token),std::move(rgb));
}
extern "C" JNIEXPORT void JNICALL
Java_io_github_genissonemilio_minishcap_GameAlbumBridge_nativeSaved(
        JNIEnv*,jclass,jlong token,jboolean success) {
    std::lock_guard<std::recursive_mutex> guard(mutex);
    menu.capture_finished(static_cast<std::uint64_t>(token),success==JNI_TRUE);
}
