#pragma once

// SPIKE S4 (docs/04 §8, docs/05): throwaway. Delete with the rest of Dev/Spikes once the results are in docs/05.
// Common UI structure under test: root layout with Game/Menu stacks; HUD screen with the dock (6 tabs, page switcher, 15 shop
// rows) and a status row; skill tree screen with 87 nodes as Common buttons in one canvas, links painted below, explicit
// lattice navigation, zoom and auto-pan. All built in C++ (no Widget Blueprints): content is built before
// UUserWidget::Initialize so UCommonButtonBase wraps it in its internal button.

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CommonButtonBase.h"
#include "GameFramework/Actor.h"

#include "SpikeUi.generated.h"

class UCanvasPanel;
class UCommonActivatableWidgetStack;
class UTextBlock;
class UWidgetSwitcher;

UCLASS()
class USpikeButtonStyle : public UCommonButtonStyle
{
	GENERATED_BODY()

public:
	USpikeButtonStyle();
};

UCLASS()
class USpikeButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	USpikeButton(const FObjectInitializer& ObjectInitializer);
	virtual bool Initialize() override;
	void SetLabel(const FString& Text);

	FString Id;

private:
	UPROPERTY()
	TObjectPtr<UTextBlock> Label;
};

UCLASS()
class USpikeHudScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;

	void SelectTab(int32 Index);
	void RefreshRows(int32 Tick);
	USpikeButton* FocusedButton() const;
	int32 CurrentTab() const { return Tab; }

	TArray<TObjectPtr<USpikeButton>> Tabs;
	TArray<TArray<TObjectPtr<USpikeButton>>> Rows;
	TFunction<void()> OnOpenTree;

private:
	UPROPERTY()
	TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY()
	TObjectPtr<UTextBlock> Status;
	// Rows and Tabs are held through the widget tree; these keep GC references for the nested arrays.
	UPROPERTY()
	TArray<TObjectPtr<USpikeButton>> AllButtons;

	int32 Tab = 0;
	TArray<int32> RememberedRow;
};

UCLASS()
class USpikeTreeLinks : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling, FSlateWindowElementList& Out,
		int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;

	TArray<TPair<FVector2D, FVector2D>> Links;
};

UCLASS()
class USpikeTreeScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;

	int32 FocusedNode() const;
	float Zoom() const { return ZoomLevel; }
	bool IsNodeOnScreen(int32 Node) const;

	UPROPERTY()
	TArray<TObjectPtr<USpikeButton>> Nodes;
	TArray<FVector2D> NodePos;
	// Explicit navigation, per node: Up, Down, Left, Right (-1 = none). The tour walks this graph.
	TArray<TStaticArray<int32, 4>> Nav;
	int32 StartNode = 0;

private:
	UPROPERTY()
	TObjectPtr<UCanvasPanel> Lattice;
	float ZoomLevel = 1.0f;
	FVector2D Pan = FVector2D::ZeroVector;
};

UCLASS()
class USpikeRootLayout : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	UPROPERTY()
	TObjectPtr<UCommonActivatableWidgetStack> GameStack;
	UPROPERTY()
	TObjectPtr<UCommonActivatableWidgetStack> MenuStack;
};

// Builds the UI and walks it with real Slate key events; writes Saved/Spikes/S4/<run>.txt (PASS/FAIL per step, timings).
// Command line: -S4Run=<name> -S4HudMode=all|game  (-S4Shot: screenshots of the dock and the tree, windowed only)
UCLASS()
class ASpikeUiActor : public AActor
{
	GENERATED_BODY()

public:
	ASpikeUiActor();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	struct FStep
	{
		enum EKind : uint8 { Key, Expect, OpenTree, CheckZoom, Refresh, Shot, Wait };
		EKind Kind = Wait;
		FKey KeyToPress;
		FString Expected; // focus id after the step ("row:p:r", "tab:i", "node:n")
		int32 Frames = 3;
		FString Note;
	};

	void BuildTour();
	FString FocusId() const;
	void Check(bool bOk, const FString& What);
	void Report();

	UPROPERTY()
	TObjectPtr<USpikeRootLayout> Root;
	UPROPERTY()
	TObjectPtr<USpikeHudScreen> Hud;
	UPROPERTY()
	TObjectPtr<USpikeTreeScreen> Tree;

	TArray<FStep> Steps;
	int32 StepIndex = 0;
	int32 WaitFrames = 30;
	bool bTourBuiltForTree = false;
	FString RunName = TEXT("default");

	TArray<FString> Log;
	int32 Passes = 0;
	int32 Fails = 0;
	int32 FocusLostFrames = 0;
	int32 RefreshFramesLeft = 0;
	TArray<double> RefreshFrameMs, NavFrameMs;
	bool bReported = false;
	int32 QuitCountdown = 0;
};
