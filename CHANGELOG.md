# Changelog

Each release needs a `## <version>` section here matching `VERSION`. The release workflow publishes that section as the GitHub release notes and fails if it is missing.

## 1.2.0

- The mod's settings rows are shown in the game's language: English, French, Italian, German, Spanish, Portuguese (Brazil), Polish, Russian, Japanese, Korean, Simplified Chinese, Traditional Chinese or Thai.
- HUD Layout replaces Stretch HUD, with a new choice, Screen Edges (the default): on a picture that is not 16:9 the HUD elements move to its edges, the health, boss bars and counters to the sides on wider pictures and to the top and bottom on narrower ones, while the posture bars stay in the centre. Stretch and 16:9 work as Stretch HUD on and off did. The setting stays under the INI key `StretchHud` (0 = 16:9, 1 = Stretch, 2 = Screen Edges).
- The black screen, the fades and the vignettes always cover the whole picture, and the black backgrounds of the loading screens, the death screen, the idol menu and the notifications (items, skills, events, messages, tutorials) cover it too. The save icon moves to its corner with Screen Edges.
- Refresh Rate in the graphics settings: Auto (left to the driver) or a rate of the display, applied right away in fullscreen.
- A chosen Aspect Ratio applies from the start; it applied only after a setting was changed.
- The HUD follows the render resolution also when the game creates it with another size, e.g. in fullscreen 2560 x 1080.

## 1.1.1

- The game starts with your sound volumes instead of its defaults, so the logos and the title no longer play loud until the save is loaded. The music, sound effects and voice volumes are remembered in `SekiroFPSUnlockAndMoreNative.ini` whenever they change; the first start after updating still uses the defaults.

## 1.0

- First release, based on SekiroImGui. The settings are rows in the game's own Settings menu instead of an overlay window, saved to `SekiroFPSUnlockAndMoreNative.ini` (log: `SekiroFPSUnlockAndMoreNative.log`).
- FPS unlock (up to 300 fps, also on loading screens) with the running and sprint speed fixes, VSync, autoloot, logos skip, death penalties and dragonrot toggles, camera reset and autorotate toggles, field of view.
- Screen Mode offers Borderless Windowed. A resolution change in borderless mode is applied to a plain window first and the screen is covered again afterwards, also when the resolution is the screen's own.
- Aspect Ratio (fill, 4:3, 16:10, 16:9, 21:9) and Stretch HUD. Stretch HUD applies at once, no resolution change needed. Off keeps the HUD at 16:9, with the vignettes, fades, awareness markers and animation culling fixed for other picture shapes.
- The game window is followed when the game recreates it, so changing the resolution does not close the game. Nothing is hooked or patched before the game has finished starting up.
