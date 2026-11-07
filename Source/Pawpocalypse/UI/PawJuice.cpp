#include "UI/PawJuice.h"

#include "CommonButtonBase.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Save/PawSettingsSubsystem.h"

// _on_enter's `disabled`: Common UI's locked look is interaction off (the button stays focusable for the ring), or the
// owner said so.
bool FPawJuice::Disabled(const UCommonButtonBase* B) const
{
	return !B->GetIsEnabled() || !B->IsInteractionEnabled() || HoverOff.Contains(MakeWeakObjectPtr(const_cast<UCommonButtonBase*>(B)));
}

void FPawJuice::SetHoverOff(UCommonButtonBase* Button, bool bOff)
{
	if (bOff)
	{
		HoverOff.Add(Button);
	}
	else
	{
		HoverOff.Remove(Button);
	}
}

Pc::PcJuice& FPawJuice::At(UWidget* W)
{
	if (Pc::PcJuice* J = Moving.Find(W))
	{
		return *J;
	}
	Pc::PcJuice& J = Moving.Add(W);
	J.Seed = NextSeed++;
	return J;
}

void FPawJuice::Apply(UWidget* W, const Pc::PcJuice& J)
{
	// Scale and rotation about the centre (_keep_pivot_centred: a button that grows from its corner lurches). The pivot is a
	// fraction of the size, so a row whose text changes width stays centred without a resized signal.
	W->SetRenderTransformPivot(FVector2D(0.5, 0.5));
	FWidgetTransform T = W->GetRenderTransform();
	T.Scale = FVector2D(J.ScaleX, J.ScaleY);
	T.Angle = static_cast<float>(FMath::RadiansToDegrees(J.Rotation));
	W->SetRenderTransform(T);
}

void FPawJuice::Hover(UCommonButtonBase* Button, UObject* Owner)
{
	if (!Button || !Owner || Wired.Contains(Button))
	{
		return;
	}
	Wired.Add(Button);
	const TWeakObjectPtr<UCommonButtonBase> Weak(Button);
	Button->OnHovered().AddWeakLambda(Owner, [this, Weak]() { if (Weak.IsValid()) { HandleHovered(Weak.Get()); } });
	Button->OnUnhovered().AddWeakLambda(Owner, [this, Weak]() { if (Weak.IsValid()) { HandleUnhovered(Weak.Get()); } });
	Button->OnPressed().AddWeakLambda(Owner, [this, Weak]() { if (Weak.IsValid()) { HandlePressed(Weak.Get()); } });
	Button->OnReleased().AddWeakLambda(Owner, [this, Weak]() { if (Weak.IsValid()) { HandleReleased(Weak.Get()); } });
}

void FPawJuice::HandleHovered(UCommonButtonBase* B)
{
	Pc::JuiceHoverEnter(At(B), Disabled(B));
}

void FPawJuice::HandleUnhovered(UCommonButtonBase* B)
{
	Pc::JuiceHoverExit(At(B));
}

void FPawJuice::HandlePressed(UCommonButtonBase* B)
{
	Pc::JuicePressDown(At(B));
}

void FPawJuice::HandleReleased(UCommonButtonBase* B)
{
	// Back to the hover size if the pointer is still on it, else to rest; the elastic settle is the "click" the eye sees.
	Pc::JuicePressUp(At(B), B->IsHovered() && !Disabled(B));
}

void FPawJuice::Punch(UWidget* W)
{
	if (W)
	{
		Pc::JuicePunch(At(W));
	}
}

void FPawJuice::PopIn(UWidget* W)
{
	if (W)
	{
		Pc::PcJuice& J = At(W);
		Pc::JuicePopIn(J);
		Apply(W, J); // shown at POP_IN_FROM this frame, not at full size for one
	}
}

void FPawJuice::Jitter(UWidget* W, bool bOn, bool bEffects)
{
	if (!W || (!bOn && !Moving.Contains(W)))
	{
		return; // jitter(control, false) on a control that never leaned: nothing to stop
	}
	Pc::JuiceSetJitter(At(W), bOn, bEffects, IsShownInTree(W));
}

void FPawJuice::Tick(double DeltaSeconds)
{
	for (auto It = Moving.CreateIterator(); It; ++It)
	{
		UWidget* W = It.Key().Get();
		if (!W)
		{
			It.RemoveCurrent();
			continue;
		}
		Pc::JuiceStep(It.Value(), DeltaSeconds, IsShownInTree(W));
		Apply(W, It.Value());
		if (Pc::JuiceAtRest(It.Value()))
		{
			It.RemoveCurrent();
		}
	}
}

const Pc::PcJuice* FPawJuice::Find(const UWidget* W) const
{
	return Moving.Find(MakeWeakObjectPtr(const_cast<UWidget*>(W)));
}

bool FPawJuice::IsJittering(const UWidget* W) const
{
	const Pc::PcJuice* J = Find(W);
	return J && J->bJitter;
}

bool FPawJuice::IsHoverWired(const UCommonButtonBase* B) const
{
	return Wired.Contains(MakeWeakObjectPtr(const_cast<UCommonButtonBase*>(B)));
}

bool FPawJuice::EffectsOn(const UWidget* W)
{
	const UWorld* World = W ? W->GetWorld() : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	const UPawSettingsSubsystem* S = GI ? GI->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
	return !S || S->Get().bEffectsEnabled;
}

bool FPawJuice::IsShownInTree(const UWidget* W)
{
	// The property, not UWidget::IsVisible(): that one asks the Slate widget and is false for any widget not built yet (a
	// screen before its first paint), where Godot's `visible` is the control's own flag.
	const UWidget* Cur = W;
	while (Cur)
	{
		const ESlateVisibility V = Cur->GetVisibility();
		if (V == ESlateVisibility::Collapsed || V == ESlateVisibility::Hidden)
		{
			return false;
		}
		if (const UPanelWidget* Parent = Cur->GetParent())
		{
			Cur = Parent;
			continue;
		}
		// The root of a widget tree: the user widget that owns the tree stands where it is placed.
		const UWidgetTree* Tree = Cast<UWidgetTree>(Cur->GetOuter());
		Cur = Tree ? Cast<UUserWidget>(Tree->GetOuter()) : nullptr;
	}
	return true;
}
