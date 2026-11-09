#include "UI/PawLayoutCheck.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/UserInterfaceSettings.h"
#include "Input/HittestGrid.h"
#include "Layout/ArrangedChildren.h"
#include "Misc/App.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SWindow.h"

namespace
{
	struct FName2
	{
		FString Path;
		UWidget* Widget = nullptr;
	};

	// Every part of every widget tree (a user widget's own parts under its instance's name) by its Slate widget.
	void Index(UUserWidget* Owner, const FString& Prefix, TMap<const SWidget*, FName2>& Out)
	{
		if (!Owner || !Owner->WidgetTree)
		{
			return;
		}
		Owner->WidgetTree->ForEachWidget([&](UWidget* W)
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
		TArray<FPart> Out;
		if (!Widget)
		{
			return Out;
		}
		const TSharedRef<SWidget> S = Widget->TakeWidget();
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
		TMap<const SWidget*, FName2> Names;
		Index(Widget, FString(), Names);
		// Shown is what the part's ANCESTORS let through (a scroll box), never the screen: a part past the edge must show as such.
		const FSlateRect Unclipped(-1.0e7f, -1.0e7f, 1.0e7f, 1.0e7f);
		Walk(S, Root, Unclipped, Names, Out, INDEX_NONE, false);
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
}

#endif
