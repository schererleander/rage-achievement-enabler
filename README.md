# RAGE Achievement Enabler

<p align="center"> <img src="https://github.com/user-attachments/assets/cd1bed54-2a41-423e-8a6c-ad1b5c3198bf" alt="RAGE Achievement Enabler" width="50%" /> </p>

The 64-bit version of RAGE disables Steam achievements by replacing the internal unlock function with a null stub. This mod intercepts that call, unlocks `ach_<id>` through `ISteamUserStats`, and stores the updated Steam stats.

## Build

```sh
nix develop
cmake --preset mingw
cmake --build --preset mingw
```

Output: `build/mingw/bin/Achievements.asi`

## Installation instructions
1. Go to the [Ultimate ASI Loader](github.com/ThirteenAG/Ultimate-ASI-Loader/releases) releases page.
2. Download the WIN64 dinput8.dll version.
3. Copy dinput8.dll into your RAGE game folder, next to Rage64.exe.
4. Download and extract this mod.
5. Copy Achievements.asi into the same folder.
