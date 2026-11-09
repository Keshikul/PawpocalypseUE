#include "UI/PawTextTesting.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"
#include "UI/PawText.h"

namespace PawTextTesting
{
	namespace
	{
		void SetSource(const FStringTablePtr& Table, const FString& Key, const FString& Source)
		{
#if WITH_EDITORONLY_DATA
			FString Notes;
			if (const FStringTableEntryConstPtr Entry = Table->FindEntry(Key))
			{
				Notes = Entry->GetDevNotes();
			}
			Table->SetSourceString(Key, Source, Notes);
#else
			Table->SetSourceString(Key, Source);
#endif
		}
	}

	bool ReadLines(FAutomationTestBase& Test, TArray<FLine>& Out)
	{
		const FString File = FPaths::Combine(FPaths::ProjectDir(), TEXT("Data/TextOverrides.csv"));
		FString Csv;
		if (!FFileHelper::LoadFileToString(Csv, *File))
		{
			Test.AddError(FString::Printf(TEXT("cannot read %s"), *File));
			return false;
		}
		const FCsvParser Parser(MoveTemp(Csv));
		const FCsvParser::FRows& Rows = Parser.GetRows();
		if (Rows.Num() < 2 || Rows[0].Num() < 4 || FString(Rows[0][0]) != TEXT("table") || FString(Rows[0][1]) != TEXT("key")
			|| FString(Rows[0][2]) != TEXT("text") || FString(Rows[0][3]) != TEXT("godot"))
		{
			Test.AddError(TEXT("Data/TextOverrides.csv: not table,key,text,godot,..."));
			return false;
		}
		for (int32 R = 1; R < Rows.Num(); ++R)
		{
			if (Rows[R].Num() < 4)
			{
				continue; // the trailing empty line
			}
			FLine Line;
			Line.Table = FName(Rows[R][0]);
			Line.Key = Rows[R][1];
			// What the table import does to a source string (FStringTable::ImportStrings).
			Line.Text = FString(Rows[R][2]).ReplaceEscapedCharWithChar();
			Line.Godot = FString(Rows[R][3]).ReplaceEscapedCharWithChar();
			Out.Add(MoveTemp(Line));
		}
		return true;
	}

	bool LoadedSource(const FName Table, const FString& Key, FString& Out)
	{
		const FStringTableConstPtr Loaded = FStringTableRegistry::Get().FindStringTable(Table);
		return Loaded.IsValid() && Loaded->GetSourceString(Key, Out);
	}

	FGodotWords::FGodotWords(FAutomationTestBase& Test)
	{
		PawText::RegisterTables();
		TArray<FLine> Lines;
		if (!ReadLines(Test, Lines))
		{
			return;
		}
		for (const FLine& Line : Lines)
		{
			const FStringTablePtr Table = FStringTableRegistry::Get().FindMutableStringTable(Line.Table);
			FString Now;
			if (!Table.IsValid() || !Table->GetSourceString(Line.Key, Now))
			{
				Test.AddError(FString::Printf(TEXT("Godot's words: %s has no %s"), *Line.Table.ToString(), *Line.Key));
				continue;
			}
			if (Now != Line.Text)
			{
				Test.AddError(FString::Printf(TEXT("Godot's words: %s %s holds '%s', not the port's '%s'"), *Line.Table.ToString(),
					*Line.Key, *Now, *Line.Text));
				continue;
			}
			SetSource(Table, Line.Key, Line.Godot);
			Swapped.Add(Line);
		}
	}

	FGodotWords::~FGodotWords()
	{
		for (const FLine& Line : Swapped)
		{
			if (const FStringTablePtr Table = FStringTableRegistry::Get().FindMutableStringTable(Line.Table))
			{
				SetSource(Table, Line.Key, Line.Text);
			}
		}
	}
}

#endif
