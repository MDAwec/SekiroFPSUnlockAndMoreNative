# Sekiro FPS Unlock And More Native

A mod for Sekiro: Shadows Die Twice (Steam, version 1.06). Its settings are rows in the game's own Settings menu, so there is no overlay and no separate program. Nexus page: https://www.nexusmods.com/sekiro/mods/2164

## Install

Copy `d3dcompiler_43.dll` into the game folder next to `sekiro.exe`. The game loads it in place of its own `d3dcompiler_43.dll`, which it forwards to the system one. Delete the file to uninstall.

On Linux/Proton nothing else is needed.

## Settings

All settings are in the game's **Settings** menu (main menu and pause menu) and saved to `sekiroimgui.ini` next to the game. The log is `sekirofpsimgui.log`.

| Page | Options |
|---|---|
| Game Options | Autoloot, Skip Logos (restart), Disable Death Penalties, Disable Dragonrot |
| Camera Options | Disable Camera Reset, Disable Camera Autorotate, Field of View |
| Graphics Options | FPS Limit (unlock), VSync, Aspect Ratio, Stretch HUD |

- **Screen Mode** also offers Borderless Windowed: the window always covers the screen while the game renders at the selected resolution.
- **Screen Resolution** lists every resolution the display supports, also in windowed mode.
- **Aspect Ratio** letterboxes the picture to 4:3, 16:10, 16:9 or 21:9, or fills the screen.
- **Stretch HUD** stretches the HUD over the whole screen. Off keeps it at 16:9, with the vignettes, fades and awareness markers fixed for wider and narrower pictures.
- The frame rate is unlocked up to 300 fps, with the game's speed fixes (running speed, sprint speed) applied. The frame rate is also unlocked on loading screens.

## Build

Needs `x86_64-w64-mingw32-g++` and `make`. `7z` is needed for `make dist`.

```
make                              # build/d3dcompiler_43.dll
make install SEKIRO_PATH=<game>   # also copies it into the game folder
make dist                         # build/SekiroFPSUnlockAndMoreNative.7z
```

## Release

Bump `VERSION` on `main` and add a `## <version>` section to `CHANGELOG.md`; the release workflow builds the archive and publishes it as a GitHub release with that section as notes. A manual run of the workflow only builds, and publishes nothing.

## License

GPLv3, see `LICENSE`.

## Credits

* Lyall - for SekiroFix (https://codeberg.org/Lyall/SekiroFix, MIT): animation culling fix for wider aspect ratios, sprint speed fix above 60 fps, fullscreen 60 hz startup patches, and the idea of an unrestricted resolution list.
* Uberhalit - for SekiroFpsUnlockAndMore.
* Katalash - for logos skip offset from Sekiro Mod Engine.
* Thevayl - inspiration for FPS unlock during loading screens.
* Cielos - for some camera adjustment offsets.
* jackfuste - for FOV offset and basic running speed fix.
* TsudaKageyu - for MinHook.
* metayeti - for mINI.
