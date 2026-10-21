#pragma once

#include <cstdint>

namespace RageAchievements {

bool InstallAchievementHook(uintptr_t moduleBase);
bool IsUnlockedOnSteam(int32_t id);

}  // namespace RageAchievements
