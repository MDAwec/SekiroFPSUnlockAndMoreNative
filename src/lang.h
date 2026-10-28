#pragma once
#include <array>
#include <string>

// The languages Sekiro ships with (Steam store list), in the order of a Text's translations.
enum Lang
{
  EN,
  FR,
  IT,
  DE,
  ES,
  PT_BR,
  PL,
  RU,
  JA,
  KO,
  ZH_CN,
  ZH_TW,
  TH,
  LANG_COUNT
};

using Text = std::array<std::wstring, LANG_COUNT>;

// The same text in every language, e.g. numbers.
Text Same(const std::wstring &s);
// The language the game shows its own texts in.
Lang GameLanguage();
