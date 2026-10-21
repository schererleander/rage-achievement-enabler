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
constexpr uintptr_t kGetAchievementVtableOffset = 0x30;
constexpr uintptr_t kSetAchievementVtableOffset = 0x38;
constexpr uintptr_t kStoreStatsVtableOffset = 0x50;

using SteamUserStatsFn = void* (*)();
using GetAchievementFn = bool (*)(void*, const char*, bool*);
using SetAchievementFn = bool (*)(void*, const char*);
using StoreStatsFn = bool (*)(void*);

uintptr_t g_moduleBase = 0;

void* GetSteamUserStats() {
    auto steamUserStats = *reinterpret_cast<SteamUserStatsFn*>(g_moduleBase + kSteamUserStatsRva);
    return steamUserStats ? steamUserStats() : nullptr;
}

template <typename Fn>
Fn GetVirtual(void* interface, uintptr_t offset) {
    auto vtable = *reinterpret_cast<void***>(interface);
    return reinterpret_cast<Fn>(vtable[offset / sizeof(void*)]);
}

void UnlockOnSteam(int32_t id) {
    void* interface = GetSteamUserStats();
    if (!interface) {
        Log("RAGE Achievements: Steam user stats is unavailable for achievement %d", id);
        return;
    }

    auto setAchievement = GetVirtual<SetAchievementFn>(interface, kSetAchievementVtableOffset);
    auto storeStats = GetVirtual<StoreStatsFn>(interface, kStoreStatsVtableOffset);

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

bool IsUnlockedOnSteam(int32_t id) {
    void* interface = GetSteamUserStats();
    if (!interface) {
        return false;
    }

    auto getAchievement = GetVirtual<GetAchievementFn>(interface, kGetAchievementVtableOffset);
    char name[16] = {};
    std::snprintf(name, sizeof(name), "ach_%d", id);
    bool achieved = false;
    return getAchievement(interface, name, &achieved) && achieved;
}

bool InstallAchievementHook(uintptr_t moduleBase) {
    g_moduleBase = moduleBase;
    return InstallHook(moduleBase + kUnlockStubRva, OnUnlockStub);
}

}  // namespace RageAchievements
