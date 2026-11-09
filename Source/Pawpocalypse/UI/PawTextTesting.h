// PawTextTesting: Godot's words for the tests that compare with Godot (2026-11-09). Tests only.
//
// The designer's text pass changed 279 English lines (plain, informative, real numbers, one word per idea; CLAUDE.md). They
// are listed in Data/TextOverrides.csv with Godot's line beside each (tools/text_overrides.py). Many engine tests render a
// screen and compare it, word for word, with a Godot fixture or with Godot's English written in the test: they prove the
// wiring, the arguments and the number formats, which the new words do not change. FGodotWords puts Godot's English back into
// the loaded string tables for one test's run and the port's English back after it; Pawpocalypse.UI.Text.PortWords proves
// the game itself loads the port's English and that the scope gives it back.
#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"

class FAutomationTestBase;

namespace PawTextTesting
{
	struct FLine
	{
		FName Table;  // ST_UI, ST_Data, ST_Tree
		FString Key;
		FString Text;  // the port's English, as the table holds it (a written "\n" already a line break)
		FString Godot; // Godot's English, likewise
	};

	// Every line of Data/TextOverrides.csv; false (with an error on Test) when it cannot be read.
	bool ReadLines(FAutomationTestBase& Test, TArray<FLine>& Out);

	// The source string a loaded table holds for Key; false when the table or the key is missing.
	bool LoadedSource(FName Table, const FString& Key, FString& Out);

	// For the scope's life the tables say what Godot says; an error on Test when a line's table does not hold the port's
	// English to begin with (a stale Data/TextOverrides.csv, or two scopes at once).
	class FGodotWords
	{
	public:
		explicit FGodotWords(FAutomationTestBase& Test);
		~FGodotWords();
		int32 Num() const { return Swapped.Num(); }

	private:
		TArray<FLine> Swapped;
	};
}

#endif
