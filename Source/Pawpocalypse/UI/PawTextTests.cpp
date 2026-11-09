// The port's own English (2026-11-09; Data/TextOverrides.csv, tools/text_overrides.py, UI/PawTextTesting.h).
//
// Pawpocalypse.UI.Text.PortWords   the game loads the port's English for every line of the designer's text pass, in all
//                                  three tables; FGodotWords swaps in Godot's line for each and gives the port's back when
//                                  it ends; a text made through PawText inside the scope says Godot's words, outside it the
//                                  port's (the scope reaches what the screens show).
//
// Negative tests: tools/sabotage_realm.py (`text_scope_*`).

#include "Misc/AutomationTest.h"
#include "UI/PawText.h"
#include "UI/PawTextTesting.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTextPortWordsTest, "Pawpocalypse.UI.Text.PortWords", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawTextPortWordsTest::RunTest(const FString& Parameters)
{
	PawText::RegisterTables();
	TArray<PawTextTesting::FLine> Lines;
	if (!PawTextTesting::ReadLines(*this, Lines))
	{
		return false;
	}
	TMap<FName, int32> PerTable;
	for (const PawTextTesting::FLine& Line : Lines)
	{
		++PerTable.FindOrAdd(Line.Table);
	}
	TestTrue(TEXT("the text pass's lines (279 on 2026-11-09)"), Lines.Num() >= 279);
	TestTrue(TEXT("lines in ST_UI"), PerTable.FindRef(PawText::TableUi) > 0);
	TestTrue(TEXT("lines in ST_Data"), PerTable.FindRef(PawText::TableData) > 0);
	TestTrue(TEXT("lines in ST_Tree"), PerTable.FindRef(PawText::TableTree) > 0);

	auto Count = [&Lines](bool bGodot)
	{
		int32 Matching = 0;
		for (const PawTextTesting::FLine& Line : Lines)
		{
			FString Now;
			Matching += PawTextTesting::LoadedSource(Line.Table, Line.Key, Now) && Now == (bGodot ? Line.Godot : Line.Text) ? 1 : 0;
		}
		return Matching;
	};
	int32 Differ = 0;
	for (const PawTextTesting::FLine& Line : Lines)
	{
		Differ += Line.Text != Line.Godot ? 1 : 0;
	}
	TestEqual(TEXT("every line changes Godot's words"), Differ, Lines.Num());
	TestEqual(TEXT("the game loads the port's English, every line"), Count(false), Lines.Num());

	const FString PortHeadline = PawText::Ui(TEXT("VICTORY_headline")).ToString();
	const FString PortBlurb = PawText::TreeBlurb("unlock_reflex").ToString();
	const FString PortCard = PawText::DataField(TEXT("CARD"), "bounty", TEXT("desc")).ToString();
	{
		const PawTextTesting::FGodotWords Godot(*this);
		TestEqual(TEXT("the scope swaps every line"), Godot.Num(), Lines.Num());
		TestEqual(TEXT("inside the scope the tables say Godot's words"), Count(true), Lines.Num());
		TestEqual(TEXT("a screen's text inside the scope: Godot's headline"), PawText::Ui(TEXT("VICTORY_headline")).ToString(),
			FString(TEXT("THE VOID IS QUIET")));
		TestNotEqual(TEXT("a tree blurb inside the scope is Godot's"), PawText::TreeBlurb("unlock_reflex").ToString(), PortBlurb);
		TestNotEqual(TEXT("a card body inside the scope is Godot's"),
			PawText::DataField(TEXT("CARD"), "bounty", TEXT("desc")).ToString(), PortCard);
	}
	TestEqual(TEXT("after the scope the port's English is back, every line"), Count(false), Lines.Num());
	TestEqual(TEXT("the port's headline"), PawText::Ui(TEXT("VICTORY_headline")).ToString(), FString(TEXT("VICTORY")));
	TestEqual(TEXT("the port's tree blurb back"), PawText::TreeBlurb("unlock_reflex").ToString(), PortBlurb);
	TestEqual(TEXT("the port's card body back"), PawText::DataField(TEXT("CARD"), "bounty", TEXT("desc")).ToString(), PortCard);
	TestEqual(TEXT("the port's card body"), PortCard, FString(TEXT("Signature enemies give x{v} Catnip.")));
	return true;
}

#endif
