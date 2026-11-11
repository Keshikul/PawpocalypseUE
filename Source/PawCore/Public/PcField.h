#pragma once

// The realm simulation (docs/03 §2 PcField, phase 2): the live mice as rows of arrays (never Actors), stepped at a fixed
// 60 Hz by the game's UPawRealmSubsystem. A port of TierController's per-frame resolves and Enemy's _process; each step
// keeps Godot's frame order (the controller's resolves, then the spawn timer, then every mouse moves, then the frame's dead
// are swept) because several rules read state another one wrote earlier in the same frame.
//
// Randomness: Godot rolled its global randf(). Here every draw is PcRng over (seed, step, entity serial, salt), so a step
// is a pure function of the field, the progress and the input: replays and bug reports reproduce exactly.
//
// SLICE 1 (2026-10-29): the pool, the spawner (golden roll, the weighted biome mix, edge position, inward heading),
// movement (wander, flier, gristle seek, golden flee with its turn-back, Misdirection), lifetime, leaving the field, Funnel
// bounces, shield cycles, damage intake, the paw (reach, shred, crit, Execute), Overkill splash (nearest survivor, never
// chains), the kill payout (signature bounty, Overkill bounty, Catnip, kill count, Catches) and the event queue.
// SLICE 2 (2026-10-30): kittens. The pool (sized to ownership, grown never shrunk, orbit slots re-spread), the claim tally
// and the sticky least-claimed target with load rebalancing, Kitten Foreman, roaming, the orbit approach, contact damage;
// and their cards and nodes: Gilded Kitten, Lone Hunter, Kitten Drag, Kitten From Signature (temporary kittens), Kitten
// Kill Explodes, Grand Finale (illusions that pull instead of bite).
// SLICE 3 (2026-10-31): the aura (contact damage, Crowd Work, Aura Slow, Overkill from aura kills), the Snare and Deadweight,
// Gridlock, the slow's one-step life and Traps, the shared damage zone (trail, echo), Edge Burn, the aura pulse and
// Standing Wave, Rainbow Poop (its timer, Poop Trail, Poop Duration), the cat's stillness clocks (Carry, Long Lunch's bank).
// SLICE 4 (2026-10-31): Pounce. Ownership, the cooldown, the press edge (a pad trigger re-fires only from released), Reflex's
// auto-pounce at its multiplier, the vulnerable-window bonus, Pounce Chain beyond the rim, Kitten Mimic Pounce, the crater
// card, the Daredevil's Cone of Shame (the cat rooted, its stillness frozen), Nine Lives Insurance, Crash Landing, the
// kill refund (Bloodlust) and Momentum.
// SLICE 5 (2026-10-31): the paw's extras (Anvil Paw, Butcher's Rhythm and Inscrutable's earned crits, Riddle, Hammerfall),
// the kill's (Cat Burglar's Cut, Overflow, the Catnip Bomb, Long Lunch, Levy), the despawn recovery (Scavenge,
// Pickpocket), the spawner's nodes (Pied Piper, Sixth Sense), Sleight of Hand, and the Prism (the beam, Refraction, Full
// Spectrum, Spectrum's kills, Critical Mass's shards).
// SLICE 6 (2026-10-31): the boss. Summoned with bait (SummonFieldBoss), its phases by HP, the shield cycle per phase and
// the relocation on every opened window, its drift; the fight clock from the first engagement and the timeout (it leaves,
// the bait is spent); its void-kitten adds that walk in and heal it; everything's damage to the boss and the adds (the
// paw when no mouse is in reach, the aura while engaging, zones, the pulse, poop, Pounce, the kittens' boss and add duty);
// the defeat (credited at once) and the travel after it; Auto-Bait and Auto-Travel.
// NOT simulated (presentation): the boss's twitch and anger, its death sequence, the banner.
//
// When the kittens are re-counted is Godot's, not "every step": on entering a biome, a Kitten Count purchase, a tree level,
// a Reawaken (the game calls SyncFieldKittens), whenever a temporary kitten is granted or expires, and when a card is taken
// (the game's ChooseCard, or Auto-Card here): a card that costs kittens takes them at once (F14, designer 2026-11-03).

#include "PcCombat.h"
#include "PcCore.h"
#include "PcData.h"
#include "PcFieldRules.h"
#include "PcSave.h"
#include "PcStats.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace Pc
{
	enum class PcFieldEventKind : uint8_t
	{
		Spawned,    // Slot entered the field at (X, Y)
		Despawned,  // Slot left: walked off the field or its lifetime ran out (no reward, no penalty)
		Killed,     // Slot died at (X, Y); Value = the Catnip credited; Flags: golden, signature, bounty, toyed
		Strike,     // the paw swiped from the cat at (X, Y) toward (ToX, ToY); Flags: crit
		Splash,     // Overkill carried from (X, Y) to Slot at (ToX, ToY); Value = the damage
		Shield,     // Slot's shield came up (Flags & Shielded) or went down
		Levelled,   // Catches levelled (a hand may be up: the UI reads PcProgress)
		KittenBlast, // Kitten Kill Explodes (the giblets) at (X, Y), radius Value; Slot = -1
		AuraPulse,   // the aura detonated (AURA_PULSE / Standing Wave) at (X, Y), radius Value
		PoopDropped, // Rainbow Poop opened a zone at (X, Y), radius Value, lasting ToX seconds
		Pounce,      // the cat pounced at (X, Y), radius Value; Flags: Auto (Reflex fired it)
		CatnipBomb,  // a Catnip bomb (the card, or Overflow) went off at (X, Y), radius Value
		Cleave,      // Hammerfall carried the swipe from (X, Y) to (ToX, ToY)
		PrismBeam,   // the Prism's beam from (X, Y) to Slot at (ToX, ToY), Value = this step's share (drawn at the
		             // presentation's own interval)
		PrismBurst,  // Critical Mass detonated around the cat at (X, Y), drawn radius Value
		BossSummoned,    // the boss came out at (X, Y)
		BossPhase,       // the boss entered phase Value
		BossWindow,      // the boss's shield turned over (Flags & Shielded: it rose; else its window opened and it relocates)
		BossHealed,      // an add reached the boss and healed it by Value
		BossDefeated,    // the boss died at (X, Y), Value = its reward; Flags: New (a first clear), Final (the last boss), Victory (the ending)
		BossTimeout,     // the fight clock ran out: the boss left, the bait is gone
		AddSpawned,      // a void-kitten add came in at (X, Y)
		AddIntercepted,  // an add was killed at (X, Y)
		AddArrived,      // an add reached the boss at (X, Y) (BossHealed follows)
		TravelRequested, // the field asks the game to travel to biome Slot (Auto-Travel, or after a boss's first clear)
	};

	// A void-kitten add (BossAdd): walks at the boss, heals it unless intercepted. Not a mouse: no reward, no kill.
	struct PcBossAdd
	{
		double X = 0.0, Y = 0.0, Hp = 0.0, Heal = 0.0, Speed = 0.0;
		double TargetX = 0.0, TargetY = 0.0;
		uint64_t Serial = 0;
	};

	// What put a damage zone on the floor (the renderer wears each in its own colour).
	enum class PcZoneKind : uint8_t
	{
		Trail,  // AURA_TRAIL
		Echo,   // Echo
		Crater, // Pounce's crater (slice 4)
	};

	// TierController.DamageZone: damage priced as a FRACTION of the player's DPS, multiplied out live.
	struct PcDamageZone
	{
		double X = 0.0, Y = 0.0, Radius = 0.0;
		double PowerFraction = 0.0;
		double Remaining = 0.0;
		double Duration = 0.0; // for the fade (Remaining / Duration)
		PcZoneKind Kind = PcZoneKind::Trail;
	};

	namespace FieldFlag
	{
		constexpr uint32_t Golden = 1 << 0;
		constexpr uint32_t Signature = 1 << 1;
		constexpr uint32_t Crit = 1 << 2;
		constexpr uint32_t Bounty = 1 << 3;    // the Overkill bounty paid
		constexpr uint32_t Toyed = 1 << 4;     // Play With Food paid
		constexpr uint32_t Shielded = 1 << 5;
		constexpr uint32_t Auto = 1 << 6;      // a Reflex pounce (drawn dimmer)
		constexpr uint32_t Earned = 1 << 7;    // a crit a counter decided (Butcher's Rhythm, Inscrutable), not luck
		constexpr uint32_t Echo = 1 << 8;      // Anvil Paw's second blow (drawn wider)
		constexpr uint32_t Execute = 1 << 9;   // Execute or Riddle finished the mouse (the red burst)
		constexpr uint32_t Boss = 1 << 10;     // the paw's swipe landed on the boss
		constexpr uint32_t New = 1 << 11;      // a boss's first-ever clear
		constexpr uint32_t Final = 1 << 12;    // the last boss of the game
		constexpr uint32_t Victory = 1 << 13;  // the ending comes up (PcProgress RaisesVictory: boss_defeated for the last boss)
		// Phase 6's effects (tier_controller.gd draws each differently):
		constexpr uint32_t Riddle = 1 << 14;   // this crit armed Riddle for the next swing (a red burst on the target)
		constexpr uint32_t Threshold = 1 << 15; // Execute's threshold claimed the mouse (red shards and a burst; Riddle's spend does not)
		constexpr uint32_t Beam = 1 << 16;     // the Prism's beam dealt the killing blow (the Spectrum pop when its multiplier pays)
	}

	struct PcFieldEvent
	{
		PcFieldEventKind Kind = PcFieldEventKind::Spawned;
		int32_t Slot = -1;
		double X = 0.0, Y = 0.0, ToX = 0.0, ToY = 0.0;
		double Value = 0.0;
		uint32_t Flags = 0;
	};

	// What the game hands the field every step.
	struct PcFieldInput
	{
		double CatX = 0.0;            // the cat (= the cursor), sampled once per frame by the game
		double CatY = 0.0;
		bool bAutoPickSetting = false; // SettingsManager.auto_pick_card
		bool bPounceDown = false;      // the Pounce action is held (mouse button or the pad's trigger past its dead zone)
		bool bAutoTravelSetting = false; // SettingsManager.auto_travel
		bool bAutoBaitSetting = false;   // SettingsManager.auto_bait
	};

	struct PcField
	{
		// --- the biome -------------------------------------------------------------------------------------------
		int64_t TierIndex = -1;       // into PcGameData::Tiers; -1 = no biome begun
		PcRect Bounds;                // the playfield (PlayfieldRect)
		uint64_t Seed = 0;
		uint64_t StepCount = 0;

		// --- the mice: one row per pooled slot (a struct of arrays, docs/04 §5) ----------------------------------
		std::vector<int32_t> Kind;    // index into PcGameData::Enemies
		std::vector<uint8_t> Active;
		std::vector<uint64_t> Serial; // unique per spawn: the RNG entity and the identity a stale reference compares
		std::vector<double> X, Y, DirX, DirY;
		std::vector<double> Hp, MaxHp, WaveReward, SpeedMult, LifetimeLeft, Bounces, InvulnTimer;
		std::vector<uint8_t> Shielded, HasFleeTarget, BeamKill;
		// Enemy._slow_multiplier / _slow_fresh: the strongest slow of the step, kept through the NEXT step's move and dropped
		// there if nothing re-applied it. Below 1 the mouse is snared: Traps for every blow it takes meanwhile.
		std::vector<double> SlowMult;
		std::vector<uint8_t> SlowFresh;
		// Grand Finale's pull, set by a kitten standing near and living one step (Godot never resets it on spawn).
		std::vector<double> IllusionPull;
		std::vector<uint8_t> IllusionFresh;
		std::vector<int32_t> Live;    // active slots in spawn order (Godot's active_enemies: ties and loops follow it)

		// --- the spawner -----------------------------------------------------------------------------------------
		double SpawnTimeLeft = 0.0;   // Godot's Timer.time_left
		uint64_t Spawns = 0;

		// --- the kittens: one row per pooled kitten (the pool grows, never shrinks) -------------------------------
		std::vector<uint8_t> KActive;
		std::vector<double> KX, KY;
		std::vector<double> KOrbitX, KOrbitY;     // the kitten's own slot around its target (configure_orbit_slot)
		std::vector<double> KRoamDirX, KRoamDirY, KRoamTimer;
		std::vector<int32_t> KTarget;             // a mouse slot, or -1; valid only while that slot still has KTargetSerial
		std::vector<uint64_t> KTargetSerial;
		std::vector<double> KDamageMult;          // Gilded Kitten / Lone Hunter on one kitten each, else 1
		std::vector<uint8_t> KGolden, KLone, KIllusory; // what the renderer paints: gold tint, halo, translucent
		double KDragSpeed = 0.0;                  // Kitten Drag px/s (0 = off); the anchor is the cat
		std::vector<double> TempKittens;          // Kitten From Signature: seconds left, one entry per temporary kitten
		std::vector<int64_t> Claims;              // scratch, per mouse slot: how many kittens are committed to it
		uint64_t KittenWakes = 0;                 // activations so far: each roll's RNG entity

		// --- the aura's clocks and the zones ---------------------------------------------------------------------
		std::vector<PcDamageZone> Zones;          // oldest first; at most max_damage_zones
		double TrailElapsed = 0.0, TrailLastX = 0.0, TrailLastY = 0.0;
		bool bTrailDropped = false;               // Godot's Vector2.INF: no drop yet this biome
		double EchoElapsed = 0.0;
		double AuraPulseElapsed = 0.0;
		double PoopTimeLeft = 0.0, PoopWait = 0.0; // the PoopTimer
		bool bPoopActive = false;
		double PoopX = 0.0, PoopY = 0.0, PoopRemaining = 0.0;

		// --- Pounce ----------------------------------------------------------------------------------------------
		double PounceCooldownLeft = 0.0;
		double PounceCooldownFull = 0.0;          // what the LAST cast set: the HUD's clock (a refund moves it forward)
		bool bPounceWasDown = false;              // the press edge
		double PounceLockLeft = 0.0;              // the Cone of Shame: the cat is rooted (Player._pounce_lock_left)
		double InsuranceWindow = 0.0;             // Nine Lives Insurance
		int64_t InsuranceKills = 0;

		// --- the boss (one per biome, not pooled) and its adds ----------------------------------------------------
		bool bBossUp = false;                     // Boss.is_active()
		double BossX = 0.0, BossY = 0.0, BossHp = 0.0;
		int64_t BossPhase = 1;
		bool bBossShielded = false;
		double BossInvulnTimer = 0.0;
		double BossAnchorX = 0.0, BossAnchorY = 0.0, BossDriftX = 0.0, BossDriftY = 0.0, BossDriftTimer = 0.0;
		uint64_t BossRolls = 0;
		bool bBossEngagedOnce = false;            // the fight clock runs from the first engagement
		double BossFightElapsed = 0.0;
		PcSummonPayment SummonPaid;               // F36: what this fight's summon cost (FightPayment)
		bool bShredOnBoss = false;                // the paw's streak is on the boss (not "on nothing")
		std::vector<PcBossAdd> Adds;              // in launch order
		double AddTimer = 0.0;
		uint64_t AddSpawns = 0;
		double AutoTravelTimer = 0.0, AutoBaitTimer = 0.0;
		double TravelDelay = -1.0;                // >= 0: the travel after a first clear, once the death sequence has played

		// --- the paw ---------------------------------------------------------------------------------------------
		double StrikeTimer = 0.0;
		double AnvilCharge = 0.0, RhythmCharge = 0.0, InscrutableCharge = 0.0; // accumulated, never rolled
		bool bRiddleArmed = false;

		// --- the kill's and the spawner's extras, the Prism ---------------------------------------------------------
		int64_t KillsSinceBomb = 0;
		double VanishTimer = 0.0;                 // Sleight of Hand
		bool bForeseen = false;                   // Sixth Sense: the next spawn, pre-rolled
		int32_t ForeseenKind = -1;
		double ForeseenX = 0.0, ForeseenY = 0.0;
		double PrismShards = 0.0;
		int32_t ShredSlot = -1;
		uint64_t ShredSerial = 0;
		int64_t ShredStacks = 0;
		double ShredExpires = 0.0;

		// --- published between steps -----------------------------------------------------------------------------
		double ThresholdNow = 1.0;    // GameState.threshold_damage_now: set at the END of a step, read by the next
		double CatX = 0.0, CatY = 0.0;
		uint64_t Deals = 0;           // card deals rolled from the field (each its own PcRngCardRolls)
		PcCombatStats Stats;          // GameState.get_stats(): rebuilt when a kill says so, or by RefreshFieldStats

		std::vector<PcFieldEvent> Events; // pushed by StepField, drained by the game (DrainFieldEvents)

		// Transient within a step.
		std::vector<int32_t> DeadThisStep;

		int64_t LiveCount() const { return static_cast<int64_t>(Live.size()); }
		int64_t AwakeKittens() const { return static_cast<int64_t>(std::count(KActive.begin(), KActive.end(), uint8_t(1))); }
	};

	// TierController._ready() for one biome: an empty field, the spawn timer primed with the biome's interval. The progress
	// must already be in the biome (PcProgress BeginBiome). Travel and Reawaken call this again; nothing in a field is
	// worth keeping (docs/03 §4).
	PAWCORE_API void BeginField(PcField& F, const PcGameData& D, const PcProgress& P, const std::string& TierId,
		const PcRect& Bounds, uint64_t Seed);

	// _sync_kitten_count(): wakes or sleeps pooled kittens (pool order, both ways) until the owned count plus the temporary
	// kittens are awake; grows the pool first if ownership has passed it. The game calls it where Godot did: a Kitten
	// Count purchase, a Reawaken, a tree level (BeginField already has).
	PAWCORE_API void SyncFieldKittens(PcField& F, const PcGameData& D, const PcProgress& P);

	// Kitten.hunt() for kitten K against the current claims (F.Claims): the field's step calls it for every awake kitten
	// after tallying the claims; exposed so the tests can drive one hunt against the Godot fixture.
	PAWCORE_API void HuntKitten(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, int32_t K, double Dt);

	// summon_boss(): the bait is bought (all-or-nothing) and the boss comes out in the middle of the field, at full HP in
	// phase 1. False when this biome has no boss, it is already up, or the bait is refused (the bait is then kept).
	PAWCORE_API bool SummonFieldBoss(PcField& F, const PcGameData& D, PcProgress& P);
	// can_summon_boss(): a boss here, not up, and the bait's Catnip affordable.
	PAWCORE_API bool CanSummonFieldBoss(const PcField& F, const PcGameData& D, const PcProgress& P);
	// _dismiss_boss(): the boss leaves without dying (the game calls it on a Reawaken).
	PAWCORE_API void DismissFieldBoss(PcField& F);
	// F36 (D16): what the boss up cost to summon, for a save taken during the fight (CaptureSave gives it back in the file);
	// null while no fight is on.
	PAWCORE_API const PcSummonPayment* FightPayment(const PcField& F);

	// The boss's own rules, one at a time (the cheat overlay; the tests drive them against a real Godot Boss node):
	// Boss.take_damage() (a kill is credited at once), Boss.heal(), Boss._process() (its drift and its shield cycle).
	PAWCORE_API void DamageFieldBoss(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Amount,
		double ShieldPierce);
	PAWCORE_API void HealFieldBoss(PcField& F, const PcGameData& D, double Amount);
	PAWCORE_API void TickFieldBoss(PcField& F, const PcGameData& D, double Dt);

	// pounce_cooldown_fraction(): 0 just fired, 1 ready, against the cooldown the last cast set.
	PAWCORE_API double PounceCooldownFraction(const PcField& F);

	// The playfield changed (resize, dock side): live mice get the new bounds too.
	PAWCORE_API void SetFieldBounds(PcField& F, const PcRect& Bounds);

	// The stats changed outside a kill (a purchase, a card, a Reawaken): GameState.get_stats() rebuilt.
	PAWCORE_API void RefreshFieldStats(PcField& F, const PcGameData& D, const PcProgress& P);

	// One fixed step (1/60 s in play).
	PAWCORE_API void StepField(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Dt);

	// One mouse of a given kind, placed and headed by the caller, with the spawn stats activate() would give it (the cheat
	// overlay, the tests). Returns its slot, or -1 for an unknown id or a full field.
	PAWCORE_API int32_t SpawnEnemyAt(PcField& F, const PcGameData& D, const PcProgress& P, const std::string& EnemyId, double X,
		double Y, double DirX, double DirY);

	// Kills every live mouse through the normal damage path (the cheat: it still pays and counts).
	PAWCORE_API void KillAllEnemies(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In);

	// The events since the last drain, oldest first; the queue is emptied.
	PAWCORE_API std::vector<PcFieldEvent> DrainFieldEvents(PcField& F);
}
