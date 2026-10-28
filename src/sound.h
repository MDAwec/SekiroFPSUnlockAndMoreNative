#pragma once

// Starts the game with the player's sound volumes instead of the defaults, before the save is loaded.
void InstallSoundFix();
// Per-frame: remembers the sound volumes of the game's options in the INI.
void UpdateSoundVolumes();
