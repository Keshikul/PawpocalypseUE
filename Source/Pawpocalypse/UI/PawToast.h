#pragma once

// The input toast (Godot input_device.gd _build_toast): "Controller connected" / "Controller disconnected" (and the pad
// audio's "No sound?", _watch_for_pad_audio) at the top edge,
// centred, on its own layer above every screen (the title, the tree, a paused game); a paper plate with an ink frame, and
// nothing in it takes the mouse. Shown TOAST_SEC, then fades over 0.4 s (UPawInputDeviceSubsystem drives it).
//
// A WIDGET BLUEPRINT (WBP_Toast; PawWidgets.h): Plate and Message are bound by name.

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "PawToast.generated.h"

class UBorder;
class UTextBlock;

UCLASS()
class PAWPOCALYPSE_API UPawToast : public UUserWidget
{
	GENERATED_BODY()

public:
	static constexpr double ShowSec = 2.4;  // TOAST_SEC
	static constexpr double FadeSec = 0.4;  // the tween's fade
	static constexpr int32 ZOrder = 120;    // the toast's CanvasLayer (layer 120: above every screen)

	virtual bool Initialize() override;

	void SetMessage(const FText& Text);
	// The controller toasts (input_device.gd): "Controller connected" or "Controller disconnected".
	void ShowConnection(bool bConnected);
	// _watch_for_pad_audio: the pad brought a sound device and the game follows the system's.
	void ShowPadAudio();
	FText GetMessage() const;
	// 1 shown, 0 gone (the fade).
	void SetFade(float Alpha);

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Plate;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Message;
};
