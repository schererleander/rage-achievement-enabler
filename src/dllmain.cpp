#include <windows.h>

#include <cstdint>

#include "Achievement.h"
#include "GameReady.h"
#include "Logger.h"

namespace {

DWORD WINAPI WorkerThread(LPVOID) {
    RageAchievements::OpenLog();
    RageAchievements::Log("RAGE Achievements: starting");

    if (RageAchievements::WaitForGameUnpack(60000)) {
        auto base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
        if (!RageAchievements::InstallAchievementHook(base)) {
            RageAchievements::Log("RAGE Achievements: failed to install unlock hook");
        }
    }
    RageAchievements::FlushLog();
    return 0;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
        if (thread) {
            CloseHandle(thread);
        }
    }
    return TRUE;
}
