#include "UI/PawUiKit.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "PcRainbow.h"
#include "Render/PawFxCanvas.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
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

	namespace
	{
		// pawpocalypse_theme.tres (the SBT_* StyleBoxTextures) and the SVGs' sizes, 2026-11-06.
		const FPawChromeSpec ChromeTable[] = {
			{ TEXT("ui_button_normal"), 256, 64, 16, 16, 16, 16 },
			{ TEXT("ui_button_hover"), 256, 64, 16, 16, 16, 16 },
			{ TEXT("ui_button_pressed"), 256, 64, 16, 16, 16, 16 },
			{ TEXT("ui_button_disabled"), 256, 64, 16, 16, 16, 16 },
			{ TEXT("ui_panel"), 128, 128, 20, 20, 20, 20 },
			{ TEXT("ui_window"), 256, 256, 48, 48, 48, 48 },
			{ TEXT("ui_bar_track"), 64, 16, 8, 4, 8, 4 },
			{ TEXT("ui_bar_fill"), 64, 16, 8, 4, 8, 4 },
			{ TEXT("ui_separator"), 64, 8, 8, 2, 8, 2 },
		};
		static_assert(UE_ARRAY_COUNT(ChromeTable) == static_cast<int32>(EPawChrome::Count), "one row per EPawChrome");

		const TCHAR* const ChromeDir = TEXT("/Game/Pawpocalypse/Art/UI/Chrome");
	}

	const FPawChromeSpec& ChromeSpec(EPawChrome Which)
	{
		return ChromeTable[FMath::Clamp(static_cast<int32>(Which), 0, static_cast<int32>(EPawChrome::Count) - 1)];
	}

	FString ChromeTexturePath(EPawChrome Which)
	{
		const FString Name = FString(TEXT("T_")) + ChromeSpec(Which).Name;
		return FString(ChromeDir) / Name + TEXT(".") + Name;
	}

	FSlateBrush Chrome(EPawChrome Which)
	{
		const FPawChromeSpec& S = ChromeSpec(Which);
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::Box;
		B.Tiling = ESlateBrushTileType::NoTile;
		B.TintColor = FSlateColor(FLinearColor::White);
		B.ImageSize = FVector2D(S.Width, S.Height);
		B.Margin = FMargin(static_cast<float>(S.Left) / S.Width, static_cast<float>(S.Top) / S.Height,
			static_cast<float>(S.Right) / S.Width, static_cast<float>(S.Bottom) / S.Height);
		B.SetResourceObject(LoadObject<UTexture2D>(nullptr, *ChromeTexturePath(Which), nullptr, LOAD_NoWarn | LOAD_Quiet));
		return B;
	}

	FSlateBrush FocusRing()
	{
		return Rounded(FLinearColor::Transparent, PawTokens::UI_FOCUS, 3.0f, 12.0f);
	}

	FSlateBrush WindowBrush()
	{
		return Chrome(EPawChrome::Window);
	}

	FLinearColor FromHsv(double H, double S, double V)
	{
		// core/math/color.cpp Color::set_hsv.
		H = FMath::Fmod(H * 6.0, 6.0);
		const double I = FMath::FloorToDouble(H);
		const double F = H - I;
		const double P = V * (1.0 - S);
		const double Q = V * (1.0 - S * F);
		const double T = V * (1.0 - S * (1.0 - F));
		switch (static_cast<int32>(I))
		{
		case 0: return PawTokens::SrgbF(V, T, P);
		case 1: return PawTokens::SrgbF(Q, V, P);
		case 2: return PawTokens::SrgbF(P, V, T);
		case 3: return PawTokens::SrgbF(P, Q, V);
		case 4: return PawTokens::SrgbF(T, P, V);
		default: return PawTokens::SrgbF(V, P, Q);
		}
	}

	FMargin WindowPadding()
	{
		return FMargin(static_cast<float>(WindowPadX), static_cast<float>(WindowPadY));
	}

	UTextBlock* MakeText(UWidgetTree* Tree, double GodotPx, const FLinearColor& Colour, bool bOverField, FName Name)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
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
		float FrameWidth, float Radius, FName Name)
	{
		UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), Name);
		FProgressBarStyle Style = Bar->GetWidgetStyle();
		Style.BackgroundImage = Rounded(Track, Frame, FrameWidth, Radius);
		Style.FillImage = Rounded(Fill, Fill, 0.0f, Radius);
		Style.MarqueeImage = NoBrush();
		Bar->SetWidgetStyle(Style);
		Bar->SetFillColorAndOpacity(FLinearColor::White);
		// Godot draws the fill StyleBox at the fill's width, both caps kept; UMG's default (Mask) cuts it off square.
		Bar->SetBarFillStyle(EProgressBarFillStyle::Scale);
		Bar->SetPercent(0.0f);
		Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Bar;
	}

	double SetBarValue(UProgressBar* Bar, double Value, double Step, double FillMinWidth)
	{
		const double Kept = Pc::GodotRangeValue(Value, 0.0, 1.0, Step);
		if (!Bar)
		{
			return Kept;
		}
		// ProgressBar::_notification(DRAW): p = round(ratio * (width - m)); drawn p + m wide when p > 0.
		const double W = Bar->GetCachedGeometry().GetLocalSize().X;
		double Percent = Kept;
		if (W > 0.0)
		{
			const double P = FMath::RoundHalfFromZero(Kept * (W - FillMinWidth));
			Percent = P > 0.0 ? FMath::Min((P + FillMinWidth) / W, 1.0) : 0.0;
		}
		Bar->SetPercent(static_cast<float>(Percent));
		return Kept;
	}

	double ThemeFillMinWidth()
	{
		const FPawChromeSpec& S = ChromeSpec(EPawChrome::BarFill);
		return static_cast<double>(S.Left + S.Right);
	}

	void PaintCanvas(const FPawFxCanvas& Canvas, FSlateWindowElementList& Out, int32 Layer, const FSlateRenderTransform& Transform)
	{
		if (Canvas.IsEmpty())
		{
			return;
		}
		TArray<FSlateVertex> Verts;
		Verts.Reserve(Canvas.Verts.Num());
		const FVector2f Uv(0.5f, 0.5f);
		for (int32 I = 0; I < Canvas.Verts.Num(); ++I)
		{
			const FVector& P = Canvas.Verts[I];
			Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(P.X, P.Y), Uv, Canvas.Colors[I]));
		}
		TArray<SlateIndex> Indices;
		Indices.Reserve(Canvas.Indices.Num());
		for (const int32 I : Canvas.Indices)
		{
			Indices.Add(static_cast<SlateIndex>(I));
		}
		const FSlateResourceHandle White =
			FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush("WhiteBrush"));
		FSlateDrawElement::MakeCustomVerts(Out, Layer, White, Verts, Indices, nullptr, 0, 0);
	}

	UProgressBar* MakeThemeBar(UWidgetTree* Tree, FName Name)
	{
		UProgressBar* Bar = MakeBar(Tree, PawTokens::UI_SLOT, PawTokens::GOLD, PawTokens::INK, 0.0f, 0.0f, Name);
		FProgressBarStyle Style = Bar->GetWidgetStyle();
		Style.BackgroundImage = Chrome(EPawChrome::BarTrack);
		Style.FillImage = Chrome(EPawChrome::BarFill);
		Bar->SetWidgetStyle(Style);
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
