#pragma once
// The pinned core uses GCC's integer CLZ builtin in its BIOS math service.
// Keep the headless Windows verification build portable without editing it.
#if defined(_MSC_VER) && !defined(__clang__)
#include <bit>
#define __builtin_clz(value) std::countl_zero(static_cast<unsigned>(value))
#endif
