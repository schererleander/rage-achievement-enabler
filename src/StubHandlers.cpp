#include "StubHandlers.h"

#include <windows.h>

#include <cstdint>
#include <cstring>

#include "Achievement.h"
#include "Hook.h"
#include "Logger.h"

namespace RageAchievements {
namespace {

constexpr uintptr_t kAwardEnabledRva = 0x41CA338;  // achievements_awardEnabled integer value
constexpr uintptr_t kAwardAchievementRva = 0xE9F880;
constexpr uintptr_t kCheckRequirementsRva = 0xE9F9C0;

constexpr uintptr_t kManagerPlayer = 0x08;
constexpr uintptr_t kManagerEntries = 0x10;
constexpr uintptr_t kManagerEntryCount = 0x18;

constexpr uintptr_t kEntrySize = 320;
constexpr uintptr_t kEntryDecl = 0x08;
constexpr uintptr_t kEntrySpecialDone = 22;
constexpr uintptr_t kEntryAchievementsDone = 28;  // bool per required achievement id
constexpr int kMaxRequiredAchievementId = 52;     // event counters start at +80
constexpr uintptr_t kEntryRacePlaces = 200;       // best raceMemoryPlace_t per required race
constexpr int kMaxRequiredRaces = (kEntrySize - kEntryRacePlaces) / sizeof(int32_t);

constexpr uintptr_t kDeclRaces = 0xC8;
constexpr uintptr_t kDeclImpaleAnEnemy = 0x101;
constexpr uintptr_t kDeclWinOnGreen = 0x103;
constexpr uintptr_t kDeclUltraNightmare = 0x105;
constexpr uintptr_t kDeclAchievements = 0x108;

constexpr uintptr_t kListData = 0x00;
constexpr uintptr_t kListNum = 0x08;

constexpr uintptr_t kRaceSize = 0x30;
constexpr uintptr_t kRaceNameData = 0x08;  // idStr::data

using AwardAchievementFn = void (*)(uintptr_t manager, uintptr_t decl, bool unlock);
using CheckRequirementsFn = void (*)(uintptr_t manager, bool unlock);

uintptr_t g_moduleBase = 0;
bool g_checkingRequirements = false;

template <typename T>
T& At(uintptr_t address, uintptr_t offset) {
    return *reinterpret_cast<T*>(address + offset);
}

bool AwardEnabled() {
    return At<int32_t>(g_moduleBase, kAwardEnabledRva) != 0;
}

// The roulette caller passes player + 0x4058 even when the player lookup fails.
bool IsManager(uintptr_t manager) {
    return manager >= 0x10000;
}

int EntryCount(uintptr_t manager) {
    return At<int32_t>(manager, kManagerEntryCount);
}

uintptr_t Entry(uintptr_t manager, int index) {
    return At<uintptr_t>(manager, kManagerEntries) + index * kEntrySize;
}

void AwardAchievement(uintptr_t manager, uintptr_t decl, bool unlock) {
    reinterpret_cast<AwardAchievementFn>(g_moduleBase + kAwardAchievementRva)(manager, decl, unlock);
}

// AwardAchievement calls back into AchievementCompletesAchievement, so guard against recursion.
void CheckRequirements(uintptr_t manager, bool unlock) {
    if (g_checkingRequirements || At<uintptr_t>(manager, kManagerPlayer) == 0) {
        return;
    }
    g_checkingRequirements = true;
    reinterpret_cast<CheckRequirementsFn>(g_moduleBase + kCheckRequirementsRva)(manager, unlock);
    g_checkingRequirements = false;
}

bool AwardFlagged(uintptr_t manager, uintptr_t flagOffset) {
    if (!AwardEnabled() || !IsManager(manager)) {
        return false;
    }

    bool awarded = false;
    for (int i = 0; i < EntryCount(manager); ++i) {
        uintptr_t entry = Entry(manager, i);
        uintptr_t decl = At<uintptr_t>(entry, kEntryDecl);
        if (decl && At<uint8_t>(decl, flagOffset)) {
            AwardAchievement(manager, decl, true);
            At<uint8_t>(entry, kEntrySpecialDone) = 1;
            awarded = true;
        }
    }
    return awarded;
}

bool ImpaledCompletesAchievement(uintptr_t manager) {
    return AwardFlagged(manager, kDeclImpaleAnEnemy);
}

bool WonOnGreenCompletesAchievement(uintptr_t manager) {
    return AwardFlagged(manager, kDeclWinOnGreen);
}

bool UltraNightmareCompletesAchievement(uintptr_t manager) {
    return AwardFlagged(manager, kDeclUltraNightmare);
}

bool RaceCompletesAchievement(uintptr_t manager, const char* raceName, int32_t place,
                              bool unlock) {
    if (!AwardEnabled() || !IsManager(manager) || !raceName) {
        return false;
    }

    bool updated = false;
    for (int i = 0; i < EntryCount(manager); ++i) {
        uintptr_t entry = Entry(manager, i);
        uintptr_t decl = At<uintptr_t>(entry, kEntryDecl);
        if (!decl) {
            continue;
        }

        uintptr_t races = At<uintptr_t>(decl, kDeclRaces + kListData);
        int raceCount = At<int32_t>(decl, kDeclRaces + kListNum);
        for (int race = 0; race < raceCount && race < kMaxRequiredRaces; ++race) {
            uintptr_t required = races + race * kRaceSize;
            const char* name = At<const char*>(required, kRaceNameData);
            int32_t& bestPlace = At<int32_t>(entry, kEntryRacePlaces + race * sizeof(int32_t));
            if (name && _stricmp(name, raceName) == 0 && place > bestPlace) {
                bestPlace = place;
                updated = true;
            }
        }
    }

    if (updated) {
        CheckRequirements(manager, unlock);
    }
    return updated;
}

bool AchievementCompletesAchievement(uintptr_t manager, int32_t id, bool unlock) {
    if (!AwardEnabled() || !IsManager(manager)) {
        return false;
    }

    bool affected = false;
    for (int i = 0; i < EntryCount(manager); ++i) {
        uintptr_t entry = Entry(manager, i);
        uintptr_t decl = At<uintptr_t>(entry, kEntryDecl);
        if (!decl) {
            continue;
        }

        auto* required = At<const int32_t*>(decl, kDeclAchievements + kListData);
        int requiredCount = At<int32_t>(decl, kDeclAchievements + kListNum);
        bool needsId = false;
        for (int j = 0; j < requiredCount; ++j) {
            needsId |= required[j] == id;
        }
        if (!needsId) {
            continue;
        }

        // The game only tracks dependencies awarded in this session, so also ask Steam.
        affected = true;
        for (int j = 0; j < requiredCount; ++j) {
            int32_t dependency = required[j];
            if (dependency < 0 || dependency >= kMaxRequiredAchievementId) {
                continue;
            }
            if (dependency == id || IsUnlockedOnSteam(dependency)) {
                At<uint8_t>(entry, kEntryAchievementsDone + dependency) = 1;
            }
        }
    }

    if (affected) {
        CheckRequirements(manager, unlock);
    }
    return affected;
}

struct StubReplacement {
    const char* name;
    uintptr_t rva;
    uintptr_t nameStringRva;
    const void* replacement;
};

const StubReplacement kStubReplacements[] = {
    {"ImpaledCompletesAchievement", 0xE9F3B0, 0x19E0908,
     reinterpret_cast<const void*>(&ImpaledCompletesAchievement)},
    {"WonOnGreenCompletesAchievement", 0xE9F480, 0x19E0968,
     reinterpret_cast<const void*>(&WonOnGreenCompletesAchievement)},
    {"UltraNightmareCompletesAchievement", 0xE9F550, 0x19E09C8,
     reinterpret_cast<const void*>(&UltraNightmareCompletesAchievement)},
    {"RaceCompletesAchievement", 0xE9F6E0, 0x19E0A88,
     reinterpret_cast<const void*>(&RaceCompletesAchievement)},
    {"AchievementCompletesAchievement", 0xE9F7B0, 0x19E0AE0,
     reinterpret_cast<const void*>(&AchievementCompletesAchievement)},
};

constexpr unsigned char kStubPrologue[] = {
    0x40, 0x57, 0x48, 0x83, 0xEC, 0x40, 0x48, 0xC7,
    0x44, 0x24, 0x20, 0xFE, 0xFF, 0xFF, 0xFF, 0x48,
};
constexpr uintptr_t kStubNameLeaOffset = 0x4A;
constexpr unsigned char kLeaR11[] = {0x4C, 0x8D, 0x1D};

bool IsExpectedStub(const StubReplacement& stub) {
    uintptr_t address = g_moduleBase + stub.rva;
    if (std::memcmp(reinterpret_cast<const void*>(address), kStubPrologue,
                    sizeof(kStubPrologue)) != 0) {
        return false;
    }

    uintptr_t lea = address + kStubNameLeaOffset;
    if (std::memcmp(reinterpret_cast<const void*>(lea), kLeaR11, sizeof(kLeaR11)) != 0) {
        return false;
    }
    int32_t displacement = 0;
    std::memcpy(&displacement, reinterpret_cast<const void*>(lea + sizeof(kLeaR11)),
                sizeof(displacement));
    return lea + 7 + displacement == g_moduleBase + stub.nameStringRva;
}

}  // namespace

bool InstallStubHandlers(uintptr_t moduleBase) {
    g_moduleBase = moduleBase;

    bool ok = true;
    for (const auto& stub : kStubReplacements) {
        if (!IsExpectedStub(stub)) {
            Log("RAGE Achievements: %s does not match the expected stub, skipping", stub.name);
            ok = false;
            continue;
        }
        if (!InstallJump(moduleBase + stub.rva, stub.replacement)) {
            ok = false;
            continue;
        }
        Log("RAGE Achievements: replaced %s", stub.name);
    }
    return ok;
}

}  // namespace RageAchievements
