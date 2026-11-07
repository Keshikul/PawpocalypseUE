#pragma once

// One half of a stick or trigger axis with Godot's dead zone (docs/05 §3): Godot binds "left stick up past 0.5" as an action,
// and Enhanced Input has no modifier for one direction of an axis. Godot (InputEventJoypadMotion::action_match): the action
// is PRESSED when the axis points this way at least the dead zone, and its STRENGTH is the axis rescaled from the dead zone
// to 1 (inverse_lerp, clamped); measured against Godot's own Input (fixture `input_axis`, Pawpocalypse.Input.HalfAxis).
//
// The output is that strength. At (or just past) the dead zone Godot is pressed with a strength near 0, and Enhanced Input calls
// a value under KINDA_SMALL_NUMBER "not actuated": a pressed axis reads at least PressedFloor (1e-4) so it stays pressed.
//
// The action it feeds must be Axis1D (a Boolean action turns the axis into true / false before modifiers run, and the
// direction is gone): tools/create_input_assets.py makes every action with an axis event Axis1D.

#include "CoreMinimal.h"
#include "InputModifiers.h"

#include "PawHalfAxis.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Paw Half Axis (Godot dead zone)"))
class PAWPOCALYPSE_API UPawHalfAxis : public UInputModifier
{
	GENERATED_BODY()

public:
	// Which half counts: +1 or -1 on Unreal's axis (Unreal's +Y is up; Godot's is down: the exporter flips it).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float Sign = 1.0f;

	// Godot's per-action dead zone.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0", ClampMax = "1"))
	float DeadZone = 0.5f;

	// The least value a pressed axis reads: Enhanced Input's actuation tolerance (KINDA_SMALL_NUMBER).
	static constexpr float PressedFloor = KINDA_SMALL_NUMBER;

	// The rule alone (the tests call it): the strength for an axis value.
	static float Strength(float AxisValue, float Sign, float DeadZone);
	// Whether Godot calls the action pressed at this axis value.
	static bool Pressed(float AxisValue, float Sign, float DeadZone);

protected:
	virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue,
		float DeltaTime) override;
};
