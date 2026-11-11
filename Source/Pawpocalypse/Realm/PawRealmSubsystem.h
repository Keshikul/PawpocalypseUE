#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Realm/PawRealmSim.h"

#include "PawRealmSubsystem.generated.h"

class UPawGameSubsystem;

// TierController's port (docs/03 §3, §5): drives PcField at the fixed step through FPawRealmSim, against the progress
// UPawGameSubsystem owns. One per game world; it runs once something starts it (APawRealmView does, in L_Realm), so
// the title map and editor worlds never step a field.
//
// Who advances it: APawRealmView, from its own tick, right before it draws. A UTickableWorldSubsystem ticks after every
// actor, so the view would always draw the previous frame's steps; driving it from the view costs nothing and removes a
// frame of latency between the cursor and the cat.
UCLASS()
class PAWPOCALYPSE_API UPawRealmSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// The current biome (or the first), a fresh field and a fresh clock. Seed 0 = from the clock (play); a fixed seed
	// replays a session (the tests, a bug report). False when there is no game data (logged).
	bool StartRealm(uint64 Seed = 0);
	bool IsRunning() const { return bRunning; }

	// One real frame (docs/03 §5). The game writes the input first (SetCat, SetPounceDown).
	void AdvanceFrame(double RealDelta);

	void SetCat(double X, double Y);
	// The Pounce input, sampled every frame. While anything holds the pause, and after it until the button is let go, a
	// press is swallowed: Godot fires Pounce from _unhandled_input, so a click a screen consumed (Take on a card) is never
	// a Pounce, and the field's edge detector would otherwise see the still-held button as a fresh press on resume.
	void SetPounceDown(bool bDown);
	bool IsPounceSwallowed() const { return bPounceSwallowed; }
	void SetBounds(const Pc::PcRect& Bounds) { Sim.SetBounds(Bounds); }
	// The field beside the dock (docs/05 §6.1: the field reads where the dock IS). The view reports its logical size, the
	// HUD the dock's rect in field units; either change re-measures the field (Pc::PlayfieldRect). A zero-width dock = none.
	void SetViewSize(double LogicalW, double LogicalH);
	void SetDock(const Pc::PcRect& Dock, bool bDockOnLeft);
	const Pc::PcRect& GetDock() const { return DockRect; }
	// A press over the dock is the dock's, never a Pounce.
	bool IsOverDock(double X, double Y) const { return DockRect.W > 0.0 && DockRect.HasPoint(X, Y); }
	// The field stops while ANY holder holds the pause (the card pick, later the tree, the pause menu, a ceremony): one
	// screen closing never resumes the field under another (catches_pick_ui.gd _other_modal_open()). SetPaused is the
	// "manual" holder (the tests, a cheat).
	void SetPauseHold(FName Holder, bool bHold);
	bool IsPauseHeldBy(FName Holder) const { return PauseHolds.Contains(Holder); }
	bool IsPaused() const { return Sim.bPaused; }
	void SetPaused(bool bPause) { SetPauseHold(TEXT("manual"), bPause); }
	void SetGameSpeed(double Speed) { Sim.GameSpeed = FMath::Max(Speed, 0.0); }
	// SettingsManager.auto_travel / auto_pick_card: the save slot's (UPawGameSubsystem::Slot()); these write it, and every
	// frame hands the slot's values to the field. Auto-pick: with Auto-Card owned, a dealt hand is taken at random.
	void SetAutoTravel(bool bOn);
	bool GetAutoTravel() const;
	void SetAutoPick(bool bOn);
	bool GetAutoPick() const;

	// Travel to biome Index (0-based): the progress, then a fresh field. Nothing is loaded (docs/03 §4).
	bool Travel(int32 Index);
	// The Summon button: false when the boss is not summonable or the bait is refused (the bait is then kept).
	bool SummonBoss();
	// The biome the field is in (its tier id), or "" before one.
	FString CurrentTierId() const;
	// After a Reawaken (the game's or the cheat's): what TierController did on the `reawakened` signal.
	void OnReawakened();
	// CardManager.choose(): take a card from the hand up (only a card in it), then the stats rebuilt, as Godot's
	// choose() refreshes them (a card's cost and Stacked Deck change them), and the kittens re-counted at once: a
	// kitten-cost card takes them now (TierController on card_taken; doc 01 §5 F14, designer 2026-11-03). False when the card
	// is not in the hand.
	bool ChooseCard(const FString& CardId);
	// The job picker's press (JobPicker._pick, PcHud PickJob): the run's job, then (Union Rules) the second; the stats
	// rebuilt, as lock_run_job() and lock_run_job_second() rebuild them. True when the picker must ask again for the second.
	// (job_taken's save and the hat's ring arrive with phases 4 and 6.)
	bool PickJob(const FString& JobId, bool bSecond);
	// CardManager.reroll_hand(): a fresh hand for one reroll; false with no hand up or no reroll left.
	bool RerollHand();
	// CardManager.banish_from_hand(): spends a banish on a card in the hand and re-deals (the level is not used). True also
	// when the re-deal comes back empty (the deck banished dry): the hand is then gone and the pick closes.
	bool BanishFromHand(const FString& CardId);
	// After anything that changes the kitten count (a Kitten Count purchase, a tree level) or the stats outside a kill.
	void OnProgressChanged(bool bKittensMayHaveChanged);

	const FPawRealmSim& GetSim() const { return Sim; }
	FPawRealmSim& GetSim() { return Sim; }
	UPawGameSubsystem* GetGame() const;

private:
	void Remeasure();

	FPawRealmSim Sim;
	bool bRunning = false;
	double ViewW = 1920.0, ViewH = 1080.0;
	Pc::PcRect DockRect;
	bool bDockLeft = false;   // the default dock side is right (PcSettings)
	TSet<FName> PauseHolds;
	bool bPounceSwallowed = false;
};
