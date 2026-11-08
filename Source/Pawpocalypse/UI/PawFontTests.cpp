// Pawpocalypse.UI.Fonts.* and .Widgets.Fonts: the game's font F_Paw (restyle R2, 2026-11-07; tools/fonts.py is the table,
// UI/PawFontSpec.h its generated copy, tools/import_fonts.py the import, tools/check_font_coverage.py the glyph gate).

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Components/TextBlock.h"
#include "Engine/Blueprint.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Misc/AutomationTest.h"
#include "UI/PawFontSpec.h"
#include "UI/PawWidgets.h"

namespace
{
	FString FacePath(const UObject* Face)
	{
		return Face ? Face->GetPathName() : FString(TEXT("(none)"));
	}

	EFontLoadingPolicy Policy(const TCHAR* Name)
	{
		const FString N = Name;
		return N == TEXT("Inline") ? EFontLoadingPolicy::Inline : N == TEXT("Stream") ? EFontLoadingPolicy::Stream : EFontLoadingPolicy::LazyLoad;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawFontsCompositeTest, "Pawpocalypse.UI.Fonts.Composite", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// F_Paw is the table: a runtime font; the default typeface's names on their faces; each sub-font's cultures, ranges and face
// (sub-face index into the .ttc) under every typeface name; the fallback. Each face asset loads as the table says. UMG's
// default font is F_Paw, so a text widget the designer adds in the editor starts in it.
bool FPawFontsCompositeTest::RunTest(const FString& Parameters)
{
	UFont* Font = PawUi::PawFont();
	if (!TestNotNull(TEXT("F_Paw exists (py tools/import_fonts.py --run)"), Font))
	{
		return false;
	}
	TestTrue(TEXT("F_Paw is a runtime font"), Font->FontCacheType == EFontCacheType::Runtime);
	const FCompositeFont& C = Font->GetInternalCompositeFont();

	auto Same = [this](const FString& Where, const FTypefaceEntry& Have, const TCHAR* Name, const TCHAR* Face, int32 SubFace)
	{
		TestEqual(Where + TEXT(": name"), Have.Name.ToString(), FString(Name));
		TestEqual(Where + TEXT(": face"), FacePath(Have.Font.GetFontFaceAsset()), FString(Face));
		TestEqual(Where + TEXT(": sub-face"), Have.Font.GetSubFaceIndex(), SubFace);
	};

	if (TestEqual(TEXT("default typefaces"), C.DefaultTypeface.Fonts.Num(), PawFontSpec::Default.Num()))
	{
		for (int32 I = 0; I < PawFontSpec::Default.Num(); ++I)
		{
			const PawFontSpec::FTypeface& T = PawFontSpec::Default[I];
			Same(FString::Printf(TEXT("default %s"), T.Name), C.DefaultTypeface.Fonts[I], T.Name, T.Face, T.SubFace);
		}
	}
	if (TestEqual(TEXT("sub-fonts"), C.SubTypefaces.Num(), PawFontSpec::SubFonts.Num()))
	{
		for (int32 I = 0; I < PawFontSpec::SubFonts.Num(); ++I)
		{
			const PawFontSpec::FSubFont& S = PawFontSpec::SubFonts[I];
			const FCompositeSubFont& Have = C.SubTypefaces[I];
			const FString Where = FString::Printf(TEXT("sub-font %s"), S.Name);
			TestEqual(Where + TEXT(": cultures"), Have.Cultures, FString(S.Cultures));
			if (TestEqual(Where + TEXT(": ranges"), Have.CharacterRanges.Num(), S.Ranges.Num()))
			{
				for (int32 R = 0; R < S.Ranges.Num(); ++R)
				{
					const FInt32Range& Range = Have.CharacterRanges[R];
					TestTrue(FString::Printf(TEXT("%s: range %d is U+%04X-U+%04X"), *Where, R, S.Ranges[R].First, S.Ranges[R].Last),
						Range.Contains(S.Ranges[R].First) && Range.Contains(S.Ranges[R].Last)
						&& !Range.Contains(S.Ranges[R].First - 1) && !Range.Contains(S.Ranges[R].Last + 1));
				}
			}
			if (TestEqual(Where + TEXT(": typefaces"), Have.Typeface.Fonts.Num(), PawFontSpec::Default.Num()))
			{
				for (int32 T = 0; T < PawFontSpec::Default.Num(); ++T)
				{
					const PawFontSpec::FTypeface& D = PawFontSpec::Default[T];
					const bool bDefault = *S.Face == 0; // the default typeface's faces
					Same(Where + TEXT(" ") + D.Name, Have.Typeface.Fonts[T], D.Name, bDefault ? D.Face : S.Face, bDefault ? D.SubFace : S.SubFace);
				}
			}
		}
	}
	if (TestEqual(TEXT("fallback typefaces"), C.FallbackTypeface.Typeface.Fonts.Num(), PawFontSpec::Default.Num()))
	{
		for (int32 T = 0; T < PawFontSpec::Default.Num(); ++T)
		{
			Same(FString(TEXT("fallback ")) + PawFontSpec::Default[T].Name, C.FallbackTypeface.Typeface.Fonts[T], PawFontSpec::Default[T].Name,
				PawFontSpec::Fallback.Face, PawFontSpec::Fallback.SubFace);
		}
	}

	for (const PawFontSpec::FFace& F : PawFontSpec::Faces)
	{
		const FString Path = FString(PawFontSpec::Folder) / F.Name + TEXT(".") + F.Name;
		const UFontFace* Face = LoadObject<UFontFace>(nullptr, *Path);
		if (TestNotNull(FString::Printf(TEXT("face %s"), F.Name), Face))
		{
			TestTrue(FString::Printf(TEXT("face %s loads %s"), F.Name, F.LoadingPolicy), Face->GetLoadingPolicy() == Policy(F.LoadingPolicy));
			TestTrue(FString::Printf(TEXT("face %s is from %s"), F.Name, F.File), Face->SourceFilename.EndsWith(F.File));
		}
	}

	// UMG's default ([SlateStyle] DefaultFontName in DefaultEngine.ini): every text widget made anywhere starts in F_Paw.
	TestEqual(TEXT("UMG's default font is F_Paw"), UWidget::GetDefaultFontName() + TEXT(".F_Paw"), FString(PawFontSpec::Font));
	const UTextBlock* Default = GetDefault<UTextBlock>();
	TestEqual(TEXT("a new text block's font"), FacePath(Default->GetFont().FontObject), FString(PawFontSpec::Font));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawWidgetsFontsTest, "Pawpocalypse.UI.Widgets.Fonts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// The restyle's font migration: a baked asset saved its fonts as values (UMG's Roboto at bake time), so after the switch to
// F_Paw none may still hold an engine font (`py tools/bake_widgets.py --run --refont`). Also proves the walker sees fonts.
bool FPawWidgetsFontsTest::RunTest(const FString& Parameters)
{
	int32 Seen = 0;
	for (UClass* Native : PawUi::BakedClasses())
	{
		UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *(PawUi::AssetPathFor(Native) + TEXT(".") + PawUi::AssetNameFor(Native)));
		if (!TestNotNull(FString::Printf(TEXT("%s has its asset"), *Native->GetName()), BP))
		{
			continue;
		}
		for (UObject* O : PawUi::LookObjects(BP))
		{
			PawUi::ForEachFont(O, [&](FSlateFontInfo& F)
			{
				++Seen;
				TestFalse(FString::Printf(TEXT("%s: %s holds the engine font %s"), *PawUi::AssetNameFor(Native), *O->GetName(), *FacePath(F.FontObject)),
					PawUi::IsEngineFont(F.FontObject));
				return false;
			});
		}
	}
	TestTrue(FString::Printf(TEXT("the walker saw the assets' fonts (%d)"), Seen), Seen > 50);
	return true;
}

#endif
