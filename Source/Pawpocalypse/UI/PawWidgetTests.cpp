// Pawpocalypse.UI.Widgets.*: every UI class is a Widget Blueprint the designer can edit (designer 2026-11-04; PawWidgets.h),
// and each baked asset still has what its C++ class needs (KB U/09 §2.1: the parity test that caught a widget held by two
// properties, buttons registered only in code, a piece nested in an older asset).

// Editor only: a cooked game has no Blueprint generator links (UClass::ClassGeneratedBy) nor property metadata (BindWidget).
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UI/PawButton.h"
#include "UI/PawGlyphs.h"
#include "UI/PawUiAssets.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "UI/PawIcons.h"
#include "UI/PawPrompts.h"
#include "UI/PawTokens.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FWidgetsWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;

		void Open()
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			World = GI->GetWorld();
		}

		void Close()
		{
			PawUi::SetCodeLayoutForced(false);
			if (GI)
			{
				GI->Shutdown();
			}
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}

		UUserWidget* Make(UClass* Class)
		{
			return CreateWidget<UUserWidget>(GI, Class);
		}
	};

	// What a tree holds, comparable across the two modes: each widget's class, and by name the words it shows.
	struct FTreeFacts
	{
		TMap<FString, int32> Classes;
		TMap<FName, FString> Words;
		// A text's look the restyle sets in code and grafts into the asset (restyle:Part): typeface, outline, shadow.
		TMap<FName, FString> Looks;
		// P4.3 (2026-11-27): every part's tilt (a code change to an angle must reach the asset: graft slot:Part).
		TMap<FName, float> Angles;
	};

	FString ClassKey(const UWidget* W)
	{
		// A piece's Widget Blueprint and its native class are the same piece.
		const UClass* C = W->GetClass();
		while (C && Cast<UWidgetBlueprintGeneratedClass>(C))
		{
			C = C->GetSuperClass();
		}
		return C ? C->GetName() : FString();
	}

	FTreeFacts Facts(const UUserWidget* Root)
	{
		FTreeFacts Out;
		if (!Root->WidgetTree)
		{
			return Out;
		}
		Root->WidgetTree->ForEachWidget([&Out](UWidget* W)
		{
			// The Common button's internal wrapper is made by every instance; it is not part of the layout.
			if (W->GetClass()->GetFName() == TEXT("CommonButtonInternalBase"))
			{
				return;
			}
			Out.Classes.FindOrAdd(ClassKey(W))++;
			Out.Angles.Add(W->GetFName(), W->GetRenderTransformAngle());
			if (const UTextBlock* T = Cast<UTextBlock>(W))
			{
				Out.Words.Add(W->GetFName(), T->GetText().ToString());
				const FSlateFontInfo& F = T->GetFont();
				// R4g: and its colour (a code change to a title's colour must reach the asset: graft text:).
				// P4.8 (2026-11-27): and its wrap (a code change to a width, e.g. the dock's, must reach the asset: graft wrapat:).
				Out.Looks.Add(W->GetFName(), FString::Printf(TEXT("typeface %s, outline %d, shadow %.1f,%.1f, colour %s, wrap %.0f%s"),
					*F.TypefaceFontName.ToString(), F.OutlineSettings.OutlineSize, T->GetShadowOffset().X, T->GetShadowOffset().Y,
					*T->GetColorAndOpacity().GetSpecifiedColor().ToFColorSRGB().ToHex(), T->GetWrapTextAt(), T->GetAutoWrapText() ? TEXT(" auto") : TEXT("")));
			}
			else if (const UPawPromptText* PT = Cast<UPawPromptText>(W))
			{
				Out.Looks.Add(W->GetFName(), FString::Printf(TEXT("prompt text %.1f px, wrap %.0f (graft props:)"), PT->TextPx, PT->WrapAt));
			}
			else if (const UPawIconText* IT = Cast<UPawIconText>(W))
			{
				Out.Looks.Add(W->GetFName(), FString::Printf(TEXT("icon text %.1f px, wrap %.0f (graft props:)"), IT->TextPx, IT->WrapAt));
			}
			else if (const UPawButton* B = Cast<UPawButton>(W))
			{
				Out.Words.Add(W->GetFName(), FString::Printf(TEXT("%s|%.1f|%.1f|%d|%d"), *B->LabelText.ToString(), B->LabelPx, B->WrapAt,
					B->bAlignLeft ? 1 : 0, B->bFlatLook ? 1 : 0));
			}
		});
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawWidgetsAreBlueprintsTest, "Pawpocalypse.UI.Widgets.AreBlueprints", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Every class in the bake list resolves to its baked Blueprint (none left native), a style to a Blueprint child of it.
bool FPawWidgetsAreBlueprintsTest::RunTest(const FString& Parameters)
{
	PawUi::ResetResolver();
	int32 Count = 0;
	for (UClass* Native : PawUi::BakedClasses())
	{
		UClass* Resolved = PawUi::Resolve(Native);
		const bool bBlueprint = Resolved && Resolved != Native && Resolved->IsChildOf(Native) && Resolved->ClassGeneratedBy != nullptr;
		if (!TestTrue(FString::Printf(TEXT("%s is a Blueprint (%s)"), *Native->GetName(), *PawUi::AssetPathFor(Native)), bBlueprint))
		{
			continue;
		}
		if (Native->IsChildOf(UUserWidget::StaticClass()))
		{
			TestNotNull(FString::Printf(TEXT("%s is a Widget Blueprint"), *Native->GetName()), Cast<UWidgetBlueprintGeneratedClass>(Resolved));
		}
		++Count;
	}
	TestTrue(TEXT("the list is not empty"), Count > 0);
	// The switch for comparisons: forced, every class is its native self.
	PawUi::SetCodeLayoutForced(true);
	for (UClass* Native : PawUi::BakedClasses())
	{
		TestTrue(FString::Printf(TEXT("%s: the code layout when forced"), *Native->GetName()), PawUi::Resolve(Native) == Native);
	}
	PawUi::SetCodeLayoutForced(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawWidgetsPaletteTest, "Pawpocalypse.UI.Widgets.Palette", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// The restyle's palette migration (docs/WORKLIST-RESTYLE-2026-11-07.md): a baked asset saved its colours as values, so after a
// palette change none may still hold a colour the palette retired (PawTokens::PALETTE_RETIRED); `py tools/bake_widgets.py
// --run --recolour` migrates them. Also proves the walker sees colours at all (a test that finds nothing proves nothing).
bool FPawWidgetsPaletteTest::RunTest(const FString& Parameters)
{
	int32 Seen = 0;
	int32 Assets = 0;
	for (UClass* Native : PawUi::BakedClasses())
	{
		UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *(PawUi::AssetPathFor(Native) + TEXT(".") + PawUi::AssetNameFor(Native)));
		if (!TestNotNull(FString::Printf(TEXT("%s has its asset"), *Native->GetName()), BP))
		{
			continue;
		}
		++Assets;
		for (UObject* O : PawUi::LookObjects(BP))
		{
			PawUi::ForEachColour(O, [&](FColor& C)
			{
				++Seen;
				const uint32 Rgb = (static_cast<uint32>(C.R) << 16) | (static_cast<uint32>(C.G) << 8) | C.B;
				TestFalse(FString::Printf(TEXT("%s: %s holds the retired colour #%06X"), *PawUi::AssetNameFor(Native), *O->GetName(), Rgb),
					PawTokens::PALETTE_RETIRED.Contains(Rgb));
				return false;
			});
		}
	}
	TestTrue(TEXT("every baked class has an asset"), Assets == PawUi::BakedClasses().Num());
	TestTrue(FString::Printf(TEXT("the walker saw the assets' colours (%d)"), Seen), Seen > 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawWidgetsColourRolesTest, "Pawpocalypse.UI.Widgets.ColourRoles", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// The plum flip's migration tells a text's colour from a surface's (restyle R3: ink text -> Bone, ink frames kept): which
// properties are TEXT. A text widget's and a text style's ColorAndOpacity and a piece's TextColour are; a shadow, an outline
// and an image's tint are not.
bool FPawWidgetsColourRolesTest::RunTest(const FString& Parameters)
{
	struct FCase { const UStruct* Owner; const TCHAR* Property; bool bText; };
	const FCase Cases[] = {
		{ UTextBlock::StaticClass(), TEXT("ColorAndOpacity"), true },
		{ UTextBlock::StaticClass(), TEXT("ShadowColorAndOpacity"), false },
		{ URichTextBlock::StaticClass(), TEXT("DefaultTextStyleOverride"), false },
		{ FTextBlockStyle::StaticStruct(), TEXT("ColorAndOpacity"), true },
		{ FTextBlockStyle::StaticStruct(), TEXT("ShadowColorAndOpacity"), false },
		{ UImage::StaticClass(), TEXT("ColorAndOpacity"), false },
		{ UBorder::StaticClass(), TEXT("BrushColor"), false },
		{ UPawIconText::StaticClass(), TEXT("TextColour"), true },
		{ UPawPromptText::StaticClass(), TEXT("TextColour"), true },
	};
	for (const FCase& C : Cases)
	{
		const FProperty* P = FindFProperty<FProperty>(C.Owner, C.Property);
		if (TestNotNull(FString::Printf(TEXT("%s.%s exists"), *C.Owner->GetName(), C.Property), P))
		{
			TestEqual(FString::Printf(TEXT("%s.%s is %s"), *C.Owner->GetName(), C.Property, C.bText ? TEXT("text") : TEXT("a surface")),
				PawUi::IsTextColour(C.Owner, P), C.bText);
		}
	}
	// R3's shapes: a brush's outline is an EDGE; a font's outline is not.
	struct FEdgeCase { const UStruct* Owner; const TCHAR* Property; bool bEdge; };
	const FEdgeCase Edges[] = {
		{ FSlateBrushOutlineSettings::StaticStruct(), TEXT("Color"), true },
		{ FFontOutlineSettings::StaticStruct(), TEXT("OutlineColor"), false },
		{ FSlateBrush::StaticStruct(), TEXT("TintColor"), false },
	};
	for (const FEdgeCase& C : Edges)
	{
		const FProperty* P = FindFProperty<FProperty>(C.Owner, C.Property);
		if (TestNotNull(FString::Printf(TEXT("%s.%s exists"), *C.Owner->GetName(), C.Property), P))
		{
			TestEqual(FString::Printf(TEXT("%s.%s is %s"), *C.Owner->GetName(), C.Property, C.bEdge ? TEXT("an edge") : TEXT("not an edge")),
				PawUi::IsEdgeColour(C.Owner, P), C.bEdge);
		}
	}
	// And the walk carries the role down: a text block's colour arrives as text, its shadow as a surface; a border's brush
	// outline as an edge, its fill as a surface.
	using PawUi::EPawColourRole;
	UTextBlock* T = NewObject<UTextBlock>();
	T->SetColorAndOpacity(FSlateColor(FLinearColor(FColor(0x11, 0x22, 0x33))));
	T->SetShadowColorAndOpacity(FLinearColor(FColor(0x44, 0x55, 0x66)));
	bool bTextSeen = false, bShadowSeen = false;
	PawUi::ForEachColourRole(T, [&](FColor& Col, EPawColourRole Role)
	{
		bTextSeen |= Col == FColor(0x11, 0x22, 0x33, 255) && Role == EPawColourRole::Text;
		bShadowSeen |= Col.R == 0x44 && Col.G == 0x55 && Col.B == 0x66 && Role == EPawColourRole::Surface;
		return false;
	});
	TestTrue(TEXT("a text block's colour walks as text"), bTextSeen);
	TestTrue(TEXT("a text block's shadow walks as a surface"), bShadowSeen);
	UBorder* B = NewObject<UBorder>();
	B->SetBrush(PawUi::Rounded(FLinearColor(FColor(0x21, 0x32, 0x43)), FLinearColor(FColor(0x54, 0x65, 0x76)), 3.0f, 8.0f));
	bool bFillSeen = false, bEdgeSeen = false;
	PawUi::ForEachColourRole(B, [&](FColor& Col, EPawColourRole Role)
	{
		bFillSeen |= Col.R == 0x21 && Col.G == 0x32 && Col.B == 0x43 && Role == EPawColourRole::Surface;
		bEdgeSeen |= Col.R == 0x54 && Col.G == 0x65 && Col.B == 0x76 && Role == EPawColourRole::Edge;
		return false;
	});
	TestTrue(TEXT("a border's fill walks as a surface"), bFillSeen);
	TestTrue(TEXT("a border's brush outline walks as an edge"), bEdgeSeen);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawWidgetsTextFloorTest, "Pawpocalypse.UI.Widgets.TextFloor", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// R4b, the text floor: nothing under 16 px at 1080p (12 pt), in every baked asset (what the game draws) and every code layout
// (what a new part grafts from); a keycap's label exempt (PawUi::UnderTextFloor). Also proves the walk sees small text: a
// 10 pt text block and a piece at 13 px are both named, and raised.
bool FPawWidgetsTextFloorTest::RunTest(const FString& Parameters)
{
	UTextBlock* Small = NewObject<UTextBlock>();
	FSlateFontInfo F = Small->GetFont();
	F.Size = 10.0f;
	Small->SetFont(F);
	TestEqual(TEXT("a 10 pt text is under the floor"), PawUi::UnderTextFloor(Small, false).Num(), 1);
	PawUi::UnderTextFloor(Small, true);
	TestEqual(TEXT("raised, it is 12 pt"), Small->GetFont().Size, 12.0f);
	UPawIconText* Piece = NewObject<UPawIconText>();
	Piece->TextPx = 13.0f;
	TestEqual(TEXT("a piece at 13 px is under the floor"), PawUi::UnderTextFloor(Piece, false).Num(), 1);

	int32 Fonts = 0;
	for (UClass* Native : PawUi::BakedClasses())
	{
		UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *(PawUi::AssetPathFor(Native) + TEXT(".") + PawUi::AssetNameFor(Native)));
		if (!TestNotNull(FString::Printf(TEXT("%s has its asset"), *Native->GetName()), BP))
		{
			continue;
		}
		for (UObject* O : PawUi::LookObjects(BP))
		{
			Fonts += PawUi::ForEachFont(O, [](FSlateFontInfo&) { return true; }) > 0 ? 1 : 0;
			for (const FString& Under : PawUi::UnderTextFloor(O, false))
			{
				AddError(FString::Printf(TEXT("%s: %s, under the 16 px floor (py tools/bake_widgets.py --run --textfloor)"),
					*PawUi::AssetNameFor(Native), *Under));
			}
		}
	}
	TestTrue(FString::Printf(TEXT("the walk saw the assets' texts (%d)"), Fonts), Fonts > 50);

	FWidgetsWorld W;
	W.Open();
	PawUi::SetCodeLayoutForced(true);
	for (UClass* Native : PawUi::BakedClasses())
	{
		if (!Native->IsChildOf(UUserWidget::StaticClass()))
		{
			continue;
		}
		UUserWidget* FromCode = W.Make(Native);
		if (FromCode && FromCode->WidgetTree)
		{
			FromCode->WidgetTree->ForEachWidget([&](UWidget* Part)
			{
				for (const FString& Under : PawUi::UnderTextFloor(Part, false))
				{
					AddError(FString::Printf(TEXT("%s's code layout: %s, under the 16 px floor"), *Native->GetName(), *Under));
				}
			});
		}
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawWidgetsIconsFromAtlasTest, "Pawpocalypse.UI.Widgets.IconsFromAtlas", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Phase 8, draw calls (2026-11-23): Slate batches by texture, so an icon drawn from its own texture is a draw call of its own.
// Every baked class, as its asset builds it (the saved brushes), live: no image (shown or not yet) draws an icon's own texture
// when the UI atlas has the icon (PawUi::UseAtlasIcons after Super::Initialize; IconBrush for what code sets later).
bool FPawWidgetsIconsFromAtlasTest::RunTest(const FString& Parameters)
{
	PawUi::SetAtlasForTest(true); // the game's way (the editor draws the textures)
	ON_SCOPE_EXIT { PawUi::SetAtlasForTest(false); };
	FWidgetsWorld W;
	W.Open();
	int32 Cells = 0;
	TFunction<void(const UUserWidget*, const FString&, int32)> Walk = [&](const UUserWidget* U, const FString& Where, int32 Depth)
	{
		if (!U || !U->WidgetTree || Depth > 16)
		{
			return;
		}
		U->WidgetTree->ForEachWidget([&](UWidget* Part)
		{
			if (const UUserWidget* Piece = Cast<UUserWidget>(Part))
			{
				Walk(Piece, Where + TEXT("/") + Piece->GetName(), Depth + 1);
				return;
			}
			// An image's picture, a border's plate, a button's four state plates (Common UI's built style).
			TArray<const FSlateBrush*> Brushes;
			if (const UImage* I = Cast<UImage>(Part))
			{
				Brushes.Add(&I->GetBrush());
			}
			else if (const UBorder* B = Cast<UBorder>(Part))
			{
				Brushes.Add(&B->Background);
			}
			else if (const UButton* Root = Cast<UButton>(Part))
			{
				const FButtonStyle& S = Root->GetStyle();
				Brushes.Append({ &S.Normal, &S.Hovered, &S.Pressed, &S.Disabled });
			}
			for (const FSlateBrush* Brush : Brushes)
			{
				const UObject* R = Brush->GetResourceObject();
				Cells += Cast<UPawUiCell>(R) ? 1 : 0;
				const FString Stem = Cast<UTexture2D>(R) && R->GetName().StartsWith(TEXT("T_")) ? R->GetName().Mid(2) : FString();
				if (!Stem.IsEmpty() && PawUi::IconCell(Stem))
				{
					AddError(FString::Printf(TEXT("%s/%s draws %s from its own texture; the atlas has it (PawUi::UseAtlasIcons)"),
						*Where, *Part->GetName(), *R->GetName()));
				}
			}
		});
	};
	for (UClass* Native : PawUi::BakedClasses())
	{
		if (Native->IsChildOf(UUserWidget::StaticClass()))
		{
			Walk(W.Make(PawUi::Resolve(Native)), PawUi::AssetNameFor(Native), 0);
		}
	}
	TestTrue(FString::Printf(TEXT("the walk saw icons drawn from the atlas (%d)"), Cells), Cells > 10);
	W.Close();
	return true;
}

namespace
{
	// WCAG 2 relative luminance of a linear colour, and the contrast ratio of two.
	double Luminance(const FLinearColor& C) { return 0.2126 * C.R + 0.7152 * C.G + 0.0722 * C.B; }
	double ContrastRatio(const FLinearColor& A, const FLinearColor& B)
	{
		const double La = Luminance(A), Lb = Luminance(B);
		return (FMath::Max(La, Lb) + 0.05) / (FMath::Min(La, Lb) + 0.05);
	}
	// What a part stands on: the nearest Border above it that paints something (a chrome plate is the card colour; a
	// drawn box its own tint). False: nothing painted under it in this tree (over the field, or a piece's own look).
	bool Backdrop(const UWidget* W, FLinearColor& Out)
	{
		for (const UPanelWidget* P = W ? W->GetParent() : nullptr; P; P = P->GetParent())
		{
			const UBorder* B = Cast<UBorder>(P);
			if (!B || B->Background.DrawAs == ESlateBrushDrawType::NoDrawType)
			{
				continue;
			}
			const UObject* Art = B->Background.GetResourceObject();
			if (Art && Art->GetName().StartsWith(TEXT("T_ui_")))
			{
				Out = PawTokens::UI_PANEL; // every chrome plate is the card colour since D18
				return true;
			}
			const FLinearColor Tint = B->Background.TintColor.GetSpecifiedColor() * B->GetBrushColor();
			if (!Art && Tint.A > 0.9f)
			{
				Out = Tint;
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawWidgetsReadableTest, "Pawpocalypse.UI.Widgets.ReadableOnPlates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// D18 (S4, 2026-11-19): every text a Widget Blueprint puts on a plate reads against it (WCAG 2 AA: 4.5, or 3 for large text,
// 24 px / 18 pt and up). The skill tree's Stardust pill and legend kept the plum era's cream text when the plates turned
// cream, and showed nothing (the store shot 07).
bool FPawWidgetsReadableTest::RunTest(const FString& Parameters)
{
	FWidgetsWorld W;
	W.Open();
	int32 Checked = 0;
	for (UClass* Native : PawUi::BakedClasses())
	{
		if (!Native->IsChildOf(UUserWidget::StaticClass()))
		{
			continue;
		}
		UClass* Baked = PawUi::Resolve(Native);
		UUserWidget* Made = Baked != Native ? W.Make(Baked) : nullptr;
		if (!Made || !Made->WidgetTree)
		{
			continue;
		}
		Made->WidgetTree->ForEachWidget([&](UWidget* Part)
		{
			FLinearColor Text;
			bool bLarge = false;
			if (const UTextBlock* T = Cast<UTextBlock>(Part))
			{
				if (!T->GetColorAndOpacity().IsColorSpecified())
				{
					return;
				}
				Text = T->GetColorAndOpacity().GetSpecifiedColor();
				bLarge = T->GetFont().Size >= 18.0f;
				// A sticker title (yellow on its thick ink outline, the concept's): the outline is what meets the plate.
				if (T->GetFont().OutlineSettings.OutlineSize >= 2)
				{
					Text = T->GetFont().OutlineSettings.OutlineColor;
					bLarge = true;
				}
			}
			else if (const UPawPromptText* PT = Cast<UPawPromptText>(Part))
			{
				Text = PT->TextColour;
				bLarge = PT->TextPx >= 24.0f;
			}
			else if (const UPawIconText* IT = Cast<UPawIconText>(Part))
			{
				Text = IT->TextColour;
				bLarge = IT->TextPx >= 24.0f;
			}
			else
			{
				return;
			}
			FLinearColor Under;
			if (Text.A < 0.05f || !Backdrop(Part, Under))
			{
				return;
			}
			++Checked;
			const double Ratio = ContrastRatio(Text, Under);
			const double Need = bLarge ? 3.0 : 4.5;
			if (Ratio < Need)
			{
				AddError(FString::Printf(TEXT("%s.%s: text %s on %s reads %.2f:1 (needs %.1f)"), *Native->GetName(), *Part->GetName(),
					*Text.ToFColor(true).ToHex(), *Under.ToFColor(true).ToHex(), Ratio, Need));
			}
		});
	}
	TestTrue(TEXT("texts on plates were checked"), Checked > 50);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawWidgetsParityTest, "Pawpocalypse.UI.Widgets.Parity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Each Widget Blueprint against its class's code layout: every bound part present in both, the same widgets, the same words.
// A designer's edit can make this differ ON PURPOSE (the asset owns the layout): the failure then names what to look at.
bool FPawWidgetsParityTest::RunTest(const FString& Parameters)
{
	FWidgetsWorld W;
	W.Open();
	for (UClass* Native : PawUi::BakedClasses())
	{
		if (!Native->IsChildOf(UUserWidget::StaticClass()))
		{
			continue;
		}
		PawUi::SetCodeLayoutForced(false);
		UClass* Baked = PawUi::Resolve(Native);
		if (Baked == Native)
		{
			AddError(FString::Printf(TEXT("%s has no Widget Blueprint"), *Native->GetName()));
			continue;
		}
		UUserWidget* FromAsset = W.Make(Baked);
		PawUi::SetCodeLayoutForced(true);
		UUserWidget* FromCode = W.Make(Native);
		PawUi::SetCodeLayoutForced(false);
		if (!TestNotNull(TEXT("made from the asset"), FromAsset) || !TestNotNull(TEXT("made from code"), FromCode))
		{
			continue;
		}
		const FString Name = Native->GetName();
		for (TFieldIterator<FObjectPropertyBase> It(Native); It; ++It)
		{
			// Our parts only (the engine's own optional ones, e.g. a Common button's InputActionWidget, are not the layout's).
			if ((It->HasMetaData(TEXT("BindWidget")) || It->HasMetaData(TEXT("BindWidgetOptional")))
				&& It->GetOwnerClass()->GetOutermost() == Native->GetOutermost())
			{
				TestNotNull(FString::Printf(TEXT("%s.%s bound from the asset"), *Name, *It->GetName()), It->GetObjectPropertyValue_InContainer(FromAsset));
				TestNotNull(FString::Printf(TEXT("%s.%s built in code"), *Name, *It->GetName()), It->GetObjectPropertyValue_InContainer(FromCode));
			}
		}
		const FTreeFacts A = Facts(FromAsset);
		const FTreeFacts C = Facts(FromCode);
		for (const TPair<FString, int32>& P : C.Classes)
		{
			TestEqual(FString::Printf(TEXT("%s: how many %s"), *Name, *P.Key), A.Classes.FindRef(P.Key), P.Value);
		}
		for (const TPair<FString, int32>& P : A.Classes)
		{
			TestTrue(FString::Printf(TEXT("%s: the asset's %s are the code's"), *Name, *P.Key), C.Classes.Contains(P.Key));
		}
		for (const TPair<FName, FString>& P : C.Words)
		{
			if (const FString* Got = A.Words.Find(P.Key))
			{
				TestEqual(FString::Printf(TEXT("%s.%s says the same"), *Name, *P.Key.ToString()), *Got, P.Value);
			}
		}
		for (const TPair<FName, FString>& P : C.Looks)
		{
			if (const FString* Got = A.Looks.Find(P.Key))
			{
				TestEqual(FString::Printf(TEXT("%s.%s looks the same (graft restyle:%s, or text: for its colour)"), *Name, *P.Key.ToString(), *P.Key.ToString()), *Got, P.Value);
			}
		}
		for (const TPair<FName, float>& P : C.Angles)
		{
			if (const float* Got = A.Angles.Find(P.Key); Got && !FMath::IsNearlyEqual(*Got, P.Value, 0.01f))
			{
				AddError(FString::Printf(TEXT("%s.%s is tilted %.2f in the asset, %.2f in code (graft slot:%s)"), *Name, *P.Key.ToString(), *Got,
					P.Value, *P.Key.ToString()));
			}
		}
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawWidgetsHeldTest, "Pawpocalypse.UI.Widgets.Held", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Review CR12: what the UI looks up on events is held by UPawUiAssets once looked up, so the GC cannot take it between two
// prompts and the next lookup does not load it again, synchronously, mid-play. A forced GC proves nothing in the editor
// (a loaded asset keeps RF_Standalone there): the test asserts the holder instead, and counts the glyph loads.
bool FPawWidgetsHeldTest::RunTest(const FString& Parameters)
{
	UPawUiAssets* Held = UPawUiAssets::Get();
	if (!TestNotNull(TEXT("the engine holds the UI's assets"), Held))
	{
		return false;
	}

	// A prompt's glyph: held, and a second redraw does not go to the package.
	const PawGlyphs::FPrompt Prompt = PawGlyphs::PromptFor(TEXT("ui_accept"), Pc::PcFamily::Xbox);
	if (!TestTrue(TEXT("Accept on an Xbox pad is a glyph"), Prompt.Kind == PawGlyphs::EKind::Glyph))
	{
		return false;
	}
	UTexture2D* Tex = PawGlyphs::Texture(Prompt.Stem);
	TestNotNull(TEXT("its texture"), Tex);
	TestTrue(TEXT("held"), Held->Glyphs.FindRef(Prompt.Stem) == Tex);
	int32 Loads = PawGlyphs::TextureLoads();
	PawGlyphs::Texture(Prompt.Stem);
	TestEqual(TEXT("the next redraw loads nothing"), PawGlyphs::TextureLoads(), Loads);

	// A stem with no texture (a keycap prompt): looked for once, then known to be missing.
	const FString None = TEXT("input_xbox_nothing_cr12");
	TestNull(TEXT("no texture for a stem with none"), PawGlyphs::Texture(None));
	Loads = PawGlyphs::TextureLoads();
	TestNull(TEXT("still none"), PawGlyphs::Texture(None));
	TestEqual(TEXT("and not looked for again"), PawGlyphs::TextureLoads(), Loads);
	TestTrue(TEXT("its absence is remembered"), Held->Glyphs.Contains(None));

	// A baked Widget Blueprint class: held once resolved.
	UClass* Native = UPawButtonStyle::StaticClass();
	UClass* Resolved = PawUi::Resolve(Native);
	TestTrue(TEXT("the button style resolves to its Blueprint"), Resolved && Resolved != Native && Resolved->ClassGeneratedBy != nullptr);
	TestTrue(TEXT("the class is held"), Held->Classes.FindRef(Native) == Resolved);
	PawUi::SetCodeLayoutForced(true);
	TestEqual(TEXT("the code layout forgets the classes"), Held->Classes.Num(), 0);
	PawUi::SetCodeLayoutForced(false);

	// The game's font.
	UFont* Font = PawUi::PawFont();
	TestNotNull(TEXT("F_Paw"), Font);
	TestTrue(TEXT("the font is held"), Held->Font == Font);
	return true;
}
#endif
