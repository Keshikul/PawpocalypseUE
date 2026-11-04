#pragma once

// What the HUD shows, decided without the engine (docs/03 §2; phase 3). A port of the DECISIONS in Godot's hud.gd (and of
// the boss panel's in tier_controller.gd): which line a row or a plate says, what is enabled, which tab wears a badge. No
// strings: each readout is a kind and its numbers; the game module turns them into FText from the string tables. The rules
// these read are the ported managers' (PcProgress, PcEconomy, PcCards, PcAchievements, PcField), already proven against
// Godot; the PcHud tests pin the decisions themselves.

#include "PcCards.h"
#include "PcCore.h"
#include "PcData.h"
#include "PcStats.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Pc
{
	struct PcField;

	// --- the shop (hud.gd _refresh_upgrade_buttons / _refresh_one_button) ----------------------------------------------

	// The fifteen rows, top to bottom: the three kitten lines together (2026-08-18).
	PAWCORE_API const std::vector<std::string>& ShopRowOrder();

	// BUY_MULTIPLIERS: x1, x5, x10, Max (-1 = as many as the Catnip on hand pays for), cycled by one button.
	PAWCORE_API const std::vector<int64_t>& BuyMultipliers();
	// _on_multiplier_button_pressed(): the next one, wrapping past Max to x1; an unknown value restarts at x1's successor.
	PAWCORE_API int64_t NextBuyMultiplier(int64_t Current);

	enum class PcShopRowKind : uint8_t
	{
		Hidden,       // its tree branch is unbought: not shown at all (designer, 2026-08-18)
		KittenLocked, // Kitten Power / Kitten Speed with no kitten owned: "buy a Kitten first", disabled
		Max,          // at its cap: "(Lv n) — MAX", disabled
		Price,        // "(Lv n) — price": the next level's price (x1), or one level's when a bulk press affords none
		Bulk,         // "(Lv n) — +levels for price": what a bulk press will actually buy
	};

	struct PcShopRow
	{
		PcShopRowKind Kind = PcShopRowKind::Hidden;
		int64_t Level = 0;
		double Price = 0.0;
		int64_t Levels = 0;   // Bulk only
		bool bEnabled = false;
	};
	PAWCORE_API PcShopRow ShopRow(const PcGameData& D, const PcProgress& P, const std::string& UpgradeId, int64_t BuyMultiplier);
	// _on_upgrade_button_pressed(): the press buys what the row says (purchase_upgrade_bulk); the levels bought.
	PAWCORE_API int64_t PressShopRow(const PcGameData& D, PcProgress& P, const std::string& UpgradeId, int64_t BuyMultiplier);

	// --- the status row --------------------------------------------------------------------------------------------

	// _on_xp_changed(): the Catches bar fills WITHIN the level (the totals are a geometric series; their ratio reads full).
	struct PcCatchesBar
	{
		int64_t Level = 0;
		double Fraction = 0.0;
	};
	PAWCORE_API PcCatchesBar CatchesBar(const PcGameData& D, const PcProgress& P);

	// _bait_is_calling(): this biome's boss can be summoned, its bait is unlocked, and it is not beaten this cycle.
	PAWCORE_API bool BaitIsCalling(const PcField& F, const PcGameData& D, const PcProgress& P);

	// _refresh_zone_progress(): the plate under the currencies.
	enum class PcZonePlateKind : uint8_t
	{
		Hidden,   // every biome open and visited: nothing to describe
		Bait,     // "Bait ready — summon <boss>" (bait first: designer, 2026-10-01)
		Ready,    // "Next biome — ready": open past the deepest biome reached this run
		Progress, // "Next biome — have / need": the first still-locked biome's Catnip gate
	};
	struct PcZonePlate
	{
		PcZonePlateKind Kind = PcZonePlateKind::Hidden;
		double Fraction = 0.0;
		double Have = 0.0, Need = 0.0;
		std::string BossId;     // Bait: the boss to name
		bool bRainbow = false;  // the rainbow outline and the lean: Bait or Ready
	};
	PAWCORE_API PcZonePlate ZonePlate(const PcField& F, const PcGameData& D, const PcProgress& P);

	// _refresh_wave_progress(): above the zone plate. Bait held / needed when this biome holds a boss, else the wave.
	struct PcWaveLine
	{
		bool bVisible = false;   // false in a biome with no waves (kills_per_wave = 0)
		double Fraction = 0.0;
		bool bBait = false;
		int64_t Held = 0, Needed = 0, Wave = 0;
		double Done = 0.0, Cost = 0.0;
	};
	PAWCORE_API PcWaveLine WaveLine(const PcGameData& D, const PcProgress& P);

	// --- the field overlay -----------------------------------------------------------------------------------------

	// _refresh_pounce_gauge(): hidden until Pounce is owned; 0 just fired, 1 ready.
	struct PcPounceClock
	{
		bool bVisible = false;
		double Fraction = 1.0;
	};
	PAWCORE_API PcPounceClock PounceClock(const PcField& F, const PcProgress& P);

	// tier_controller.gd _refresh_boss_status_label() / _refresh_boss_timer_label(), while the boss is up.
	struct PcBossPanel
	{
		bool bVisible = false;
		double Fraction = 0.0;   // HP left, 0..1
		int64_t Phase = 1;
		bool bShielded = false;
		bool bTimerVisible = false; // a time limit, and the fight engaged
		double Remaining = 0.0;     // seconds
		bool bTimerDanger = false;  // ten seconds or less: red
	};
	PAWCORE_API PcBossPanel BossPanel(const PcField& F, const PcGameData& D);

	// --- the dock's tabs (hud.gd _build_tabs / _refresh_tab_bar) ------------------------------------------------------

	enum class PcDockTab : uint8_t { Upgrades, Reawaken, Realms, Catches, Achievements, Jobs, Count };
	struct PcTabState
	{
		bool bVisible = true;
		bool bBadge = false;     // the star: something actionable inside, never on the open tab (Achievements excepted)
		int64_t Count = 0;       // Achievements: rewards waiting, shown even while open
	};
	// JobsSeen = how many jobs were held when the Jobs tab was last open (-1 before the session's first HUD seeded it).
	PAWCORE_API PcTabState TabState(const PcField& F, const PcGameData& D, const PcProgress& P, PcDockTab Tab, PcDockTab Active,
		int64_t JobsSeen);
	// _any_upgrade_affordable(), _any_realm_newly_open() / _realm_is_new().
	PAWCORE_API bool AnyUpgradeAffordable(const PcGameData& D, const PcProgress& P);
	PAWCORE_API bool IsRealmNew(const PcGameData& D, const PcProgress& P, int64_t Index);

	// --- the Realms page (hud.gd _refresh_travel_panel, _refresh_stage_label, _refresh_bait_button) --------------------

	enum class PcRealmRowKind : uint8_t
	{
		NeedsCatnip, // "<biome> — earn <amount> more": its passage is open, its Catnip gate is not
		Here,        // "<biome> — you are here", disabled, in the good colour
		Travel,      // "Travel: <biome>"
		TravelBoss,  // "Travel: <biome> — <boss>", in the danger colour (cleared: "(cleared)", in the Stardust colour)
		Locked,      // "<biome> — locked": its evolution stage is not reached, or a boss before it is not beaten this run
	};
	struct PcRealmRow
	{
		PcRealmRowKind Kind = PcRealmRowKind::Locked;
		double Owed = 0.0;           // NeedsCatnip
		std::string BossId;          // TravelBoss
		bool bCleared = false;       // TravelBoss: beaten (ever)
		bool bEnabled = false;
		bool bNew = false;           // open past the deepest biome reached this run: the row warms
		bool bShowReward = false;    // Cartography: "(x<multiplier> Catnip)" appended to a travel row
		double RewardMultiplier = 1.0;
	};
	// The rows read the passage (the stage AND this cycle's bosses: is_passage_open, F22, designer 2026-11-03) and the Catnip.
	PAWCORE_API PcRealmRow RealmRow(const PcGameData& D, const PcProgress& P, int64_t Index);

	// _refresh_stage_label(): the stage, and the biome of the boss whose death grants the next one ("" = "the next boss").
	struct PcStageLine
	{
		int64_t Stage = 1;
		bool bFullyEvolved = false;
		std::string NextBossTier;
	};
	PAWCORE_API PcStageLine StageLine(const PcGameData& D, const PcProgress& P);

	// _refresh_bait_button(): shown in every biome until every boss has fallen this run.
	enum class PcBaitKind : uint8_t
	{
		Hidden,       // every boss beaten this cycle
		Elsewhere,    // no boss here: "Bait — the next boss is in <biome>", disabled
		BossHere,     // the fight is on: "<boss> is here"
		BaitOwed,     // "<boss> — <n> more bait"
		CatnipShort,  // "Bait <boss> — earn <amount> more"
		Ready,        // "Buy Bait — summon <boss>"
	};
	struct PcBaitButton
	{
		PcBaitKind Kind = PcBaitKind::Hidden;
		std::string BossId;
		std::string NextBossTier;   // Elsewhere
		int64_t BaitOwed = 0;
		double CatnipShort = 0.0;
		bool bEnabled = false;      // can summon and the bait is unlocked
		bool bCalling = false;      // BaitIsCalling: the button leans
	};
	PAWCORE_API PcBaitButton BaitButton(const PcField& F, const PcGameData& D, const PcProgress& P);

	// --- the Reawaken page (hud.gd _refresh_reawaken_page) -------------------------------------------------------------

	enum class PcReawakenButtonKind : uint8_t
	{
		Confirm,  // the second press armed: "Confirm — this resets your life"
		Early,    // after the first, before the suggestion: "Reawaken early — cash out +<amount>"
		Reawaken, // "Reawaken (Reawaken <n>)"
		NotYet,   // below the gate: "Not worth it yet — only +<amount>", disabled
	};
	struct PcReawakenPage
	{
		double Earned = 0.0, Required = 0.0, Fraction = 0.0; // this cycle's Catnip toward the (suggested) point
		bool bFirst = true;           // no Reawaken yet: the bar is a gate; after it, a suggestion
		double Pending = 0.0;         // the Stardust it would pay now
		double TotalAfter = 0.0;      // the spendable Stardust after it
		double MultiplierNow = 1.0, MultiplierAfter = 1.0;
		bool bReady = false;          // can_reawaken(): the button is enabled
		PcReawakenButtonKind Button = PcReawakenButtonKind::NotYet;
		int64_t Count = 0;            // Reawakens so far
		bool bUnicorn = false;        // "The Unicorn has awakened — Rainbow Poop active"
	};
	PAWCORE_API PcReawakenPage ReawakenPage(const PcGameData& D, const PcProgress& P, bool bConfirmArmed);

	// --- the Catches page (hud.gd _refresh_catches_page) ----------------------------------------------------------------

	// CardData._fmt(): a card's {v} / {c} without trailing zeros. 2.0 -> "2", 0.35 -> "0.35", 0.1 -> "0.1".
	PAWCORE_API std::string CardNumber(double Value);

	// One row per card in the deck, in deck order (every card, not only the taken ones: designer, 2026-09-11).
	struct PcCatchesRow
	{
		std::string CardId;
		bool bHeld = false;       // "<name>  (Lv <level>/<max>)" in the good colour, then describe(level)
		int64_t Level = 0, MaxLevel = 0;
		double Value = 0.0;       // held: value_at(level), cost_at(level), for describe()'s {v} and {c}
		double Cost = 0.0;
		PcCardHint Hint;          // not held: the hint, or "Not taken yet." when None; muted
	};
	struct PcCatchesPage
	{
		std::vector<PcCatchesRow> Rows;
		int64_t Taken = 0;          // 0: "No cards taken yet this life..."; else "<n> card(s) taken this life."
		bool bPickingAuto = false;  // + " Picking automatically." (the setting, as Godot reads it, owned or not)
		bool bAutoPickShown = false; // the Auto-pick box: only once Auto-Card is owned (owns_auto_card())
	};
	PAWCORE_API PcCatchesPage CatchesPage(const PcGameData& D, const PcProgress& P, bool bAutoPickSetting);

	// --- the Achievements page (hud.gd _refresh_achievement_list) ------------------------------------------------------

	enum class PcAchievementRowKind : uint8_t
	{
		Claimed,  // "★  <name>\n     <description>", in the Upgrades tab's colour
		Earned,   // "◆  <name>\n     <description>", in the good colour (claimable or not)
		Secret,   // "☆  ???", disabled grey
		Progress, // "☆  <name>\n     <value> / <threshold>", muted
	};
	struct PcAchievementRow
	{
		std::string Id;
		PcAchievementRowKind Kind = PcAchievementRowKind::Progress;
		double Value = 0.0, Threshold = 0.0;  // Progress
		bool bClaimable = false;              // the Claim button: "Claim <amount>" with the Stardust icon
		double Reward = 0.0;
	};
	struct PcAchievementsPage
	{
		std::vector<PcAchievementRow> Rows;   // in the data's order
		int64_t Earned = 0, Total = 0;
		int64_t Waiting = 0;                  // > 0: the summary names them and their Stardust; Claim all shows
		double WaitingStardust = 0.0;
	};
	PAWCORE_API PcAchievementsPage AchievementsPage(const PcGameData& D, const PcProgress& P);

	// --- the Jobs page (hud.gd _refresh_job_page, _job_unlock_text) ----------------------------------------------------

	// The row's button is a statement, never a control (swapping mid-run is what the lock prevents): Worn / Owned / Locked.
	enum class PcJobRowKind : uint8_t
	{
		Worn,          // held and locked in for this run (run_job): name + description, in the text colour
		Owned,         // held: name + description, muted
		NoNode,        // "Locked — no skill tree node grants this yet."
		Closed,        // "Closed — you took <branch> instead."
		BossGate,      // "Locked — beat the boss in <biome>, then buy it in the skill tree."
		ReawakenGate,  // "Locked — Reawaken once / <n> times, then buy it in the skill tree."
		Ready,         // "Ready — buy it in the skill tree for <n> Stardust."
	};
	struct PcJobRow
	{
		std::string JobId;
		PcJobRowKind Kind = PcJobRowKind::NoNode;
		std::string RivalBranch;    // Closed
		std::string BossTier;       // BossGate
		int64_t Reawakens = 0;      // ReawakenGate
		double Cost = 0.0;          // Ready: whole Stardust (int(cost))
	};
	// One row per job, in all_jobs()'s order.
	PAWCORE_API std::vector<PcJobRow> JobRows(const PcGameData& D, const PcProgress& P);

	// --- the card pick (catches_pick_ui.gd _on_hand_offered, _make_card, _refresh_reroll) ------------------------------

	struct PcPickCard
	{
		std::string CardId;
		CardFlavour Flavour = CardFlavour::AURA; // the header band's colour and word (FLAVOUR_COLOURS / FLAVOUR_NAMES)
		int64_t Level = 1;       // next_level_of(): the level taking it gives
		int64_t MaxLevel = 1;
		bool bNew = true;        // level <= 1: "NEW" (the alert colour); else "LEVEL <level> of <max>", muted
		double Value = 0.0;      // describe(level): {v} and {c} at the level taking it gives
		double Cost = 0.0;
		bool bCosts = false;     // cost != NONE: the alert-red frame and "This one costs you something."
	};
	struct PcCardPick
	{
		bool bOpen = false;                   // a hand is up (the screen shows and pauses the world)
		int64_t Level = 0;                    // the title: "LEVEL <n>" (catches_level)
		std::vector<PcPickCard> Cards;        // in deal order; each has a Take
		std::vector<std::string> Foresight;   // Blueprint: "Next hand will hold: <cards>" under the subtitle, when any
		int64_t Rerolls = 0;                  // > 0: "Reroll  (<n> left)"; 0: no button
		bool bBanish = false;                 // a banish held: every card shows Banish under Take
	};
	PAWCORE_API PcCardPick CardPick(const PcGameData& D, const PcProgress& P);

	// --- the job picker (job_picker.gd open(), _add_row, _pick) ----------------------------------------------------------
	// Asked once, after a Reawaken's spend step, when JobPicker.should_ask() (PcJobs ShouldAskForJob); the travel waits for
	// the answer. Under Union Rules a first answer that leaves another job held asks again, for the second slot.

	struct PcJobPickRow
	{
		std::string JobId;     // "" = the baseline ("No job", or "No second job"), always the last row
		bool bLastRun = false; // JobId == current_job: "<name>   (last run)", and the selection opens on it
	};
	struct PcJobPicker
	{
		bool bSecond = false;           // asking for Union Rules' second job: the primary is not offered again
		std::vector<PcJobPickRow> Rows; // the held jobs that have data, by id; then the baseline
		int64_t Focus = 0;              // the row the selection opens on: the last-run row, else the first
		int64_t SharePercent = 0;       // bSecond: every blurb opens "At <n>% strength." = roundi(union_rules_share() x 100)
	};
	PAWCORE_API PcJobPicker JobPicker(const PcGameData& D, const PcProgress& P, bool bSecond);
	// _pick(): a press on a row. The first answer locks the run's job (and the preference); then, with Union Rules and
	// another job held, returns true: ask again for the second. Otherwise (and after the second answer) the second slot is
	// locked and the run counts as asked. A refused lock is not an error here, as in Godot: the run is asked either way.
	PAWCORE_API bool PickJob(const PcGameData& D, PcProgress& P, bool bSecond, const std::string& Id);

	// --- the victory screen (victory_screen.gd) ---------------------------------------------------------------------------

	// VictoryScreen.format_duration(): the run time in whole seconds (int(maxf(seconds, 0.0)): negatives read 0, fractions
	// drop). With hours: "{h}h {m}m {s}s", minutes and seconds two digits; without: "{m}m {s}s", seconds two digits.
	struct PcDuration
	{
		bool bHours = false;
		int64_t H = 0;
		int64_t M = 0;
		int64_t S = 0;
	};
	PAWCORE_API PcDuration DurationParts(double Seconds);
}
