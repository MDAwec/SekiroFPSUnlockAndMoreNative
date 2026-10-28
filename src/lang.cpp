#include "lang.h"
#include "offset.h"
#include "utils.h"

Text Same(const std::wstring &s)
{
  Text t;
  t.fill(s);
  return t;
}

Lang GameLanguage()
{
  // the engine's locales jpnJP, engGB, fraFR, itaIT, deuDE, spaES, spaAR, porBR, engUS, nldNL, polPL, rusRU, danDK,
  // norNO, finFI, sweSE, turTR, porPT, zhoTW, korKR, zhoCN, thaTH; those the game has no texts for fall back to English
  constexpr Lang locales[] = {JA, EN, FR, IT, DE, ES, ES, PT_BR, EN, EN, PL, RU, EN, EN, EN, EN, EN, PT_BR, ZH_TW, KO, ZH_CN, TH};
  int locale = *(const int *)GetAbsoluteAddress(game_language_offset);
  return locale >= 0 && locale < (int)(sizeof(locales) / sizeof(locales[0])) ? locales[locale] : EN;
}
