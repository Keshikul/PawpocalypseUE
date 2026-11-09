// PawLayout: where a screen's parts land on a screen of a given size at a given UI size, measured headless (R4h, 2026-11-08).
// The tests lay a screen out the way a frame does (prepass, paint, which ticks, then arrange) and read every named part's
// rectangle in screen pixels, clipping included. Pawpocalypse.UI.Layout.* use it to prove nothing leaves the screen or
// hides under another part at 100 % and 150 % (WORKLIST-RESTYLE R4's exit). Tests only.
#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Layout/SlateRect.h"

class UUserWidget;
class UWidget;

namespace PawLayout
{
	struct FPart
	{
		FString Path;      // the part's name; a part inside a user widget: "Instance/Part"
		FSlateRect Rect;   // where it is drawn, render transform included, in screen px
		FSlateRect Shown;  // what its clipping ancestors let through
		bool bShown = false; // false: clipped away entirely
		FSlateRect Layout; // where its box is before any render transform (a tilted logo: its untilted box)
		bool bTilted = false; // it or an ancestor is rotated (a render transform with a turn, not only a scale)
		int32 Parent = INDEX_NONE;     // the nearest listed ancestor's index
		TWeakObjectPtr<UWidget> Widget; // the part itself
	};

	// True when Parts[Index] lies inside a listed part named Ancestor (its path's last name).
	bool IsUnder(const TArray<FPart>& Parts, int32 Index, const FString& Ancestor);
	const FPart* FindWidget(const TArray<FPart>& Parts, const UWidget* Widget);

	// The scale the game lays out at on a Screen px window at UiSize (1.5 = 150 %): the project's DPI curve times the size.
	float GameScale(FVector2D Screen, float UiSize);

	// Screen: the window in px (1920 x 1080); Scale: the whole layout scale (GameScale). Painted Frames times first (paint
	// ticks the widgets, as the game's first frames do), then arranged. Only painted parts are listed (collapsed and hidden
	// ones are not).
	TArray<FPart> Arrange(UUserWidget* Widget, FVector2D Screen, float Scale, int32 Frames = 2);

	const FPart* Find(const TArray<FPart>& Parts, const FString& Path);
	// Every listed part whose path starts with Prefix.
	TArray<const FPart*> FindAll(const TArray<FPart>& Parts, const FString& Prefix);

	// True when A and B share more than Slack px both ways.
	bool Overlap(const FSlateRect& A, const FSlateRect& B, float Slack = 1.0f);
	// What a part is judged by against the screen's edge: what shows of it; a tilted part, its untilted box (a few degrees
	// of tilt lift a long logo's corner off the screen while its letters stay on).
	FSlateRect Footprint(const FPart& P);
	// True when R lies inside the screen (Slack px of grace).
	bool OnScreen(const FSlateRect& R, FVector2D Screen, float Slack = 1.0f);
	FString Describe(const FSlateRect& R);
}

#endif
