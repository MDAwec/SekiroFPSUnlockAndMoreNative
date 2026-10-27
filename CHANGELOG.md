# Changelog

Each release needs a `## <version>` section here matching `VERSION`. The release workflow publishes that section as the GitHub release notes and fails if it is missing.

## 1.7.0

- New: the settings are rows in the game's own Settings menu instead of an overlay window.
- New: Aspect Ratio (fill, 4:3, 16:10, 16:9, 21:9) and Stretch HUD options. Stretch HUD applies at once, no resolution change needed.
- New: Screen Mode offers Borderless Windowed.
- New: aspect ratio fixes for animation culling, HUD vignettes and fades, and awareness markers. Sprint speed above 60 fps and the 60 hz fullscreen lock are fixed too.
- Fixed: changing the resolution could close the game. The game recreates its window, and the mod took the old window being gone as the game exiting.
- Fixed: the screen-sized resolution could not be selected in borderless mode. The game resizes its window with `MoveWindow`, which did not reach the mod when the size stayed the same, so the game dropped the new resolution after 10 seconds.
- Changed: nothing is hooked or patched before the game has finished starting up. The executable hash check is gone.
