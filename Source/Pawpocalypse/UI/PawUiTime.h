#pragma once

#include "CoreMinimal.h"

// The time a widget steps its own motion by (the tweens, the ceremonies, the tree's sky, the rainbow, the card faces).
//
// In the game as players run it: Slate's delta, untouched. Under the engine's fixed step (-UseFixedTimeStep: the store clips,
// tools/store_clips.py, record the game frame by frame, each frame 1/60 s of game time however long its file takes) Slate
// still hands a widget the REAL time since its last tick, and a screenshot with the widgets paints, so ticks, the window a
// second time (measured 2026-11-25: the Unicorn ceremony's 3.75 s lasted about 0.6 s of a take, every UI motion 4-6x fast).
// `Slate.UseFixedDeltaTime 1` does not reach a widget's tick (5.8.3: the paint passes FSlateApplication::GetDeltaTime(), the
// real one). So under a fixed step a widget steps by that step, once per engine frame (0 for a second paint in a frame).
namespace PawUi
{
	PAWPOCALYPSE_API float StepDelta(const UObject* Widget, float SlateDelta);
}
