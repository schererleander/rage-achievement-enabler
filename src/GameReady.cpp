#include "GameReady.h"

#include <windows.h>

#include <cstdint>
#include <cstring>

#include "Logger.h"

namespace RageAchievements {
namespace {

constexpr uintptr_t kReadySignatureRva = 0x2D9320;
constexpr unsigned char kReadySignature[] = {
    0x48, 0x89, 0x5C, 0x24, 0x20, 0x55, 0x57, 0x41, 0x55, 0x48, 0x8D, 0x6C,
    0x24, 0xB9, 0x48, 0x81, 0xEC, 0xD0, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x0D,
    0xD4, 0x71, 0x7D, 0x02, 0xFF, 0x15, 0x9E, 0xA2,
};

}  // namespace

bool WaitForGameUnpack(unsigned timeoutMs) {
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    auto* target = reinterpret_cast<const unsigned char*>(base + kReadySignatureRva);

    for (unsigned waited = 0; waited < timeoutMs; waited += 100) {
        if (std::memcmp(target, kReadySignature, sizeof(kReadySignature)) == 0) {
            Log("RAGE Achievements: game unpacked after %u ms", waited);
            return true;
        }
        Sleep(100);
    }

    Log("RAGE Achievements: timed out waiting for the supported game executable");
    return false;
}

}  // namespace RageAchievements
