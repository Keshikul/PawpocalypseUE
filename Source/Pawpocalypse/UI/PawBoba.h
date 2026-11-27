#pragma once

// Boba in the menus (P4.3; playtest 2026-11-26, notes 2 and 3). The rules are PawCore's PcBoba (the clock, the kinds, the
// paths); this draws them.
//
// UPawBobaSubsystem (the game instance's): the menus say when they open and close (the title "main_menu", "pause",
// "settings", "victory"), the skill tree suppresses her (CLAUDE.md: never over the skill tree), the title card's click forces
// one. It ticks while paused (menus pause the game), plays `boba_cameo` when one starts, and draws on a layer of its own over
// every menu (UPawBobaLayer, in the viewport above them) that never takes input. The effects setting turns her off.
//
// UPawBobaLayer: A WIDGET BLUEPRINT (WBP_BobaLayer; PawWidgets.h): Root, Photo bound by name. The nyan trail is painted.

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PcBoba.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"

#include <utility>
#include <vector>

#include "PawBoba.generated.h"

class UImage;
class UOverlay;
class UTexture2D;

UCLASS()
class PAWPOCALYPSE_API UPawBobaLayer : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	// Her photo at a pose (Size: her height, px), and the trail behind her (newest first; empty for none).
	void ShowPose(UTexture2D* Photo, double Size, double Aspect, const Pc::PcBobaPose& Pose,
		const std::vector<std::pair<double, double>>& Trail);
	void HideCameo();

	// What the tests read.
	bool IsPhotoShown() const;
	const UTexture2D* GetPhoto() const;
	FVector2D GetPhotoCentre() const { return Centre; }
	int32 GetTrailPoints() const { return Trail.Num(); }

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> Root;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Photo;

	TArray<FVector2f> Trail;
	double TrailBand = 0.0;
	FVector2D Centre = FVector2D::ZeroVector;
};

UCLASS()
class PAWPOCALYPSE_API UPawBobaSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// The subsystem of the game instance that owns a world object (a widget), or null.
	static UPawBobaSubsystem* Of(const UObject* WorldContext);

	void EnterMenu(const FString& Tag);
	void LeaveMenu(const FString& Tag);
	void Suppress(const FString& Tag, bool bOn);
	// A cameo now: a kind by the weights (the title card's click), or this kind. False when she cannot come (one in flight,
	// the effects off, no photo, the tree up).
	bool Force();
	bool ForceKind(Pc::PcBobaKind Kind);
	// This kind after Delay seconds, if the menu Tag is still open then (the victory's nyan, 1.5 s after it lands).
	void ForceLater(Pc::PcBobaKind Kind, double Delay, const FString& Tag);

	// A photo's pixel size: the texture's, or its source's in an editor run that has not built the texture's platform data
	// (a headless test reports 0 x 0 otherwise).
	static FVector2D PhotoSize(const UTexture2D* Photo);
	// Her photos (DA_Boba, sorted), the first is the title card's.
	int32 PhotoCount() const { return Photos.Num(); }
	UTexture2D* GetPhoto(int32 Index) const;

	// What the tests read and drive.
	const Pc::PcBoba& GetState() const { return State; }
	bool IsShowing() const { return State.Cameo.bLive; }
	UPawBobaLayer* GetLayer() const { return Layer; }
	void SetSeed(uint64 Seed) { State.Seed = Seed; State.Draws = 0; }
	void TickOnce(float DeltaSeconds) { Tick(DeltaSeconds); }

	// FTickableGameObject: menus pause the game, she does not.
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UPawBobaSubsystem, STATGROUP_Tickables); }
	virtual ETickableTickType GetTickableTickType() const override { return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Always; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickableInEditor() const override { return false; }

private:
	void LoadPhotos();
	void EnsureLayer();
	bool EffectsOn() const;
	FVector2D ScreenSize() const;
	void Draw();
	void Started();

	Pc::PcBoba State;
	std::vector<double> Aspects;
	UPROPERTY()
	TArray<TObjectPtr<UTexture2D>> Photos;
	UPROPERTY()
	TObjectPtr<UPawBobaLayer> Layer;
	double LaterDelay = -1.0;
	Pc::PcBobaKind LaterKind = Pc::PcBobaKind::Nyan;
	FString LaterTag;
};
