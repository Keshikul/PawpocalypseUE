#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcLocales.h"

#include <algorithm>
#include <cstdlib>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// The languages against the REAL locales.gd (fixture `locales`, with the port's fix F52): available() for 104 loaded sets,
// from_system() over 85 OS locales and from_steam() over 28 Steam names in each, and every language's picker label with
// its own "(draft)" from Godot's .po files. Then what Godot does not have: Dutch and Danish, Unreal's '-' spellings, and
// apply_language()'s choice (LocaleResolve).

namespace
{
	std::string DataDir()
	{
		const char* Env = std::getenv("PAWCORE_DATA_DIR");
		return Env ? std::string(Env) : std::string("Data");
	}

	std::vector<std::string> Split(const std::string& S, char By)
	{
		std::vector<std::string> Out;
		size_t Start = 0;
		while (true)
		{
			const size_t At = S.find(By, Start);
			Out.push_back(S.substr(Start, At == std::string::npos ? std::string::npos : At - Start));
			if (At == std::string::npos)
			{
				return Out;
			}
			Start = At + 1;
		}
	}

	std::vector<std::string> List(const std::string& Joined)
	{
		return Joined.empty() ? std::vector<std::string>() : Split(Joined, ',');
	}

	std::string Join(const std::vector<std::string>& Items)
	{
		std::string Out;
		for (size_t I = 0; I < Items.size(); ++I)
		{
			Out += (I ? "," : "") + Items[I];
		}
		return Out;
	}

	std::vector<std::string> AvailableFor(const std::string& LoadedJoined)
	{
		const std::vector<std::string> Loaded = List(LoadedJoined);
		return Pc::LocalesAvailable(std::set<std::string>(Loaded.begin(), Loaded.end()));
	}

	std::vector<std::string> All()
	{
		std::set<std::string> Every;
		for (const Pc::PcLanguage& L : Pc::Languages())
		{
			Every.insert(L.Code);
		}
		return Pc::LocalesAvailable(Every);
	}
}

TEST_CASE("Locales: available, from_system, from_steam and the picker's labels match the REAL locales.gd (fixture locales)")
{
	std::string Text;
	REQUIRE_MESSAGE(Pc::ReadTextFile(DataDir() + "/../Source/PawCore/Tests/Data/locales_godot.tsv", Text), "missing locales_godot.tsv");
	std::istringstream In(Text);
	std::string Line;
	int Avail = 0, System = 0, Steam = 0, Labels = 0, Fixed = 0;
	std::set<std::string> Results;
	while (std::getline(In, Line))
	{
		if (Line.empty() || Line[0] == '#')
		{
			continue;
		}
		const std::vector<std::string> C = Split(Line, '\t');
		REQUIRE(C.size() >= 4);
		if (C[0] == "avail")
		{
			CHECK_MESSAGE(Join(AvailableFor(C[1])) == C[3], "available() with " << C[1]);
			++Avail;
		}
		else if (C[0] == "system")
		{
			const std::string Got = Pc::LocaleFromSystem(C[1], AvailableFor(C[2]));
			CHECK_MESSAGE(Got == C[3], "from_system(" << C[1] << ") with " << C[2]);
			// The same locale as Unreal's platform spells it ("pt-PT"): the same answer.
			std::string Dashed = C[1];
			for (char& Ch : Dashed)
			{
				Ch = Ch == '_' ? '-' : Ch;
			}
			CHECK_MESSAGE(Pc::LocaleFromSystem(Dashed, AvailableFor(C[2])) == C[3], "from_system(" << Dashed << ")");
			Results.insert(Got);
			// Rows where Godot itself (with the fix) gives the fixed answer: proof the fixture was made with F52.
			Fixed += (C[1] == "pt_PT" && C[3] == "pt") || (C[1] == "es_AR" && C[3] == "es_MX") || (C[1] == "zh_MO" && C[3] == "zh_Hant") ? 1 : 0;
			++System;
		}
		else if (C[0] == "steam")
		{
			CHECK_MESSAGE(Pc::LocaleFromSteam(C[1], AvailableFor(C[2])) == C[3], "from_steam(" << C[1] << ") with " << C[2]);
			++Steam;
		}
		else if (C[0] == "label")
		{
			REQUIRE(C.size() == 5);
			CHECK(Pc::LocaleNativeName(C[1]) == C[2]);
			CHECK_MESSAGE(Pc::LocalePickerLabel(C[1], C[4]) == C[3], "picker_label(" << C[1] << ")");
			CHECK(Pc::LocaleReviewed(C[1]) == (C[1] == "en"));
			const Pc::PcLanguage* L = Pc::LanguageByCode(C[1]);
			REQUIRE(L != nullptr);
			CHECK(Pc::LanguageByCulture(L->Culture) == L);
			++Labels;
		}
	}
	// Coverage: every kind of row, every language labelled, and from_system reached every language and English.
	CHECK(Avail == 104);
	CHECK(System == 104 * 85);
	CHECK(Steam == 104 * 28);
	CHECK(Labels == 21);
	CHECK(Results.size() == 21);
	CHECK(Fixed > 0);
}

TEST_CASE("Locales: F52, the variant a region asks for (the port's fix, docs/01 §5)")
{
	const std::vector<std::string> Every = All();
	CHECK(Pc::LocaleFromSystem("pt_PT", Every) == "pt");     // Godot: pt_BR
	CHECK(Pc::LocaleFromSystem("pt-AO", Every) == "pt");     // Godot: pt_BR
	CHECK(Pc::LocaleFromSystem("pt_BR", Every) == "pt_BR");
	CHECK(Pc::LocaleFromSystem("es_AR", Every) == "es_MX");  // Godot: es
	CHECK(Pc::LocaleFromSystem("es-419", Every) == "es_MX"); // Windows' Latin American Spanish; Godot: es
	CHECK(Pc::LocaleFromSystem("es_ES", Every) == "es");
	CHECK(Pc::LocaleFromSystem("es", Every) == "es");
	CHECK(Pc::LocaleFromSystem("zh_MO", Every) == "zh_Hant"); // Godot: zh_Hans
	CHECK(Pc::LocaleFromSystem("zh-Hant-TW", Every) == "zh_Hant");
	CHECK(Pc::LocaleFromSystem("zh-CN", Every) == "zh_Hans");
	// Without the Latin American file, Spain's; without Portugal's, Brazil's.
	std::vector<std::string> NoLatam = Every;
	NoLatam.erase(std::find(NoLatam.begin(), NoLatam.end(), "es_MX"));
	CHECK(Pc::LocaleFromSystem("es_AR", NoLatam) == "es");
	std::vector<std::string> NoPortugal = Every;
	NoPortugal.erase(std::find(NoPortugal.begin(), NoPortugal.end(), "pt"));
	CHECK(Pc::LocaleFromSystem("pt_PT", NoPortugal) == "pt_BR");
}

TEST_CASE("Locales: Dutch and Danish (the port's two, designer 2026-11-18)")
{
	const std::vector<Pc::PcLanguage>& L = Pc::Languages();
	REQUIRE(L.size() == 23);
	CHECK(std::string(L[21].Code) == "nl");
	CHECK(std::string(L[22].Code) == "da");
	CHECK(Pc::LocaleNativeName("nl") == "Nederlands");
	CHECK(Pc::LocaleNativeName("da") == "Dansk");
	// Listed only once loaded, after Swedish.
	CHECK(Join(Pc::LocalesAvailable({ "nl", "da", "sv" })) == "en,sv,nl,da");
	CHECK(Join(Pc::LocalesAvailable({})) == "en");
	const std::vector<std::string> Every = All();
	CHECK(Pc::LocaleFromSystem("nl_BE", Every) == "nl");
	CHECK(Pc::LocaleFromSystem("da-DK", Every) == "da");
	CHECK(Pc::LocaleFromSystem("da-DK", Pc::LocalesAvailable({ "de" })) == "en");
	CHECK(Pc::LocaleFromSteam("dutch", Every) == "nl");
	CHECK(Pc::LocaleFromSteam("danish", Every) == "da");
	CHECK(Pc::LocaleFromSteam("danish", Pc::LocalesAvailable({ "nl" })) == "en");
	// Every culture is unique and finds its language back.
	std::set<std::string> Cultures;
	for (const Pc::PcLanguage& One : L)
	{
		CHECK(Cultures.insert(One.Culture).second);
		CHECK(Pc::LanguageByCulture(One.Culture) == &One);
	}
	CHECK(Pc::LanguageByCode("xa") == nullptr);
	CHECK(Pc::LocaleNativeName("xx") == "xx");
}

TEST_CASE("Locales: apply_language's choice (settings_manager.gd)")
{
	const std::vector<std::string> Some = Pc::LocalesAvailable({ "de", "fr", "ja" });
	// The setting when it is available.
	CHECK(Pc::LocaleResolve("fr", Some, "de_DE", "") == "fr");
	CHECK(Pc::LocaleResolve("en", Some, "de_DE", "") == "en");
	// "system", or a language not available: the system's.
	CHECK(Pc::LocaleResolve("system", Some, "de_DE", "") == "de");
	CHECK(Pc::LocaleResolve("ru", Some, "ja-JP", "") == "ja");
	CHECK(Pc::LocaleResolve("system", Some, "ru_RU", "") == "en");
	// Under Steam, the language set for this game in Steam, over the OS.
	CHECK(Pc::LocaleResolve("system", Some, "de_DE", "french") == "fr");
	CHECK(Pc::LocaleResolve("system", Some, "de_DE", "russian") == "en");
	CHECK(Pc::LocaleResolve("ja", Some, "de_DE", "french") == "ja");
}

#endif
