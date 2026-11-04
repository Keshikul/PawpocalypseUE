#include "UI/PawUiKit.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"

namespace PawUi
{
	FSlateBrush Rounded(const FLinearColor& Fill, const FLinearColor& Frame, float FrameWidth, float Radius)
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.TintColor = FSlateColor(Fill);
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		B.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
		B.OutlineSettings.Color = FSlateColor(Frame);
		B.OutlineSettings.Width = FrameWidth;
		return B;
	}

	FSlateBrush RoundedCorners(const FLinearColor& Fill, const FLinearColor& Frame, float FrameWidth, const FVector4& Radii)
	{
		FSlateBrush B = Rounded(Fill, Frame, FrameWidth, 0.0f);
		B.OutlineSettings.CornerRadii = Radii;
		return B;
	}

	FSlateBrush NoBrush()
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::NoDrawType;
		return B;
	}

	FSlateBrush WindowBrush()
	{
		return Rounded(PawTokens::UI_PANEL, PawTokens::UI_PANEL_FRAME, static_cast<float>(WindowFrame), static_cast<float>(WindowRadius));
	}

	FMargin WindowPadding()
	{
		return FMargin(static_cast<float>(WindowPadX), static_cast<float>(WindowPadY));
	}

	UTextBlock* MakeText(UWidgetTree* Tree, double GodotPx, const FLinearColor& Colour, bool bOverField)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = T->GetFont();
		Font.Size = FontPt(GodotPx);
		if (bOverField)
		{
			Font.OutlineSettings.OutlineSize = static_cast<int32>(OverFieldOutline);
			Font.OutlineSettings.OutlineColor = PawTokens::INK;
		}
		T->SetFont(Font);
		T->SetColorAndOpacity(FSlateColor(bOverField ? PawTokens::UI_PANEL : Colour));
		T->SetVisibility(ESlateVisibility::HitTestInvisible);
		return T;
	}

	void SetTextColour(UTextBlock* Text, const FLinearColor& Colour)
	{
		if (Text)
		{
			Text->SetColorAndOpacity(FSlateColor(Colour));
		}
	}

	UProgressBar* MakeBar(UWidgetTree* Tree, const FLinearColor& Track, const FLinearColor& Fill, const FLinearColor& Frame,
		float FrameWidth, float Radius)
	{
		UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>();
		FProgressBarStyle Style = Bar->GetWidgetStyle();
		Style.BackgroundImage = Rounded(Track, Frame, FrameWidth, Radius);
		Style.FillImage = Rounded(Fill, Fill, 0.0f, Radius);
		Style.MarqueeImage = NoBrush();
		Bar->SetWidgetStyle(Style);
		Bar->SetFillColorAndOpacity(FLinearColor::White);
		Bar->SetPercent(0.0f);
		Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Bar;
	}

	void SetBarFill(UProgressBar* Bar, const FLinearColor& Fill, float Radius)
	{
		if (!Bar)
		{
			return;
		}
		FProgressBarStyle Style = Bar->GetWidgetStyle();
		Style.FillImage = Rounded(Fill, Fill, 0.0f, Radius);
		Bar->SetWidgetStyle(Style);
	}

	int32 NearestInRow(int32 RowCount, int32 From, int32 FromCount)
	{
		const double X = (From + 0.5) / FMath::Max(FromCount, 1);
		int32 Best = 0;
		double BestD = TNumericLimits<double>::Max();
		for (int32 K = 0; K < RowCount; ++K)
		{
			const double D = FMath::Abs((K + 0.5) / RowCount - X);
			if (D < BestD - 1e-9) // a tie goes to the first, as Godot's strict <
			{
				BestD = D;
				Best = K;
			}
		}
		return Best;
	}

	TArray<UWidget*> WireRows(const TArray<TArray<UWidget*>>& Rows)
	{
		TArray<TArray<UWidget*>> Clean;
		for (const TArray<UWidget*>& Row : Rows)
		{
			TArray<UWidget*> Kept;
			for (UWidget* W : Row)
			{
				if (W && W->GetVisibility() != ESlateVisibility::Collapsed && W->GetVisibility() != ESlateVisibility::Hidden)
				{
					Kept.Add(W);
				}
			}
			if (Kept.Num() > 0)
			{
				Clean.Add(MoveTemp(Kept));
			}
		}
		TArray<UWidget*> Flat;
		const int32 N = Clean.Num();
		for (int32 I = 0; I < N; ++I)
		{
			const TArray<UWidget*>& Row = Clean[I];
			const TArray<UWidget*>& Above = Clean[(I - 1 + N) % N];
			const TArray<UWidget*>& Below = Clean[(I + 1) % N];
			for (int32 J = 0; J < Row.Num(); ++J)
			{
				UWidget* W = Row[J];
				Flat.Add(W);
				W->SetNavigationRuleExplicit(EUINavigation::Left, Row[(J - 1 + Row.Num()) % Row.Num()]);
				W->SetNavigationRuleExplicit(EUINavigation::Right, Row[(J + 1) % Row.Num()]);
				W->SetNavigationRuleExplicit(EUINavigation::Up, Above[NearestInRow(Above.Num(), J, Row.Num())]);
				W->SetNavigationRuleExplicit(EUINavigation::Down, Below[NearestInRow(Below.Num(), J, Row.Num())]);
			}
		}
		for (int32 K = 0; K < Flat.Num(); ++K)
		{
			Flat[K]->SetNavigationRuleExplicit(EUINavigation::Next, Flat[(K + 1) % Flat.Num()]);
			Flat[K]->SetNavigationRuleExplicit(EUINavigation::Previous, Flat[(K - 1 + Flat.Num()) % Flat.Num()]);
		}
		return Flat;
	}
}
