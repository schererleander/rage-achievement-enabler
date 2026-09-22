#include "Achievement.h"

#include <windows.h>

#include <cstdint>
#include <cstdio>

#include "Hook.h"
#include "Logger.h"

namespace RageAchievements {
namespace {

constexpr uintptr_t kUnlockStubRva = 0x3CD480;
constexpr uintptr_t kSteamUserStatsRva = 0x14F4010;
constexpr uintptr_t kSetAchievementVtableOffset = 0x38;
constexpr uintptr_t kStoreStatsVtableOffset = 0x50;

using SteamUserStatsFn = void* (*)();
using SetAchievementFn = bool (*)(void*, const char*);
using StoreStatsFn = bool (*)(void*);

uintptr_t g_moduleBase = 0;

void UnlockOnSteam(int32_t id) {
    auto steamUserStats = *reinterpret_cast<SteamUserStatsFn*>(g_moduleBase + kSteamUserStatsRva);
    if (!steamUserStats) {
        Log("RAGE Achievements: SteamUserStats is unavailable for achievement %d", id);
        return;
    }

    void* interface = steamUserStats();
    if (!interface) {
        Log("RAGE Achievements: Steam user stats is unavailable for achievement %d", id);
        return;
    }

    auto vtable = *reinterpret_cast<void***>(interface);
    auto setAchievement = reinterpret_cast<SetAchievementFn>(
        vtable[kSetAchievementVtableOffset / sizeof(void*)]);
    auto storeStats = reinterpret_cast<StoreStatsFn>(
        vtable[kStoreStatsVtableOffset / sizeof(void*)]);

    char name[16] = {};
    std::snprintf(name, sizeof(name), "ach_%d", id);
    bool set = setAchievement(interface, name);
    bool stored = storeStats(interface);
    Log("RAGE Achievements: %s (SetAchievement=%s, StoreStats=%s)", name,
        set ? "true" : "false", stored ? "true" : "false");
    FlushLog();
}

void OnUnlockStub(uintptr_t, CONTEXT* context) {
    UnlockOnSteam(static_cast<int32_t>(context->Rdx));
}

}  // namespace

bool InstallAchievementHook(uintptr_t moduleBase) {
    g_moduleBase = moduleBase;
    return InstallHook(moduleBase + kUnlockStubRva, OnUnlockStub);
}

}  // namespace RageAchievements
