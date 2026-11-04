#pragma once

// The job layer (docs/03 §2 PcJobs): which jobs the player holds, the one locked in for the run, Union Rules' second job,
// and the trade each job makes on stats that already exist. A port of GameState's job functions (game_state.gd "Jobs")
// and JobData's two predicates (job_data.gd), plus the job picker's two rules (job_picker.gd).
//
// Every job is a TRADE, never an upgrade: a trade is not a multiplier, so the layer spends nothing from the exponent
// budget. A job is picked once, before the run, and cannot be swapped mid-run; that is what turns the tree from a
// shopping list into a build.

#include "PcCore.h"
#include "PcData.h"
#include "PcStats.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Pc
{
	// all_jobs(): boss drops first, by biome id; then the Reawaken-gated ones by required_reawakens. Ties keep the data's
	// order (Godot's sort_custom is an insertion sort below 16 elements, so it is stable there; the fixture pins it).
	PAWCORE_API std::vector<const PcJob*> AllJobs(const PcGameData& D);
	// holds_job()
	PAWCORE_API bool HoldsJob(const PcProgress& P, const std::string& Id);
	// job_is_available(): held, and nothing else. The job's own `dropped_by_biome` / `required_reawakens` only say HOW it is
	// earned; whether it can be had is a question about its skill tree node, the only thing that grants it.
	PAWCORE_API bool JobIsAvailable(const PcGameData& D, const PcProgress& P, const std::string& Id);
	// offer_job(): hands a job over; false for an unknown id or one already held. The caller emits job_offered on true.
	PAWCORE_API bool OfferJob(const PcGameData& D, PcProgress& P, const std::string& Id);

	// lock_run_job(): the commitment, offered once per run. "" (a bare neck) is legal; a job not held is refused. Also sets
	// the standing preference. The caller rebuilds the stats and emits job_taken on true.
	PAWCORE_API bool LockRunJob(PcProgress& P, const std::string& Id);
	// lock_run_job_second(): "" always clears it; otherwise refused without Union Rules, for the primary, or for a job not
	// held. The caller rebuilds the stats on true.
	PAWCORE_API bool LockRunJobSecond(const PcGameData& D, PcProgress& P, const std::string& Id);
	// take_job(): the standing preference only (the picker's default), changeable any time; false when not held or
	// unchanged. Never what the run plays with.
	PAWCORE_API bool TakeJob(PcProgress& P, const std::string& Id);
	// The Reawaken's part: a new run asks again. The preference (CurrentJob) stays, so the picker offers it back.
	PAWCORE_API void ClearRunJobs(PcProgress& P);

	// has_union_rules(): whether a second job may be worn at all.
	PAWCORE_API bool HasUnionRules(const PcGameData& D, const PcProgress& P);
	// union_rules_share(): the share of its own effect the second job contributes, clamped to [0, 1].
	PAWCORE_API double UnionRulesShare(const PcGameData& D, const PcProgress& P);
	// equipped_job_data(): READS RunJob, NOT CurrentJob. Exactly one place answers "which job is in effect".
	PAWCORE_API const PcJob* EquippedJob(const PcGameData& D, const PcProgress& P);
	// secondary_job_data(): null without Union Rules even when a second job is stored (a save made with the node and loaded
	// after a respec must not keep paying out), and null when it names the primary.
	PAWCORE_API const PcJob* SecondaryJob(const PcGameData& D, const PcProgress& P);

	// job_term(): one job multiplier with the second job folded in at its share. EVERY job multiplier goes through here.
	// Halving a MULTIPLIER is not halving the number: x2.5 at half strength is x1.75, not x1.25.
	PAWCORE_API double JobTerm(const PcGameData& D, const PcProgress& P, double PcJob::* Field);
	// get_job_crit_chance_bonus(): ADDITIVE, so the second job's share scales the number itself.
	PAWCORE_API double JobCritChanceBonus(const PcGameData& D, const PcProgress& P);
	// get_job_extra_cards(): counted, and the second job's share FLOORED: half a card is no card.
	PAWCORE_API int64_t JobExtraCards(const PcGameData& D, const PcProgress& P);
	// get_job_pounce_lock_sec(): a COST in seconds, so the second job's share is ADDED: two rooting jobs root you longer.
	PAWCORE_API double JobPounceLockSec(const PcGameData& D, const PcProgress& P);
	// get_job_hat_path(): the primary job's hat, "" for a bare head. The second job wears no hat.
	PAWCORE_API std::string JobHatPath(const PcGameData& D, const PcProgress& P);

	// JobData.is_neutral(): changes nothing at all (the validator rejects such a job).
	PAWCORE_API bool IsJobNeutral(const PcJob& J);
	// JobData.has_a_cost(): gives something up. enemy_health reads the other way: tougher enemies ARE the cost.
	PAWCORE_API bool JobHasACost(const PcJob& J);

	// JobPicker.should_ask(): a decision worth interrupting for. Nothing held is not a choice; one held still asks (Godot
	// counts held jobs >= 1).
	PAWCORE_API bool ShouldAskForJob(const PcProgress& P);
	// JobPicker._other_jobs_than(): held jobs other than `Id`. Union Rules asks for a second job only when this is > 0.
	PAWCORE_API int64_t OtherJobsThan(const PcProgress& P, const std::string& Id);
}
