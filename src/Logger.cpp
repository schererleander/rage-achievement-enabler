#include "Logger.h"

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace RageAchievements {
namespace {

FILE* g_logFile = nullptr;
CRITICAL_SECTION g_logLock;
bool g_logLockInitialized = false;

}  // namespace

void OpenLog() {
    InitializeCriticalSection(&g_logLock);
    g_logLockInitialized = true;

    char path[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    char* lastSlash = std::strrchr(path, '\\');
    if (lastSlash) {
        *(lastSlash + 1) = '\0';
    } else {
        path[0] = '\0';
    }
    std::strncat(path, "Achievements.log", MAX_PATH - std::strlen(path) - 1);
    g_logFile = std::fopen(path, "a");
}

void Log(const char* format, ...) {
    char message[512] = {};
    va_list args;
    va_start(args, format);
    std::vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    OutputDebugStringA(message);
    OutputDebugStringA("\n");
    if (!g_logLockInitialized) {
        return;
    }

    EnterCriticalSection(&g_logLock);
    if (g_logFile) {
        std::fputs(message, g_logFile);
        std::fputc('\n', g_logFile);
    }
    LeaveCriticalSection(&g_logLock);
}

void FlushLog() {
    if (!g_logLockInitialized) {
        return;
    }
    EnterCriticalSection(&g_logLock);
    if (g_logFile) {
        std::fflush(g_logFile);
    }
    LeaveCriticalSection(&g_logLock);
}

}  // namespace RageAchievements
