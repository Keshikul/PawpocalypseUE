#include "Input/PawHalfAxis.h"

#include "Math/UnrealMathUtility.h"

bool UPawHalfAxis::Pressed(float AxisValue, float Sign, float DeadZone)
{
	// same_direction && |axis| >= deadzone
	const bool bSameWay = Sign >= 0.0f ? AxisValue >= 0.0f : AxisValue <= 0.0f;
	return bSameWay && FMath::Abs(AxisValue) >= DeadZone && AxisValue != 0.0f;
}

float UPawHalfAxis::Strength(float AxisValue, float Sign, float DeadZone)
{
	if (!Pressed(AxisValue, Sign, DeadZone))
	{
		return 0.0f;
	}
	if (DeadZone >= 1.0f)
	{
		return 1.0f;
	}
	return FMath::Clamp((FMath::Abs(AxisValue) - DeadZone) / (1.0f - DeadZone), 0.0f, 1.0f);
}

FInputActionValue UPawHalfAxis::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue,
	float DeltaTime)
{
	const float Axis = CurrentValue.Get<float>();
	float Out = Strength(Axis, Sign, DeadZone);
	if (Pressed(Axis, Sign, DeadZone))
	{
		// Enhanced Input calls a value actuated from KINDA_SMALL_NUMBER up (FInputActionValue::IsNonZero): a pressed axis whose
		// strength is below that (at or just past the dead zone) reads as that floor, so it stays pressed.
		Out = FMath::Max(Out, PressedFloor);
	}
	return FInputActionValue(EInputActionValueType::Axis1D, FVector(Out, 0.0, 0.0));
}
