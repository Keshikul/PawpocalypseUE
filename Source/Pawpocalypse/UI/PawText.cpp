#include "UI/PawText.h"

#include "Core/PawLog.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"
#include "GeneralProjectSettings.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/PolyglotTextData.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"
#include "PcFormat.h"
#include "PcHud.h"
#include "PcLocales.h"
#include "PcTitle.h"

#include <set>
#include "PcTreeText.h"

namespace PawText
{
	const FName TableUi(TEXT("ST_UI"));
	const FName TableData(TEXT("ST_Data"));
	const FName TableTree(TEXT("ST_Tree"));

	namespace
	{
		const TCHAR* Folder = TEXT("Pawpocalypse/Localization/Strings/");

		void RegisterOne(const FName Id)
		{
			if (FStringTableRegistry::Get().FindStringTable(Id).IsValid())
			{
				return;
			}
			const FString File = FString(Folder) + Id.ToString() + TEXT(".csv");
			// LOCTABLE_FROMFILE_GAME: the path is relative to the project's Content folder (staged loose: DefaultGame.ini).
			FStringTableRegistry::Get().Internal_LocTableFromFile(Id, Id.ToString(), File, FPaths::ProjectContentDir());
			if (!FStringTableRegistry::Get().FindStringTable(Id).IsValid())
			{
				UE_LOG(LogPaw, Error, TEXT("String table %s did not load from %s%s"), *Id.ToString(), *FPaths::ProjectContentDir(), *File);
			}
		}
	}

	void RegisterTables()
	{
		RegisterOne(TableUi);
		RegisterOne(TableData);
		RegisterOne(TableTree);
		if (FParse::Param(FCommandLine::Get(), TEXT("PseudoLoc")) && !IsPseudoLocale())
		{
			SetPseudoLocale(true);
		}
	}

	// --- The pseudo-locale ------------------------------------------------------------------------------------------------

	namespace
	{
		// The real English of every entry while the pseudo-locale is on (table -> key -> English).
		TMap<FName, TMap<FString, FString>>& RealEnglish()
		{
			static TMap<FName, TMap<FString, FString>> Held;
			return Held;
		}

		void SetEnglish(const FName Table, const FString& Key, const FString& Source)
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

		TCHAR Accented(TCHAR C)
		{
			switch (C)
			{
			case TEXT('a'): return TEXT('å'); case TEXT('e'): return TEXT('ë'); case TEXT('i'): return TEXT('ï');
			case TEXT('o'): return TEXT('ö'); case TEXT('u'): return TEXT('ü'); case TEXT('y'): return TEXT('ÿ');
			case TEXT('c'): return TEXT('ç'); case TEXT('n'): return TEXT('ñ');
			case TEXT('A'): return TEXT('Å'); case TEXT('E'): return TEXT('Ë'); case TEXT('I'): return TEXT('Ï');
			case TEXT('O'): return TEXT('Ö'); case TEXT('U'): return TEXT('Ü'); case TEXT('Y'): return TEXT('Ý');
			case TEXT('C'): return TEXT('Ç'); case TEXT('N'): return TEXT('Ñ');
			default: return C;
			}
		}
	}

	FString Pseudo(const FString& English)
	{
		if (!NeedsWords(English))
		{
			return English;
		}
		FString Out = TEXT("[");
		int32 Letters = 0;
		for (int32 I = 0; I < English.Len(); ++I)
		{
			const TCHAR C = English[I];
			if (C == TEXT('{'))
			{
				// A {placeholder} goes through as it is (FText::Format fills it).
				int32 J = I + 1;
				while (J < English.Len() && (FChar::IsAlnum(English[J]) || English[J] == TEXT('_')))
				{
					++J;
				}
				if (J < English.Len() && English[J] == TEXT('}') && J > I + 1)
				{
					Out += English.Mid(I, J - I + 1);
					I = J;
					continue;
				}
			}
			Letters += FChar::IsAlpha(C) ? 1 : 0;
			Out.AppendChar(Accented(C));
		}
		// About 40 % more: the longest real languages run 30-40 % over the English in a UI line. In words of up to 6 letters,
		// as a language's words are: one long padding word could never wrap (the language pass: a 28-letter one did).
		for (int32 P = 0; P < (Letters * 2 + 4) / 5; ++P)
		{
			if (P % 6 == 0)
			{
				Out += TEXT(" ");
			}
			Out.AppendChar((P & 1) ? TEXT('ñ') : TEXT('ë'));
		}
		Out += TEXT("]");
		return Out;
	}

	bool IsPseudoLocale()
	{
		return RealEnglish().Num() > 0;
	}

	void SetPseudoLocale(bool bOn)
	{
		if (bOn == IsPseudoLocale())
		{
			return;
		}
		for (const FName Table : { TableUi, TableData, TableTree })
		{
			const FStringTableConstPtr Loaded = FStringTableRegistry::Get().FindStringTable(Table);
			if (!Loaded.IsValid())
			{
				continue;
			}
			if (bOn)
			{
				TMap<FString, FString>& Held = RealEnglish().FindOrAdd(Table);
				Loaded->EnumerateSourceStrings([&Held](const FString& Key, const FString& English)
				{
					Held.Add(Key, English);
					return true;
				});
				// Names never translated stay as they are (the logo, Boba, Discord, the pad brands: their comments say so).
				static const TSet<FString> Names = { TEXT("TITLE_name"), TEXT("TITLE_boba_name"), TEXT("TITLE_discord"),
					TEXT("SETTINGS_prompts_xbox"), TEXT("SETTINGS_prompts_playstation"), TEXT("SETTINGS_prompts_steam_deck") };
				for (const TPair<FString, FString>& Line : Held)
				{
					if (!Names.Contains(Line.Key))
					{
						SetEnglish(Table, Line.Key, Pseudo(Line.Value));
					}
				}
			}
			else if (const TMap<FString, FString>* Held = RealEnglish().Find(Table))
			{
				for (const TPair<FString, FString>& Line : *Held)
				{
					SetEnglish(Table, Line.Key, Line.Value);
				}
			}
		}
		if (!bOn)
		{
			RealEnglish().Empty();
		}
		// The translations again: with the English pseudo, no row matches its Source (none is shown), so English shows the
		// pseudo text; with the English back, every row is in use again.
		RegisterTranslations();
		UE_LOG(LogPaw, Log, TEXT("Pseudo-locale %s"), bOn ? TEXT("on") : TEXT("off"));
	}

	FText Ui(const TCHAR* Key)
	{
		return FText::FromStringTable(TableUi, Key);
	}

	FText Ui(const TCHAR* Key, const FFormatNamedArguments& Args)
	{
		return FText::Format(Ui(Key), Args);
	}

	FText Data(const TCHAR* Prefix, const std::string& Id)
	{
		return FText::FromStringTable(TableData, FString::Printf(TEXT("%s_%s_name"), Prefix, UTF8_TO_TCHAR(Id.c_str())));
	}

	FText DataField(const TCHAR* Prefix, const std::string& Id, const TCHAR* Field)
	{
		return FText::FromStringTable(TableData, FString::Printf(TEXT("%s_%s_%s"), Prefix, UTF8_TO_TCHAR(Id.c_str()), Field));
	}

	FText Verbatim(const FString& Built)
	{
		return FText::AsCultureInvariant(Built);
	}

	FText TreeLabel(const std::string& Id)
	{
		return FText::FromStringTable(TableTree, FString::Printf(TEXT("TREE_%s_label"), UTF8_TO_TCHAR(Id.c_str())));
	}

	FText TreeBlurb(const std::string& Id)
	{
		return FText::FromStringTable(TableTree, FString::Printf(TEXT("TREE_%s_blurb"), UTF8_TO_TCHAR(Id.c_str())));
	}

	FText TreeEffect(const std::vector<Pc::PcTreePhrase>& Parts)
	{
		TArray<FText> Texts;
		for (const Pc::PcTreePhrase& P : Parts)
		{
			const FText Phrase = FText::FromStringTable(TableTree, UTF8_TO_TCHAR(P.Key.c_str()));
			if (P.Arg.empty())
			{
				Texts.Add(Phrase);
				continue;
			}
			// The number as GDScript printed it: culture-invariant, as Godot's (phase 7 decides the separators).
			FFormatNamedArguments Args;
			Args.Add(UTF8_TO_TCHAR(P.Arg.c_str()), FText::AsCultureInvariant(FString(UTF8_TO_TCHAR(P.Number.c_str()))));
			Texts.Add(FText::Format(Phrase, Args));
		}
		return FText::Join(Ui(TEXT("TREE_list_sep")), Texts);
	}

	FText Amount(double Value)
	{
		return FText::AsCultureInvariant(FString(UTF8_TO_TCHAR(Pc::ShortAmount(Value).c_str())));
	}

	FText CardNumber(double Value)
	{
		return FText::AsCultureInvariant(FString(UTF8_TO_TCHAR(Pc::CardNumber(Value).c_str())));
	}

	FText Count(int64 Value)
	{
		return FText::AsCultureInvariant(FString::Printf(TEXT("%lld"), static_cast<long long>(Value)));
	}

	FText TwoDigits(int64 Value)
	{
		return FText::AsCultureInvariant(FString::Printf(TEXT("%02lld"), static_cast<long long>(Value)));
	}

	FText Fixed(double Value, int32 Decimals)
	{
		return FText::AsCultureInvariant(FString(UTF8_TO_TCHAR(Pc::FixedLikeGodot(Value, FMath::Max(0, Decimals)).c_str())));
	}

	FText SlotClock(double Seconds)
	{
		const Pc::PcClock C = Pc::SlotClock(Seconds);
		FFormatNamedArguments Args;
		switch (C.Form)
		{
		case Pc::PcClockForm::HoursMinutes:
			Args.Add(TEXT("h"), Count(C.H));
			Args.Add(TEXT("m"), TwoDigits(C.M));
			return Ui(TEXT("TITLE_clock_hm"), Args);
		case Pc::PcClockForm::MinutesSeconds:
			Args.Add(TEXT("m"), Count(C.M));
			Args.Add(TEXT("s"), TwoDigits(C.S));
			return Ui(TEXT("TITLE_clock_ms"), Args);
		default:
			Args.Add(TEXT("s"), Count(C.S));
			return Ui(TEXT("TITLE_clock_s"), Args);
		}
	}

	FText Mice(int64 Kills)
	{
		const Pc::PcKills K = Pc::KillsText(Kills);
		FFormatNamedArguments Args;
		Args.Add(TEXT("n"), FText::AsCultureInvariant(FString(UTF8_TO_TCHAR(K.Number.c_str()))));
		return Ui(K.Form == Pc::PcKillsForm::Millions ? TEXT("TITLE_mice_m")
			: K.Form == Pc::PcKillsForm::Thousands ? TEXT("TITLE_mice_k") : TEXT("TITLE_mice"), Args);
	}

	FText When(double Unix, double Now)
	{
		const Pc::PcWhen W = Pc::WhenSaved(Unix, Now);
		FFormatNamedArguments Args;
		Args.Add(TEXT("n"), Count(W.N));
		switch (W.Form)
		{
		case Pc::PcWhenForm::Unknown: return Ui(TEXT("TITLE_when_unknown"));
		case Pc::PcWhenForm::JustNow: return Ui(TEXT("TITLE_when_now"));
		case Pc::PcWhenForm::Minutes: return Ui(TEXT("TITLE_when_min"), Args);
		case Pc::PcWhenForm::Hours: return Ui(TEXT("TITLE_when_hr"), Args);
		case Pc::PcWhenForm::Days: return Ui(TEXT("TITLE_when_days"), Args);
		default:
			return FText::AsCultureInvariant(FString::Printf(TEXT("%04lld-%02lld-%02lld"), static_cast<long long>(W.Year),
				static_cast<long long>(W.Month), static_cast<long long>(W.Day)));
		}
	}

	FText Version()
	{
		return FText::AsCultureInvariant(TEXT("v") + GetDefault<UGeneralProjectSettings>()->ProjectVersion);
	}

	FText Duration(double Seconds)
	{
		const Pc::PcDuration T = Pc::DurationParts(Seconds);
		FFormatNamedArguments Args;
		Args.Add(TEXT("s"), TwoDigits(T.S));
		if (T.bHours)
		{
			Args.Add(TEXT("h"), Count(T.H));
			Args.Add(TEXT("m"), TwoDigits(T.M));
			return Ui(TEXT("DURATION_hms"), Args);
		}
		Args.Add(TEXT("m"), Count(T.M));
		return Ui(TEXT("DURATION_ms"), Args);
	}

	bool HasUi(const TCHAR* Key)
	{
		const FStringTableConstPtr Table = FStringTableRegistry::Get().FindStringTable(TableUi);
		FString Out;
		return Table.IsValid() && Table->GetSourceString(FTextKey(Key), Out);
	}

	// --- The translations ----------------------------------------------------------------------------------------------

	namespace
	{
		const TCHAR* TranslationsFolder = TEXT("Pawpocalypse/Localization/Translations");

		struct FTranslations
		{
			TArray<FPolyglotTextData> Polyglot;          // one per table entry that has a translation in use
			TMap<FString, TMap<FString, FString>> Lines; // culture -> "Table|Key" -> text
			TMap<FString, FTranslationCount> Counts;     // culture -> lines in use, left out, missing
			TSet<FString> Cultures;                      // with at least one line in use
			int32 NeedWords = 0;                         // the tables' lines that need words
		};

		bool bUnfinishedForTest = false;

		// The {Name} placeholders of a line, sorted (tools/loc_status.py TOKEN: a letter or '_', then letters, digits, '_').
		TArray<FString> Placeholders(const FString& Line)
		{
			TArray<FString> Out;
			for (int32 I = 0; I < Line.Len(); ++I)
			{
				if (Line[I] != TEXT('{') || I + 1 >= Line.Len() || !(FChar::IsAlpha(Line[I + 1]) || Line[I + 1] == TEXT('_')))
				{
					continue;
				}
				int32 J = I + 1;
				while (J < Line.Len() && (FChar::IsAlnum(Line[J]) || Line[J] == TEXT('_')))
				{
					++J;
				}
				if (J < Line.Len() && Line[J] == TEXT('}'))
				{
					Out.Add(Line.Mid(I, J - I + 1));
					I = J;
				}
			}
			Out.Sort();
			return Out;
		}

		FTranslations& Held()
		{
			static FTranslations Translations;
			return Translations;
		}

		FString LineId(const FName Table, const FString& Key)
		{
			return Table.ToString() + TEXT("|") + Key;
		}

		// One culture's file for one table, read as the engine reads a string table (FCsvParser, then each cell unescaped:
		// FStringTable::ImportStringsFromCSVString), so Source compares with the table's English exactly.
		void ReadTable(FTranslations& Out, TMap<FString, int32>& Index, const FString& Culture, const FName Table, const FString& Path)
		{
			FString Csv;
			if (!FFileHelper::LoadFileToString(Csv, *Path))
			{
				return;
			}
			const FStringTableConstPtr Loaded = FStringTableRegistry::Get().FindStringTable(Table);
			if (!Loaded.IsValid())
			{
				return;
			}
			const FCsvParser Parser(Csv);
			const FCsvParser::FRows& Rows = Parser.GetRows();
			if (Rows.Num() == 0)
			{
				return;
			}
			int32 KeyAt = INDEX_NONE, TextAt = INDEX_NONE, SourceAt = INDEX_NONE;
			for (int32 C = 0; C < Rows[0].Num(); ++C)
			{
				const FString Head = Rows[0][C];
				KeyAt = Head == TEXT("Key") ? C : KeyAt;
				TextAt = Head == TEXT("Text") ? C : TextAt;
				SourceAt = Head == TEXT("Source") ? C : SourceAt;
			}
			if (KeyAt == INDEX_NONE || TextAt == INDEX_NONE || SourceAt == INDEX_NONE)
			{
				UE_LOG(LogPaw, Error, TEXT("Translations: %s has no Key, Text and Source columns"), *Path);
				return;
			}
			FTranslationCount& Count = Out.Counts.FindOrAdd(Culture);
			for (int32 R = 1; R < Rows.Num(); ++R)
			{
				const TArray<const TCHAR*>& Cells = Rows[R];
				if (!Cells.IsValidIndex(KeyAt) || !Cells.IsValidIndex(TextAt) || !Cells.IsValidIndex(SourceAt))
				{
					++Count.Left;
					continue;
				}
				const FString Key = FString(Cells[KeyAt]).ReplaceEscapedCharWithChar();
				const FString Text = FString(Cells[TextAt]).ReplaceEscapedCharWithChar();
				const FString Source = FString(Cells[SourceAt]).ReplaceEscapedCharWithChar();
				FString English;
				// Left out: no words, no such line, its English changed since (stale), or its placeholders are not the
				// English's (a broken format would show raw).
				if (Text.IsEmpty() || !Loaded->GetSourceString(Key, English) || English != Source || Placeholders(Text) != Placeholders(English))
				{
					++Count.Left;
					continue;
				}
				const FString Id = LineId(Table, Key);
				const int32* At = Index.Find(Id);
				const int32 Slot = At ? *At : Index.Add(Id, Out.Polyglot.Emplace(ELocalizedTextSourceCategory::Game, Table.ToString(), Key,
					English, FString(TEXT("en"))));
				Out.Polyglot[Slot].AddLocalizedString(Culture, Text);
				Out.Lines.FindOrAdd(Culture).Add(Id, Text);
				Out.Cultures.Add(Culture);
				++Count.Lines;
			}
		}
	}

	bool NeedsWords(const FString& English)
	{
		// tools/loc_status.py needs(): letters once the {placeholders} and the multipliers (x5, x10) are gone.
		FString Bare;
		const TArray<FString> Marks = Placeholders(English);
		Bare = English;
		for (const FString& Mark : Marks)
		{
			Bare.ReplaceInline(*Mark, TEXT(" "), ESearchCase::CaseSensitive);
		}
		for (int32 I = 0; I < Bare.Len(); ++I)
		{
			const bool bWordStart = I == 0 || !(FChar::IsAlnum(Bare[I - 1]) || Bare[I - 1] == TEXT('_'));
			if (Bare[I] == TEXT('x') && bWordStart && I + 1 < Bare.Len() && FChar::IsDigit(Bare[I + 1]))
			{
				int32 J = I + 1;
				while (J < Bare.Len() && FChar::IsDigit(Bare[J]))
				{
					++J;
				}
				if (J == Bare.Len() || !(FChar::IsAlnum(Bare[J]) || Bare[J] == TEXT('_')))
				{
					I = J - 1;
					continue;
				}
			}
			if (FChar::IsAlpha(Bare[I]))
			{
				return true;
			}
		}
		return false;
	}

	void RegisterTranslations(const FString& Root)
	{
		FTranslations Fresh;
		TMap<FString, int32> Index;
		const FString Base = Root.IsEmpty() ? FPaths::ProjectContentDir() / TranslationsFolder : Root;
		TArray<FString> Cultures;
		IFileManager::Get().FindFiles(Cultures, *(Base / TEXT("*")), false, true);
		Cultures.Sort();
		for (const FString& Culture : Cultures)
		{
			for (const FName Table : { TableUi, TableData, TableTree })
			{
				ReadTable(Fresh, Index, Culture, Table, Base / Culture / (Table.ToString() + TEXT(".csv")));
			}
		}
		// Missing: every line that needs words with no translation in use. A language with none missing is complete.
		for (const FName Table : { TableUi, TableData, TableTree })
		{
			const FStringTableConstPtr Loaded = FStringTableRegistry::Get().FindStringTable(Table);
			if (!Loaded.IsValid())
			{
				continue;
			}
			Loaded->EnumerateSourceStrings([&Fresh, Table](const FString& Key, const FString& English)
			{
				if (NeedsWords(English))
				{
					++Fresh.NeedWords;
					const FString Id = LineId(Table, Key);
					for (TPair<FString, FTranslationCount>& Count : Fresh.Counts)
					{
						const TMap<FString, FString>* Lines = Fresh.Lines.Find(Count.Key);
						Count.Value.Missing += Lines && Lines->Contains(Id) ? 0 : 1;
					}
				}
				return true;
			});
		}
		Held() = MoveTemp(Fresh);
		FTextLocalizationManager::Get().RegisterPolyglotTextData(Held().Polyglot);
		UE_LOG(LogPaw, Log, TEXT("Translations: %d cultures, %d table entries translated"), Held().Cultures.Num(), Held().Polyglot.Num());
	}

	const TSet<FString>& LoadedCultures()
	{
		return Held().Cultures;
	}

	int32 LinesNeedingWords()
	{
		return Held().NeedWords;
	}

	bool ShowUnfinishedLanguages()
	{
		return bUnfinishedForTest || FParse::Param(FCommandLine::Get(), TEXT("PawDraftLanguages"));
	}

	void SetShowUnfinishedLanguagesForTest(bool bShow)
	{
		bUnfinishedForTest = bShow;
	}

	FTranslationCount TranslationCount(const FString& Culture)
	{
		const FTranslationCount* Count = Held().Counts.Find(Culture);
		return Count ? *Count : FTranslationCount();
	}

	FString TranslationIn(const FName Table, const FString& Key, const FString& Culture)
	{
		const TMap<FString, FString>* Lines = Held().Lines.Find(Culture);
		const FString* Line = Lines ? Lines->Find(LineId(Table, Key)) : nullptr;
		return Line ? *Line : FString();
	}

	bool ShowCulture(const FString& Culture)
	{
#if WITH_EDITOR
		if (GIsEditor)
		{
			// The editor shows game text in English unless told otherwise. The game localisation preview cannot do it here:
			// it needs a native game culture, read from a compiled Game.locmeta, and this game has none (its translations are
			// these files), so EnableGameLocalizationPreview returns at once. With this switch on, polyglot game text follows
			// the current culture in the editor too (TextLocalizationManager.cpp; KB U/20 §3). Off again for English.
			if (IConsoleVariable* Force = IConsoleManager::Get().FindConsoleVariable(TEXT("Localization.ForceLoadGameLocalizationInEditor")))
			{
				Force->Set(Culture != TEXT("en"), ECVF_SetByCode);
			}
		}
#endif
		const bool bSet = FInternationalization::Get().SetCurrentCulture(Culture);
		// The culture's own loading may run on a task: done before the translations write their display strings.
		FTextLocalizationManager::Get().WaitForAsyncTasks();
		FTextLocalizationManager::Get().RegisterPolyglotTextData(Held().Polyglot);
		return bSet;
	}

	TArray<FString> Languages(bool bUnfinishedToo)
	{
		// Godot lists only loaded .po files, "so an unfinished language cannot be chosen by accident" (locales.gd), and its
		// files were complete (its loc gate): here, a language with no line missing; unfinished ones on request.
		std::set<std::string> Loaded;
		for (const FString& Culture : Held().Cultures)
		{
			const Pc::PcLanguage* L = Pc::LanguageByCulture(TCHAR_TO_UTF8(*Culture));
			if (L && (bUnfinishedToo || TranslationCount(Culture).Missing == 0))
			{
				Loaded.insert(L->Code);
			}
		}
		TArray<FString> Out;
		for (const std::string& Code : Pc::LocalesAvailable(Loaded))
		{
			Out.Add(UTF8_TO_TCHAR(Code.c_str()));
		}
		return Out;
	}

	FText LanguageLabel(const FString& Code)
	{
		const std::string Utf8Code = TCHAR_TO_UTF8(*Code);
		const Pc::PcLanguage* L = Pc::LanguageByCode(Utf8Code);
		FString Marker = L ? TranslationIn(TableUi, TEXT("SETTINGS_language_draft"), UTF8_TO_TCHAR(L->Culture)) : FString();
		if (Marker.IsEmpty())
		{
			Marker = Ui(TEXT("SETTINGS_language_draft")).ToString();
		}
		return Verbatim(UTF8_TO_TCHAR(Pc::LocalePickerLabel(Utf8Code, TCHAR_TO_UTF8(*Marker)).c_str()));
	}
}
