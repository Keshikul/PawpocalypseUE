#include "UI/PawLayoutCheck.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Engine/UserInterfaceSettings.h"
#include "Input/HittestGrid.h"
#include "Layout/ArrangedChildren.h"
#include "Misc/App.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	struct FName2
	{
		FString Path;
		UWidget* Widget = nullptr;
	};

	void Index(UUserWidget* Owner, const FString& Prefix, TMap<const SWidget*, FName2>& Out);

	// Every part of every widget tree (a user widget's own parts under its instance's name) by its Slate widget.
	void IndexTree(UWidgetTree* Tree, const FString& Prefix, TMap<const SWidget*, FName2>& Out)
	{
		if (!Tree)
		{
			return;
		}
		Tree->ForEachWidget([&](UWidget* W)
		{
			if (!W)
			{
				return;
			}
			const FString Path = Prefix + W->GetName();
			if (const TSharedPtr<SWidget> S = W->GetCachedWidget())
			{
				Out.FindOrAdd(S.Get(), FName2{ Path, W });
			}
			if (UUserWidget* Child = Cast<UUserWidget>(W))
			{
				Index(Child, Path + TEXT("/"), Out);
			}
		});
	}

	void Index(UUserWidget* Owner, const FString& Prefix, TMap<const SWidget*, FName2>& Out)
	{
		if (Owner)
		{
			IndexTree(Owner->WidgetTree, Prefix, Out);
		}
	}

	// Lays S out (Frames painted frames, then a prepass), names its parts with IndexNames and walks it (below).
	TArray<PawLayout::FPart> ArrangeSlate(const TSharedRef<SWidget>& S, TMap<const SWidget*, FName2>& Names, FVector2D Screen,
		float Scale, int32 Frames, TFunctionRef<void(TMap<const SWidget*, FName2>&)> IndexNames);

	void Walk(const TSharedRef<SWidget>& S, const FGeometry& G, const FSlateRect& Clip, const TMap<const SWidget*, FName2>& Names,
		TArray<PawLayout::FPart>& Out, int32 Parent, bool bTilted)
	{
		const FSlateRect R = G.GetRenderBoundingRect();
		if (const TOptional<FSlateRenderTransform>& T = S->GetRenderTransform(); T.IsSet())
		{
			float A = 1.0f, B = 0.0f, C = 0.0f, D = 1.0f;
			T->GetMatrix().GetMatrix(A, B, C, D);
			bTilted = bTilted || !FMath::IsNearlyZero(B) || !FMath::IsNearlyZero(C);
		}
		bool bShown = false;
		const FSlateRect Shown = Clip.IntersectionWith(R, bShown);
		if (const FName2* Name = Names.Find(&S.Get()))
		{
			PawLayout::FPart P;
			P.Path = Name->Path;
			P.Rect = R;
			P.Shown = Shown;
			P.bShown = bShown;
			P.Layout = G.GetLayoutBoundingRect();
			P.bTilted = bTilted;
			P.Parent = Parent;
			P.Widget = Name->Widget;
			P.Desired = S->GetDesiredSize();
			P.Scale = G.Scale;
			Parent = Out.Add(P);
		}
		const FSlateRect Inner = S->GetClipping() != EWidgetClipping::Inherit ? Shown : Clip;
		FArrangedChildren Arranged(EVisibility::Visible);
		S->ArrangeChildren(G, Arranged);
		for (int32 I = 0; I < Arranged.Num(); ++I)
		{
			Walk(Arranged[I].Widget, Arranged[I].Geometry, Inner, Names, Out, Parent, bTilted);
		}
	}
}

namespace PawLayout
{
	TArray<FPart> Arrange(UUserWidget* Widget, FVector2D Screen, float Scale, int32 Frames)
	{
		if (!Widget)
		{
			return TArray<FPart>();
		}
		const TSharedRef<SWidget> S = Widget->TakeWidget();
		TMap<const SWidget*, FName2> Names;
		return ArrangeSlate(S, Names, Screen, Scale, Frames, [Widget](TMap<const SWidget*, FName2>& N) { Index(Widget, FString(), N); });
	}

	TArray<FPart> ArrangeTree(UWidgetTree* Tree, FVector2D Screen, float Scale, int32 Frames)
	{
		if (!Tree || !Tree->RootWidget)
		{
			return TArray<FPart>();
		}
		const TSharedRef<SWidget> S = Tree->RootWidget->TakeWidget();
		TMap<const SWidget*, FName2> Names;
		return ArrangeSlate(S, Names, Screen, Scale, Frames, [Tree](TMap<const SWidget*, FName2>& N) { IndexTree(Tree, FString(), N); });
	}
}

namespace
{
	TArray<PawLayout::FPart> ArrangeSlate(const TSharedRef<SWidget>& S, TMap<const SWidget*, FName2>& Names, FVector2D Screen,
		float Scale, int32 Frames, TFunctionRef<void(TMap<const SWidget*, FName2>&)> IndexNames)
	{
		TArray<PawLayout::FPart> Out;
		const FGeometry Root = FGeometry::MakeRoot(Screen / Scale, FSlateLayoutTransform(Scale));
		const FSlateRect Whole(0.0f, 0.0f, static_cast<float>(Screen.X), static_cast<float>(Screen.Y));
		const TSharedRef<SWindow> Window = SNew(SWindow);
		for (int32 F = 0; F < Frames; ++F)
		{
			S->SlatePrepass(Scale);
			FSlateWindowElementList List(Window);
			FHittestGrid Grid;
			FPaintArgs Args(&Window.Get(), Grid, FVector2D::ZeroVector, FApp::GetCurrentTime() + F * 0.016, 0.016f);
			S->Paint(Args, Root, Whole, List, 0, FWidgetStyle(), true);
		}
		S->SlatePrepass(Scale);
		IndexNames(Names);
		// Shown is what the part's ANCESTORS let through (a scroll box), never the screen: a part past the edge must show as such.
		const FSlateRect Unclipped(-1.0e7f, -1.0e7f, 1.0e7f, 1.0e7f);
		Walk(S, Root, Unclipped, Names, Out, INDEX_NONE, false);
		return Out;
	}
}

namespace
{
	void WalkWords(const TSharedRef<SWidget>& S, const FGeometry& G, const TMap<const SWidget*, FName2>& Names, const FString& Path,
		TArray<PawLayout::FWords>& Out)
	{
		const FName2* Name = Names.Find(&S.Get());
		const FName Type = S->GetType();
		const FString Here = Name ? Name->Path : Path + TEXT("/") + Type.ToString();
		FString Says;
		if (Type == TEXT("STextBlock"))
		{
			Says = StaticCastSharedRef<STextBlock>(S)->GetText().ToString();
		}
		else if (Type == TEXT("SRichTextBlock"))
		{
			Says = StaticCastSharedRef<SRichTextBlock>(S)->GetText().ToString();
		}
		Says.TrimStartAndEndInline();
		if (!Says.IsEmpty())
		{
			Out.Add({ Here, Says });
		}
		if (Name && Name->Widget)
		{
			FString Tip = Name->Widget->GetToolTipText().ToString();
			Tip.TrimStartAndEndInline();
			if (!Tip.IsEmpty())
			{
				Out.Add({ Here + TEXT(" (tooltip)"), Tip });
			}
		}
		FArrangedChildren Arranged(EVisibility::Visible);
		S->ArrangeChildren(G, Arranged);
		for (int32 I = 0; I < Arranged.Num(); ++I)
		{
			WalkWords(Arranged[I].Widget, Arranged[I].Geometry, Names, Name ? Name->Path : Path, Out);
		}
	}
}

namespace PawLayout
{
	TArray<FWords> ArrangeWords(UUserWidget* Widget, FVector2D Screen, float Scale, int32 Frames)
	{
		TArray<FWords> Out;
		if (!Widget)
		{
			return Out;
		}
		const TSharedRef<SWidget> S = Widget->TakeWidget();
		TMap<const SWidget*, FName2> Names;
		ArrangeSlate(S, Names, Screen, Scale, Frames, [Widget](TMap<const SWidget*, FName2>& N) { Index(Widget, FString(), N); });
		WalkWords(S, FGeometry::MakeRoot(Screen / Scale, FSlateLayoutTransform(Scale)), Names, FString(), Out);
		return Out;
	}

	float GameScale(FVector2D Screen, float UiSize)
	{
		const FIntPoint Size(FMath::RoundToInt(Screen.X), FMath::RoundToInt(Screen.Y));
		return GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(Size) * UiSize;
	}

	FSlateRect Footprint(const FPart& P)
	{
		if (!P.bTilted)
		{
			return P.Shown;
		}
		return P.Layout;
	}

	const FPart* Find(const TArray<FPart>& Parts, const FString& Path)
	{
		return Parts.FindByPredicate([&Path](const FPart& P) { return P.Path == Path; });
	}

	TArray<const FPart*> FindAll(const TArray<FPart>& Parts, const FString& Prefix)
	{
		TArray<const FPart*> All;
		for (const FPart& P : Parts)
		{
			if (P.Path.StartsWith(Prefix))
			{
				All.Add(&P);
			}
		}
		return All;
	}

	bool IsUnder(const TArray<FPart>& Parts, int32 Index, const FString& Ancestor)
	{
		for (int32 I = Parts.IsValidIndex(Index) ? Parts[Index].Parent : INDEX_NONE; Parts.IsValidIndex(I); I = Parts[I].Parent)
		{
			const FString& Path = Parts[I].Path;
			int32 Slash = INDEX_NONE;
			const FString Last = Path.FindLastChar(TEXT('/'), Slash) ? Path.Mid(Slash + 1) : Path;
			if (Last == Ancestor)
			{
				return true;
			}
		}
		return false;
	}

	const FPart* FindWidget(const TArray<FPart>& Parts, const UWidget* Widget)
	{
		return Parts.FindByPredicate([Widget](const FPart& P) { return P.Widget.Get() == Widget; });
	}

	bool Overlap(const FSlateRect& A, const FSlateRect& B, float Slack)
	{
		const float W = FMath::Min(A.Right, B.Right) - FMath::Max(A.Left, B.Left);
		const float H = FMath::Min(A.Bottom, B.Bottom) - FMath::Max(A.Top, B.Top);
		return W > Slack && H > Slack;
	}

	bool OnScreen(const FSlateRect& R, FVector2D Screen, float Slack)
	{
		return R.Left >= -Slack && R.Top >= -Slack && R.Right <= Screen.X + Slack && R.Bottom <= Screen.Y + Slack;
	}

	FString Describe(const FSlateRect& R)
	{
		return FString::Printf(TEXT("(%.0f, %.0f)-(%.0f, %.0f)"), R.Left, R.Top, R.Right, R.Bottom);
	}

	TArray<FTextMiss> TextMisses(const TArray<FPart>& Parts, float Slack)
	{
		TArray<FTextMiss> Out;
		for (const FPart& P : Parts)
		{
			const UWidget* W = P.Widget.Get();
			FString Text;
			if (const UTextBlock* T = Cast<UTextBlock>(W))
			{
				Text = T->GetText().ToString();
			}
			else if (const URichTextBlock* R = Cast<URichTextBlock>(W))
			{
				Text = R->GetText().ToString();
			}
			else
			{
				continue;
			}
			if (!P.bShown || Text.TrimStartAndEnd().IsEmpty())
			{
				continue;
			}
			// The box before any render transform (a pop or a tilt changes what is drawn, not what the text was given).
			const float BoxW = P.Layout.Right - P.Layout.Left, BoxH = P.Layout.Bottom - P.Layout.Top;
			const float NeedW = static_cast<float>(P.Desired.X) * P.Scale, NeedH = static_cast<float>(P.Desired.Y) * P.Scale;
			const bool bWholeHeight = P.Shown.Top <= P.Rect.Top + Slack && P.Shown.Bottom >= P.Rect.Bottom - Slack;
			FString Why;
			if (NeedW > BoxW + Slack)
			{
				Why = FString::Printf(TEXT("needs %.0f px wide, has %.0f"), NeedW, BoxW);
			}
			// Taller: only by a line's worth (a quarter of the box). A display face's line height runs a little over the
			// box it was sized for while its letters fit (the logo, the ceremony's headline): that is not a text cut short.
			else if (NeedH > BoxH * 1.25f + Slack)
			{
				Why = FString::Printf(TEXT("needs %.0f px high, has %.0f"), NeedH, BoxH);
			}
			else if (bWholeHeight && (P.Shown.Left > P.Rect.Left + Slack || P.Shown.Right < P.Rect.Right - Slack))
			{
				Why = FString::Printf(TEXT("cut sideways: %s of %s shows"), *Describe(P.Shown), *Describe(P.Rect));
			}
			if (!Why.IsEmpty())
			{
				Out.Add(FTextMiss{ P.Path, Text.Replace(TEXT("\n"), TEXT(" / ")), Why });
			}
		}
		return Out;
	}
}

#endif
