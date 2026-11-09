#pragma once

// The skill tree (docs/05 §6.2; Godot skill_tree_ui.gd, skill_tree_connector_layer.gd): a Menu-stack screen over an opaque
// sky that holds the field still (pause holder "tree"). The shape, the navigation, the node states and every word are
// PawCore's (PcTreeLayout, PcTreeText); this draws them.
//
// TWO MODES (designer 2026-08-20): BROWSE (T / pad Y from the HUD) is read-only, closed by a corner X or Back; the SPEND step
// (after a Reawaken) is the only time Stardust is spent: nodes buy on press, the close button is a big Continue, and leaving
// always asks once (designer 2026-10-01), saying whether Stardust is left. Leaving the spend step calls OnSpendingFinished
// (the HUD travels to the starting biome).
//
// The canvas: one canvas panel under a render transform (pan, zoom about its origin, Godot's canvas.position / scale);
// links and the rainbow / job rings painted beneath the nodes (UPawTreeLinks), a gold halo behind each READY node, the
// nodes (UPawTreeNode: a Common button with the frame of its state, a counter on ladders), captions under them, the hub
// plate. Fit on open (PcTreeLayout's fit zoom), wheel and buttons zoom about the view's centre, drag pans; RT / LT zoom and
// the right stick pans on a pad; Q / X jumps to the next affordable node. The tooltip follows the mouse's node, or docks
// bottom left for the keys and pad (designer 2026-10-01).

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CommonButtonBase.h"
#include "PcTreeLayout.h"
#include "UI/PawJuice.h"

#include "PawSkillTreeScreen.generated.h"

class UBorder;
class UCanvasPanel;
class UPawButton;
class UPawPromptText;
class UPawRealmSubsystem;
class UPawTreeSky;
class UPawUnicornCeremony;
class UPawGameSubsystem;
class USizeBox;
class UTextBlock;
class UVerticalBox;

// The links and rings, painted in canvas space under the nodes (skill_tree_connector_layer.gd).
UCLASS()
class PAWPOCALYPSE_API UPawTreeLinks : public UUserWidget
{
	GENERATED_BODY()

public:
	struct FLink
	{
		FVector2D A, B;
		FLinearColor Colour;
		bool bOwned = false;
		bool bShut = false;
	};
	TArray<FLink> Links;
	TArray<FVector2D> Rainbow;  // centres of the verb nodes (a rainbow ring in every state)
	TArray<FVector2D> Pulse;    // the Unicorn: breathing rings behind its own
	TArray<FVector2D> JobRings; // the job nodes: one gold ring
	double RainbowPhase = 0.0;
	double PulseClock = 0.0;

	// The ring path around a node: a rounded rectangle traced just outside the frame (_ring_path).
	static TArray<FVector2D> RingPath(const FVector2D& Centre, const FVector2D& Half);

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling, FSlateWindowElementList& Out,
		int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
};

UCLASS()
class PAWPOCALYPSE_API UPawTreeNodeStyle : public UCommonButtonStyle
{
	GENERATED_BODY()

public:
	UPawTreeNodeStyle();
};

// One node: the frame of its state (LOCKED / READY / OWNED in the trunk's colour, heavier and rounder for an unlock), the
// counter on a ladder. The icon art arrives with phase 6.
UCLASS()
class PAWPOCALYPSE_API UPawTreeNode : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPawTreeNode(const FObjectInitializer& ObjectInitializer);
	virtual bool Initialize() override;

	void SetLook(Pc::PcTreeNodeState State, const FLinearColor& Trunk, bool bUnlock, const FText& Counter, bool bShine);
	Pc::PcTreeNodeState GetState() const { return State; }
	FText GetCounter() const;
	FLinearColor GetFrameFill() const;
	FLinearColor GetFrameEdge() const;
	float GetFrameRadius() const; // R4f: a skill round, an unlock a rounded square
	bool IsShining() const { return bShine; }

	FString BranchId;
	// The branch's picture (its icon_path), 56 px in the middle of the node (phase 6 slice 5b).
	void SetIconPath(const FString& Path);
	FString GetIconName() const;

	// _frame(): the three states as one series, READY lerped a third of the way from LOCKED to OWNED.
	static void FrameColours(Pc::PcTreeNodeState State, const FLinearColor& Trunk, FLinearColor& OutFill, FLinearColor& OutEdge);

protected:
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;

private:
	void OnNodeReady();
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Frame;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CounterText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UImage> Icon;
	Pc::PcTreeNodeState State = Pc::PcTreeNodeState::Locked;
	bool bShine = false;
	double ShineClock = 0.0;
};

UCLASS()
class PAWPOCALYPSE_API UPawSkillTreeScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	static const FName PauseHolder; // "tree"

	virtual bool Initialize() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	// Set before the screen is activated (it reads it on activation): browse, or the spend step.
	void SetSpendMode(bool bSpend) { bSpendMode = bSpend; }
	bool IsSpendMode() const { return bSpendMode; }
	// Leaving the spend step (never a browse): the HUD's travel to the starting biome.
	TFunction<void()> OnSpendingFinished;

	// Every node's look, the links, the Stardust line, the close button (the purchase and the open call it).
	void RefreshAll();
	// A node pressed: in the spend step, one level (PcTree's rule); a browse never buys. True when bought.
	bool PressNode(const FString& Id);
	// The close button or Back: the spend step asks once, then leaves; a browse leaves.
	void PressClose();
	bool IsCloseArmed() const { return bConfirmLeaving; }
	// Q / X: the next node that can be bought after the selected one; focuses it. Empty when none.
	FString JumpToAffordable();
	// The zoom about the view's centre (the wheel, the buttons, the triggers), clamped.
	void ZoomBy(double Delta);
	void SetViewSize(const FVector2D& Size);
	void FitAndCentre();
	double GetZoom() const { return Zoom; }
	FVector2D GetPan() const { return Pan; }
	FVector2D GetSteer() const { return Stick; }   // the steer stick as the pan reads it (Godot: +Y down)
	double GetZoomHeld() const { return ZoomHeld; } // zoom_in's strength less zoom_out's (keys 1, triggers by strength)
	// The tooltip for a node, as the player reads it (shown docked or beside the node; hidden with an empty id).
	void ShowTip(const FString& Id, bool bDocked);
	FString GetTipBranch() const { return TipBranch; }

	// The tree's motion (UiJuice, phase 6 slice 5c): NativeTick steps it; the tests read and drive it.
	FPawJuice& GetJuice() { return Juice; }
	UBorder* GetWarningPlate() const { return WarningPlate; }

	// What the tests read (and only read).
	UPawTreeNode* GetNode(const FString& Id) const;
	int32 GetNodeCount() const { return Nodes.Num(); }
	const Pc::PcTreeLayout& GetLayout() const { return Layout; }
	FText GetCaption(const FString& Id) const;
	bool IsHaloShown(const FString& Id) const;
	FText GetStardustText() const;
	UPawButton* GetCloseButton() const { return CloseButton; }
	FText GetWarningText() const;
	bool IsWarningShown() const;
	// The tooltip's lines as shown (PawGlyphs::Neutral: a prompt is <g:stem> or <k:label>).
	TArray<FText> GetTipLines() const;
	FString GetLegend() const;
	FText GetTipTitle() const;
	const UPawTreeLinks* GetLinks() const { return LinksLayer; }
	UPawTreeSky* GetSky() const { return TreeSky; } // skill_tree_sky.gd (phase 6 slice 5d)
	// The Unicorn's arrival (unicorn_ceremony.gd, phase 6 slice 6a): played by buying the Unicorn; while it plays the tree
	// buys nothing and hands it every key (ceremony_playing()).
	UPawUnicornCeremony* GetUnicorn() const { return Unicorn; }
	bool IsCeremonyPlaying() const;
	static FString LastSelected();

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& Geometry, const FAnalogInputEvent& Event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;

private:
	void BuildCanvas();
	void BuildChrome(class UOverlay* Root);
	void OnLayoutReady();
	void WriteNavigation();
	void ApplyTransform();
	void StyleClose();
	void Follow(const FString& Id, bool bInstant);
	void OnNodeFocused(const FString& Id);
	FVector2D ContentSize() const;
	FVector2D NodeOnScreen(const FString& Id) const;
	UPawRealmSubsystem* Realm() const;
	UPawGameSubsystem* Game() const;
	bool CanTakeFocus() const;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawTreeSky> TreeSky;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawUnicornCeremony> Unicorn;
	UPROPERTY()
	TObjectPtr<UPawTreeLinks> LinksLayer;
	UPROPERTY()
	TMap<FString, TObjectPtr<UPawTreeNode>> Nodes;
	UPROPERTY()
	TMap<FString, TObjectPtr<UBorder>> Halos;
	UPROPERTY()
	TMap<FString, TObjectPtr<UTextBlock>> Captions;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StardustText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> CloseButton;
	UPROPERTY()
	TObjectPtr<class UOverlaySlot> CloseSlot;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> WarningPlate;
	FPawJuice Juice; // hover pops, the READY nodes' lean, the purchase punch, the leaving plate's pop-in (skill_tree_ui.gd)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WarningText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> TipPanel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TipTitle;
	UPROPERTY()
	TArray<TObjectPtr<UPawPromptText>> TipLines;
	UPROPERTY()
	TObjectPtr<class UCanvasPanelSlot> TipSlot;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawPromptText> LegendPrompts;

	Pc::PcTreeLayout Layout;
	TArray<FString> Order; // branch order, as FString
	bool bSpendMode = false;
	bool bConfirmLeaving = false;
	double Zoom = 1.0;
	FVector2D Pan = FVector2D::ZeroVector;
	FVector2D ViewSize = FVector2D(1920.0, 1080.0);
	bool bDragging = false;
	FVector2D DragLast = FVector2D::ZeroVector;
	FString TipBranch;
	bool bTipDocked = false;
	double HaloClock = 0.0;
	double RefreshClock = 0.0;
	// The pad: the right stick pans, the triggers zoom (held), the follow glides.
	FVector2D Stick = FVector2D::ZeroVector;   // the pan: Player.steer_input(), +Y down
	TMap<FKey, float> SteerAxes;               // the steer axes' last values, for Input.get_vector
	double ZoomHeld = 0.0;      // get_action_strength("zoom_in") - get_action_strength("zoom_out")
	bool bZoomInKey = false;    // a zoom_in key held (= ): strength 1
	bool bZoomOutKey = false;   // a zoom_out key held (- )
	TMap<FKey, float> ZoomAxes; // the triggers' last values: their strength past the dead zone
	void RefreshZoom();
	bool bFollowing = false;
	// R4h: the view moved by the player since the open (a zoom, a pan, a drag, a follow). Until then a new view size refits:
	// the real size arrives on the first tick (Godot reads content_area.size on open; 150 % UI is 1280 x 720).
	bool bViewTouched = false;
	FVector2D FollowFrom = FVector2D::ZeroVector, FollowTo = FVector2D::ZeroVector;
	double FollowT = 0.0;
};
