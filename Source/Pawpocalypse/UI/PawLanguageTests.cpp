// Pawpocalypse.UI.Language.*: the translations at run time (phase 7; PawText::RegisterTranslations, ShowCulture; KB U/20 §3).
//
// Pawpocalypse.UI.Language.Translations  the translation files load: 22 cultures (Godot's 20, Dutch and Danish), each with
//                                         its lines in use; German shown live (a text made in English before the switch follows
//                                         it, a formatted line, a data name), Japanese; a line whose English changed shows
//                                         the English until translated again (stale), and German again once it matches;
//                                         English back.
// Pawpocalypse.UI.Language.Labels        every language's picker label is Godot's (fixture `locales`, with D20: the native
//                                         name alone, no "(draft)"), Dutch's and Danish's own, and
//                                         the list is Godot's order with Dutch and Danish after Swedish.
// Pawpocalypse.UI.Language.Loader        crafted files: a good row, a stale one, broken placeholders, no text, no such line,
//                                         a line break; the counts (in use, left out, missing); which lines need words; only
//                                         a complete language is offered, unless unfinished ones are asked for.
//
// Negative tests: tools/sabotage_realm.py (`language_*`).

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/FileManager.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PcLocales.h"
#include "UI/PawText.h"

namespace
{
	FString S(const FText& T) { return T.ToString(); }

	// Shows a culture for the scope and the one before it after (the editor's own culture is never left changed).
	struct FCultureScope
	{
		FString Was;
		FCultureScope() : Was(FInternationalization::Get().GetCurrentCulture()->GetName()) {}
		~FCultureScope() { PawText::ShowCulture(Was); }
	};

	void SetSource(const FName Table, const FString& Key, const FString& Source)
	{
		const FStringTablePtr Loaded = FStringTableRegistry::Get().FindMutableStringTable(Table);
		if (!Loaded.IsValid())
		{
			return;
		}
#if WITH_EDITORONLY_DATA
		FString Notes;
		if (const FStringTableEntryConstPtr Entry = Loaded->FindEntry(Key))
		{
			Notes = Entry->GetDevNotes();
		}
		Loaded->SetSourceString(Key, Source, Notes);
#else
		Loaded->SetSourceString(Key, Source);
#endif
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawLanguageTranslationsTest, "Pawpocalypse.UI.Language.Translations", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawLanguageTranslationsTest::RunTest(const FString& Parameters)
{
	PawText::RegisterTables();
	PawText::RegisterTranslations();
	const TSet<FString>& Cultures = PawText::LoadedCultures();
	TestEqual(TEXT("22 languages translated: Godot's 20, Dutch and Danish (2026-11-20)"), Cultures.Num(), 22);
	TestTrue(TEXT("Dutch and Danish, the port's own"), Cultures.Contains(TEXT("nl")) && Cultures.Contains(TEXT("da")));
	for (const FString& Culture : Cultures)
	{
		const PawText::FTranslationCount Count = PawText::TranslationCount(Culture);
		TestTrue(FString::Printf(TEXT("%s: its lines in use (%d, %d left out)"), *Culture, Count.Lines, Count.Left), Count.Lines >= 500);
		TestEqual(FString::Printf(TEXT("%s: in use and missing make every line that needs words"), *Culture), Count.Lines + Count.Missing,
			PawText::LinesNeedingWords());
		TestNotNull(FString::Printf(TEXT("%s is a language of the game"), *Culture), Pc::LanguageByCulture(TCHAR_TO_UTF8(*Culture)));
		// Offered only when complete (Godot: "so an unfinished language cannot be chosen by accident").
		const FString Code = UTF8_TO_TCHAR(Pc::LanguageByCulture(TCHAR_TO_UTF8(*Culture))->Code);
		TestEqual(FString::Printf(TEXT("%s offered only when complete"), *Culture), PawText::Languages(false).Contains(Code), Count.Missing == 0);
		TestTrue(FString::Printf(TEXT("%s offered when unfinished ones are asked for"), *Culture), PawText::Languages(true).Contains(Code));
	}
	TestTrue(TEXT("the tables' lines that need words (tools/loc_status.py: 837 on 2026-11-20)"), PawText::LinesNeedingWords() > 800);

	const FCultureScope Restore;
	const FText MadeInEnglish = PawText::Ui(TEXT("TAB_realms"));
	TestEqual(TEXT("English first"), S(MadeInEnglish), FString(TEXT("Realms")));
	TestTrue(TEXT("German shown"), PawText::ShowCulture(TEXT("de")));
	TestEqual(TEXT("a line in German"), S(PawText::Ui(TEXT("TAB_upgrades"))), FString(TEXT("Verbesserungen")));
	TestEqual(TEXT("a text made before the switch follows it"), S(MadeInEnglish), FString(TEXT("Reiche")));
	FFormatNamedArguments Args;
	Args.Add(TEXT("name"), PawText::Verbatim(TEXT("Shred")));
	Args.Add(TEXT("level"), PawText::Count(3));
	Args.Add(TEXT("price"), PawText::Verbatim(TEXT("1.23K")));
	TestEqual(TEXT("a formatted line, German's order"), S(PawText::Ui(TEXT("SHOP_row_price"), Args)), FString(TEXT("Shred (Stufe 3) — 1.23K")));
	FFormatNamedArguments Payout;
	Payout.Add(TEXT("n"), PawText::Count(3));
	Payout.Add(TEXT("stardust"), PawText::Count(12));
	TestEqual(TEXT("a line over two lines"), S(PawText::Ui(TEXT("CEREMONY_payout"), Payout)), FString(TEXT("Wiedererwachen 3\n+12 Sternenstaub")));
	TestEqual(TEXT("a line in German, in that culture"), PawText::TranslationIn(PawText::TableUi, TEXT("TAB_realms"), TEXT("de")),
		FString(TEXT("Reiche")));
	TestTrue(TEXT("Japanese shown"), PawText::ShowCulture(TEXT("ja")));
	TestEqual(TEXT("a line in Japanese"), S(PawText::Ui(TEXT("TAB_upgrades"))), FString(TEXT("強化")));

	// A line whose English changed: the old translation is not shown (tools/loc_status.py lists it stale) until the English
	// matches what it was translated from again.
	PawText::ShowCulture(TEXT("de"));
	FString English;
	FStringTableRegistry::Get().FindStringTable(PawText::TableUi)->GetSourceString(TEXT("TAB_upgrades"), English);
	const int32 LeftBefore = PawText::TranslationCount(TEXT("de")).Left;
	SetSource(PawText::TableUi, TEXT("TAB_upgrades"), TEXT("Shop"));
	PawText::RegisterTranslations();
	PawText::ShowCulture(TEXT("de"));
	TestEqual(TEXT("changed English: the English shown"), S(PawText::Ui(TEXT("TAB_upgrades"))), FString(TEXT("Shop")));
	TestEqual(TEXT("and left out"), PawText::TranslationCount(TEXT("de")).Left, LeftBefore + 1);
	TestTrue(TEXT("and not in use"), PawText::TranslationIn(PawText::TableUi, TEXT("TAB_upgrades"), TEXT("de")).IsEmpty());
	SetSource(PawText::TableUi, TEXT("TAB_upgrades"), English);
	PawText::RegisterTranslations();
	PawText::ShowCulture(TEXT("de"));
	TestEqual(TEXT("the English it was made for: German again"), S(PawText::Ui(TEXT("TAB_upgrades"))), FString(TEXT("Verbesserungen")));

	TestTrue(TEXT("English shown"), PawText::ShowCulture(TEXT("en")));
	TestEqual(TEXT("English again"), S(PawText::Ui(TEXT("TAB_upgrades"))), FString(TEXT("Upgrades")));
	TestEqual(TEXT("the early text too"), S(MadeInEnglish), FString(TEXT("Realms")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawLanguageLabelsTest, "Pawpocalypse.UI.Language.Labels", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawLanguageLabelsTest::RunTest(const FString& Parameters)
{
	PawText::RegisterTables();
	PawText::RegisterTranslations();
	// Godot's picker_label() for every language, as the fixture recorded it (with D20: the name alone, no "(draft)").
	TArray<FString> Rows;
	const FString Fixture = FPaths::ProjectDir() / TEXT("Source/PawCore/Tests/Data/locales_godot.tsv");
	if (!TestTrue(TEXT("the locales fixture"), FFileHelper::LoadFileToStringArray(Rows, *Fixture)))
	{
		return false;
	}
	int32 Labels = 0;
	TArray<FString> GodotOrder;
	for (const FString& Row : Rows)
	{
		TArray<FString> Cells;
		Row.ParseIntoArray(Cells, TEXT("\t"), false);
		if (Cells.Num() == 5 && Cells[0] == TEXT("label"))
		{
			TestEqual(FString::Printf(TEXT("%s's label is Godot's"), *Cells[1]), S(PawText::LanguageLabel(Cells[1])), Cells[3]);
			GodotOrder.Add(Cells[1]);
			++Labels;
		}
	}
	TestEqual(TEXT("every language Godot has"), Labels, 21);
	// The port's two (Godot had neither): each in its own name.
	TestEqual(TEXT("Dutch's label"), S(PawText::LanguageLabel(TEXT("nl"))), FString(TEXT("Nederlands")));
	TestEqual(TEXT("Danish's label"), S(PawText::LanguageLabel(TEXT("da"))), FString(TEXT("Dansk")));
	// Every language is in use here, in Godot's order, Dutch and Danish after Swedish (PcLocales).
	const int32 Sv = GodotOrder.IndexOfByKey(TEXT("sv"));
	if (TestTrue(TEXT("Swedish in Godot's list"), Sv != INDEX_NONE))
	{
		GodotOrder.Insert(TEXT("da"), Sv + 1);
		GodotOrder.Insert(TEXT("nl"), Sv + 1);
	}
	TestEqual(TEXT("the list is Godot's, in Godot's order"), FString::Join(PawText::Languages(true), TEXT(",")), FString::Join(GodotOrder, TEXT(",")));
	return true;
}

namespace
{
	FString Cell(const FString& S)
	{
		return TEXT("\"") + S.Replace(TEXT("\""), TEXT("\"\"")) + TEXT("\"");
	}

	FString Row(const FString& Key, const FString& Text, const FString& Source)
	{
		return Cell(Key) + TEXT(",") + Cell(Text) + TEXT(",") + Cell(Source) + TEXT("\n");
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawLanguageLoaderTest, "Pawpocalypse.UI.Language.Loader", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawLanguageLoaderTest::RunTest(const FString& Parameters)
{
	PawText::RegisterTables();
	// Which lines need words (tools/loc_status.py needs()).
	TestFalse(TEXT("x5 is a mark"), PawText::NeedsWords(TEXT("x5")));
	TestFalse(TEXT("x10 too"), PawText::NeedsWords(TEXT("x10")));
	TestFalse(TEXT("placeholders and a slash"), PawText::NeedsWords(TEXT("{value} / {threshold}")));
	TestFalse(TEXT("a plus"), PawText::NeedsWords(TEXT("+{amount}")));
	TestFalse(TEXT("question marks"), PawText::NeedsWords(TEXT("???")));
	TestFalse(TEXT("a multiplier after a placeholder"), PawText::NeedsWords(TEXT("{n}x5")));
	TestTrue(TEXT("words"), PawText::NeedsWords(TEXT("Buy amount")));
	TestTrue(TEXT("a word with a placeholder"), PawText::NeedsWords(TEXT("Level {level}")));
	TestTrue(TEXT("an x before a placeholder is a letter"), PawText::NeedsWords(TEXT("x{n}")));
	TestTrue(TEXT("Max"), PawText::NeedsWords(TEXT("Max")));

	// Crafted files: German with one of each kind of row; Danish complete (every line that needs words, "Da: " + the English:
	// Danish has no files of its own, so nothing it registers is ever shown elsewhere).
	const FString Root = FPaths::ProjectSavedDir() / TEXT("Automation/PawLanguageLoader");
	IFileManager::Get().DeleteDirectory(*Root, false, true);
	FString Payout;
	FStringTableRegistry::Get().FindStringTable(PawText::TableUi)->GetSourceString(TEXT("CEREMONY_payout"), Payout);
	TestTrue(TEXT("CEREMONY_payout is two lines"), Payout.Contains(TEXT("\n")));
	FString German = TEXT("\"Key\",\"Text\",\"Source\"\n");
	German += Row(TEXT("TAB_upgrades"), TEXT("Verbesserungen"), TEXT("Upgrades"));
	German += Row(TEXT("TAB_realms"), TEXT("Reiche"), TEXT("Old realms"));                                                // stale
	German += Row(TEXT("SHOP_row_price"), TEXT("{name} (Stufe {lvl}) — {price}"), TEXT("{name} (Lv {level}) — {price}")); // {lvl}
	German += Row(TEXT("TAB_jobs"), TEXT(""), TEXT("Jobs"));                                                              // no text
	German += Row(TEXT("NO_SUCH_KEY"), TEXT("Nichts"), TEXT("Nothing"));                                                  // no line
	// The English written with an escaped line break, as a hand-made file may: read as the table reads it.
	German += Row(TEXT("CEREMONY_payout"), TEXT("Wiedererwachen {n}\n+{stardust} Sternenstaub"), Payout.Replace(TEXT("\n"), TEXT("\\n")));
	TestTrue(TEXT("the German file"), FFileHelper::SaveStringToFile(German, *(Root / TEXT("de/ST_UI.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
	int32 DanishLines = 0;
	for (const FName Table : { PawText::TableUi, PawText::TableData, PawText::TableTree })
	{
		FString Danish = TEXT("\"Key\",\"Text\",\"Source\"\n");
		FStringTableRegistry::Get().FindStringTable(Table)->EnumerateSourceStrings([&Danish, &DanishLines](const FString& Key, const FString& English)
		{
			if (PawText::NeedsWords(English))
			{
				Danish += Row(Key, TEXT("Da: ") + English, English);
				++DanishLines;
			}
			return true;
		});
		TestTrue(TEXT("a Danish file"), FFileHelper::SaveStringToFile(Danish, *(Root / TEXT("da") / (Table.ToString() + TEXT(".csv"))),
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
	}

	PawText::RegisterTranslations(Root);
	TestEqual(TEXT("two cultures"), PawText::LoadedCultures().Num(), 2);
	const PawText::FTranslationCount De = PawText::TranslationCount(TEXT("de"));
	TestEqual(TEXT("German: two rows in use (the plain one, the two-line one)"), De.Lines, 2);
	TestEqual(TEXT("four left out: stale, placeholders, no text, no line"), De.Left, 4);
	TestEqual(TEXT("the rest missing"), De.Missing, PawText::LinesNeedingWords() - 2);
	TestEqual(TEXT("in use"), PawText::TranslationIn(PawText::TableUi, TEXT("TAB_upgrades"), TEXT("de")), FString(TEXT("Verbesserungen")));
	TestEqual(TEXT("the escaped English matched"), PawText::TranslationIn(PawText::TableUi, TEXT("CEREMONY_payout"), TEXT("de")),
		FString(TEXT("Wiedererwachen {n}\n+{stardust} Sternenstaub")));
	TestTrue(TEXT("stale: not in use"), PawText::TranslationIn(PawText::TableUi, TEXT("TAB_realms"), TEXT("de")).IsEmpty());
	TestTrue(TEXT("other placeholders: not in use"), PawText::TranslationIn(PawText::TableUi, TEXT("SHOP_row_price"), TEXT("de")).IsEmpty());
	TestTrue(TEXT("no text: not in use"), PawText::TranslationIn(PawText::TableUi, TEXT("TAB_jobs"), TEXT("de")).IsEmpty());
	const PawText::FTranslationCount Da = PawText::TranslationCount(TEXT("da"));
	TestTrue(TEXT("Danish complete"), Da.Lines == DanishLines && Da.Missing == 0 && Da.Left == 0 && DanishLines == PawText::LinesNeedingWords());
	TestEqual(TEXT("offered: English and the complete language"), FString::Join(PawText::Languages(false), TEXT(",")), FString(TEXT("en,da")));
	TestEqual(TEXT("asked for unfinished ones too: German, in the picker's order"), FString::Join(PawText::Languages(true), TEXT(",")),
		FString(TEXT("en,de,da")));
	{
		const FCultureScope Restore;
		PawText::ShowCulture(TEXT("da"));
		TestEqual(TEXT("Danish shown"), S(PawText::Ui(TEXT("PAUSE_title"))), FString(TEXT("Da: PAUSED")));
	}

	// The game's own files again.
	PawText::RegisterTranslations();
	IFileManager::Get().DeleteDirectory(*Root, false, true);
	TestEqual(TEXT("the game's files again: 22 cultures"), PawText::LoadedCultures().Num(), 22);
	return true;
}

#endif