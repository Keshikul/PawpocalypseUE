#pragma once

// The small kit every screen is built from (docs/05 §7; KB U/09 §4): rounded-box brushes with an ink frame (no nine-slice
// art), text blocks in the palette, the progress bar. Colours come only from PawTokens (generated); sizes from
// PawUiConstants.

#include "CoreMinimal.h"
#include "Layout/Margin.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

class UTextBlock;
class UProgressBar;
class UWidget;
class UWidgetTree;

namespace PawUi
{
	// Godot's font_size is pixels; Slate's is points at 96 DPI.
	inline float FontPt(double GodotPx) { return static_cast<float>(GodotPx * 0.75); }

	// A filled rounded box with an ink frame (Width 0 = no frame).
	FSlateBrush Rounded(const FLinearColor& Fill, const FLinearColor& Frame, float FrameWidth, float Radius);
	// The same with its own corner radii (top-left, top-right, bottom-right, bottom-left): a card's header band.
	FSlateBrush RoundedCorners(const FLinearColor& Fill, const FLinearColor& Frame, float FrameWidth, const FVector4& Radii);
	// A brush that draws nothing (a default FSlateBrush draws white: KB U/09 §3).
	FSlateBrush NoBrush();
	// The theme's WindowPanel plate (paper, ink frame; PawUiConstants Window*) and its content margins.
	FSlateBrush WindowBrush();
	FMargin WindowPadding();

	// A text block in the palette. bOverField: the light fill and ink outline every label over the live field wears
	// (hud.gd _outline_over_playfield); the dock's text sits on paper and needs none.
	UTextBlock* MakeText(UWidgetTree* Tree, double GodotPx, const FLinearColor& Colour, bool bOverField = false);
	void SetTextColour(UTextBlock* Text, const FLinearColor& Colour);

	// A bar: a framed track and a rounded fill. Never takes input.
	UProgressBar* MakeBar(UWidgetTree* Tree, const FLinearColor& Track, const FLinearColor& Fill, const FLinearColor& Frame,
		float FrameWidth, float Radius);
	void SetBarFill(UProgressBar* Bar, const FLinearColor& Fill, float Radius);

	// MenuFocus.wire() (menu_focus.gd): a screen described as ROWS (top to bottom, each left to right) gets explicit
	// navigation on every widget, wrapping at every edge so focus never leaves the screen. Collapsed or hidden widgets and
	// rows left empty drop out first. Up / down land on the widget of the next row nearest in x, rows centred and their
	// widgets of equal width (the card pick's hand, its Banish row, the Reroll button): Godot measures the laid-out rects,
	// which for such rows is the same as comparing (k + 0.5) / count. Returns the wired widgets in reading order.
	TArray<UWidget*> WireRows(const TArray<TArray<UWidget*>>& Rows);
	// The widget of Row nearest (by centred share) to the From-th of FromCount.
	int32 NearestInRow(int32 RowCount, int32 From, int32 FromCount);
}
