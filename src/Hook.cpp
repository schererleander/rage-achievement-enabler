#include "Hook.h"

#include <cstring>

#include "Logger.h"

namespace RageAchievements {
namespace {

struct Hook {
    uintptr_t address = 0;
    unsigned char originalByte = 0;
    HookCallback callback = nullptr;
    bool armed = false;
};

Hook g_hook;
void* g_handler = nullptr;
thread_local bool t_rearmPending = false;

void WriteByte(uintptr_t address, unsigned char value) {
    auto* target = reinterpret_cast<void*>(address);
    DWORD oldProtection = 0;
    VirtualProtect(target, 1, PAGE_EXECUTE_READWRITE, &oldProtection);
    *reinterpret_cast<unsigned char*>(target) = value;
    VirtualProtect(target, 1, oldProtection, &oldProtection);
    FlushInstructionCache(GetCurrentProcess(), target, 1);
}

LONG CALLBACK ExceptionHandler(EXCEPTION_POINTERS* info) {
    CONTEXT* context = info->ContextRecord;

    if (info->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP && t_rearmPending) {
        t_rearmPending = false;
        WriteByte(g_hook.address, 0xCC);
        g_hook.armed = true;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    if (info->ExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT || !g_hook.armed ||
        reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress) != g_hook.address) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    g_hook.callback(g_hook.address, context);
    WriteByte(g_hook.address, g_hook.originalByte);
    g_hook.armed = false;
    context->Rip = g_hook.address;
    t_rearmPending = true;
    context->EFlags |= 0x100;
    return EXCEPTION_CONTINUE_EXECUTION;
}

}  // namespace

bool InstallHook(uintptr_t address, HookCallback callback) {
    if (g_handler || !callback) {
        return false;
    }

    g_handler = AddVectoredExceptionHandler(1, ExceptionHandler);
    if (!g_handler) {
        Log("RAGE Achievements: AddVectoredExceptionHandler failed (%lu)", GetLastError());
        return false;
    }

    auto* target = reinterpret_cast<void*>(address);
    DWORD oldProtection = 0;
    if (!VirtualProtect(target, 1, PAGE_EXECUTE_READWRITE, &oldProtection)) {
        Log("RAGE Achievements: VirtualProtect failed at %p (%lu)", target, GetLastError());
        return false;
    }

    g_hook.address = address;
    g_hook.originalByte = *reinterpret_cast<unsigned char*>(target);
    g_hook.callback = callback;
    *reinterpret_cast<unsigned char*>(target) = 0xCC;
    VirtualProtect(target, 1, oldProtection, &oldProtection);
    FlushInstructionCache(GetCurrentProcess(), target, 1);
    g_hook.armed = true;
    Log("RAGE Achievements: unlock hook armed at %p", target);
    return true;
}

bool InstallJump(uintptr_t address, const void* target) {
    unsigned char patch[14] = {0xFF, 0x25, 0x00, 0x00, 0x00, 0x00};
    auto destination = reinterpret_cast<uintptr_t>(target);
    std::memcpy(patch + 6, &destination, sizeof(destination));

    auto* code = reinterpret_cast<void*>(address);
    DWORD oldProtection = 0;
    if (!VirtualProtect(code, sizeof(patch), PAGE_EXECUTE_READWRITE, &oldProtection)) {
        Log("RAGE Achievements: VirtualProtect failed at %p (%lu)", code, GetLastError());
        return false;
    }
    std::memcpy(code, patch, sizeof(patch));
    VirtualProtect(code, sizeof(patch), oldProtection, &oldProtection);
    FlushInstructionCache(GetCurrentProcess(), code, sizeof(patch));
    return true;
}

}  // namespace RageAchievements
