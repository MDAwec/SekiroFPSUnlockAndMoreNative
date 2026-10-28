#include "hud.h"
#include "MinHook.h"
#include "offset.h"
#include "utils.h"

#include <atomic>
#include <cmath>
#include <cstring>
#include <map>
#include <string>

// The HUD is the AS3 movie 01_000_fe. Its elements are clips below the root (PlayerHUD/Gauge, PlayerHUD/ItemPanel,
// BossList, ...), laid out on a 1920 x 1080 stage that Scaleform fits into the viewport (ShowAll), which leaves room
// around the stage on pictures that are not 16:9. When Scaleform recomputes the layout of the movie for a new viewport
// (the HUD viewport hook in fixes.cpp), the room is remembered, and the clips are moved into it during the game's next
// call into the movie: the hooked GetMember/SetMember/Invoke of the AS3 ObjectInterface, through which the game reads
// and writes the HUD all the time. No pointer to a HUD object is kept, a reloaded HUD can be a new movie at the address
// of the old one. Loading screens get their black background stretched the same way.

namespace
{
// GFx::Value as the game lays it out: list links (while managed), ObjectInterface*, type, data. A managed value is
// linked into a list by its address, so values are never copied, only filled and released in place.
struct GValue
{
  int64_t link[2] = {-1, -1};
  void ***oi = nullptr; // object whose first field is its vtable
  uint32_t type = 0;
  uint32_t pad = 0;
  union
  {
    void *data;
    double number;
  };
  GValue() : data(nullptr) {}
  GValue(const GValue &) = delete;
  GValue &operator=(const GValue &) = delete;
};

constexpr uint32_t VT_NUMBER = 5, VT_DISPLAY_OBJECT = 10, VT_TYPE_MASK = 0x8f, VT_MANAGED = 0x40;

// ObjectInterface vtable slots
using GetMemberFn = bool (*)(void *oi, void *data, const char *name, GValue *out, bool isDisplayObject);
using SetMemberFn = bool (*)(void *oi, void *data, const char *name, const GValue *value, bool isDisplayObject);
using InvokeFn = bool (*)(void *oi, void *data, GValue *result, const char *name, const GValue *args, size_t count, bool isDisplayObject);
using ReleaseFn = void (*)(void *oi, GValue *val, void *data);
GetMemberFn fnGetMember;
SetMemberFn fnSetMember;
InvokeFn fnInvoke;

bool IsDisplayObject(const GValue &v) { return v.oi && (v.type & VT_TYPE_MASK) == VT_DISPLAY_OBJECT; }

bool GetMember(const GValue &obj, const char *name, GValue &out) { return fnGetMember(obj.oi, obj.data, name, &out, true); }

void Release(GValue &v)
{
  if (v.type & VT_MANAGED)
    ((ReleaseFn)v.oi[0][2])(v.oi, &v, v.data);
  v.type = 0;
  v.data = nullptr;
}

// calls f with the clip at an "a/b/c" path below obj, and returns false when it is missing
template <typename F>
bool AtPath(const GValue &obj, const char *path, F &&f)
{
  const char *slash = strchr(path, '/');
  std::string name = slash ? std::string(path, slash) : std::string(path);
  GValue child;
  bool ok = GetMember(obj, name.c_str(), child) && IsDisplayObject(child);
  if (ok)
    ok = slash ? AtPath(child, slash + 1, f) : (f(child), true);
  Release(child);
  return ok;
}

// the room a viewport has around the 16:9 stage, per side, in stage units: to the left and right on wider pictures,
// above and below on narrower ones
struct Room
{
  double x = 0, y = 0;
};

Room EdgeRoom(float width, float height)
{
  Room room;
  if (width <= 0 || height <= 0)
    return room;
  if (width * 9 > height * 16)
    room.x = (1080.0 * width / height - 1920.0) / 2;
  else if (width * 9 < height * 16)
    room.y = (1920.0 * height / width - 1080.0) / 2;
  return room;
}

// clip and property -> its value without the shift, the shift applied
std::map<std::pair<void *, std::string>, std::pair<double, double>> placed;

// sets a numeric property of a clip to its own value plus shift
void Shift(const GValue &clip, const char *property, double shift)
{
  GValue value;
  if (!GetMember(clip, property, value) || (value.type & VT_TYPE_MASK) != VT_NUMBER)
    return Release(value);
  double now = value.number;
  auto key = std::make_pair(clip.data, std::string(property));
  auto it = placed.find(key);
  // positions are kept in twips, so a written value reads back rounded to 0.05; a value that does not read back as
  // ours was set by the game (also in a new HUD), and is the one to shift from
  bool ours = it != placed.end() && fabs(now - (it->second.first + it->second.second)) < 0.1;
  if (ours && it->second.second == shift)
    return;
  double base = ours ? it->second.first : now;
  placed[key] = {base, shift};
  GValue v;
  v.type = VT_NUMBER;
  v.number = base + shift;
  fnSetMember(clip.oi, clip.data, property, &v, true);
}

struct Anchor
{
  const char *path; // instance names below the HUD root
  int sideX;        // -1 left edge, +1 right edge, 0 stays
  int sideY;        // -1 top edge, +1 bottom edge, 0 stays
};
constexpr Anchor anchors[] = {
    {"PlayerHUD/Gauge", -1, +1}, {"PlayerHUD/Gauge/SP", +1, 0}, // the player's posture stays in the centre
    {"PlayerHUD/ItemPanel", +1, +1},
    {"Money", +1, +1}, {"PlayerCount", +1, +1}, {"GetItemList", +1, +1},
    {"SkillPoint", +1, -1}, {"BloodGhostRec", +1, -1},
    {"BossList", -1, -1}, {"SemiBossList", -1, -1},
};
constexpr const char *bossPosture[] = {"SP", "SPFrame", "SPCenter", "Damage"}; // stays in the centre

void PlaceHud(const GValue &root, Room room)
{
  for (const Anchor &a : anchors)
    AtPath(root, a.path, [&](const GValue &clip) {
      Shift(clip, "x", a.sideX * room.x);
      Shift(clip, "y", a.sideY * room.y);
    });
  for (const char *list : {"BossList", "SemiBossList"})
    for (int i = 0; i < 3; i++)
      for (const char *part : bossPosture)
      {
        char path[64];
        snprintf(path, sizeof(path), "%s/Item_%d_0/%s", list, i, part);
        AtPath(root, path, [&](const GValue &clip) { Shift(clip, "x", room.x); });
      }
}

// the black background of a loading screen covers the stage and the room on each side
void StretchLoadingBackground(const GValue &root, Room room)
{
  AtPath(root, "instance1", [&](const GValue &background) {
    Shift(background, "x", -room.x);
    Shift(background, "y", -room.y);
    Shift(background, "width", 2 * room.x);
    Shift(background, "height", 2 * room.y);
  });
}

Room hudRoom, loadingRoom;
std::atomic<bool> hudPending, loadingPending;

// data is a display object the game works with right now; the movie it is in is placed when it is to be
thread_local bool placing;
void PlaceFrom(void *oi, void *data)
{
  if ((!hudPending && !loadingPending) || placing)
    return;
  placing = true;
  GValue obj, root, child;
  obj.oi = (void ***)oi;
  obj.type = VT_DISPLAY_OBJECT; // borrowed from the call, not managed
  obj.data = data;
  bool isRoot = GetMember(obj, "root", root) && IsDisplayObject(root);
  if (isRoot && loadingPending && GetMember(root, "BackImage", child) && IsDisplayObject(child))
  {
    loadingPending = false;
    StretchLoadingBackground(root, loadingRoom);
  }
  Release(child);
  if (isRoot && hudPending && GetMember(root, "PlayerHUD", child) && IsDisplayObject(child))
  {
    hudPending = false;
    if (placed.size() > 1000) // clips of HUDs that are gone
      placed.clear();
    PlaceHud(root, hudRoom);
  }
  Release(child);
  Release(root);
  placing = false;
}

bool DetourGetMember(void *oi, void *data, const char *name, GValue *out, bool isDisplayObject)
{
  if (isDisplayObject)
    PlaceFrom(oi, data);
  return fnGetMember(oi, data, name, out, isDisplayObject);
}

bool DetourSetMember(void *oi, void *data, const char *name, const GValue *value, bool isDisplayObject)
{
  if (isDisplayObject)
    PlaceFrom(oi, data);
  return fnSetMember(oi, data, name, value, isDisplayObject);
}

bool DetourInvoke(void *oi, void *data, GValue *result, const char *name, const GValue *args, size_t count, bool isDisplayObject)
{
  if (isDisplayObject)
    PlaceFrom(oi, data);
  return fnInvoke(oi, data, result, name, args, count, isDisplayObject);
}

// The game's lookup of a clip by name in its movie wrappers (the GFx::Value at +0x30). The first one shows the AS3
// ObjectInterface, whose calls are hooked from then on.
using ChildLookupFn = void *(*)(uint8_t *parent, uint8_t *out, void *name);
ChildLookupFn fnChildLookup;

template <typename T>
bool HookSlot(void **vtable, int slot, T detour, T *original)
{
  return MH_CreateHook(vtable[slot], (LPVOID)detour, (LPVOID *)original) == MH_OK && MH_EnableHook(vtable[slot]) == MH_OK;
}

void *DetourChildLookup(uint8_t *parent, uint8_t *out, void *name)
{
  void *r = fnChildLookup(parent, out, name);
  static bool hooked;
  const GValue &clip = *(const GValue *)(parent + 0x30);
  if (!hooked && IsDisplayObject(clip))
  {
    hooked = true;
    void **vtable = clip.oi[0];
    if (!HookSlot(vtable, 4, &DetourGetMember, &fnGetMember) || !HookSlot(vtable, 5, &DetourSetMember, &fnSetMember) ||
        !HookSlot(vtable, 6, &DetourInvoke, &fnInvoke))
      LogMessage(LogLevel::Error, false, "Scaleform ObjectInterface hooks failed, HUD Layout Screen Edges disabled");
  }
  return r;
}
} // namespace

void InstallHudLayout()
{
  constexpr uint8_t expected[] = {0x48, 0x89, 0x54, 0x24, 0x10, 0x53, 0x48, 0x83, 0xEC, 0x30};
  LPVOID target = (LPVOID)GetAbsoluteAddress(scaleform_child_lookup_offset);
  if (memcmp(target, expected, sizeof(expected)) != 0 || MH_CreateHook(target, (LPVOID)&DetourChildLookup, (LPVOID *)&fnChildLookup) != MH_OK ||
      MH_EnableHook(target) != MH_OK)
    LogMessage(LogLevel::Error, false, "Scaleform child lookup hook failed, HUD Layout Screen Edges disabled");
}

void HudMovieViewport(float width, float height)
{
  hudRoom = HUD_LAYOUT == HUD_EDGES ? EdgeRoom(width, height) : Room();
  hudPending = true;
}

void LoadingMovieViewport(float width, float height)
{
  loadingRoom = EdgeRoom(width, height);
  loadingPending = true;
}
