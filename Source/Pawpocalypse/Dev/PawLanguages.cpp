// The packaged language check (docs/09 L5; phase 7, docs/WORKLIST-LOC-2026-11-18.md §3 step 3): `paw.Languages [start=<code>]`,
// development builds only. tools/packaged_languages.py runs it in the packaged build, on the title, twice: once with
// -culture=<code> and start=<code> (the start check alone: a -culture= stands in for the setting, so it would hold every
// pick), once without (every language picked).
//
// The editor's tests prove the translations with the editor's files: every culture's ICU data, the loose translation files
// beside the project. A package has only what was staged. In the running game this checks:
//   - start=<code or culture> (alone): the language the game applied at start is that one (a -culture= or a saved setting; the engine itself
//     refuses a culture with no compiled .locres, KB U/20 §3), and the engine shows it;
//   - the title's language list offers every language that has a translation file (a language with a line missing or left
//     out drops out of the list: complete only, as Godot);
//   - every language offered, picked in that list as a player picks it (UPawMainMenu::PressLanguage, the setting's own path):
//     complete (no line missing, none left out), current in the engine (ICU has its culture: the staged
//     InternationalizationPreset), every line of the three string tables showing its translation, every character of them
//     in a face of F_Paw that has it (the face the composite picks for it, no fallback: no empty box), and in the languages
//     written without spaces a line break inside a run of letters (ICU's break data: Thai's needs its dictionary);
//   - a screenshot of the title with the list open in each language (Saved/PawLanguages/<culture>.png).
// Logs PAW_LANG lines; the verdict "PAW_LANG PASS" or "PAW_LANG FAIL <reasons>"; writes Saved/PawLanguages/report.txt; quits.
// Run with -NoSave: the picks are never written anywhere.
//   Pawpocalypse.exe /Game/Pawpocalypse/Maps/L_Title -NoSave -windowed -ResX=1280 -ResY=800 -culture=ru -ExecCmds="paw.Languages start=ru"
//   Pawpocalypse.exe /Game/Pawpocalypse/Maps/L_Title -NoSave -windowed -ResX=1280 -ResY=800 -ExecCmds="paw.Languages"

#include "Containers/StringConv.h"
#include "Containers/Ticker.h"
#include "Core/PawLog.h"
#include "Core/PawPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Fonts/FontCache.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "Internationalization/BreakIterator.h"
#include "Internationalization/Culture.h"
#include "Internationalization/IBreakIterator.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PcLocales.h"
#include "Rendering/SlateRenderer.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawMainMenu.h"
#include "UI/PawText.h"
#include "UI/PawWidgets.h"
#include "UnrealClient.h"

#if !UE_BUILD_SHIPPING

namespace
{
	constexpr int32 SettleFrames = 60;  // the title up and drawn before the first look
	constexpr int32 DrawnFrames = 20;   // a pick drawn (and its faces loaded) before the glyphs and the shot
	constexpr int32 ShotFrames = 15;    // a screenshot landed before the next pick
	constexpr int32 GlyphTries = 4;     // a face still loading answers "no glyph": asked again, DrawnFrames apart

	// Written without spaces: a line wraps only where ICU's break rules (and, for Thai, its dictionary) find a break.
	bool NoSpaces(const FString& Culture)
	{
		return Culture == TEXT("th") || Culture == TEXT("ja") || Culture == TEXT("zh-Hans") || Culture == TEXT("zh-Hant");
	}

	bool ScriptLetter(const UTF32CHAR C)
	{
		return (C >= 0x0E01 && C <= 0x0E4E)                         // Thai letters and marks
			|| (C >= 0x3041 && C <= 0x30FF && C != 0x30FB)            // kana (not the middle dot)
			|| (C >= 0x3400 && C <= 0x4DBF) || (C >= 0x4E00 && C <= 0x9FFF); // Han
	}

	// Characters that draw nothing: no glyph needed.
	bool Invisible(const UTF32CHAR C)
	{
		return C <= 0x20 || (C >= 0x200B && C <= 0x200D) || C == 0x2060 || C == 0xFEFF;
	}

	TArray<UTF32CHAR> Codepoints(const FString& S)
	{
		TArray<UTF32CHAR> Out;
		for (int32 I = 0; I < S.Len(); ++I)
		{
			UTF32CHAR C = static_cast<uint16>(S[I]);
			if (StringConv::IsHighSurrogate(C) && I + 1 < S.Len() && StringConv::IsLowSurrogate(static_cast<uint16>(S[I + 1])))
			{
				C = StringConv::EncodeSurrogate(static_cast<uint16>(S[I]), static_cast<uint16>(S[I + 1]));
				++I;
			}
			Out.Add(C);
		}
		return Out;
	}

	FString Hex(const UTF32CHAR C)
	{
		return FString::Printf(TEXT("U+%04X"), static_cast<uint32>(C));
	}

	FString Short(const FString& S)
	{
		FString One = S.Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT(""));
		return One.Len() > 60 ? One.Left(57) + TEXT("...") : One;
	}

	struct FLook
	{
		FString Code, Culture;
		bool bOffered = false;
		PawText::FTranslationCount Count;
		FString Engine;
		int32 Lines = 0, Wrong = 0, Breaks = -1;
		TArray<FString> WrongSamples;
		TSet<UTF32CHAR> Chars;
		TArray<UTF32CHAR> NoGlyph;
		FString Longest;
		FString Shot;
	};

	struct FCheck
	{
		FString Start;                 // start=: the language the game must have applied at start
		int32 Wait = SettleFrames;
		int32 Next = 0;                // the next language of Pc::Languages() to pick
		int32 GlyphTry = 0;
		int32 QuitIn = -1;
		enum class EStage : uint8 { Settle, StartShot, Pick, Drawn, Shot, Done } Stage = EStage::Settle;
		FLook Current;
		TArray<FString> Offered;       // the title's list, as the player sees it
		TArray<FString> Out, Fail;
		FTSTicker::FDelegateHandle Handle;

		static UPawMainMenu* Menu()
		{
			UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
			APawPlayerController* PC = World ? Cast<APawPlayerController>(World->GetFirstPlayerController()) : nullptr;
			return PC ? PC->GetMainMenu() : nullptr;
		}

		static UPawSettingsSubsystem* Settings()
		{
			UGameInstance* GI = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetGameInstance() : nullptr;
			return GI ? GI->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
		}

		void Say(const FString& Line)
		{
			Out.Add(Line);
			UE_LOG(LogPaw, Display, TEXT("PAW_LANG %s"), *Line);
		}

		// Every line of the three tables as the game shows it now, against the translation in use for the culture (English
		// where none is: a line with no words, or English itself).
		void ReadLines(FLook& L)
		{
			const bool bEnglish = L.Culture == TEXT("en");
			for (const FName Table : { PawText::TableUi, PawText::TableData, PawText::TableTree })
			{
				const FStringTableConstPtr Loaded = FStringTableRegistry::Get().FindStringTable(Table);
				if (!Loaded.IsValid())
				{
					L.WrongSamples.Add(Table.ToString() + TEXT(" not loaded"));
					++L.Wrong;
					continue;
				}
				Loaded->EnumerateSourceStrings([this, &L, Table, bEnglish](const FString& Key, const FString& English)
				{
					const FString Translated = bEnglish ? FString() : PawText::TranslationIn(Table, Key, L.Culture);
					const FString Want = Translated.IsEmpty() ? English : Translated;
					const FString Shown = FText::FromStringTable(Table, Key).ToString();
					++L.Lines;
					if (Shown != Want)
					{
						++L.Wrong;
						if (L.WrongSamples.Num() < 3)
						{
							L.WrongSamples.Add(FString::Printf(TEXT("%s %s shows \"%s\", not \"%s\""), *Table.ToString(), *Key, *Short(Shown), *Short(Want)));
						}
					}
					for (const UTF32CHAR C : Codepoints(Shown))
					{
						if (!Invisible(C))
						{
							L.Chars.Add(C);
						}
					}
					if (Codepoints(Shown).Num() > Codepoints(L.Longest).Num() && !Translated.IsEmpty())
					{
						L.Longest = Shown;
					}
					return true;
				});
			}
		}

		// The face F_Paw picks for each character (its ranges, the culture's sub-font, else the default typeface) must have it:
		// Slate would otherwise draw it from a fallback font or as an empty box. Both weights the UI uses (Regular = Bold).
		void ReadGlyphs(FLook& L)
		{
			L.NoGlyph.Reset();
			UFont* Font = PawUi::PawFont();
			if (!Font || !FSlateApplication::IsInitialized())
			{
				L.NoGlyph.Add(0);
				return;
			}
			const TSharedRef<FSlateFontCache> Cache = FSlateApplication::Get().GetRenderer()->GetFontCache();
			for (const TCHAR* Typeface : { TEXT("Regular"), TEXT("Black") })
			{
				const FSlateFontInfo Info(Font, 16, FName(Typeface));
				for (const UTF32CHAR C : L.Chars)
				{
					float Scale = 1.0f;
					const FFontData& Data = Cache->GetFontDataForCodepoint(Info, C, Scale);
					if (!Cache->CanLoadCodepoint(Data, C, EFontFallback::FF_NoFallback))
					{
						L.NoGlyph.AddUnique(C);
					}
				}
			}
			L.NoGlyph.Sort();
		}

		// Where a line may wrap inside a run of letters (the longest translated line).
		void ReadBreaks(FLook& L)
		{
			if (!NoSpaces(L.Culture))
			{
				return;
			}
			L.Breaks = 0;
			const TSharedRef<IBreakIterator> It = FBreakIterator::CreateLineBreakIterator();
			It->SetString(L.Longest);
			for (int32 At = It->MoveToNext(); At != INDEX_NONE && At < L.Longest.Len(); At = It->MoveToNext())
			{
				if (At > 0 && ScriptLetter(static_cast<uint16>(L.Longest[At - 1])) && ScriptLetter(static_cast<uint16>(L.Longest[At])))
				{
					++L.Breaks;
				}
			}
		}

		void Begin(const FString& Code)
		{
			Current = FLook();
			Current.Code = Code;
			const Pc::PcLanguage* Lang = Pc::LanguageByCode(TCHAR_TO_UTF8(*Code));
			Current.Culture = Lang ? UTF8_TO_TCHAR(Lang->Culture) : Code;
			Current.bOffered = Offered.Contains(Code);
			Current.Count = PawText::TranslationCount(Current.Culture);
		}

		// What the game shows right after the pick (the words follow at once); the glyphs and the shot wait for the drawing.
		void ReadShown()
		{
			Current.Engine = FInternationalization::Get().GetCurrentLanguage()->GetName();
			ReadLines(Current);
			ReadBreaks(Current);
		}

		void Report(const TCHAR* What)
		{
			FLook& L = Current;
			TArray<FString> Why;
			if (!L.bOffered)
			{
				Why.Add(TEXT("not offered"));
			}
			if (L.Count.Missing > 0 || L.Count.Left > 0)
			{
				Why.Add(FString::Printf(TEXT("%d missing, %d left out"), L.Count.Missing, L.Count.Left));
			}
			if (L.bOffered && L.Engine != L.Culture)
			{
				Why.Add(FString::Printf(TEXT("the engine shows '%s'"), *L.Engine));
			}
			if (L.Wrong > 0)
			{
				Why.Add(FString::Printf(TEXT("%d lines not in it"), L.Wrong));
			}
			if (L.NoGlyph.Num() > 0)
			{
				TArray<FString> Codes;
				for (int32 I = 0; I < FMath::Min(L.NoGlyph.Num(), 12); ++I)
				{
					Codes.Add(Hex(L.NoGlyph[I]));
				}
				Why.Add(FString::Printf(TEXT("%d characters with no glyph (%s)"), L.NoGlyph.Num(), *FString::Join(Codes, TEXT(" "))));
			}
			if (L.Breaks == 0)
			{
				Why.Add(TEXT("no line break inside its words"));
			}
			Say(FString::Printf(TEXT("%s %s culture=%s offered=%d lines=%d missing=%d left=%d engine=%s wrong=%d chars=%d noglyph=%d breaks=%s shot=%s %s"),
				What, *L.Code, *L.Culture, L.bOffered ? 1 : 0, L.Lines, L.Count.Missing, L.Count.Left, *L.Engine, L.Wrong, L.Chars.Num(),
				L.NoGlyph.Num(), L.Breaks < 0 ? TEXT("-") : *FString::FromInt(L.Breaks), L.Shot.IsEmpty() ? TEXT("-") : *FPaths::GetCleanFilename(L.Shot),
				Why.Num() ? *(TEXT("FAIL ") + FString::Join(Why, TEXT("; "))) : TEXT("ok")));
			for (const FString& Sample : L.WrongSamples)
			{
				Say(TEXT("  ") + Sample);
			}
			if (Why.Num())
			{
				Fail.Add(FString::Printf(TEXT("%s %s: %s"), What, *L.Code, *FString::Join(Why, TEXT("; "))));
			}
		}

		void Finish()
		{
			Say(Fail.Num() ? TEXT("FAIL ") + FString::Join(Fail, TEXT(" | ")) : FString(TEXT("PASS")));
			FFileHelper::SaveStringArrayToFile(Out, *(FPaths::ProjectSavedDir() / TEXT("PawLanguages/report.txt")), FFileHelper::EEncodingOptions::ForceUTF8);
			Stage = EStage::Done;
			QuitIn = 30;
		}

		bool Tick(float)
		{
			if (QuitIn >= 0)
			{
				if (QuitIn-- == 0)
				{
					FPlatformMisc::RequestExit(false, TEXT("paw.Languages"));
				}
				return true;
			}
			if (Wait > 0)
			{
				--Wait;
				return true;
			}
			UPawMainMenu* M = Menu();
			UPawSettingsSubsystem* S = Settings();
			if (!M || !S)
			{
				if (Stage == EStage::Settle)
				{
					return true; // the title is still loading
				}
				Fail.Add(TEXT("the title went away"));
				Finish();
				return true;
			}
			switch (Stage)
			{
			case EStage::Settle:
			{
				// The language the game started in, before any pick.
				const FPawLanguageApplied& Applied = S->GetLanguageApplied();
				TArray<FString> IcuCultures;
				FInternationalization::Get().GetCultureNames(IcuCultures);
				Say(FString::Printf(TEXT("ICU knows %d cultures; translations loaded for %d; %d lines need words"), IcuCultures.Num(),
					PawText::LoadedCultures().Num(), PawText::LinesNeedingWords()));
				Offered = PawText::Languages(false);
				Begin(Applied.Code);
				Current.bOffered = true; // the start language need not be in the list (English when the asked one is not)
				ReadShown();
				ReadGlyphs(Current);
				if (!Start.IsEmpty())
				{
					// The start check alone (a -culture= stands in for the setting, so every pick would show it): shot, report, quit.
					if (Applied.Code != Start && Applied.Culture != Start)
					{
						Fail.Add(FString::Printf(TEXT("start: the game applied '%s' (%s), not '%s'"), *Applied.Code, *Applied.Culture, *Start));
					}
					Current.Shot = FPaths::ProjectSavedDir() / TEXT("PawLanguages") / TEXT("start_") + Current.Culture + TEXT(".png");
					FScreenshotRequest::RequestScreenshot(Current.Shot, true, false);
					Wait = ShotFrames;
					Stage = EStage::StartShot;
					return true;
				}
				Report(TEXT("start"));
				// The list as the player opens it.
				M->PressCluster(2);
				Offered = M->GetLanguageCodes();
				Say(TEXT("offered ") + FString::Join(Offered, TEXT(",")));
				for (const FString& Culture : PawText::LoadedCultures())
				{
					const Pc::PcLanguage* Lang = Pc::LanguageByCulture(TCHAR_TO_UTF8(*Culture));
					if (!Lang || !Offered.Contains(UTF8_TO_TCHAR(Lang->Code)))
					{
						Fail.Add(FString::Printf(TEXT("%s has translations and is not offered"), *Culture));
					}
				}
				Stage = EStage::Pick;
				return true;
			}
			case EStage::StartShot:
				Report(TEXT("start"));
				Finish();
				return true;
			case EStage::Pick:
			{
				const std::vector<Pc::PcLanguage>& All = Pc::Languages();
				// Languages the game knows but has no translation file for are not looked at (none today).
				while (Next < static_cast<int32>(All.size()) && FString(UTF8_TO_TCHAR(All[Next].Code)) != TEXT("en")
					&& !PawText::LoadedCultures().Contains(UTF8_TO_TCHAR(All[Next].Culture)))
				{
					++Next;
				}
				if (Next >= static_cast<int32>(All.size()))
				{
					Finish();
					return true;
				}
				const FString Code = UTF8_TO_TCHAR(All[Next++].Code);
				Begin(Code);
				if (!Current.bOffered)
				{
					Report(TEXT("lang"));
					return true;
				}
				// The player's pick: the row's press (the setting, the culture, the list closed), then the list opened again.
				M->PressLanguage(Code);
				ReadShown();
				if (!M->IsLayerShown(UPawMainMenu::ELayer::Language))
				{
					M->PressCluster(2);
				}
				GlyphTry = 0;
				Wait = DrawnFrames;
				Stage = EStage::Drawn;
				return true;
			}
			case EStage::Drawn:
			{
				ReadGlyphs(Current);
				if (Current.NoGlyph.Num() > 0 && ++GlyphTry < GlyphTries)
				{
					Wait = DrawnFrames; // a face may still be loading
					return true;
				}
				Current.Shot = FPaths::ProjectSavedDir() / TEXT("PawLanguages") / Current.Culture + TEXT(".png");
				FScreenshotRequest::RequestScreenshot(Current.Shot, true, false);
				Wait = ShotFrames;
				Stage = EStage::Shot;
				return true;
			}
			case EStage::Shot:
				Report(TEXT("lang"));
				Stage = EStage::Pick;
				return true;
			case EStage::Done:
				return true;
			}
			return true;
		}
	};

	TUniquePtr<FCheck> GCheck;

	FAutoConsoleCommand LanguagesCommand(TEXT("paw.Languages"),
		TEXT("paw.Languages [start=<code>]: every language picked on the title and checked as shown (lines, glyphs, breaks); report to Saved/PawLanguages, quit."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			if (GCheck)
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Languages: already running"));
				return;
			}
			GCheck = MakeUnique<FCheck>();
			for (const FString& Arg : Args)
			{
				FString Value;
				if (Arg.Split(TEXT("="), nullptr, &Value) && Arg.StartsWith(TEXT("start=")))
				{
					GCheck->Start = Value;
				}
			}
			GCheck->Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float Dt) { return GCheck->Tick(Dt); }));
		}));
}

#endif
