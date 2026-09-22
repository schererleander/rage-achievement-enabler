#pragma once

#include <windows.h>

#include <cstdint>

namespace RageAchievements {

using HookCallback = void (*)(uintptr_t address, CONTEXT* context);

bool InstallHook(uintptr_t address, HookCallback callback);

}  // namespace RageAchievements
