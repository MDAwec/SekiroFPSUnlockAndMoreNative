#include "MinHook.h"
#include "patches.h"
#include "offset.h"
#include "utils.h"

#include <cstring>

uintptr_t fovData;
uintptr_t speedData;
constexpr double PI = 3.14159265358979323846;

typedef struct Patch
{
  uintptr_t addr;
  const void *data;
  size_t size;
  const char *str;
  bool write() const
  {
    return WriteProtectedMemory(addr, data, size, str);
  }
} Patch;

constexpr static const float speedFixMatrix[]{
    15.0f, 16.0f, 16.6667f, 18.0f, 18.6875f, 18.8516f, 20.0f,
    24.0f, 25.0f, 27.5f, 30.0f, 32.0f, 38.5f, 40.0f,
    48.0f, 49.5f, 50.0f, 57.2958f, 60.0f, 64.0f, 66.75f,
    67.0f, 78.8438f, 80.0f, 84.0f, 90.0f, 93.8f, 100.0f,
    120.0f, 127.0f, 128.0f, 130.0f, 140.0f, 144.0f, 150.0f};

void ApplySpeedFix(float fps)
{
  uintptr_t speedFixAddr = GetAbsoluteAddress(speedfix_offset + 15);
  float idealSpeedFix = fps / 2.0f;
  float closestSpeedFix = 30.0f; // Default value for 60 fps
  if (!speedData)
    speedData = AllocateMemoryNearAddress(speedFixAddr, sizeof(float));
  int32_t relative = (int32_t)(speedData - (speedFixAddr + sizeof(int32_t)));

  for (const float val : speedFixMatrix)
    if (abs(idealSpeedFix - val) < abs(idealSpeedFix - closestSpeedFix))
      closestSpeedFix = val;

  Patch patches[] = {
      {speedData, &closestSpeedFix, sizeof(float), "Write speed value to allocated memory"},
      {speedFixAddr, &relative, sizeof(int32_t), "Patch speed value"},
  };

  for (const Patch &patch : patches)
    if (!patch.write())
      return;
}

// Credit: Lyall's SekiroFix. A delta check makes sprinting slow down above 60 fps, so the jbe over it is made unconditional.
static void ApplySprintFix(bool skip)
{
  uintptr_t addr = GetAbsoluteAddress(sprint_speed_jump_offset);
  uint8_t current = *(uint8_t *)addr;
  uint8_t wanted = skip ? 0xEB : 0x76;
  if ((current == 0x76 || current == 0xEB) && current != wanted)
    WriteProtectedMemory(addr, &wanted, sizeof(wanted), "Patch sprint speed");
}

void ApplyFramelockPatch()
{
  if (!INITIALIZED && !FPSUNLOCK_ENABLED)
    return;

  // FPS_LIMIT 0 means unlimited
  float fpsLimit = FPSUNLOCK_ENABLED ? (FPS_LIMIT ? std::clamp(FPS_LIMIT, 30, 300) : 1000) : 60.0f;
  const float targetDelta = 1.0f / (GAME_LOADING ? 300.0f : fpsLimit);

  if (!WriteProtectedMemory(GetAbsoluteAddress(fps_offset), &targetDelta, sizeof(float), "Patch framelock"))
    return;

  if (!GAME_LOADING)
  {
    ApplySpeedFix(fpsLimit);
    ApplySprintFix(fpsLimit > 60.0f);
  }
}

void ApplyAutolootPatch()
{
  if (!AUTOLOOT_ENABLED && !INITIALIZED)
    return;

  constexpr uint8_t patchEnable[2] = {0xB0, 0x01};  // mov al,1
  constexpr uint8_t patchDisable[2] = {0x32, 0xC0}; // xor al,al

  WriteProtectedMemory(GetAbsoluteAddress(autoloot_offset), AUTOLOOT_ENABLED ? patchEnable : patchDisable, sizeof(uint8_t) * 2, "Patch autoloot");
}

void ApplyIntroPatch()
{
  if (!INTROSKIP_ENABLED && !INITIALIZED)
    return;

  constexpr uint8_t patchEnable[1] = {0x75};

  WriteProtectedMemory(GetAbsoluteAddress(introskip_offset), patchEnable, sizeof(char), "Patch logos skip");
}

// Borderless windowed: the window always covers the screen while the game keeps rendering at the selected
// resolution. The game derives its render size from the window's client rect and sizes the window to the
// selected resolution itself, so its SetWindowPos size is remembered and GetClientRect reports it.
static BOOL(WINAPI *fnSetWindowPos)(HWND, HWND, int, int, int, int, UINT);
static BOOL(WINAPI *fnGetClientRect)(HWND, LPRECT);
static BOOL(WINAPI *fnMoveWindow)(HWND, int, int, int, int, BOOL);
static LONG virtualWidth, virtualHeight;

// The game recreates its window on resolution changes; a new "CSclass" window of this process becomes the game window right away.
static bool IsGameWindow(HWND hWnd)
{
  if (hWnd == GAME_WINDOW)
    return true;
  char className[16] = {};
  if (!hWnd || GetWindow(hWnd, GW_OWNER) || !GetClassNameA(hWnd, className, sizeof(className)) || strcmp(className, "CSclass"))
    return false;
  GAME_WINDOW = hWnd;
  return true;
}

static bool BorderlessCover() { return INITIALIZED && BORDERLESS_ENABLED && !FULLSCREEN_STATE; }

// A resolution change: the game resizes its window and reads the client size back. The window is a plain
// window of the requested size meanwhile (no faked client size). The size is applied as width+1, height+1
// first and then as requested, so the window always changes size, also when the requested resolution is the
// one it already has. The next frame ApplyBorderlessPatch remembers the resulting client size and covers the
// screen again.
static bool resizing;

static bool BeginResolutionChange(HWND hWnd)
{
  if (resizing || !IsGameWindow(hWnd) || !BorderlessCover())
    return false;
  virtualWidth = virtualHeight = 0;
  return true;
}

static BOOL WINAPI DetourSetWindowPos(HWND hWnd, HWND after, int x, int y, int cx, int cy, UINT flags)
{
  if ((flags & SWP_NOSIZE) || !BeginResolutionChange(hWnd))
    return fnSetWindowPos(hWnd, after, x, y, cx, cy, flags);

  resizing = true;
  fnSetWindowPos(hWnd, after, x, y, cx + 1, cy + 1, flags);
  BOOL result = fnSetWindowPos(hWnd, after, x, y, cx, cy, flags);
  resizing = false;
  return result;
}

static BOOL WINAPI DetourMoveWindow(HWND hWnd, int x, int y, int cx, int cy, BOOL repaint)
{
  if (!BeginResolutionChange(hWnd))
    return fnMoveWindow(hWnd, x, y, cx, cy, repaint);

  resizing = true;
  fnMoveWindow(hWnd, x, y, cx + 1, cy + 1, repaint);
  BOOL result = fnMoveWindow(hWnd, x, y, cx, cy, repaint);
  resizing = false;
  return result;
}

static BOOL WINAPI DetourGetClientRect(HWND hWnd, LPRECT rect)
{
  if (IsGameWindow(hWnd) && BorderlessCover() && virtualWidth > 0 && virtualHeight > 0)
  {
    *rect = {0, 0, virtualWidth, virtualHeight};
    return TRUE;
  }
  return fnGetClientRect(hWnd, rect);
}

void InstallWindowHooks()
{
  HMODULE user32 = GetModuleHandleA("user32.dll");
  LPVOID setWindowPos = (LPVOID)GetProcAddress(user32, "SetWindowPos");
  LPVOID getClientRect = (LPVOID)GetProcAddress(user32, "GetClientRect");
  LPVOID moveWindow = (LPVOID)GetProcAddress(user32, "MoveWindow");
  if (MH_CreateHook(setWindowPos, (LPVOID)&DetourSetWindowPos, (LPVOID *)&fnSetWindowPos) != MH_OK || MH_EnableHook(setWindowPos) != MH_OK ||
      MH_CreateHook(getClientRect, (LPVOID)&DetourGetClientRect, (LPVOID *)&fnGetClientRect) != MH_OK || MH_EnableHook(getClientRect) != MH_OK ||
      MH_CreateHook(moveWindow, (LPVOID)&DetourMoveWindow, (LPVOID *)&fnMoveWindow) != MH_OK || MH_EnableHook(moveWindow) != MH_OK)
    LogMessage(LogLevel::Error, false, "Window hooks failed");
}

// Called every frame while borderless windowed: only touches the window when it differs.
void ApplyBorderlessPatch()
{
  if (FULLSCREEN_STATE || (!INITIALIZED && !BORDERLESS_ENABLED))
    return;

  if (BORDERLESS_ENABLED)
  {
    constexpr LONG_PTR style = WS_POPUP | WS_VISIBLE;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    RECT cur;
    GetWindowRect(GAME_WINDOW, &cur);
    bool restyle = GetWindowLongPtr(GAME_WINDOW, GWL_STYLE) != style;
    if (!restyle && cur.left == 0 && cur.top == 0 && cur.right == screenWidth && cur.bottom == screenHeight)
      return;

    if (virtualWidth <= 0 || virtualHeight <= 0)
    {
      // the real client size is the selected resolution
      RECT client;
      fnGetClientRect(GAME_WINDOW, &client);
      virtualWidth = client.right;
      virtualHeight = client.bottom;
    }
    if (restyle)
      SetWindowLongPtr(GAME_WINDOW, GWL_STYLE, style);
    fnSetWindowPos(GAME_WINDOW, HWND_TOP, 0, 0, screenWidth, screenHeight, SWP_NOREDRAW | (restyle ? SWP_FRAMECHANGED : 0));
  }
  else if (INITIALIZED)
  {
    virtualWidth = virtualHeight = 0;
    uintptr_t curResAddr = GetAbsoluteAddress(resolution_pointer_offset + 3);
    uintptr_t curResValue = DereferenceStaticX64Pointer(curResAddr, 6);

    int width = *(int32_t *)curResValue;
    int height = *(int32_t *)(curResValue + 4);

    SetWindowLongPtr(GAME_WINDOW, GWL_STYLE, WS_VISIBLE | WS_CAPTION | WS_BORDER | WS_CLIPSIBLINGS | WS_DLGFRAME | WS_SYSMENU | WS_GROUP | WS_MINIMIZEBOX);
    RECT wr = {0, 0, (LONG)width, (LONG)height};
    DWORD style = GetWindowLong(GAME_WINDOW, GWL_STYLE);
    DWORD exStyle = GetWindowLong(GAME_WINDOW, GWL_EXSTYLE);
    AdjustWindowRectEx(&wr, style, false, exStyle);
    SetWindowPos(GAME_WINDOW, HWND_TOP, 0, 0, wr.right - wr.left, wr.bottom - wr.top,
                 SWP_NOREDRAW);
  }
}

void ApplyFovPatch()
{
  if (!INITIALIZED && !FOV_ENABLED)
    return;

  uintptr_t fovAddr = GetAbsoluteAddress(fovsetting_offset) + 8;
  if (!fovData)
    fovData = AllocateMemoryNearAddress(fovAddr, sizeof(float));

  float fov = std::clamp(FOV_VALUE, -95, 95);
  float fovValue = FOV_ENABLED ? PI / 180.0 * (fov / 100.0f + 1) : 0.0174533f;
  int32_t relative = (int32_t)(fovData - (fovAddr + sizeof(int32_t)));

  Patch patches[] = {
      {fovData, &fovValue, sizeof(float), "Write FOV value to allocated memory"},
      {fovAddr, &relative, sizeof(relative), "Patch FOV"},
  };

  for (const Patch &patch : patches)
    if (!patch.write())
      return;
}


// The render system builds its render targets letterboxed to an aspect ratio pair kept per output (0:0 = fill the window).
// AspectRatio: 0 = fill, 1 = 4:3, 2 = 16:10, 3 = 16:9, 4 = 21:9.
using RenderTargetsFn = char (*)(uint64_t *sys);
using RenderResizeFn = void (*)(void *sys, int width, int height);
static RenderTargetsFn fnRenderTargets;
static RenderResizeFn fnRenderResize;

static char DetourRenderTargets(uint64_t *sys)
{
  constexpr uint32_t ratios[][2] = {{0, 0}, {4, 3}, {16, 10}, {16, 9}, {21, 9}};
  const uint32_t *ratio = ratios[std::clamp(ASPECT_RATIO_MODE, 0, 4)];
  // outputs are 352 bytes each, between sys[20] and sys[21]; the output index selects the pair at +0x218, 40 bytes apart
  for (uint8_t *out = (uint8_t *)sys[20]; out != (uint8_t *)sys[21]; out += 352)
  {
    uint32_t *pair = (uint32_t *)((uint8_t *)sys + 0x218 + 40 * *(uint32_t *)(out + 304));
    pair[0] = ratio[0];
    pair[1] = ratio[1];
  }
  return fnRenderTargets(sys);
}

// Installs the hook on first use; with rebuild, also recreates the render targets so a changed ratio shows right away.
void ApplyAspectRatio(bool rebuild)
{
  static bool hooked;
  constexpr uint8_t expected[] = {0x48, 0x8B, 0xC4, 0x55, 0x41, 0x54, 0x41, 0x55};
  if (!hooked)
  {
    LPVOID target = (LPVOID)GetAbsoluteAddress(render_targets_offset);
    if (memcmp(target, expected, sizeof(expected)) != 0 ||
        MH_CreateHook(target, (LPVOID)&DetourRenderTargets, (LPVOID *)&fnRenderTargets) != MH_OK || MH_EnableHook(target) != MH_OK)
      return LogMessage(LogLevel::Error, false, "Aspect ratio hook failed");
    fnRenderResize = (RenderResizeFn)GetAbsoluteAddress(render_resize_offset);
    hooked = true;
  }

  uint8_t *sys = *(uint8_t **)GetAbsoluteAddress(render_system_offset);
  if (!rebuild || !sys)
    return;
  uint32_t width = *(uint32_t *)(sys + 0x200), height = *(uint32_t *)(sys + 0x204);
  if (width && height)
    fnRenderResize(sys, width, height);
}

void ApplyCamResetPatch()
{
  if (!INITIALIZED && !DISABLE_CAMRESET_ENABLED)
    return;

  uintptr_t camResetAddr = GetAbsoluteAddress(camreset_lockon_offset) + 6;
  constexpr uint8_t camResetDisable[1] = {0x00};
  constexpr uint8_t camResetEnable[1] = {0x01};

  WriteProtectedMemory(camResetAddr, DISABLE_CAMRESET_ENABLED ? camResetDisable : camResetEnable, sizeof(uint8_t), "Patch cam reset");
}

// Removes hardvalue of 60 hz before call of ResizeTarget
void ApplyFullscreenHZPatch()
{
  // Credit: Lyall's SekiroFix. Two more places hard code 60 hz: `mov [..], 3Ch`, the immediate is zeroed.
  for (uintptr_t offset : {fullscreen_refresh_rate_offset, fullscreen_refresh_rate_startup_offset})
  {
    uintptr_t addr = GetAbsoluteAddress(offset);
    constexpr uint8_t zero = 0;
    if (*(uint8_t *)addr == 0xC7 && *(uint8_t *)(addr + 3) == 0x3C)
      WriteProtectedMemory(addr + 3, &zero, sizeof(zero), "Patch fullscreen refresh rate");
  }

  uintptr_t numAddr = GetAbsoluteAddress(fullscreen_refreshrate_num);

  constexpr uint8_t patchEnable[18] = {
      0xB8, 0x00, 0x00, 0x00, 0x00, // mov eax, 0           Refresh Rate Numerator before calling ResizeTarget
      0x89, 0x44, 0x24, 0x28,       // mov [rsp+28], eax
      0xB8, 0x00, 0x00, 0x00, 0x00, // mov eax, 0           Refresh Rate Denumerator before calling ResizeTarget
      0x89, 0x44, 0x24, 0x2C        // mov [rsp+2C], eax
  };

  uintptr_t detour = InjectShellcodeWithJumpBack(numAddr, patchEnable, sizeof(patchEnable), 14, "Write fullscreen hz fix");

  WriteProtectedMemory(numAddr, RelJmpBytes(14, (int32_t)(detour - (numAddr + 5))).data(), 14, "Patch fullscreen hz fix");
}

// Dragonrot
void ApplyDragonrotPatch()
{
  if (!INITIALIZED && !DISABLE_DRAGONROT_ENABLED)
    return;

  uintptr_t dragonrotAddr = GetAbsoluteAddress(dragonrot_effect_offset) + 13;

  uint8_t dragonrotDisable[] = {0x90, 0x90, 0x90, 0xE9};
  uint8_t dragonrotEnable[] = {0x84, 0xC0, 0x0F, 0x85};

  WriteProtectedMemory(dragonrotAddr, DISABLE_DRAGONROT_ENABLED ? dragonrotDisable : dragonrotEnable, sizeof(dragonrotDisable), "Patch dragonrot");
}

// Death penalties
uintptr_t deathPenalties1Addr;
uintptr_t deathPenalties2Addr;
uint8_t deathPenalties1OrigBytes[5];
uint8_t deathPenalties2OrigBytes[32];
void ApplyDeathPenaltiesPatch()
{
  if (!INITIALIZED)
  {
    deathPenalties1Addr = GetAbsoluteAddress(deathpenalties1_offset) + 11;
    deathPenalties2Addr = GetAbsoluteAddress(deathpenalties2_offset);

    ReadProcessMemory(GetCurrentProcess(), (LPVOID)deathPenalties1Addr, deathPenalties1OrigBytes, sizeof(deathPenalties1OrigBytes), nullptr);
    ReadProcessMemory(GetCurrentProcess(), (LPVOID)deathPenalties2Addr, deathPenalties2OrigBytes, sizeof(deathPenalties2OrigBytes), nullptr);
  }

  if (!INITIALIZED && !DISABLE_DEATH_PENALTIES_ENABLED)
    return;

  constexpr uint8_t disablePenalties1[] = {0x90, 0x90, 0x90, 0x90, 0x90};
  constexpr uint8_t disablePenalties2[] = {
      0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
      0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
      0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
      0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};

  WriteProtectedMemory(deathPenalties1Addr, DISABLE_DEATH_PENALTIES_ENABLED ? disablePenalties1 : deathPenalties1OrigBytes, sizeof(disablePenalties1), "Patch death penalties 1");
  WriteProtectedMemory(deathPenalties2Addr, DISABLE_DEATH_PENALTIES_ENABLED ? disablePenalties2 : deathPenalties2OrigBytes, sizeof(disablePenalties2), "Patch death penalties 2");
}

// Autorotate
uintptr_t camAdjustPitchAddr;
uintptr_t camAdjustPitchXYAddr;
uintptr_t camAdjustYawZAddr;
uintptr_t camAdjustYawXYAddr;
uintptr_t camAdjustPitchCode;
uintptr_t camAdjustPitchXYCode;
uintptr_t camAdjustYawZCode;
uintptr_t camAdjustYawXYCode;
uint8_t camAdjustPitchOrigBytes[7];
uint8_t camAdjustPitchXYOrigBytes[12];
uint8_t camAdjustYawZAddrOrigBytes[8];
uint8_t camAdjustYawXYAddrOrigBytes[8];
void ApplyCamAutorotatePatch()
{
  if (!INITIALIZED)
  {
    camAdjustPitchAddr = GetAbsoluteAddress(camadjust_pitch_offset);
    camAdjustPitchXYAddr = GetAbsoluteAddress(camadjust_pitch_xy_offset);
    camAdjustYawZAddr = GetAbsoluteAddress(camadjust_yaw_z_offset) + 5;
    camAdjustYawXYAddr = GetAbsoluteAddress(camadjust_yaw_xy_offset) + 5;

    ReadProcessMemory(GetCurrentProcess(), (LPVOID)camAdjustPitchAddr, camAdjustPitchOrigBytes, sizeof(camAdjustPitchOrigBytes), nullptr);
    ReadProcessMemory(GetCurrentProcess(), (LPVOID)camAdjustPitchXYAddr, camAdjustPitchXYOrigBytes, sizeof(camAdjustPitchXYOrigBytes), nullptr);
    ReadProcessMemory(GetCurrentProcess(), (LPVOID)camAdjustYawZAddr, camAdjustYawZAddrOrigBytes, sizeof(camAdjustYawZAddrOrigBytes), nullptr);
    ReadProcessMemory(GetCurrentProcess(), (LPVOID)camAdjustYawXYAddr, camAdjustYawXYAddrOrigBytes, sizeof(camAdjustYawXYAddrOrigBytes), nullptr);

    // Camera Pitch (Z-axis)
    constexpr uint8_t INJECT_CAMADJUST_PITCH_SHELLCODE[] = {
        0x0F, 0x28, 0xA6, 0x70, 0x01, 0x00, 0x00, // movaps xmm4, [rsi+0x170]
        0x0F, 0x29, 0xA5, 0x70, 0x08, 0x00, 0x00  // movaps [rbp+0x870], xmm4
    };

    // Camera Yaw (Z-axis)
    constexpr uint8_t INJECT_CAMADJUST_YAW_Z_SHELLCODE[] = {
        0xF3, 0x0F, 0x10, 0x86, 0x74, 0x01, 0x00, 0x00, // movss xmm0, [rsi+0x174]
        0xF3, 0x0F, 0x11, 0x86, 0x74, 0x01, 0x00, 0x00  // movss [rsi+0x174], xmm0
    };

    // Camera Pitch (XY-axis)
    constexpr uint8_t INJECT_CAMADJUST_PITCH_XY_SHELLCODE[] = {
        0xF3, 0x0F, 0x10, 0x86, 0x70, 0x01, 0x00, 0x00, // movss xmm0, [rsi+0x170]
        0xF3, 0x0F, 0x11, 0x00,                         // movss [rax], xmm0
        0xF3, 0x0F, 0x10, 0x00,                         // movss xmm0, [rax]
        0xF3, 0x0F, 0x11, 0x86, 0x70, 0x01, 0x00, 0x00  // movss [rsi+0x170], xmm0
    };

    // Camera Yaw (XY-axis)
    constexpr uint8_t INJECT_CAMADJUST_YAW_XY_SHELLCODE[] = {
        0xF3, 0x0F, 0x10, 0x86, 0x74, 0x01, 0x00, 0x00, // movss xmm0, [rsi+0x174]
        0xF3, 0x0F, 0x11, 0x86, 0x74, 0x01, 0x00, 0x00  // movss [rsi+0x174], xmm0
    };

    camAdjustPitchCode = InjectShellcodeWithJumpBack(camAdjustPitchAddr, INJECT_CAMADJUST_PITCH_SHELLCODE, sizeof(INJECT_CAMADJUST_PITCH_SHELLCODE), sizeof(camAdjustPitchOrigBytes), "Write cam adjust pitch code to allocated memory");
    camAdjustPitchXYCode = InjectShellcodeWithJumpBack(camAdjustPitchXYAddr, INJECT_CAMADJUST_PITCH_XY_SHELLCODE, sizeof(INJECT_CAMADJUST_PITCH_XY_SHELLCODE), sizeof(camAdjustPitchXYOrigBytes), "Write cam adjust pitch XY code to allocated memory");
    camAdjustYawZCode = InjectShellcodeWithJumpBack(camAdjustYawZAddr, INJECT_CAMADJUST_YAW_Z_SHELLCODE, sizeof(INJECT_CAMADJUST_YAW_Z_SHELLCODE), sizeof(camAdjustYawZAddrOrigBytes), "Write cam adjust yaw Z code to allocated memory");
    camAdjustYawXYCode = InjectShellcodeWithJumpBack(camAdjustYawXYAddr, INJECT_CAMADJUST_YAW_XY_SHELLCODE, sizeof(INJECT_CAMADJUST_YAW_XY_SHELLCODE), sizeof(camAdjustYawXYAddrOrigBytes), "Write cam adjust yaw XY code to allocated memory");
  }

  if (!INITIALIZED && !DISABLE_CAMERA_AUTOROTATE_ENABLED)
    return;

  WriteProtectedMemory(camAdjustPitchAddr, DISABLE_CAMERA_AUTOROTATE_ENABLED ? RelJmpBytes(7, (int32_t)(camAdjustPitchCode - (camAdjustPitchAddr + 5))).data() : camAdjustPitchOrigBytes, sizeof(camAdjustPitchOrigBytes), "Patch cam adjust pitch");
  WriteProtectedMemory(camAdjustPitchXYAddr, DISABLE_CAMERA_AUTOROTATE_ENABLED ? RelJmpBytes(12, (int32_t)(camAdjustPitchXYCode - (camAdjustPitchXYAddr + 5))).data() : camAdjustPitchXYOrigBytes, sizeof(camAdjustPitchXYOrigBytes), "Patch cam adjust pitch XY");
  WriteProtectedMemory(camAdjustYawZAddr, DISABLE_CAMERA_AUTOROTATE_ENABLED ? RelJmpBytes(8, (int32_t)(camAdjustYawZCode - (camAdjustYawZAddr + 5))).data() : camAdjustYawZAddrOrigBytes, sizeof(camAdjustYawZAddrOrigBytes), "Patch cam adjust yaw Z");
  WriteProtectedMemory(camAdjustYawXYAddr, DISABLE_CAMERA_AUTOROTATE_ENABLED ? RelJmpBytes(8, (int32_t)(camAdjustYawXYCode - (camAdjustYawXYAddr + 5))).data() : camAdjustYawXYAddrOrigBytes, sizeof(camAdjustYawXYAddrOrigBytes), "Patch cam adjust yaw XY");
}
