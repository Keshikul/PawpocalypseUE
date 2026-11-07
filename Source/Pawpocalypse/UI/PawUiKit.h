#pragma once

// The small kit every screen is built from (docs/05 §7; KB U/09 §4): the theme's nine-patch chrome where Godot's theme draws
// it (phase 6 slice 5a), rounded-box brushes with an ink frame where Godot draws a StyleBoxFlat, text blocks in the palette,
// the progress bar. Colours come only from PawTokens (generated); sizes from PawUiConstants.

#include "CoreMinimal.h"
#include "Layout/Margin.h"
#include "Styling/SlateBrush.h"
#include "Rendering/SlateRenderTransform.h"
#include "Styling/SlateTypes.h"

class FPawFxCanvas;
class FSlateWindowElementList;
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

	// THE CHROME (phase 6 slice 5a): pawpocalypse_theme.tres's StyleBoxTextures over Godot's own SVGs, rasterised at the SVG's
	// own size (tools/raster_svg.py group chrome; tools/import_art.py chrome -> /Game/Pawpocalypse/Art/UI/Chrome/T_<name>).
	// Godot draws texture_margin 1:1 in the source and on screen; a Box brush does the same with ImageSize = the SVG's size and
	// Margin = texture_margin / size per side. Only what Godot's theme draws wears it: a screen that overrides the theme with
	// a StyleBoxFlat (the dock, the title's and the pause menu's windows, the toast, the boss panel) keeps its Rounded brush.
	// ui_dock.svg has no entry: hud.tscn's DockPanel overrides it, so Godot draws it nowhere.
	enum class EPawChrome : uint8
	{
		ButtonNormal, ButtonHover, ButtonPressed, ButtonDisabled, // Button / OptionButton (and PopupMenu's hover)
		Panel,                                                    // Panel, PanelContainer, PopupMenu's panel
		Window,                                                   // the WindowPanel variation (the job picker, the victory)
		BarTrack, BarFill,                                        // ProgressBar background / fill
		Separator,                                                // HSeparator
		Count
	};
	struct FPawChromeSpec
	{
		const TCHAR* Name;        // the SVG's stem; the texture is T_<Name>
		int32 Width, Height;      // the SVG's size, px
		int32 Left, Top, Right, Bottom; // texture_margin_*, px
	};
	const FPawChromeSpec& ChromeSpec(EPawChrome Which);
	FString ChromeTexturePath(EPawChrome Which); // the object path of T_<Name>
	// The Box brush for a piece of chrome; its texture loaded once (a missing texture leaves the brush empty: the
	// Pawpocalypse.UI.Chrome test names it).
	FSlateBrush Chrome(EPawChrome Which);
	// The theme's focus ring (SBF_focus): no fill, 3 px UI_FOCUS, corner radius 12, drawn FocusRingExpand px outside the
	// control (expand_margin 3). A button paints it over its plate when selected (UPawButton::NativePaint).
	FSlateBrush FocusRing();
	inline constexpr float FocusRingExpand = 3.0f;

	// Godot's Color.from_hsv(h, s, v) (sRGB, as every Godot colour), in Slate's linear space.
	FLinearColor FromHsv(double H, double S, double V);

	// The theme's WindowPanel plate (Chrome Window) and its content margins (SBT_window: 22 / 20).
	FSlateBrush WindowBrush();
	FMargin WindowPadding();

	// A text block in the palette. bOverField: the light fill and ink outline every label over the live field wears
	// (hud.gd _outline_over_playfield); the dock's text sits on paper and needs none.
	// Name: the widget's name (a bound part carries its property's name, so a Widget Blueprint binds it: PawWidgets.h).
	UTextBlock* MakeText(UWidgetTree* Tree, double GodotPx, const FLinearColor& Colour, bool bOverField = false, FName Name = NAME_None);
	void SetTextColour(UTextBlock* Text, const FLinearColor& Colour);

	// A bar: a framed track and a rounded fill. Never takes input.
	UProgressBar* MakeBar(UWidgetTree* Tree, const FLinearColor& Track, const FLinearColor& Fill, const FLinearColor& Frame,
		float FrameWidth, float Radius, FName Name = NAME_None);
	void SetBarFill(UProgressBar* Bar, const FLinearColor& Fill, float Radius);
	// The theme's ProgressBar: the chrome's track and fill (Godot's bars that keep the theme: the wave bar).
	UProgressBar* MakeThemeBar(UWidgetTree* Tree, FName Name = NAME_None);
	// A Godot ProgressBar's value and fill (phase 6 slice 5d). Range.set_value snaps the value to the bar's step from 0 (a
	// ProgressBar's default 0.01; the XP bar's 0.0001) and holds it in 0..1; Godot then draws the fill StyleBox
	// round(r * (w - m)) + m wide, nothing below one px (m: the fill style's minimum width: 16 for the theme's nine-patch, 0
	// for a flat fill), both caps kept: MakeBar sets UMG's Scale fill style (Mask cut the cap off square). Needs the bar's
	// laid-out width (before its first layout: the plain ratio). Returns the value the bar kept.
	double SetBarValue(UProgressBar* Bar, double Value, double Step, double FillMinWidth);
	double ThemeFillMinWidth();
	inline constexpr double XpBarStep = 0.0001; // hud.tscn XPBar/Bar step

	// Paints a canvas of Godot draw calls (FPawFxCanvas: phase 6's effects, the rainbows, the tree's sky) in Slate at Layer,
	// in the space of Transform (a geometry's accumulated render transform); its sRGB vertex colours as they are.
	void PaintCanvas(const FPawFxCanvas& Canvas, FSlateWindowElementList& Out, int32 Layer, const FSlateRenderTransform& Transform);

	// MenuFocus.wire() (menu_focus.gd): a screen described as ROWS (top to bottom, each left to right) gets explicit
	// navigation on every widget, wrapping at every edge so focus never leaves the screen. Collapsed or hidden widgets and
	// rows left empty drop out first. Up / down land on the widget of the next row nearest in x, rows centred and their
	// widgets of equal width (the card pick's hand, its Banish row, the Reroll button): Godot measures the laid-out rects,
	// which for such rows is the same as comparing (k + 0.5) / count. Returns the wired widgets in reading order.
	TArray<UWidget*> WireRows(const TArray<TArray<UWidget*>>& Rows);
	// The widget of Row nearest (by centred share) to the From-th of FromCount.
	int32 NearestInRow(int32 RowCount, int32 From, int32 FromCount);
}
