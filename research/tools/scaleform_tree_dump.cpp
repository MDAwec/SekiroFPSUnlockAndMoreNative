// Reference, not built (the Makefile builds src/ only): logs the display list of a movie with the bounds of each clip
// on the stage. Paste into src/hud.cpp (it uses its GValue helpers) and call it where a root is at hand, e.g. in
// PlaceFrom or in a menu's per frame update hook:
//
//   std::string tree;
//   DumpStage(root, root, tree, 3, "");
//   LogMessage(LogLevel::Info, false, "%s:%s", Url(root).c_str(), tree.c_str());
//
// Each clip comes out as /path/name[left,top widthxheight a<alpha>], in stage units (1920 x 1080). A background is a
// clip of about [0,0 1920x1080]; unnamed clips are called instanceN by the AS3 compiler.

void DumpStage(const GValue &obj, const GValue &root, std::string &out, int depth, const std::string &path)
{
  ForEachChild(obj, [&](const GValue &clip) {
    GValue name;
    std::string n = path + "/" + (GetMember(clip, "name", name) ? String(name) : "?");
    Release(name);
    double l = 0, t = 0, w = 0, h = 0;
    Bounds(clip, &root, l, t, w, h);
    char line[200];
    snprintf(line, sizeof(line), " %s[%.0f,%.0f %.0fx%.0f a%.2f]", n.c_str(), l, t, w, h, Number(clip, "alpha"));
    out += line;
    if (depth > 1)
      DumpStage(clip, root, out, depth - 1, n);
  });
}
