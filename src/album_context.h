#pragma once
#include <cstdint>
namespace fusion {
struct MenuSnapshot {
    std::uint16_t mode;
    std::uint8_t pause_flag,phase,screen,substage;
};
// USA metadata: gMainGameMode, gPauseScreenFlag, gUnk_3000b85 and
// gNonGameplayRam.pauseScreenData. Manual pause is flag 2; navigation dialogs,
// map downloads, sleep, fades and status transitions must retain native input.
inline bool album_in_inventory(const MenuSnapshot& s) {
    return s.mode==3 && s.pause_flag==2 && s.phase==2 &&
        (s.screen==0 || s.screen==1) && s.substage==0;
}
}
