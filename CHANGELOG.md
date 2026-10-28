# Changelog

Each release needs a `## <version>` section here matching `VERSION`. The release workflow publishes that section as the GitHub release notes and fails if it is missing.

## 1.1

- The game starts with your sound volumes instead of its defaults, so the logos and the title no longer play loud until the save is loaded. The music, sound effects and voice volumes are remembered in `SekiroFPSUnlockAndMoreNative.ini` whenever they change; the first start after updating still uses the defaults.

## 1.0

- First release, based on SekiroImGui. The settings are rows in the game's own Settings menu instead of an overlay window, saved to `SekiroFPSUnlockAndMoreNative.ini` (log: `SekiroFPSUnlockAndMoreNative.log`).
- FPS unlock (up to 300 fps, also on loading screens) with the running and sprint speed fixes, VSync, autoloot, logos skip, death penalties and dragonrot toggles, camera reset and autorotate toggles, field of view.
- Screen Mode offers Borderless Windowed. A resolution change in borderless mode is applied to a plain window first and the screen is covered again afterwards, also when the resolution is the screen's own.
- Aspect Ratio (fill, 4:3, 16:10, 16:9, 21:9) and Stretch HUD. Stretch HUD applies at once, no resolution change needed. Off keeps the HUD at 16:9, with the vignettes, fades, awareness markers and animation culling fixed for other picture shapes.
- The game window is followed when the game recreates it, so changing the resolution does not close the game. Nothing is hooked or patched before the game has finished starting up.
