#pragma once

#include <cstdint>
#include <array>
#include <functional>
#include <string>
#include <vector>

namespace minish {

// This page belongs to the port's pause menu, not to the cartridge save.
// Native menu chrome is borrowed from the current frame; ROM glyphs are decoded
// in memory from the user's verified cartridge, never shipped as source/assets.
struct PhotoEntry {
    std::string id, title;
    std::vector<std::uint8_t> thumbnail; // 60x36, RGB24
};

class PhotoMenu {
public:
    static constexpr int width = 240, height = 160, page_size = 6;
    enum Keys : unsigned { A=1, B=2, Select=4, Start=8, Right=16, Left=32,
                          Up=64, Down=128, R=256, L=512 };
    enum class View { Closed, Album, Camera, Photo };
    // A return of true consumes this guest menu invocation, including its input.
    bool update(unsigned new_keys, unsigned native_screen, unsigned native_next,
                bool stable);
    void reset();
    void remember_scene(const std::uint8_t* rgb, int w, int h);
    bool load_native_font(const std::uint8_t* rom, std::size_t size);
    bool render(std::vector<std::uint8_t>& rgb,
                const std::uint8_t* native_frame = nullptr) const;
    bool receive_page(std::uint64_t token, int page, int total,
                      std::vector<PhotoEntry> entries);
    bool receive_photo(std::uint64_t token, std::vector<std::uint8_t> rgb);
    void capture_finished(std::uint64_t token, bool success);

    View view() const { return view_; }
    bool active() const { return view_ != View::Closed; }
    int selected() const { return selected_; }
    int page() const { return page_; }
    int total() const { return total_; }
    bool saving() const { return saving_; }
    std::uint64_t token() const { return token_; }

    std::function<void(int, std::uint64_t)> load_page;
    std::function<void(const std::string&, std::uint64_t)> load_photo;
    std::function<void(const std::vector<std::uint8_t>&, int, int,
                       std::uint64_t)> save_photo;
private:
    void open(unsigned screen, bool forward);
    void reload();
    void move(int dx, int dy);
    View view_ = View::Closed;
    unsigned parked_screen_ = 0;
    bool entered_forward_ = true;
    int selected_ = 0, page_ = 0, total_ = 0;
    bool loading_ = false, saving_ = false, photo_loading_ = false;
    std::uint64_t token_ = 0, photo_token_ = 0, capture_token_ = 0;
    std::string message_;
    std::vector<PhotoEntry> entries_;
    std::vector<std::uint8_t> scene_, photo_;
    int scene_w_ = 0, scene_h_ = 0;
    std::array<std::uint8_t,128*8*16> font_{};
    bool native_font_ = false;
};

// A short, frame-clocked dissolve keeps the original background/frame exact.
// It owns presentation only: original L/R/START dispatch is never delayed.
class PhotoMenuPresentation {
public:
    static constexpr unsigned transition_frames = 8;
    bool compose(const PhotoMenu& menu, const std::uint8_t* native_frame,
                 std::uint64_t frame, bool inventory_visible,
                 std::vector<std::uint8_t>& output);
    void reset();
    static bool album_content_pixel(int x,int y);
private:
    PhotoMenu::View view_ = PhotoMenu::View::Closed;
    std::uint64_t start_ = 0;
    bool transition_ = false, full_frame_ = false;
    std::vector<std::uint8_t> from_, previous_;
};

} // namespace minish
