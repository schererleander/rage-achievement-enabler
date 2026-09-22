# RAGE Achievements

The 64-bit version of RAGE disables Steam achievements by replacing the internal unlock function with a null stub. This mod intercepts that call, unlocks `ach_<id>` through `ISteamUserStats`, and stores the updated Steam stats.

## Build

```sh
nix develop
cmake --preset mingw
cmake --build --preset mingw
```

Output: `build/mingw/bin/Achievements.asi`

To deploy automatically after building:

```sh
cmake --preset mingw -DACHIEVEMENTS_DEPLOY_DIR="/path/to/RAGE/bin"
cmake --build --preset mingw
```

## Install

1. Install [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) as `dinput8.dll` next to `Rage64.exe`.
2. Copy `Achievements.asi` to the same directory.
3. Launch the game through Steam. Check `Achievements.log` for troubleshooting.

