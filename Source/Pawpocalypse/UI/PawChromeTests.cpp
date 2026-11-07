// Pawpocalypse.UI.Chrome (phase 6 slice 5a): the theme's nine-patch chrome. Every brush against Godot's own numbers
// (pawpocalypse_theme.tres, held here apart from PawUi's table so a wrong row is caught), the button looks as the style
// Blueprints resolve them (a C++ default reaches BP_*Style only while the Blueprint saves no value of its own), every asset
// part the chrome was restyled into, and the selection ring painted over a hovered button.

// Editor only: the style Blueprints and the Widget Blueprints are resolved through their generator links.
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Input/HittestGrid.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Rendering/DrawElements.h"
#include "Types/PaintArgs.h"
#include "UI/PawButton.h"
#include "UI/PawHudScreen.h"
#include "UI/PawJobPicker.h"
#include "UI/PawSettingsDialog.h"
#include "UI/PawTokens.h"
#include "UI/PawUiKit.h"
#include "UI/PawVictoryScreen.h"
#include "UI/PawWidgets.h"
#include "UI/PawRainbow.h"
#include "Widgets/SWindow.h"

namespace
{
	using PawUi::EPawChrome;

	// pawpocalypse_theme.tres (texture_margin_*) and each SVG's width / height, read 2026-11-06.
	struct FGodotChrome { EPawChrome Which; const TCHAR* Texture; int32 W, H, L, T, R, B; };
	const FGodotChrome Godot[] = {
		{ EPawChrome::ButtonNormal, TEXT("T_ui_button_normal"), 256, 64, 16, 16, 16, 16 },
		{ EPawChrome::ButtonHover, TEXT("T_ui_button_hover"), 256, 64, 16, 16, 16, 16 },
		{ EPawChrome::ButtonPressed, TEXT("T_ui_button_pressed"), 256, 64, 16, 16, 16, 16 },
		{ EPawChrome::ButtonDisabled, TEXT("T_ui_button_disabled"), 256, 64, 16, 16, 16, 16 },
		{ EPawChrome::Panel, TEXT("T_ui_panel"), 128, 128, 20, 20, 20, 20 },
		{ EPawChrome::Window, TEXT("T_ui_window"), 256, 256, 48, 48, 48, 48 },
		{ EPawChrome::BarTrack, TEXT("T_ui_bar_track"), 64, 16, 8, 4, 8, 4 },
		{ EPawChrome::BarFill, TEXT("T_ui_bar_fill"), 64, 16, 8, 4, 8, 4 },
		{ EPawChrome::Separator, TEXT("T_ui_separator"), 64, 8, 8, 2, 8, 2 },
	};

	// Is this brush the chrome piece: a Box over its texture, at the SVG's size, the margins Godot's 1:1.
	FString ChromeProblem(const FSlateBrush& Got, EPawChrome Which)
	{
		for (const FGodotChrome& G : Godot)
		{
			if (G.Which != Which)
			{
				continue;
			}
			const UObject* Tex = Got.GetResourceObject();
			if (!Tex || Tex->GetName() != G.Texture)
			{
				return FString::Printf(TEXT("texture %s, not %s"), Tex ? *Tex->GetName() : TEXT("none"), G.Texture);
			}
			if (Got.DrawAs != ESlateBrushDrawType::Box)
			{
				return TEXT("not drawn as a Box");
			}
			if (!Got.ImageSize.Equals(FVector2D(G.W, G.H)))
			{
				return FString::Printf(TEXT("image size %s, not %d x %d"), *Got.ImageSize.ToString(), G.W, G.H);
			}
			const FMargin Want(float(G.L) / G.W, float(G.T) / G.H, float(G.R) / G.W, float(G.B) / G.H);
			const FMargin& M = Got.Margin;
			if (!FMath::IsNearlyEqual(M.Left, Want.Left) || !FMath::IsNearlyEqual(M.Top, Want.Top)
				|| !FMath::IsNearlyEqual(M.Right, Want.Right) || !FMath::IsNearlyEqual(M.Bottom, Want.Bottom))
			{
				return FString::Printf(TEXT("margins %.4f %.4f %.4f %.4f, not Godot's %d %d %d %d px"), M.Left, M.Top, M.Right, M.Bottom,
					G.L, G.T, G.R, G.B);
			}
			if (!Got.TintColor.GetSpecifiedColor().Equals(FLinearColor::White))
			{
				return TEXT("tinted");
			}
			return FString();
		}
		return TEXT("no Godot row");
	}

	struct FChromeWorld
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
	};

	TSharedPtr<SWidget> FindSlate(const TSharedRef<SWidget>& W, const FName Type)
	{
		if (W->GetType() == Type)
		{
			return W;
		}
		FChildren* Children = W->GetChildren();
		for (int32 I = 0; Children && I < Children->Num(); ++I)
		{
			if (TSharedPtr<SWidget> Found = FindSlate(Children->GetChildAt(I), Type))
			{
				return Found;
			}
		}
		return nullptr;
	}

	// Paints the widget off screen; counts the selection rings drawn (rounded boxes with a 3 px UI_FOCUS outline).
	int32 RingsPainted(UUserWidget* Widget)
	{
		const TSharedRef<SWidget> S = Widget->TakeWidget();
		S->SlatePrepass(1.0f);
		const FVector2f Size(240.0f, 46.0f);
		const FGeometry Root = FGeometry::MakeRoot(FVector2D(Size), FSlateLayoutTransform());
		const TSharedRef<SWindow> Window = SNew(SWindow);
		FSlateWindowElementList List(Window);
		FHittestGrid Grid;
		FPaintArgs Args(&Window.Get(), Grid, FVector2D::ZeroVector, FApp::GetCurrentTime(), 0.016f);
		S->Paint(Args, Root, FSlateRect(0.0f, 0.0f, Size.X, Size.Y), List, 0, FWidgetStyle(), true);
		int32 Rings = 0;
		for (const FSlateRoundedBoxElement& E : List.GetUncachedDrawElements().Get<(uint8)EElementType::ET_RoundedBox>())
		{
			// An outline only: a filled ring would cover the plate it marks.
			if (FMath::IsNearlyEqual(E.GetOutlineWeight(), 3.0f) && E.GetOutlineColor().Equals(PawTokens::UI_FOCUS, 0.01f)
				&& E.GetTint().A < 0.01f)
			{
				++Rings;
			}
		}
		return Rings;
	}

	template <typename T>
	T* Part(UUserWidget* W, const TCHAR* Name)
	{
		return W && W->WidgetTree ? Cast<T>(W->WidgetTree->FindWidget(FName(Name))) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawChromeTest, "Pawpocalypse.UI.Chrome", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawChromeTest::RunTest(const FString& Parameters)
{
	using namespace PawUi;

	// 1. The table: every piece is Godot's, its texture imported at the SVG's own size.
	TestEqual(TEXT("one Godot row per piece"), static_cast<int32>(UE_ARRAY_COUNT(Godot)), static_cast<int32>(EPawChrome::Count));
	for (const FGodotChrome& G : Godot)
	{
		const FSlateBrush B = Chrome(G.Which);
		const FString Problem = ChromeProblem(B, G.Which);
		TestTrue(FString::Printf(TEXT("%s: %s"), G.Texture, Problem.IsEmpty() ? TEXT("Godot's") : *Problem), Problem.IsEmpty());
		const UTexture2D* Tex = Cast<UTexture2D>(B.GetResourceObject());
		if (TestNotNull(FString::Printf(TEXT("%s imported (py tools/import_art.py --run chrome)"), G.Texture), Tex))
		{
			TestEqual(FString::Printf(TEXT("%s is the SVG's own size"), G.Texture),
				FIntPoint(Tex->Source.GetSizeX(), Tex->Source.GetSizeY()), FIntPoint(G.W, G.H));
			TestEqual(FString::Printf(TEXT("%s: UI texture, never mipped"), G.Texture), Tex->MipGenSettings.GetValue(),
				TextureMipGenSettings::TMGS_NoMipmaps);
		}
	}
	// Godot's own rule (ci/check_ui_theme.gd): the margins fit the shortest control that wears them (Button 34, Panel 42,
	// WindowPanel 240, ProgressBar 10 px). The separator's 2 + 2 fill its 4 px exactly: why its line never shows (F41).
	const TPair<EPawChrome, int32> Shortest[] = { { EPawChrome::ButtonNormal, 34 }, { EPawChrome::Panel, 42 },
		{ EPawChrome::Window, 240 }, { EPawChrome::BarTrack, 10 }, { EPawChrome::BarFill, 10 } };
	for (const TPair<EPawChrome, int32>& S : Shortest)
	{
		const FPawChromeSpec& Spec = ChromeSpec(S.Key);
		TestTrue(FString::Printf(TEXT("%s: top + bottom margins under %d px"), Spec.Name, S.Value), Spec.Top + Spec.Bottom < S.Value);
	}

	// 2. The looks as the game resolves them: through the style Blueprints, which save no value of their own (step 1 of 5a:
	// a C++ default reaches BP_ButtonStyle). A designer's edit in BP_*Style would show here as a difference, on purpose.
	struct FStateCheck { const TCHAR* State; const FSlateBrush* Brush; EPawChrome Want; };
	auto CheckStyle = [this](UClass* Native, const TCHAR* Label, TArray<FStateCheck> States)
	{
		UClass* Resolved = Resolve(Native);
		TestTrue(FString::Printf(TEXT("%s resolves to its Blueprint"), Label), Resolved && Resolved != Native);
		for (const FStateCheck& C : States)
		{
			const FString Problem = ChromeProblem(*C.Brush, C.Want);
			TestTrue(FString::Printf(TEXT("%s %s: %s"), Label, C.State, Problem.IsEmpty() ? TEXT("the chrome") : *Problem), Problem.IsEmpty());
		}
	};
	{
		const UPawButtonStyle* S = GetDefault<UPawButtonStyle>(Resolve(UPawButtonStyle::StaticClass()));
		CheckStyle(UPawButtonStyle::StaticClass(), TEXT("BP_ButtonStyle"), {
			{ TEXT("normal"), &S->NormalBase, EPawChrome::ButtonNormal }, { TEXT("hovered"), &S->NormalHovered, EPawChrome::ButtonHover },
			{ TEXT("pressed"), &S->NormalPressed, EPawChrome::ButtonPressed }, { TEXT("selected (a toggle held: the open tab)"), &S->SelectedBase, EPawChrome::ButtonPressed },
			{ TEXT("selected hovered"), &S->SelectedHovered, EPawChrome::ButtonPressed }, { TEXT("disabled"), &S->Disabled, EPawChrome::ButtonDisabled } });
		TestEqual(TEXT("BP_ButtonStyle: content margins 16 x 6"), S->ButtonPadding, FMargin(16.0f, 6.0f));
	}
	{
		const UPawLockedButtonStyle* S = GetDefault<UPawLockedButtonStyle>(Resolve(UPawLockedButtonStyle::StaticClass()));
		CheckStyle(UPawLockedButtonStyle::StaticClass(), TEXT("BP_LockedButtonStyle"), {
			{ TEXT("normal"), &S->NormalBase, EPawChrome::ButtonDisabled }, { TEXT("hovered"), &S->NormalHovered, EPawChrome::ButtonDisabled },
			{ TEXT("pressed"), &S->NormalPressed, EPawChrome::ButtonDisabled }, { TEXT("disabled"), &S->Disabled, EPawChrome::ButtonDisabled } });
	}
	{
		// The settings lists: the theme's OptionButton and its PopupMenu.
		const UPawOption* O = GetDefault<UPawOption>();
		const FButtonStyle& B = O->GetWidgetStyle().ComboButtonStyle.ButtonStyle;
		for (const FStateCheck& C : TArray<FStateCheck>{ { TEXT("normal"), &B.Normal, EPawChrome::ButtonNormal },
			{ TEXT("hovered"), &B.Hovered, EPawChrome::ButtonHover }, { TEXT("pressed"), &B.Pressed, EPawChrome::ButtonPressed },
			{ TEXT("disabled"), &B.Disabled, EPawChrome::ButtonDisabled },
			{ TEXT("popup panel"), &O->GetWidgetStyle().ComboButtonStyle.MenuBorderBrush, EPawChrome::Panel },
			{ TEXT("popup hover"), &O->GetItemStyle().EvenRowBackgroundHoveredBrush, EPawChrome::ButtonHover } })
		{
			const FString Problem = ChromeProblem(*C.Brush, C.Want);
			TestTrue(FString::Printf(TEXT("a settings list %s: %s"), C.State, Problem.IsEmpty() ? TEXT("the chrome") : *Problem), Problem.IsEmpty());
		}
	}

	// 3. The asset parts the chrome was restyled into (py tools/bake_widgets.py --graft "<Class>=restyle:<Part>"), and the
	// code layout they came from.
	FChromeWorld W;
	W.Open();
	struct FPartCheck { UClass* Native; const TCHAR* Name; EPawChrome Want; bool bBarFill; };
	const FPartCheck Parts[] = {
		{ UPawJobPicker::StaticClass(), TEXT("Panel"), EPawChrome::Window, false },
		{ UPawVictoryScreen::StaticClass(), TEXT("Panel"), EPawChrome::Window, false },
		{ UPawHudScreen::StaticClass(), TEXT("TabSeparator"), EPawChrome::Separator, false },
		{ UPawHudScreen::StaticClass(), TEXT("WaveBar"), EPawChrome::BarTrack, true },
		{ UPawRainbowBar::StaticClass(), TEXT("Track"), EPawChrome::BarTrack, false }, // the Reawaken bar since 5d
		{ UPawSettingsDialog::StaticClass(), TEXT("SectionSeparator"), EPawChrome::Separator, false },
	};
	for (const bool bCode : { false, true })
	{
		TMap<UClass*, UUserWidget*> Made;
		for (const FPartCheck& P : Parts)
		{
			UUserWidget*& Widget = Made.FindOrAdd(P.Native);
			if (!Widget)
			{
				PawUi::SetCodeLayoutForced(bCode);
				Widget = CreateWidget<UUserWidget>(W.GI, bCode ? P.Native : Resolve(P.Native));
				PawUi::SetCodeLayoutForced(false);
			}
			const FString Where = FString::Printf(TEXT("%s.%s (%s)"), *P.Native->GetName(), P.Name, bCode ? TEXT("code") : TEXT("asset"));
			FString Problem = TEXT("missing");
			if (const UBorder* B = Part<UBorder>(Widget, P.Name))
			{
				Problem = ChromeProblem(B->Background, P.Want);
				if (Problem.IsEmpty() && P.Want == EPawChrome::Window && !(B->GetPadding() == WindowPadding()))
				{
					Problem = TEXT("padding is not SBT_window's content margins 22 / 20");
				}
				if (Problem.IsEmpty() && P.Want == EPawChrome::Separator && FMath::Abs(B->GetPadding().GetTotalSpaceAlong<Orient_Vertical>() - 4.0f) > 0.01f
					&& !Part<USizeBox>(Widget, TEXT("SectionSeparatorBox")))
				{
					Problem = TEXT("not 4 px tall (HSeparator: max(separation 4, the style's 2 + 2))");
				}
			}
			else if (const UProgressBar* Bar = Part<UProgressBar>(Widget, P.Name))
			{
				Problem = ChromeProblem(Bar->GetWidgetStyle().BackgroundImage, P.Want);
				if (Problem.IsEmpty() && P.bBarFill)
				{
					Problem = ChromeProblem(Bar->GetWidgetStyle().FillImage, EPawChrome::BarFill);
				}
			}
			TestTrue(Where + TEXT(": ") + (Problem.IsEmpty() ? TEXT("the chrome") : Problem), Problem.IsEmpty());
		}
		if (const USizeBox* Box = Part<USizeBox>(Made.FindRef(UPawSettingsDialog::StaticClass()), TEXT("SectionSeparatorBox")))
		{
			TestEqual(FString::Printf(TEXT("the settings separator is 4 px tall (%s)"), bCode ? TEXT("code") : TEXT("asset")),
				Box->GetHeightOverride(), 4.0f);
		}
		else
		{
			AddError(TEXT("the settings dialog has no SectionSeparatorBox"));
		}
	}

	// 4. The selection ring (SBF_focus): painted over a button only while it is selected (hovered), 3 px outside it.
	{
		const FSlateBrush Ring = FocusRing();
		TestTrue(TEXT("the ring: a rounded outline, no fill"), Ring.DrawAs == ESlateBrushDrawType::RoundedBox
			&& Ring.TintColor.GetSpecifiedColor().A == 0.0f);
		TestTrue(TEXT("the ring: 3 px of UI_FOCUS, radius 12"), Ring.OutlineSettings.Color.GetSpecifiedColor().Equals(PawTokens::UI_FOCUS)
			&& Ring.OutlineSettings.Width == 3.0f && Ring.OutlineSettings.CornerRadii == FVector4(12.0, 12.0, 12.0, 12.0));
		TestEqual(TEXT("the ring: expand_margin 3"), FocusRingExpand, 3.0f);
		// The rule: hovered, or focused unless the mouse gave the focus (Godot hides mouse focus).
		TestTrue(TEXT("ring: hovered"), UPawButton::RingRule(true, false, true));
		TestTrue(TEXT("ring: focused by keys / a pad"), UPawButton::RingRule(false, true, false));
		TestFalse(TEXT("no ring: focused by the mouse"), UPawButton::RingRule(false, true, true));
		TestFalse(TEXT("no ring: neither"), UPawButton::RingRule(false, false, false));
		for (const bool bLocked : { false, true })
		{
			UPawButton* Button = CreateWidget<UPawButton>(W.GI, Resolve(UPawButton::StaticClass()));
			if (!TestNotNull(TEXT("a button"), Button))
			{
				continue;
			}
			Button->SetLocked(bLocked);
			const TCHAR* Kind = bLocked ? TEXT("a locked button") : TEXT("a button");
			// Held for the whole check: a temporary root frees the tree, and the next TakeWidget builds another.
			const TSharedRef<SWidget> Root = Button->TakeWidget();
			const TSharedPtr<SWidget> Slate = FindSlate(Root, TEXT("SCommonButton"));
			if (!TestTrue(FString::Printf(TEXT("%s has its Common button"), Kind), Slate.IsValid()))
			{
				continue;
			}
			TestFalse(FString::Printf(TEXT("%s: no ring at rest"), Kind), Button->ShowsRing());
			TestEqual(FString::Printf(TEXT("%s: no ring painted at rest"), Kind), RingsPainted(Button), 0);
			if (bLocked)
			{
				// A locked row is never hovered (Common UI's interaction is off): its ring is the focus's (RingRule).
				Slate->OnMouseEnter(FGeometry(), FPointerEvent());
				TestFalse(FString::Printf(TEXT("%s: the mouse does not hover it"), Kind), Button->ShowsRing());
				Slate->OnMouseLeave(FPointerEvent());
				continue;
			}
			Slate->OnMouseEnter(FGeometry(), FPointerEvent());
			TestTrue(FString::Printf(TEXT("%s: the ring when hovered"), Kind), Button->ShowsRing());
			TestEqual(FString::Printf(TEXT("%s: one ring painted when hovered"), Kind), RingsPainted(Button), 1);
			Slate->OnMouseLeave(FPointerEvent());
			TestFalse(FString::Printf(TEXT("%s: the ring goes with the hover"), Kind), Button->ShowsRing());
		}
	}
	W.Close();
	return true;
}

#endif
