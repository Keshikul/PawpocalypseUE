#pragma once

// The skill tree (docs/05 §6.2; Godot skill_tree_ui.gd, skill_tree_connector_layer.gd): a Menu-stack screen over an opaque
// sky that holds the field still (pause holder "tree"). The shape, the navigation, the node states and every word are
// PawCore's (PcTreeLayout, PcTreeText); this draws them.
//
// TWO MODES (designer 2026-08-20): BROWSE (T / pad Y from the HUD) is read-only, closed by a corner X or Back; the SPEND step
// (after a Reawaken) is the only time Stardust is spent: nodes buy on press, the close button is a big Continue, and leaving
// always asks (designer 2026-10-01), in the confirm dialog on the Modal stack since P4.7 (playtest 2026-11-26, note 27: a
// real Leave / Stay question, not "press again"): "You still have Stardust to spend." or "Start the next run?". Leaving the
// spend step calls OnSpendingFinished (the HUD travels to the starting biome).
//
// The canvas: one canvas panel under a render transform (pan, zoom about its origin, Godot's canvas.position / scale);
// the links and the hub's disc painted beneath the nodes (UPawTreeLinks), a gold halo behind each READY node, the nodes
// (UPawTreeNode: a Common button wearing a sticker of its kind and state, with its level and price on chips), each
// branch's name in the field, the hub's paw and name. THE COMPASS (concept C1, designer 2026-11-26: "Compass is good. You
// can build it"): one root, a slice per trunk, a square for a skill with levels, a circle for a one-time unlock, a diamond
// for a job; owned skills in their trunk's colour, the rest dark; the tree revealed as it is bought (PcTreeLayout's
// TreeNodeReveal: owned or on sale in full, one ring ahead faint, the rest hidden and out of the pad's reach). Fit on open
// to what is revealed, wheel and buttons zoom about the view's centre, drag pans; RT / LT zoom and the right stick pans on
// a pad; Q / X jumps to the next affordable node. The tooltip follows the mouse's node, or docks bottom left for the keys
// and pad (designer 2026-10-01).

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CommonButtonBase.h"
#include "PcTreeLayout.h"
#include "PcTreeText.h"
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

// A node's sticker shape (C1): a skill with levels a rounded square, a one-time unlock a circle, a job a diamond.
enum class EPawNodeShape : uint8
{
	Square,
	Circle,
	Diamond,
};

// The links, the hub's disc and the Unicorn's rings, painted in canvas space under the nodes (skill_tree_connector_layer.gd).
UCLASS()
class PAWPOCALYPSE_API UPawTreeLinks : public UUserWidget
{
	GENERATED_BODY()

public:
	struct FLink
	{
		FVector2D A, B;
		FLinearColor Colour;  // the trunk's link colour (drawn where owned)
		bool bOwned = false;  // both ends bought
		bool bShut = false;   // the road not taken
		bool bNext = false;   // to a skill on sale now: dashed
		float Alpha = 1.0f;   // its far end's reveal: 0 hidden (not drawn), faint one ring ahead, 1 shown
	};
	TArray<FLink> Links;
	TArray<FVector2D> Pulse;    // the Unicorn (when shown): rainbow rings breathing behind its own
	FVector2D HubAt = FVector2D::ZeroVector;
	bool bHub = false;          // the hub's yellow disc, its rim and shadow (the concept's drawRoot)
	double RainbowPhase = 0.0;
	double PulseClock = 0.0;

	// The ring path around a node: a circle (the Unicorn is one), from the top, clockwise, closed.
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

// A skill tree node's plate: its sticker (C1, the Compass concept's drawSkill; D18's hexagon until 2026-11-26, hence the
// name): a hard shadow, a die-cut rim, the edge (a gold ring when it can be bought now), the fill, in the node's shape. A
// piece the node holds as its first part, so it paints UNDER the icon (a user widget's own NativePaint runs after its
// children: the node paints its chips over the icon that way).
UCLASS()
class PAWPOCALYPSE_API UPawHexPlate : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	void SetLook(EPawNodeShape InShape, const FLinearColor& InFill, const FLinearColor& InEdge, const FLinearColor& InRim, bool bInReadyRing);
	EPawNodeShape GetShape() const { return Shape; }
	FLinearColor GetFill() const { return Fill; }
	FLinearColor GetEdge() const { return Edge; }
	FLinearColor GetRim() const { return Rim; }
	bool IsReadyRing() const { return bReadyRing; }
	int32 GetPaints() const { return Paints; }
	// The shape's outline at Size (a square's side, a circle's diameter; a diamond reaches 0.66 x Size from its centre: the
	// concept's), not closed.
	static TArray<FVector2D> ShapePath(EPawNodeShape Shape, const FVector2D& Centre, double Size);

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling, FSlateWindowElementList& Out,
		int32 LayerId, const FWidgetStyle& InStyle, bool bParentEnabled) const override;

private:
	EPawNodeShape Shape = EPawNodeShape::Square;
	FLinearColor Fill = FLinearColor::Transparent;
	FLinearColor Edge = FLinearColor::Transparent;
	FLinearColor Rim = FLinearColor::Transparent;
	bool bReadyRing = false;
	mutable int32 Paints = 0;
};

// What a node wears (the screen's RefreshAll decides it from PawCore's state and reveal).
struct FPawNodeLook
{
	Pc::PcTreeNodeState State = Pc::PcTreeNodeState::Locked;
	Pc::PcTreeReveal Reveal = Pc::PcTreeReveal::Full;
	EPawNodeShape Shape = EPawNodeShape::Square;
	FLinearColor Card = FLinearColor::White; // the trunk's: an owned node's fill
	bool bOwned = false;
	FText Counter;            // a ladder bought at least once: "3/10" on a chip, green when full
	bool bMaxed = false;
	bool bCheck = false;      // a one-time unlock owned: a green tick
	FText Price;              // on sale now (shown, not owned): the price on a chip under it
	bool bAffordable = false; // yellow when it can be paid, pale when not
	bool bShine = false;
};

// One node: its sticker (the plate), its picture, its chips: the level on an owned ladder, a tick on an owned unlock, the
// price on what is on sale (the concept: "readable without hovering").
UCLASS()
class PAWPOCALYPSE_API UPawTreeNode : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPawTreeNode(const FObjectInitializer& ObjectInitializer);
	virtual bool Initialize() override;

	void SetLook(const FPawNodeLook& InLook);
	const FPawNodeLook& GetLook() const { return Look; }
	Pc::PcTreeNodeState GetState() const { return Look.State; }
	FText GetCounter() const { return Look.Counter; }
	FText GetPrice() const { return Look.Price; }
	FLinearColor GetFrameFill() const;
	FLinearColor GetFrameEdge() const;
	// The plate (its shape, colours, how many times it painted: tests), the old frame clear, how many times the chips painted.
	int32 GetPlatePaints() const;
	int32 GetChipPaints() const { return ChipPaints; }
	double GetChipScale() const { return ChipScale; } // the last paint's chip scale (>= 1: zoomed out, the chips keep their size)
	const UPawHexPlate* GetPlate() const { return Plate; }
	bool IsFrameClear() const;
	bool IsShining() const { return Look.bShine; }

	FString BranchId;
	// The screen this node tells when focus enters it (review CR9: the selection follows focus by this event, not by asking
	// every node every frame).
	TWeakObjectPtr<class UPawSkillTreeScreen> Screen;
	// A node's picture (its icon_path), Px square in the middle of the node (phase 6 slice 5b).
	void SetIconPath(const FString& Path, double Px);
	FString GetIconName() const;

	// The sticker's fill and edge by state (C1): owned the trunk's card on ink; on sale now dark in the gold ring; the rest dark.
	static void FrameColours(Pc::PcTreeNodeState State, const FLinearColor& Card, FLinearColor& OutFill, FLinearColor& OutEdge);

protected:
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling, FSlateWindowElementList& Out,
		int32 LayerId, const FWidgetStyle& InStyle, bool bParentEnabled) const override;

private:
	void OnNodeReady();
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Frame;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CounterText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UImage> Icon;
	FPawNodeLook Look;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawHexPlate> Plate; // the sticker under the icon
	double ShineClock = 0.0;
	UPROPERTY()
	FSlateBrush StardustBrush;     // the price chip's star (made once)
	mutable int32 ChipPaints = 0;
	mutable double ChipScale = 1.0;
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
	// The close button or Back: the spend step asks (the confirm dialog), a browse leaves.
	void PressClose();
	// The Modal stack the leave question opens on (the HUD hands it over). Without one the spend step leaves at once.
	void SetModalStack(class UCommonActivatableWidgetStack* Stack);
	// The leave question is up; answering it (the tests and the drivers: Leave or Stay). False when none is up.
	bool IsAskingToLeave() const;
	bool AnswerLeave(bool bLeave);
	class UPawConfirmDialog* GetLeaveDialog() const;
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
	// NativeTick's tooltip step (public for the tests): the mouse's node, else the pad's docked tip, every frame.
	void UpdateTip();
	// How many times the tooltip's lines were built (review CR9: when the node or its numbers change, not every frame).
	int32 GetTipBuilds() const { return TipBuilds; }
	// A node took focus (its focus-path event): the selection follows it (spike S4), unless it already is the selection.
	void NodeFocused(const FString& Id);

	// The tree's motion (UiJuice, phase 6 slice 5c): NativeTick steps it; the tests read and drive it.
	FPawJuice& GetJuice() { return Juice; }

	// What the tests read (and only read).
	UPawTreeNode* GetNode(const FString& Id) const;
	int32 GetNodeCount() const { return Nodes.Num(); }
	const Pc::PcTreeLayout& GetLayout() const { return Layout; }
	bool IsHaloShown(const FString& Id) const;
	// The reveal (C1): what is on the board (owned, on sale, one ring ahead); the rest is hidden and out of the pad's reach.
	bool IsShown(const FString& Id) const { return Shown.Contains(Id); }
	int32 GetShownCount() const { return Shown.Num(); }
	// What the fit frames: the shown nodes, the hub and the branch names.
	Pc::PcRect2 GetFitBounds() const;
	// A trunk's name in the field (empty when it has none) and where it stands (canvas space).
	FText GetBranchLabel(const FString& Trunk) const;
	FVector2D GetBranchLabelAt(const FString& Trunk) const;
	// How much the names are grown against the canvas so they keep the text floor on screen (1 at 1:1 or closer).
	double GetNameScale() const { return NameScale; }
	FText GetStardustText() const;
	UPawButton* GetCloseButton() const { return CloseButton; }
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
	// The node the tree was left on (UPawUiMemory, review CR14).
	FString LastSelected() const;

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
	// UPawUiMemory's; a tree with no game instance keeps its own.
	FString& LastSelectedRef() const;
	mutable FString LoneLastSelected;
	void BuildCanvas();
	void BuildChrome(class UOverlay* Root);
	void OnLayoutReady();
	void WriteNavigation();
	std::set<std::string> ShownIds() const;
	void ApplyTransform();
	void StyleClose();
	void Follow(const FString& Id, bool bInstant);
	void LeaveNow();
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
	TMap<FString, TObjectPtr<UTextBlock>> BranchLabels; // per trunk
	UPROPERTY()
	TObjectPtr<UBorder> HubName;
	double NameScale = 1.0;
	TSet<FString> Shown;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StardustText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> CloseButton;
	UPROPERTY()
	TObjectPtr<class UOverlaySlot> CloseSlot;
	FPawJuice Juice; // hover pops, the READY nodes' lean, the purchase punch (skill_tree_ui.gd)
	TWeakObjectPtr<class UCommonActivatableWidgetStack> ModalStack;
	TWeakObjectPtr<class UPawConfirmDialog> LeaveDialog;
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
	double Zoom = 1.0;
	FVector2D Pan = FVector2D::ZeroVector;
	FVector2D ViewSize = FVector2D(1920.0, 1080.0);
	bool bDragging = false;
	FVector2D DragLast = FVector2D::ZeroVector;
	FString TipBranch;
	bool bTipDocked = false;
	// What the tooltip's lines were last built from (review CR9).
	FString TipBuiltFor;
	Pc::PcTreeTip TipBuilt;
	bool bTipBuilt = false;
	int32 TipBuilds = 0;
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
