#include "runtime.h"
#if defined(__ANDROID__)
#include "android_port.h"
#endif

int main(int argc, char** argv) {
    gbarecomp::RunOptions options;
    options.builtin_game_name = "Metroid Fusion";
    options.builtin_rom_sha1 = "ca33f4348c2c05dd330d37b97e2c5a69531dfe87";
    options.builtin_rom_crc32 = 0x6C75479Cu;
    options.mod_game_id = "metroid-fusion-us";
    options.launcher_region = "USA";
    options.launcher_game_config = "game.toml";
    options.launcher_expose_widescreen = false;
    options.launcher_expose_adaptive_view = false;
#if defined(__ANDROID__)
    fusion::configure_android(options);
#endif
    return gbarecomp::run_game(argc, argv, options);
}
