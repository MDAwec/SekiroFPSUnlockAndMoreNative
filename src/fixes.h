#pragma once

// Aspect ratio fixes (animation culling, HUD vignettes/fades, awareness markers), after Lyall's SekiroFix.
void InstallFixes();
// Per-frame: follows the picture size of the render system.
void UpdateFixes();
// Places the HUD movies again, e.g. after the HUD stretch changed.
void RelayoutHud();
