#pragma once
#include <cstdint>

namespace minish {
struct MenuSnapshot {
    std::uint8_t task, state, substate, phase, subtask, menu_phase, screen;
};
// USA metadata only: pinned zeldaret/tmc main.h, game.h, subtask.h,
// pauseMenu.h and linker.ld (5ab63f00). No guest code or memory is changed.
inline bool album_in_inventory(const MenuSnapshot& s) {
    const bool page=s.screen==1 || s.screen==2 || s.screen==4 || s.screen==5 ||
        s.screen==6 || s.screen==7 || s.screen==8;
    return s.task==2 && s.state==2 && s.substate==7 && s.phase==2 &&
        s.subtask==1 && s.menu_phase==2 && page;
}
}
