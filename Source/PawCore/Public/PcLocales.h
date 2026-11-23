#pragma once

// The languages the game knows (Godot scripts/systems/locales.gd; proven against it, fixture `locales`): each one's name in
// itself, Unreal's culture for it, Steam's name for it; which the player may pick; what "system" and Steam resolve to; the
// picker's label. The setting stores Godot's code ("zh_Hans", "es_MX", "pt"), so settings.cfg keeps Godot's values; the game
// module turns a code into its culture (PcLanguage::Culture) when it applies it.
//
// The port's own (D16, docs/01 §5):
//   F52  from_system picked the wrong variant for a region it does not name: a Portugal system got Brazilian Portuguese
//        (pt_BR comes first in ORDER and begins with "pt_"), a Spanish system outside Spain and Mexico got Spain's Spanish
//        though the game has Latin American Spanish, Macau got Simplified Chinese. Fixed: the language itself before a
//        variant of it, Spanish with a region other than Spain is Latin American, Macau is Traditional.
//   Dutch and Danish join the 21 (designer, 2026-11-18: "Add dutch and danish"), after Swedish.
//   No test locale in the list: Godot's "xa" (a .po of pseudo text, listed in the editor only); the port's pseudo text is a
//   launch flag (docs/WORKLIST-LOC-2026-11-18.md §3, step 4).

#include "PcCore.h"

#include <set>
#include <string>
#include <vector>

namespace Pc
{
	struct PcLanguage
	{
		const char* Code;    // Godot's locale code, the settings value: "en", "zh_Hans", "es_MX", "pt"
		const char* Culture; // Unreal's culture (the translation files' folder): "en", "zh-Hans", "es-419", "pt-PT"
		const char* Native;  // its name in itself, UTF-8 (NATIVE_NAMES: never translated)
		const char* Steam;   // ISteamApps::GetCurrentGameLanguage's name (STEAM_TO_LOCALE)
	};

	// Every language, in the picker's order (ORDER: English, then by Steam share; Dutch and Danish last).
	PAWCORE_API const std::vector<PcLanguage>& Languages();
	// The language with this code, or with this culture; null when there is none.
	PAWCORE_API const PcLanguage* LanguageByCode(const std::string& Code);
	PAWCORE_API const PcLanguage* LanguageByCulture(const std::string& Culture);

	// The setting's value for "follow the system" (settings_manager.gd's default).
	extern PAWCORE_API const char* const LocaleSystem; // "system"
	extern PAWCORE_API const char* const LocaleEnglish; // "en"

	// available(): English and every loaded language (a code in Loaded), in order. English is the source text: always there.
	PAWCORE_API std::vector<std::string> LocalesAvailable(const std::set<std::string>& Loaded);
	// native_name(): the language's name in itself; an unknown code is its own name.
	PAWCORE_API std::string LocaleNativeName(const std::string& Code);
	// picker_label(): the name in itself, and nothing else. Godot adds its own word for "(draft)" to every language but
	// English (REVIEWED); the port drops it (D20, designer 2026-11-23: "drop the draft"; no native read will come).
	PAWCORE_API std::string LocalePickerLabel(const std::string& Code);
	// from_system(): the OS locale ("de_DE", "pt-PT", "zh-Hant-TW", "es-419": '_' or '-') to an available code; else English.
	PAWCORE_API std::string LocaleFromSystem(const std::string& OsLocale, const std::vector<std::string>& Available);
	// from_steam(): Steam's language name ("brazilian", "latam") to an available code; else English.
	PAWCORE_API std::string LocaleFromSteam(const std::string& SteamLanguage, const std::vector<std::string>& Available);
	// apply_language()'s choice: the setting when it is available; else (system, or a language not available) the system's,
	// or Steam's when the game runs under Steam (SteamLanguage not empty).
	PAWCORE_API std::string LocaleResolve(const std::string& Setting, const std::vector<std::string>& Available, const std::string& OsLocale,
		const std::string& SteamLanguage);
}
