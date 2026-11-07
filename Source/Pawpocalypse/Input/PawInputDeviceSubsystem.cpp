#include "Input/PawInputDeviceSubsystem.h"

#include "Audio/PawAudioSubsystem.h"
#include "Core/PawLog.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/App.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "HAL/PlatformTime.h"
#include "Input/Events.h"
#include "Input/PawInputMap.h"
#include "Input/PawKeyCodes.h"
#include "InputCoreTypes.h"
#include "Save/PawSettingsSubsystem.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Widget.h"
#include "Framework/Application/SlateUser.h"
#include "Layout/WidgetPath.h"
#include "UI/PawPrompts.h"
#include "UI/PawToast.h"
#include "UI/PawUiConstants.h"
#include "UI/PawWidgets.h"

namespace
{
	// Godot's joypad axis (0 LX, 1 LY, 2 RX, 3 RY, 4 LT, 5 RT) for an Unreal axis key, and the sign that turns Unreal's value
	// into Godot's (+Y down there, up here). -1: not a pad axis.
	int32 GodotAxisOf(const FKey& Key, float& Flip)
	{
		Flip = 1.0f;
		if (Key == EKeys::Gamepad_LeftX) { return 0; }
		if (Key == EKeys::Gamepad_LeftY) { Flip = -1.0f; return 1; }
		if (Key == EKeys::Gamepad_RightX) { return 2; }
		if (Key == EKeys::Gamepad_RightY) { Flip = -1.0f; return 3; }
		if (Key == EKeys::Gamepad_LeftTriggerAxis) { return 4; }
		if (Key == EKeys::Gamepad_RightTriggerAxis) { return 5; }
		return -1;
	}

	// Every event, before any widget; consumes only what the dock takes (ConsumeForDock).
	class FPawInputObserver : public IInputProcessor
	{
	public:
		explicit FPawInputObserver(UPawInputDeviceSubsystem* InOwner) : Owner(InOwner) {}

		virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}
		virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& Event) override
		{
			UPawInputDeviceSubsystem* O = Owner.Get();
			if (O) { O->ObserveKey(Event, true); }
			return O && O->ConsumesKey(Event.GetKey());
		}
		virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& Event) override
		{
			UPawInputDeviceSubsystem* O = Owner.Get();
			if (O) { O->ObserveKey(Event, false); }
			return O && O->ConsumesKey(Event.GetKey());
		}
		virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& Event) override
		{
			UPawInputDeviceSubsystem* O = Owner.Get();
			if (O) { O->ObserveAnalog(Event); }
			return O && O->ConsumesKey(Event.GetKey());
		}
		virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& Event) override
		{
			if (UPawInputDeviceSubsystem* O = Owner.Get()) { O->ObserveMouseMove(Event); }
			return false;
		}
		virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& Event) override
		{
			if (UPawInputDeviceSubsystem* O = Owner.Get()) { O->ObserveMouseButton(Event, true); }
			return false;
		}
		virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& Event) override
		{
			if (UPawInputDeviceSubsystem* O = Owner.Get()) { O->ObserveMouseButton(Event, false); }
			return false;
		}
		virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& Event, const FPointerEvent* Gesture) override
		{
			// Godot sends the wheel as mouse BUTTON presses.
			if (UPawInputDeviceSubsystem* O = Owner.Get()) { O->ObserveMouseButton(Event, true); }
			return false;
		}
		virtual const TCHAR* GetDebugName() const override { return TEXT("PawInputObserver"); }

	private:
		TWeakObjectPtr<UPawInputDeviceSubsystem> Owner;
	};
}

void UPawInputDeviceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UPawSettingsSubsystem>();
	State.BootMs = NowMs();
	ReadSettings();
	if (UPawSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UPawSettingsSubsystem>())
	{
		Settings->OnChanged.AddWeakLambda(this, [this]()
		{
			const Pc::PcFamily Before = State.Family;
			ReadSettings();
			if (State.Family != Before)
			{
				OnFamilyChanged.Broadcast(State.Family);
			}
			OnControlsChanged.Broadcast();
		});
	}
	if (!GIsEditor && FSlateApplication::IsInitialized())
	{
		Processor = MakeShared<FPawInputObserver>(this);
		// FIRST (index 0): the dock's keys are taken before Common UI's analog cursor and Slate see them.
		FSlateApplication::Get().RegisterInputPreProcessor(Processor, 0);
		// project.godot joypads/ignore_joypad_on_unfocused_application=true: a pad does nothing while the game is in the
		// background (Slate's default; set here so it is the game's decision, not an accident).
		FSlateApplication::Get().SetHandleDeviceInputWhenApplicationNotActive(false);
		// A key let go while the game was in the background never says so: forget what was held.
		ActivationHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddWeakLambda(this, [this](bool bActive)
		{
			if (!bActive)
			{
				HeldKeys.Reset();
				AxisNow.Reset();
			}
		});
		UE_LOG(LogPaw, Log, TEXT("Input: watching every device (prompts: %hs)"), State.PromptStyle.c_str());
		// Keys by position through this keyboard's layout (PawKeyCodes): what the movement and zoom keys are here.
		auto Keys = [](const TCHAR* Action)
		{
			TArray<FString> Names;
			for (const FKey& K : PawInput::KeysOf(Action, false))
			{
				if (!K.IsGamepadKey())
				{
					Names.Add(K.ToString());
				}
			}
			return FString::Join(Names, TEXT(" "));
		};
		UE_LOG(LogPaw, Log, TEXT("Input: on this keyboard, up is %s, previous tab %s, zoom in %s, zoom out %s"), *Keys(TEXT("menu_up")),
			*Keys(TEXT("tab_prev")), *Keys(TEXT("zoom_in")), *Keys(TEXT("zoom_out")));
		ConnectionHandle = IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().AddWeakLambda(this,
			[this](EInputDeviceConnectionState NewState, FPlatformUserId, FInputDeviceId Device)
			{
				ObserveConnection(Device.GetId(), NewState == EInputDeviceConnectionState::Connected);
			});
	}
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UPawInputDeviceSubsystem::Tick));
}

void UPawInputDeviceSubsystem::Deinitialize()
{
	if (Processor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(Processor);
		FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(ActivationHandle);
	}
	Processor.Reset();
	if (ConnectionHandle.IsValid())
	{
		IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().Remove(ConnectionHandle);
	}
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	Super::Deinitialize();
}

int64 UPawInputDeviceSubsystem::NowMs() const
{
	return Clock ? Clock() : static_cast<int64>(FPlatformTime::Seconds() * 1000.0);
}

void UPawInputDeviceSubsystem::ReadSettings()
{
	const UPawSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UPawSettingsSubsystem>();
	const Pc::PcSettings S = Settings ? Settings->Get() : Pc::PcSettings();
	State.PromptStyle = S.PromptStyle;
	State.bSwapSticks = S.bSwapSticks;
	// InputDevice.apply_bindings(), on controls_changed: the defaults, the saved bindings, the two swaps, the steer actions'
	// dead zone (clamped 0.05 .. 0.9), made the live mapping context; then every local player's Enhanced Input rebuilds.
	if (!bBindingsApplied || !(AppliedBindings == S.Bindings) || bAppliedSwapSticks != S.bSwapSticks
		|| bAppliedSwapConfirm != S.bSwapConfirmBack || AppliedDeadzone != S.StickDeadzone)
	{
		bBindingsApplied = true;
		AppliedBindings = S.Bindings;
		bAppliedSwapSticks = S.bSwapSticks;
		bAppliedSwapConfirm = S.bSwapConfirmBack;
		AppliedDeadzone = S.StickDeadzone;
		++BindingsApplied;
		PawInput::ApplyBindings(S);
		for (ULocalPlayer* Player : GetGameInstance()->GetLocalPlayers())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player))
			{
				Input->RequestRebuildControlMappings();
			}
		}
	}
	Pc::DeviceRefreshFamily(State);
}

void UPawInputDeviceSubsystem::SetDetectedForTest(Pc::PcFamily Detected)
{
	const Pc::PcFamily Before = State.Family;
	State.Detected = Detected;
	Pc::DeviceRefreshFamily(State);
	if (State.Family != Before)
	{
		OnFamilyChanged.Broadcast(State.Family);
	}
}

void UPawInputDeviceSubsystem::Observe(const Pc::PcDeviceEvent& E)
{
	const Pc::PcFamily Family = State.Family;
	const Pc::PcCursorSource Cursor = State.Cursor;
	const bool bHidden = State.bPointerHidden;
	const Pc::PcDeviceToast ToastBefore = State.Toast;
	if ((E.Kind == Pc::PcDeviceEventKind::PadButton || E.Kind == Pc::PcDeviceEventKind::PadMotion) && !State.DeviceFamilies.count(E.Device))
	{
		// A pad first heard from: XInput calls every pad "XInput Controller" (family_from_info: Xbox).
		Pc::DeviceRegister(State, E.Device, Pc::DeviceFamilyFromInfo(0, 0, "XInput Controller", "", false));
	}
	Pc::DeviceObserve(State, E, NowMs());
	Changed(Family, Cursor, bHidden, ToastBefore);
}

bool UPawInputDeviceSubsystem::IsActionHeld(const TCHAR* Action) const
{
	for (const FKey& K : HeldKeys)
	{
		if (PawInput::KeyIs(K, Action))
		{
			return true;
		}
	}
	for (const TPair<FKey, float>& A : AxisNow)
	{
		if (PawInput::IsPressed(Action, A.Key, A.Value))
		{
			return true;
		}
	}
	return false;
}

bool UPawInputDeviceSubsystem::TakeActionDown(const TCHAR* Action)
{
	bool bAny = false;
	for (int32 I = DownsSinceAsked.Num() - 1; I >= 0; --I)
	{
		if (PawInput::KeyIs(DownsSinceAsked[I], Action))
		{
			bAny = true;
			DownsSinceAsked.RemoveAt(I);
		}
	}
	return bAny;
}

void UPawInputDeviceSubsystem::ObserveKey(const FKeyEvent& Event, bool bDown)
{
	const FKey Key = Event.GetKey();
	if (bDown && !Event.IsRepeat() && PawInput::EventOfKey(Key).Kind != Pc::PcInputKind::JoyAxis && !Key.IsAnalog())
	{
		DownsSinceAsked.Add(Key);
		if (DownsSinceAsked.Num() > 32)
		{
			DownsSinceAsked.RemoveAt(0); // nobody asked for a while: keep the latest
		}
	}
	// What is held, for the dock's polling: a key or button, not a stick's or trigger's digital stand-in (its analog value
	// is what counts, below).
	if (PawInput::EventOfKey(Key).Kind != Pc::PcInputKind::JoyAxis && !Key.IsAnalog())
	{
		if (bDown)
		{
			HeldKeys.Add(Key);
		}
		else
		{
			HeldKeys.Remove(Key);
		}
	}
	// A stick or trigger past Slate's threshold also arrives as a digital key: Godot never sees that, only the axis (below).
	if (PawInput::IsStandIn(Key) || Key.IsAnalog())
	{
		return;
	}
	Pc::PcDeviceEvent E;
	E.Kind = Key.IsGamepadKey() ? Pc::PcDeviceEventKind::PadButton : Pc::PcDeviceEventKind::Key;
	E.bPressed = bDown;
	E.bEcho = Event.IsRepeat();
	E.Device = Event.GetInputDeviceId().GetId();
	E.bPressAction = PawInput::KeyIs(Key, TEXT("press"));
	Observe(E);
}

void UPawInputDeviceSubsystem::ObserveAnalog(const FAnalogInputEvent& Event)
{
	float Flip = 1.0f;
	const FKey Key = Event.GetKey();
	AxisNow.Add(Key, Event.GetAnalogValue());
	const int32 Axis = GodotAxisOf(Key, Flip);
	if (Axis < 0)
	{
		return;
	}
	Pc::PcDeviceEvent E;
	E.Kind = Pc::PcDeviceEventKind::PadMotion;
	E.Device = Event.GetInputDeviceId().GetId();
	E.Axis = Axis;
	E.AxisValue = static_cast<double>(Event.GetAnalogValue() * Flip);
	// is_action("press") matches the axis whichever way it points; is_action_pressed past the dead zone this way.
	E.bPressAction = PawInput::KeyIs(Key, TEXT("press"));
	E.bPressPressed = PawInput::IsPressed(TEXT("press"), Key, Event.GetAnalogValue());
	Observe(E);
}

void UPawInputDeviceSubsystem::ObserveMouseMove(const FPointerEvent& Event)
{
	Pc::PcDeviceEvent E;
	E.Kind = Pc::PcDeviceEventKind::MouseMotion;
	E.MotionLength = Event.GetCursorDelta().Size();
	Observe(E);
}

FName UPawInputDeviceSubsystem::PawCursorPath(bool bClosed)
{
	// Under Content (SetHardwareCursor's root), without the extension.
	return bClosed ? FName(TEXT("Pawpocalypse/UI/Cursors/ui_cursor_paw_closed")) : FName(TEXT("Pawpocalypse/UI/Cursors/ui_cursor_paw_open"));
}

void UPawInputDeviceSubsystem::ApplyPaw()
{
	// _apply_paw(): the arrow and the pointing hand (a button is an arrow in this theme). Only in a game with a window.
	if (GIsEditor || !FApp::CanEverRender() || !GEngine || !GEngine->GameViewport || !FSlateApplication::IsInitialized())
	{
		return;
	}
	const FName Path = PawCursorPath(bPawClosed);
	if (!bPawReady)
	{
		// Both paws loaded now (the viewport caches them by path): nothing is read from disk on a click. The other paw first,
		// on the arrow, which the current one then takes back below.
		GEngine->GameViewport->SetHardwareCursor(EMouseCursor::Default, PawCursorPath(!bPawClosed), PawHotSpot());
	}
	const bool bArrow = GEngine->GameViewport->SetHardwareCursor(EMouseCursor::Default, Path, PawHotSpot());
	const bool bHand = GEngine->GameViewport->SetHardwareCursor(EMouseCursor::Hand, Path, PawHotSpot());
	if (bArrow && bHand)
	{
		++PawApplied;
	}
	else if (!bPawReady)
	{
		UE_LOG(LogPaw, Warning, TEXT("Input: the paw cursor did not load (%s)"), *Path.ToString());
	}
	if (!bPawReady)
	{
		bPawReady = true;
		UE_LOG(LogPaw, Log, TEXT("Input: the paw cursor is the pointer (%s)"), bArrow && bHand ? TEXT("loaded") : TEXT("FAILED"));
	}
}

void UPawInputDeviceSubsystem::ObserveMouseButton(const FPointerEvent& Event, bool bDown)
{
	// The paw closes while the left button is held (input_device.gd _input).
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton && bDown != bPawClosed)
	{
		bPawClosed = bDown;
		ApplyPaw();
	}
	Pc::PcDeviceEvent E;
	E.Kind = Pc::PcDeviceEventKind::MouseButton;
	E.bPressed = bDown;
	Observe(E);
}

void UPawInputDeviceSubsystem::ObserveConnection(int32 Device, bool bConnected)
{
	const Pc::PcFamily Family = State.Family;
	const Pc::PcCursorSource Cursor = State.Cursor;
	const bool bHidden = State.bPointerHidden;
	State.Toast = Pc::PcDeviceToast::None; // each connection decides its own toast
	Pc::DeviceConnection(State, Device, bConnected, Pc::DeviceFamilyFromInfo(0, 0, "XInput Controller", "", false), NowMs());
	const Pc::PcDeviceToast Shown = State.Toast;
	Changed(Family, Cursor, bHidden, Shown);
	if (Shown != Pc::PcDeviceToast::None)
	{
		ShowToast(Shown);
	}
	// _on_joy_connection_changed: the connect toast, then _watch_for_pad_audio().
	if (bConnected && Shown == Pc::PcDeviceToast::Connected)
	{
		PadAudioWatches.Add({ OutputDevices(), NowMs() + Pc::DevicePadAudioWaitMs });
	}
}

std::vector<std::string> UPawInputDeviceSubsystem::OutputDevices() const
{
	// AudioServer.get_output_device_list()
	std::vector<std::string> Names;
	if (const UPawAudioSubsystem* Audio = GetGameInstance()->GetSubsystem<UPawAudioSubsystem>())
	{
		for (const FString& Name : Audio->OutputDeviceList())
		{
			Names.push_back(TCHAR_TO_UTF8(*Name));
		}
	}
	return Names;
}

void UPawInputDeviceSubsystem::RunPadAudioWatches()
{
	const int64 Now = NowMs();
	for (int32 I = 0; I < PadAudioWatches.Num();)
	{
		if (Now < PadAudioWatches[I].DueMs)
		{
			++I;
			continue;
		}
		const std::vector<std::string> Before = MoveTemp(PadAudioWatches[I].Before);
		PadAudioWatches.RemoveAt(I);
		const UPawSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UPawSettingsSubsystem>();
		const std::string Output = Settings ? Settings->Get().OutputDevice : std::string("Default");
		if (Pc::DevicePadAudioToast(Before, OutputDevices(), Output))
		{
			ShowToast(Pc::PcDeviceToast::PadAudio);
		}
	}
}

void UPawInputDeviceSubsystem::NoteWarp()
{
	Pc::DeviceWarp(State, NowMs());
}

void UPawInputDeviceSubsystem::ClaimStick()
{
	const bool bHidden = State.bPointerHidden;
	Pc::DeviceClaimStick(State);
	if (State.bPointerHidden != bHidden)
	{
		ApplyPointer();
	}
}

bool UPawInputDeviceSubsystem::PressCountsFor(int64 OpenedMs) const
{
	return Pc::DevicePressCountsFor(State, OpenedMs, NowMs());
}

void UPawInputDeviceSubsystem::SetPointerOverField(bool bOver)
{
	const bool bHidden = State.bPointerHidden;
	Pc::DeviceSetOverField(State, bOver);
	if (State.bPointerHidden != bHidden)
	{
		ApplyPointer();
	}
}

void UPawInputDeviceSubsystem::Changed(Pc::PcFamily Before, Pc::PcCursorSource CursorBefore, bool bHiddenBefore, Pc::PcDeviceToast ToastBefore)
{
	if (State.Family != Before)
	{
		OnFamilyChanged.Broadcast(State.Family);
	}
	if (State.bPointerHidden != bHiddenBefore)
	{
		ApplyPointer();
	}
	if (State.Toast != Pc::PcDeviceToast::None && State.Toast != ToastBefore)
	{
		ShowToast(State.Toast);
	}
}

void UPawInputDeviceSubsystem::ApplyPointer()
{
	// _hide_pointer(): the OS pointer hidden while the stick steers or over the open field. The viewport's cursor (a widget
	// under the pointer keeps its own: the dock shows it).
	if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
	{
		PC->CurrentMouseCursor = State.bPointerHidden ? EMouseCursor::None : EMouseCursor::Default;
	}
}

void UPawInputDeviceSubsystem::ShowToast(Pc::PcDeviceToast Which)
{
	// show_toast(): its own layer above every screen (the title, the tree, a paused game), nothing in it takes the mouse.
	if (!Toast)
	{
		Toast = CreateWidget<UPawToast>(GetGameInstance(), PawUi::Resolve<UPawToast>());
	}
	if (!Toast)
	{
		return;
	}
	if (Which == Pc::PcDeviceToast::PadAudio)
	{
		Toast->ShowPadAudio();
	}
	else
	{
		Toast->ShowConnection(Which == Pc::PcDeviceToast::Connected);
	}
	if (!Toast->IsInViewport() && GetGameInstance()->GetGameViewportClient())
	{
		Toast->AddToViewport(UPawToast::ZOrder);
	}
	Toast->SetFade(1.0f);
	ToastLeft = UPawToast::ShowSec + UPawToast::FadeSec;
}

void UPawInputDeviceSubsystem::SetDockArea(UWidget* Dock, bool bDockOnLeft)
{
	DockArea = Dock;
	bDockLeft = bDockOnLeft;
}

FVector2D UPawInputDeviceSubsystem::FocusPromptAt(const FVector2D& TopLeft, const FVector2D& Size, const FVector2D& PromptSize, bool bInDock,
	bool bDockOnLeft)
{
	const double Y = TopLeft.Y + (Size.Y - PromptSize.Y) * 0.5;
	if (bInDock)
	{
		// _place_row_prompt: r.end.x + GAP with the dock on the left, r.position.x - GAP - size.x on the right.
		return FVector2D(bDockOnLeft ? TopLeft.X + Size.X + PawUi::RowPromptGap : TopLeft.X - PawUi::RowPromptGap - PromptSize.X, Y);
	}
	// _place_focus_prompt: r.end.x - size.x - FOCUS_PROMPT_INSET.
	return FVector2D(TopLeft.X + Size.X - PromptSize.X - PawUi::FocusPromptInset, Y);
}

void UPawInputDeviceSubsystem::PlaceFocusPrompt()
{
	UGameViewportClient* Viewport = GetGameInstance()->GetGameViewportClient();
	if (!Viewport || !FSlateApplication::IsInitialized())
	{
		return;
	}
	if (!FocusPrompt)
	{
		FocusPrompt = CreateWidget<UPawPromptIcon>(GetGameInstance(), PawUi::Resolve<UPawPromptIcon>());
		if (!FocusPrompt)
		{
			return;
		}
		FocusPrompt->Action = TEXT("press");
		FocusPrompt->AddToViewport(UPawToast::ZOrder - 1);
		FocusPrompt->SetAlignmentInViewport(FVector2D::ZeroVector);
	}
	FSlateApplication& Slate = FSlateApplication::Get();
	const TSharedPtr<SWidget> Focused = Slate.GetUserFocusedWidget(0);
	// owner is BaseButton: Slate's buttons (a Common button, a check box, an option's combo).
	const FString Type = Focused.IsValid() ? Focused->GetTypeAsString() : FString();
	const bool bButton = Type.Contains(TEXT("Button")) || Type.Contains(TEXT("CheckBox")) || Type.Contains(TEXT("ComboBox"));
	FWidgetPath Path;
	const bool bFound = bButton && Slate.FindPathToWidget(Focused.ToSharedRef(), Path) && Path.IsValid();
	if (!bFound || !FocusPromptShown(true, State.bLastWasPointer))
	{
		FocusPrompt->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const TSharedPtr<SWidget> Dock = DockArea.IsValid() ? DockArea->GetCachedWidget() : nullptr;
	const bool bInDock = Dock.IsValid() && Path.ContainsWidget(Dock.Get());
	const int32 Px = static_cast<int32>(bInDock ? PawUi::RowPromptPx : PawUi::FocusPromptPx);
	if (FocusPrompt->IconPx != Px)
	{
		FocusPrompt->SetIconPx(Px);
	}
	FocusPrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
	// The button's rect in the viewport's units (DPI removed), where the prompt is placed.
	const FGeometry& G = Path.Widgets.Last().Geometry;
	FVector2D Pixel, TopLeft, BottomRight;
	USlateBlueprintLibrary::AbsoluteToViewport(GetGameInstance(), G.LocalToAbsolute(FVector2D::ZeroVector), Pixel, TopLeft);
	USlateBlueprintLibrary::AbsoluteToViewport(GetGameInstance(), G.LocalToAbsolute(G.GetLocalSize()), Pixel, BottomRight);
	const FVector2D At = FocusPromptAt(TopLeft, BottomRight - TopLeft, FocusPrompt->GetPromptSize(), bInDock, bDockLeft);
	FocusPrompt->SetPositionInViewport(At, false);
}

bool UPawInputDeviceSubsystem::Tick(float DeltaSeconds)
{
	// _build_paw_cursor() once the game has its window (the viewport exists a frame after startup).
	if (!bPawReady && !GIsEditor && GEngine && GEngine->GameViewport)
	{
		ApplyPaw();
	}
	PlaceFocusPrompt();
	RunPadAudioWatches();
	if (ToastLeft > 0.0)
	{
		ToastLeft -= DeltaSeconds;
		if (Toast)
		{
			Toast->SetFade(ToastLeft <= 0.0 ? 0.0f : static_cast<float>(FMath::Min(1.0, ToastLeft / UPawToast::FadeSec)));
		}
	}
	return true;
}

FText UPawInputDeviceSubsystem::GetToastText() const
{
	return Toast ? Toast->GetMessage() : FText::GetEmpty();
}

bool UPawInputDeviceSubsystem::IsToastShown() const
{
	return ToastLeft > 0.0;
}
