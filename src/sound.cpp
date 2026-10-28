#include "sound.h"
#include "MinHook.h"
#include "offset.h"
#include "utils.h"

#include <atomic>
#include <cstring>

// The game starts with the default volumes (7 of 10) and only reads the player's volumes from the save
// a while later, so the logos and the title play loud. The volumes are remembered in the INI and put into the
// option data before the game first applies them; the save then loads the same values.
namespace
{
using VolumeUpdateFn = uint64_t (*)(float dt);
VolumeUpdateFn fnVolumeUpdate;

constexpr const char *volumeKeys[] = {"SoundVolumeMusic", "SoundVolumeEffects", "SoundVolumeVoice"};
int *const volumeValues[] = {&SOUND_VOLUME_MUSIC, &SOUND_VOLUME_EFFECTS, &SOUND_VOLUME_VOICE};

// option data the remembered volumes were put into; the game creates it again with the defaults e.g. on return to title
std::atomic<uint8_t *> restoredOptions;

// music, sound effects and voice volume (0-10) of the game's option data
uint8_t *OptionVolumes()
{
  uintptr_t gameDataMan = *(uintptr_t *)GetAbsoluteAddress(game_data_man_offset);
  if (!gameDataMan)
    return nullptr;
  uint8_t *options = *(uint8_t **)(gameDataMan + 0x50);
  return options ? options + 4 : nullptr;
}

uint64_t DetourVolumeUpdate(float dt)
{
  uint8_t *volumes = OptionVolumes();
  if (volumes && volumes != restoredOptions)
  {
    for (int i = 0; i < 3; i++)
      if (*volumeValues[i] >= 0 && *volumeValues[i] <= 10)
        volumes[i] = (uint8_t)*volumeValues[i];
    restoredOptions = volumes;
    LogMessage(LogLevel::Info, false, "Startup sound volumes set to %d/%d/%d.", volumes[0], volumes[1], volumes[2]);
  }
  return fnVolumeUpdate(dt);
}

// sekiro.exe is decrypted by its Steam stub after the DLL is loaded, the code is executable again once it is done
bool CodeReady(LPVOID target, const uint8_t *expected, size_t size)
{
  MEMORY_BASIC_INFORMATION mbi;
  return VirtualQuery(target, &mbi, sizeof(mbi)) == sizeof(mbi) &&
         (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) &&
         memcmp(target, expected, size) == 0;
}
} // namespace

void InstallSoundFix()
{
  constexpr uint8_t expected[] = {0x48, 0x8B, 0xC4, 0x55, 0x56, 0x57, 0x48, 0x8B, 0xEC};
  LPVOID target = (LPVOID)GetAbsoluteAddress(sound_volume_update_offset);
  for (int waited = 0; !CodeReady(target, expected, sizeof(expected)); waited++)
  {
    if (waited >= 30000)
      return LogMessage(LogLevel::Error, false, "Sound volume update: unexpected bytes, startup volume fix disabled");
    Sleep(1);
  }

  MH_STATUS status = MH_CreateHook(target, (LPVOID)&DetourVolumeUpdate, (LPVOID *)&fnVolumeUpdate);
  if (status == MH_OK)
    status = MH_EnableHook(target);
  if (status != MH_OK)
    LogMessage(LogLevel::Error, false, "Sound volume update hook failed: %s", MH_StatusToString(status));
}

void UpdateSoundVolumes()
{
  // until the remembered volumes are in, the option data holds the game's defaults
  uint8_t *volumes = OptionVolumes();
  if (!volumes || volumes != restoredOptions)
    return;

  for (int i = 0; i < 3; i++)
    if (volumes[i] <= 10 && volumes[i] != *volumeValues[i])
    {
      *volumeValues[i] = volumes[i];
      SetIniValue(volumeKeys[i], *volumeValues[i]);
    }
}
