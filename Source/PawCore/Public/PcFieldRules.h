#pragma once

// The field's pure rules (phase 2): the GameState getters the realm reads, the biome's weighted spawn pool, the playfield
// rectangle, and one enemy's spawn stats and damage intake. Each names its Godot original and keeps the reasoning. PcField
// (the stateful simulation) is built on these; tests check every one against the REAL Godot code
// (tools/godot_fixtures/field_rules_fixture.gd).

#include "PcCore.h"
#include "PcData.h"
#include "PcStats.h"

#include <cstdint>
#include <vector>

namespace Pc
{
	// --- what GameState tells the field ------------------------------------------------------------------------------

	// get_spawn_rate_multiplier(): the Spawn Rate line x the spawn-rate card costs x the job x Swarm x (1 + Infestation).
	// Everything the player has bought that says "more mice"; the cat's damage is NOT in it (designer, 2026-09-14).
	PAWCORE_API double SpawnRateMultiplier(const PcGameData& D, const PcProgress& P);
	// TierController._spawn_rate_per_sec(): the biome's authored rate times those levers, never below a hair above zero
	// (the cadence divides by it).
	PAWCORE_API double SpawnRatePerSec(const PcGameData& D, const PcProgress& P, const PcTier& Tier);
	// TierController._refresh_spawn_rate(): a timer fires at most once a frame, so past one spawn per min_spawn_interval
	// the interval holds at one frame and a BATCH spawns per tick (without it the rate flat-lines at the frame rate).
	struct PcSpawnCadence
	{
		int64_t Batch = 1;
		double Interval = 0.0;
	};
	PAWCORE_API PcSpawnCadence SpawnCadence(const PcGameData& D, double PerSec);

	// get_threshold_damage_multiplier(): Swarm Logic (at crowd_threshold live mice or more) x Last Stand (below
	// last_stand_threshold). Owning both is not a permanent buff: there is a middle where neither pays.
	PAWCORE_API double ThresholdDamageMultiplier(const PcGameData& D, const PcProgress& P, int64_t LiveEnemies);
	// get_snared_damage_multiplier(): Traps, at least x1, for anything wading through a Snare.
	PAWCORE_API double SnaredDamageMultiplier(const PcGameData& D, const PcProgress& P);
	// get_play_with_food_multiplier(): x1 unless the Snare is unlocked AND Play With Food is owned.
	PAWCORE_API double PlayWithFoodMultiplier(const PcGameData& D, const PcProgress& P);
	// get_funnel_bounces(): 0 unless the Snare is unlocked AND Funnel is owned; fractional (a chance at the next bounce).
	PAWCORE_API double FunnelBounces(const PcGameData& D, const PcProgress& P);
	// get_enemy_drift(): Misdirection's pull of wandering mice toward the cat, never negative.
	PAWCORE_API double EnemyDrift(const PcGameData& D, const PcProgress& P);
	// get_prism_kill_multiplier(): Spectrum, x1 unless the Prism is owned and Spectrum bought.
	PAWCORE_API double PrismKillMultiplier(const PcGameData& D, const PcProgress& P);

	// --- the biome's pool (TierData) ---------------------------------------------------------------------------------

	// total_spawn_weight(): negative weights count as 0; `SignatureWeight` scales every signature entry (the
	// SIGNATURE_FREQUENCY card's home since biomes became a mixed pool, 2026-08-18).
	PAWCORE_API double TotalSpawnWeight(const PcGameData& D, const PcTier& Tier, double SignatureWeight = 1.0);
	// pick_enemy(): deterministic for a roll in 0..1 (clamped to 0.9999); nullptr for an empty pool, the first entry when
	// every weight is 0.
	PAWCORE_API const PcEnemy* PickEnemy(const PcGameData& D, const PcTier& Tier, double Roll, double SignatureWeight = 1.0);

	// --- the playfield (TierController.playfield_rect) ---------------------------------------------------------------

	struct PcRect
	{
		double X = 0.0, Y = 0.0, W = 0.0, H = 0.0;

		double Right() const { return X + W; }
		double Bottom() const { return Y + H; }
		double CentreX() const { return X + W * 0.5; }
		double CentreY() const { return Y + H * 0.5; }
		// Rect2.has_point(): the far edges are OUTSIDE (why spawns are inset from them).
		bool HasPoint(double PX, double PY) const { return PX >= X && PY >= Y && PX < X + W && PY < Y + H; }
	};

	// The field beside the dock: the dock's side is read off where the dock IS (its centre against the view's), the
	// setting only stands in when there is no dock rect yet (`Dock.W <= 0`, the headless case). Never outside the view,
	// never narrower than min_playfield_width (it grows AWAY from the dock, under the panel if it must).
	PAWCORE_API PcRect PlayfieldRect(const PcGameData& D, double ViewW, double ViewH, const PcRect& Dock, double FallbackMargin,
		bool bDockOnLeft);

	// --- one enemy: spawn stats and damage intake (Enemy.activate / Enemy.take_damage) -------------------------------

	// What activate() samples at spawn: card costs and the wave are applied THEN, so a mouse already on screen is never
	// retroactively buffed under the cursor.
	struct PcEnemySpawnStats
	{
		double Hp = 0.0;
		double MaxHp = 0.0;               // kept: Kitten Kill Explodes is a fraction of the victim's MAX hp
		double WaveRewardMultiplier = 1.0;
		double SpeedMultiplier = 1.0;     // the MOUSE_SPEED card cost
		double FunnelBounces = 0.0;       // whole bounces; the caller adds one more with chance = the fraction
	};
	PAWCORE_API PcEnemySpawnStats EnemySpawnStats(const PcGameData& D, const PcProgress& P, const PcEnemy& Enemy);

	// The state take_damage() reads besides the blow.
	struct PcBlowTarget
	{
		const PcEnemy* Data = nullptr;
		double Hp = 0.0;
		double WaveRewardMultiplier = 1.0;
		bool bShielded = false;
		bool bSnared = false;         // slowed this step (_slow_multiplier < 1)
		bool bKilledByBeam = false;   // Prism's beam, for ONE blow
	};
	struct PcBlowResult
	{
		bool bKilled = false;
		double Overkill = 0.0;        // the damage past the last hit point (0 unless killed): Overkill splash and bounty
		double HpAfter = 0.0;
		double Reward = 0.0;          // what `killed` carries: reward_catnip x wave x Play With Food x Spectrum
		bool bToyedWith = false;      // Play With Food paid (> x1)
	};
	// Shielded: nothing lands unless Shield Pierce lets a share through. Snared: x Traps. Then x the threshold multiplier
	// published for this step (every source arrives here, so a multiplier wired at the sources would miss one). Play With
	// Food keys on the HP BEFORE the blow against the BASE hp (a one-shot never qualifies).
	PAWCORE_API PcBlowResult ApplyBlow(const PcGameData& D, const PcProgress& P, PcBlowTarget& Target, double Amount,
		double ShieldPierce, double ThresholdNow);

	// --- kittens (Kitten.gd and TierController's pool) ---------------------------------------------------------------

	// TierController._ready(): the pool is Kitten Count's own cap PLUS the most Vitality can add (both from /data, not a
	// second number that could drift) PLUS headroom for temporary kittens, so Kitten From Signature has somewhere to put
	// one when the player is already at cap. It still grows if ownership passes it (the cap rises per biome and Reawaken).
	PAWCORE_API int64_t KittenPoolBaseSize(const PcGameData& D);

	// configure_orbit_slot(): an evenly spaced angle per kitten around whatever it hunts, so a pool converging on one mouse
	// rings it instead of stacking (random angles clustered by chance at small pool sizes).
	PAWCORE_API void KittenOrbitSlot(const PcGameData& D, int64_t Index, int64_t Total, double& OutX, double& OutY);

	// GameState.kittens_seek_clusters(): Kitten Foreman owned.
	PAWCORE_API bool KittensSeekClusters(const PcGameData& D, const PcProgress& P);

	// GameState.get_kitten_illusion_pull(): Grand Finale's pull on mice near a kitten (0 = off).
	PAWCORE_API double KittenIllusionPull(const PcGameData& D, const PcProgress& P);

	// Kitten._pick_target(): the LEAST-CLAIMED mouse in detection range, distance only breaking ties between equally
	// crowded mice (ranking by distance first and treating claims as yes/no is what made every surplus kitten converge on
	// one mouse: 1/1/8 instead of 4/3/3). Kitten Foreman inverts the primary key: the MOST company wins, so the pool goes
	// for the thickest part of the crowd. Candidates are `Order` (the live mice, in Godot's active_enemies order: ties go
	// to the first); X, Y, Claims are indexed by slot. Returns a slot, or -1 (nothing spotted: the kitten roams).
	PAWCORE_API int32_t PickKittenTarget(const PcGameData& D, double OX, double OY, const std::vector<int32_t>& Order,
		const std::vector<uint8_t>& Active, const std::vector<double>& X, const std::vector<double>& Y,
		const std::vector<int64_t>& Claims, bool bForeman);

	// Kitten._is_overcrowded(): this kitten's mouse carries at least REBALANCE_MARGIN more kittens than some other mouse
	// in range. The margin must be > 1: at 1, two kittens on a 2-claim vs 1-claim pair would swap every frame forever.
	PAWCORE_API bool KittenOvercrowded(const PcGameData& D, double OX, double OY, int32_t Target,
		const std::vector<int32_t>& Order, const std::vector<uint8_t>& Active, const std::vector<double>& X,
		const std::vector<double>& Y, const std::vector<int64_t>& Claims);

	// --- the aura, the Snare and the zones (TierController's aura passes, Enemy.apply_slow) ---------------------------

	// is_aura_owned(): the Aura is a purchase (v5); everything that draws or resolves it asks here.
	PAWCORE_API bool IsAuraOwned(const PcProgress& P);

	// get_aura_reach_multiplier(): the permanent half of the aura's size, from the Reach ladder (x1 unbought).
	PAWCORE_API double AuraReachMultiplier(const PcGameData& D, const PcProgress& P);

	// get_carry_radius_bonus(): Carry grows the aura while the cat holds still, full after carry_full_seconds (an
	// uncapped version rewards leaving the game alone). Reads PcProgress::PlayerStillSeconds.
	PAWCORE_API double CarryRadiusBonus(const PcGameData& D, const PcProgress& P);

	// get_aura_radius_multiplier(): (the shop's Aura Radius line + Carry) x Reach x the aura-radius card costs.
	PAWCORE_API double AuraRadiusMultiplier(const PcGameData& D, const PcProgress& P);

	// The aura's radius in px: base_aura_radius x AuraRadiusMultiplier.
	PAWCORE_API double AuraRadius(const PcGameData& D, const PcProgress& P);

	// get_snare_multiplier(): the speed every GROUND mouse keeps (x1 without the Snare). The branches store the slow
	// FRACTION, rising with the rung; Deadweight REPLACES the Snare's value rather than stacking.
	PAWCORE_API double SnareMultiplier(const PcGameData& D, const PcProgress& P);

	// get_gridlock_multiplier(): the speed a flier keeps over snared ground (x1 unless strictly between 0 and 1).
	PAWCORE_API double GridlockMultiplier(const PcGameData& D, const PcProgress& P);

	// get_aura_crowd_scaling() (Crowd Work), get_aura_echo_power() (Echo), get_aura_beat_seconds() (Standing Wave).
	PAWCORE_API double AuraCrowdScaling(const PcGameData& D, const PcProgress& P);
	PAWCORE_API double AuraEchoPower(const PcGameData& D, const PcProgress& P);
	PAWCORE_API double AuraBeatSeconds(const PcGameData& D, const PcProgress& P);

	// is_poop_ability_unlocked(): the Unicorn AND Rainbow Poop.
	PAWCORE_API bool PoopUnlocked(const PcProgress& P);

	// get_long_lunch_payout(): Long Lunch's bank as a multiple of one kill's reward, full after lunch_full_seconds.
	PAWCORE_API double LongLunchPayout(const PcGameData& D, const PcProgress& P);

	// Enemy.apply_slow(): keeps the STRONGEST slow of the step (a second, weaker one never speeds a mouse up). A slow
	// outside (0, 1) is ignored. Fliers refuse every slow (it is their verb) except Gridlock's (bReachesFliers).
	PAWCORE_API void ApplySlow(double& SlowMultiplier, uint8_t& bSlowFresh, bool bFlier, double Multiplier, bool bReachesFliers);

	// --- Pounce (TierController's Pounce, the Reflex and Daredevil nodes) --------------------------------------------

	// is_pounce_owned(): a skill-tree purchase. Before it is bought the ability does not exist: no cooldown ticks, no
	// auto-fire, the button is ignored (a cooldown ticking for an ability not owned would hand over a free first cast).
	PAWCORE_API bool IsPounceOwned(const PcProgress& P);

	// is_auto_pounce_unlocked() / get_auto_pounce_multiplier(): Reflex fires the instant it is ready, at its own multiplier
	// (it cannot time the vulnerable window: automation trades peak output for not having to click).
	PAWCORE_API bool AutoPounceUnlocked(const PcProgress& P);
	PAWCORE_API double AutoPounceMultiplier(const PcGameData& D, const PcProgress& P);

	// has_pounce_insurance() (Nine Lives Insurance), get_pounce_crater_power() (Crash Landing), get_pounce_lock_refund()
	// (Momentum: seconds of the Cone of Shame each kill takes off).
	PAWCORE_API bool HasPounceInsurance(const PcGameData& D, const PcProgress& P);
	PAWCORE_API double PounceCraterPower(const PcGameData& D, const PcProgress& P);
	PAWCORE_API double PounceLockRefund(const PcGameData& D, const PcProgress& P);

	// --- the paw's and the kill's extras, the spawner's nodes, the Prism ---------------------------------------------

	// get_guaranteed_crit_share() (Butcher's Rhythm), has_deterministic_crit() (Inscrutable), get_double_strike_share()
	// (Anvil Paw), get_paw_cleave_count() (Hammerfall), get_crit_execute_chance() (Riddle).
	PAWCORE_API double GuaranteedCritShare(const PcGameData& D, const PcProgress& P);
	PAWCORE_API bool DeterministicCrit(const PcGameData& D, const PcProgress& P);
	PAWCORE_API double DoubleStrikeShare(const PcGameData& D, const PcProgress& P);
	PAWCORE_API int64_t PawCleaveCount(const PcGameData& D, const PcProgress& P);
	PAWCORE_API double CritExecuteChance(const PcGameData& D, const PcProgress& P);

	// get_overkill_catnip_share() (Cat Burglar's Cut), get_overflow_ratio(), get_levy_multiple(), get_pickpocket_share().
	PAWCORE_API double OverkillCatnipShare(const PcGameData& D, const PcProgress& P);
	PAWCORE_API double OverflowRatio(const PcGameData& D, const PcProgress& P);
	PAWCORE_API double LevyMultiple(const PcGameData& D, const PcProgress& P);
	PAWCORE_API double PickpocketShare(const PcGameData& D, const PcProgress& P);

	// get_pied_piper_radius() (mice arrive in a ring around the cat, 0 = from the edges), has_spawn_foresight() (Sixth
	// Sense), get_vanish_interval() (Sleight of Hand: 60 / the node's vanishings per minute, 0 = off).
	PAWCORE_API double PiedPiperRadius(const PcGameData& D, const PcProgress& P);
	PAWCORE_API bool SpawnForesight(const PcGameData& D, const PcProgress& P);
	PAWCORE_API double VanishInterval(const PcGameData& D, const PcProgress& P);

	// The Prism: prism_owned(); get_prism_beam_dps() (aura DPS x the node, 0 unowned); prism_sweeps() (Full Spectrum);
	// get_prism_chain_count() (Refraction); get_prism_shard_payload() (Critical Mass, seconds of aura DPS).
	PAWCORE_API bool PrismOwned(const PcProgress& P);
	PAWCORE_API double PrismBeamDps(const PcGameData& D, const PcProgress& P, const PcCombatStats& Stats);
	PAWCORE_API bool PrismSweeps(const PcProgress& P);
	PAWCORE_API int64_t PrismChainCount(const PcGameData& D, const PcProgress& P);
	PAWCORE_API double PrismShardPayload(const PcGameData& D, const PcProgress& P);

	// --- the boss's automation ----------------------------------------------------------------------------------------

	// owns_auto_travel() / owns_auto_bait(): the nodes bought (automation is earned, not toggled); the game's settings
	// switch them on (has_auto_travel() = owned AND the setting).
	PAWCORE_API bool OwnsAutoTravel(const PcGameData& D, const PcProgress& P);
	PAWCORE_API bool OwnsAutoBait(const PcGameData& D, const PcProgress& P);
}
