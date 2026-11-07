#include "Input/PawNavigationConfig.h"

#include "Input/PawHalfAxis.h"
#include "Input/PawInputMap.h"
#include "InputMappingContext.h"

FPawNavigationConfig::FPawNavigationConfig()
{
	// Godot's rules replace Slate's defaults whole.
	KeyEventRules.Reset();
	KeyActionRules.Reset();
	const TPair<const TCHAR*, EUINavigation> Directions[] = {
		{ TEXT("ui_up"), EUINavigation::Up }, { TEXT("ui_down"), EUINavigation::Down },
		{ TEXT("ui_left"), EUINavigation::Left }, { TEXT("ui_right"), EUINavigation::Right },
	};
	for (const TPair<const TCHAR*, EUINavigation>& D : Directions)
	{
		for (const FKey& Key : PawInput::KeysOf(D.Key, false))
		{
			// The stick's halves navigate as Slate's analog navigation (below), not as keys (a key too would move twice).
			if (!Key.IsAnalog() && !Key.IsAxis1D())
			{
				KeyEventRules.Add(Key, D.Value);
			}
		}
	}
	for (const FKey& Key : PawInput::KeysOf(TEXT("ui_accept")))
	{
		KeyActionRules.Add(Key, EUINavigationAction::Accept);
	}
	for (const FKey& Key : PawInput::KeysOf(TEXT("ui_cancel")))
	{
		KeyActionRules.Add(Key, EUINavigationAction::Back);
	}
	// The analog side: the axes the ui_* actions are bound to, past their dead zone.
	if (const UInputMappingContext* IMC = PawInput::LiveContext())
	{
		for (const FEnhancedActionKeyMapping& M : IMC->GetMappings())
		{
			const UPawHalfAxis* Half = M.Modifiers.Num() == 1 ? Cast<UPawHalfAxis>(M.Modifiers[0]) : nullptr;
			if (!Half || !M.Action)
			{
				continue;
			}
			const FString Action = M.Action->ActionDescription.ToString();
			if (Action == TEXT("ui_left") || Action == TEXT("ui_right"))
			{
				AnalogHorizontalKey = M.Key;
				AnalogNavigationHorizontalThreshold = Half->DeadZone;
			}
			else if (Action == TEXT("ui_up") || Action == TEXT("ui_down"))
			{
				AnalogVerticalKey = M.Key;
				AnalogNavigationVerticalThreshold = Half->DeadZone;
			}
		}
	}
}

EUINavigation FPawNavigationConfig::DirectionOf(const FKey& Key) const
{
	const EUINavigation* D = KeyEventRules.Find(Key);
	return D ? *D : EUINavigation::Invalid;
}

EUINavigationAction FPawNavigationConfig::ActionOf(const FKey& Key) const
{
	const EUINavigationAction* A = KeyActionRules.Find(Key);
	return A ? *A : EUINavigationAction::Invalid;
}
