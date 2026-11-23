#include "PcLocales.h"

#include <algorithm>

namespace Pc
{
	const char* const LocaleSystem = "system";
	const char* const LocaleEnglish = "en";

	namespace
	{
		bool Has(const std::vector<std::string>& List, const std::string& Code)
		{
			return std::find(List.begin(), List.end(), Code) != List.end();
		}

		bool StartsWith(const std::string& S, const std::string& Prefix)
		{
			return S.size() >= Prefix.size() && S.compare(0, Prefix.size(), Prefix) == 0;
		}

		bool Contains(const std::string& S, const char* Part)
		{
			return S.find(Part) != std::string::npos;
		}
	}

	const std::vector<PcLanguage>& Languages()
	{
		// locales.gd ORDER with NATIVE_NAMES and STEAM_TO_LOCALE; Unreal's culture names: Godot's es_MX is Steam's Latin
		// American Spanish (es-419), its plain "pt" is Portugal's (pt-PT); docs/WORKLIST-LOC-2026-11-18.md §2a.
		static const std::vector<PcLanguage> List = {
			{ "en", "en", "English", "english" },
			{ "zh_Hans", "zh-Hans", "简体中文", "schinese" },
			{ "ru", "ru", "Русский", "russian" },
			{ "es", "es", "Español (España)", "spanish" },
			{ "pt_BR", "pt-BR", "Português (Brasil)", "brazilian" },
			{ "de", "de", "Deutsch", "german" },
			{ "ja", "ja", "日本語", "japanese" },
			{ "fr", "fr", "Français", "french" },
			{ "pl", "pl", "Polski", "polish" },
			{ "ko", "ko", "한국어", "koreana" },
			{ "zh_Hant", "zh-Hant", "繁體中文", "tchinese" },
			{ "tr", "tr", "Türkçe", "turkish" },
			{ "th", "th", "ไทย", "thai" },
			{ "es_MX", "es-419", "Español (Latinoamérica)", "latam" },
			{ "uk", "uk", "Українська", "ukrainian" },
			{ "it", "it", "Italiano", "italian" },
			{ "cs", "cs", "Čeština", "czech" },
			{ "hu", "hu", "Magyar", "hungarian" },
			{ "pt", "pt-PT", "Português (Portugal)", "portuguese" },
			{ "vi", "vi", "Tiếng Việt", "vietnamese" },
			{ "sv", "sv", "Svenska", "swedish" },
			// The port's two (designer, 2026-11-18): every line new, no Godot translation.
			{ "nl", "nl", "Nederlands", "dutch" },
			{ "da", "da", "Dansk", "danish" },
		};
		return List;
	}

	const PcLanguage* LanguageByCode(const std::string& Code)
	{
		for (const PcLanguage& L : Languages())
		{
			if (Code == L.Code)
			{
				return &L;
			}
		}
		return nullptr;
	}

	const PcLanguage* LanguageByCulture(const std::string& Culture)
	{
		for (const PcLanguage& L : Languages())
		{
			if (Culture == L.Culture)
			{
				return &L;
			}
		}
		return nullptr;
	}

	std::vector<std::string> LocalesAvailable(const std::set<std::string>& Loaded)
	{
		std::vector<std::string> Out;
		for (const PcLanguage& L : Languages())
		{
			const std::string Code = L.Code;
			if (Code == LocaleEnglish || Loaded.count(Code) != 0)
			{
				Out.push_back(Code);
			}
		}
		return Out;
	}

	std::string LocaleNativeName(const std::string& Code)
	{
		const PcLanguage* L = LanguageByCode(Code);
		return L ? std::string(L->Native) : Code;
	}

	std::string LocalePickerLabel(const std::string& Code)
	{
		// D20 (designer, 2026-11-23: "drop the draft"): Godot adds "(draft)" to every language but English; the port does not.
		return LocaleNativeName(Code);
	}

	std::string LocaleFromSystem(const std::string& OsLocale, const std::vector<std::string>& Available)
	{
		// Godot's OS.get_locale() says "de_DE"; Unreal's platform says "de-DE": one spelling.
		std::string Wanted = OsLocale;
		std::replace(Wanted.begin(), Wanted.end(), '-', '_');
		if (Has(Available, Wanted))
		{
			return Wanted;
		}
		const size_t Cut = Wanted.find('_');
		const std::string Lang = Wanted.substr(0, Cut);
		// The script forms of Chinese: the OS says zh_CN / zh_TW, the files say zh_Hans / zh_Hant. PORT FIX F52: Macau is
		// Traditional, as Taiwan and Hong Kong.
		if (Lang == "zh")
		{
			const bool bTraditional = Contains(Wanted, "TW") || Contains(Wanted, "HK") || Contains(Wanted, "MO") || Contains(Wanted, "Hant");
			const std::string Script = bTraditional ? "zh_Hant" : "zh_Hans";
			return Has(Available, Script) ? Script : std::string(LocaleEnglish);
		}
		// PORT FIX F52: Spanish from anywhere but Spain is the Latin American translation (Godot found it for Mexico only).
		const std::string Region = Cut == std::string::npos ? std::string() : Wanted.substr(Wanted.rfind('_') + 1);
		if (Lang == "es" && !Region.empty() && Region != "ES" && Has(Available, "es_MX"))
		{
			return "es_MX";
		}
		// PORT FIX F52: the language itself before a variant of it (Godot took the first in ORDER: Portugal got pt_BR).
		for (const std::string& Code : Available)
		{
			if (Code == Lang)
			{
				return Code;
			}
		}
		for (const std::string& Code : Available)
		{
			if (StartsWith(Code, Lang + "_"))
			{
				return Code;
			}
		}
		return LocaleEnglish;
	}

	std::string LocaleFromSteam(const std::string& SteamLanguage, const std::vector<std::string>& Available)
	{
		for (const PcLanguage& L : Languages())
		{
			if (SteamLanguage == L.Steam)
			{
				return Has(Available, L.Code) ? std::string(L.Code) : std::string(LocaleEnglish);
			}
		}
		return LocaleEnglish;
	}

	std::string LocaleResolve(const std::string& Setting, const std::vector<std::string>& Available, const std::string& OsLocale,
		const std::string& SteamLanguage)
	{
		if (Setting != LocaleSystem && Has(Available, Setting))
		{
			return Setting;
		}
		// Under Steam, "system" means the language the player set for this game in Steam, which can differ from the OS.
		if (!SteamLanguage.empty())
		{
			return LocaleFromSteam(SteamLanguage, Available);
		}
		return LocaleFromSystem(OsLocale, Available);
	}
}
