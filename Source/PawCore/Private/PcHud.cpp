#include "PcHud.h"

#include "PcAchievements.h"
#include "PcCards.h"
#include "PcEconomy.h"
#include "PcField.h"
#include "PcFieldRules.h"
#include "PcJobs.h"
#include "PcProgress.h"
#include "PcTree.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Pc
{
	namespace
	{
		const PcCard* CardById(const PcGameData& D, const std::string& Id)
		{
			for (const PcCard& C : D.Cards)
			{
				if (C.Id == Id)
				{
					return &C;
				}
			}
			return nullptr;
		}

		const PcTier* TierById(const PcGameData& D, const std::string& Id)
		{
			for (const PcTier& T : D.Tiers)
			{
				if (T.Id == Id)
				{
					return &T;
				}
			}
			return nullptr;
		}
	}

	const std::vector<std::string>& ShopRowOrder()
	{
		static const std::vector<std::string> Order = { "paw_damage", "attack_speed", "crit_chance", "shred", "execute",
			"aura_radius", "currency_find", "spawn_rate", "kitten_count", "kitten_power", "kitten_speed", "shield_pierce",
			"pounce_power", "pounce_cooldown", "overkill" };
		return Order;
	}

	const std::vector<int64_t>& BuyMultipliers()
	{
		static const std::vector<int64_t> Values = { 1, 5, 10, -1 };
		return Values;
	}

	int64_t NextBuyMultiplier(int64_t Current)
	{
		const std::vector<int64_t>& V = BuyMultipliers();
		size_t Index = 0; // an unknown value counts as x1 (Godot's loop leaves index at 0)
		for (size_t I = 0; I < V.size(); ++I)
		{
			if (V[I] == Current)
			{
				Index = I;
				break;
			}
		}
		return V[(Index + 1) % V.size()];
	}

	PcShopRow ShopRow(const PcGameData& D, const PcProgress& P, const std::string& UpgradeId, int64_t BuyMultiplier)
	{
		PcShopRow Row;
		// _refresh_kitten_power_button() / _refresh_kitten_speed_button() test the lock BEFORE visibility.
		if ((UpgradeId == "kitten_power" && IsKittenPowerLocked(D, P)) || (UpgradeId == "kitten_speed" && IsKittenSpeedLocked(D, P)))
		{
			Row.Kind = PcShopRowKind::KittenLocked;
			return Row;
		}
		if (!IsUpgradeVisible(P, UpgradeId))
		{
			return Row;
		}
		Row.Level = UpgradeLevel(P, UpgradeId);
		if (IsUpgradeAtCap(D, P, UpgradeId))
		{
			Row.Kind = PcShopRowKind::Max;
			return Row;
		}
		if (BuyMultiplier == 1)
		{
			Row.Kind = PcShopRowKind::Price;
			Row.Price = UpgradeCostById(D, P, UpgradeId);
			// The purchase's own test, epsilon included (F30, designer 2026-11-04; hud.gd 1678).
			Row.bEnabled = !(P.Catnip + D.Code.CurrencyEpsilon < Row.Price);
			return Row;
		}
		const PcBulkPreview Bulk = PreviewBulkPurchase(D, P, UpgradeId, BuyMultiplier);
		if (Bulk.Levels <= 0)
		{
			// "buy 0 levels for 0 Catnip" says nothing about what the player is short of: name ONE level's price.
			Row.Kind = PcShopRowKind::Price;
			Row.Price = UpgradeCostById(D, P, UpgradeId);
			return Row;
		}
		Row.Kind = PcShopRowKind::Bulk;
		Row.Levels = Bulk.Levels;
		Row.Price = Bulk.Cost;
		Row.bEnabled = true;
		return Row;
	}

	int64_t PressShopRow(const PcGameData& D, PcProgress& P, const std::string& UpgradeId, int64_t BuyMultiplier)
	{
		return PurchaseUpgradeBulk(D, P, UpgradeId, BuyMultiplier);
	}

	PcCatchesBar CatchesBar(const PcGameData& D, const PcProgress& P)
	{
		PcCatchesBar Bar;
		Bar.Level = P.CatchesLevel;
		const double Floor = TotalCatchesForLevel(D, P.CatchesLevel);
		const double Span = TotalCatchesForLevel(D, P.CatchesLevel + 1) - Floor;
		Bar.Fraction = Span > 0.0 ? std::clamp((P.CatchesPoints - Floor) / Span, 0.0, 1.0) : 0.0;
		return Bar;
	}

	bool BaitIsCalling(const PcField& F, const PcGameData& D, const PcProgress& P)
	{
		if (F.TierIndex < 0 || F.TierIndex >= static_cast<int64_t>(D.Tiers.size()))
		{
			return false;
		}
		const PcTier& Tier = D.Tiers[static_cast<size_t>(F.TierIndex)];
		const PcBoss* Boss = Tier.BossData.empty() ? nullptr : D.FindBoss(Tier.BossData);
		return Boss && CanSummonFieldBoss(F, D, P) && IsBaitUnlocked(D, P, *Boss) && !IsBossClearedThisCycle(P, Tier.Id);
	}

	PcZonePlate ZonePlate(const PcField& F, const PcGameData& D, const PcProgress& P)
	{
		PcZonePlate Plate;
		const PcTier* NextLocked = nullptr;
		for (const PcTier& T : D.Tiers)
		{
			if (!IsBiomeUnlocked(D, P, T))
			{
				NextLocked = &T;
				break;
			}
		}
		bool bCanTravel = false;
		for (size_t I = 0; I < D.Tiers.size(); ++I)
		{
			if (IsBiomeUnlocked(D, P, D.Tiers[I]) && static_cast<int64_t>(I) > P.DeepestBiomeIndex)
			{
				bCanTravel = true;
				break;
			}
		}
		const bool bBait = BaitIsCalling(F, D, P);
		Plate.bRainbow = bCanTravel || bBait;
		if (bBait)
		{
			Plate.Kind = PcZonePlateKind::Bait;
			Plate.Fraction = 1.0;
			Plate.BossId = D.Tiers[static_cast<size_t>(F.TierIndex)].BossData;
			return Plate;
		}
		if (bCanTravel)
		{
			Plate.Kind = PcZonePlateKind::Ready;
			Plate.Fraction = 1.0;
			return Plate;
		}
		if (!NextLocked)
		{
			return Plate;
		}
		Plate.Kind = PcZonePlateKind::Progress;
		Plate.Need = NextLocked->CatnipToUnlock;
		Plate.Have = std::max(Plate.Need - CatnipUntilUnlocked(P, *NextLocked), 0.0);
		Plate.Fraction = Plate.Need <= 0.0 ? 0.0 : std::clamp(Plate.Have / Plate.Need, 0.0, 1.0);
		return Plate;
	}

	PcWaveLine WaveLine(const PcGameData& D, const PcProgress& P)
	{
		PcWaveLine Line;
		const PcWaveProgress Progress = WaveProgress(D, P);
		if (Progress.Costs <= 0.0)
		{
			return Line;
		}
		Line.bVisible = true;
		Line.Done = Progress.Done;
		Line.Cost = Progress.Costs;
		Line.Fraction = std::clamp(Progress.Done / Progress.Costs, 0.0, 1.0);
		Line.Wave = P.CurrentWaveIndex;
		const PcTier* Realm = TierById(D, P.CurrentBiomeId);
		const PcBoss* Boss = Realm && !Realm->BossData.empty() ? D.FindBoss(Realm->BossData) : nullptr;
		Line.Needed = Boss ? Boss->BaitRequired : 0;
		if (Line.Needed > 0)
		{
			Line.bBait = true;
			Line.Held = std::min(BaitHeld(D, P, P.CurrentBiomeId), Line.Needed);
		}
		return Line;
	}

	PcPounceClock PounceClock(const PcField& F, const PcProgress& P)
	{
		PcPounceClock Clock;
		Clock.bVisible = IsPounceOwned(P);
		Clock.Fraction = Clock.bVisible ? std::clamp(PounceCooldownFraction(F), 0.0, 1.0) : 1.0;
		return Clock;
	}

	PcBossPanel BossPanel(const PcField& F, const PcGameData& D)
	{
		PcBossPanel Panel;
		if (!F.bBossUp || F.TierIndex < 0 || F.TierIndex >= static_cast<int64_t>(D.Tiers.size()))
		{
			return Panel;
		}
		const PcBoss* Boss = D.FindBoss(D.Tiers[static_cast<size_t>(F.TierIndex)].BossData);
		if (!Boss)
		{
			return Panel;
		}
		Panel.bVisible = true;
		Panel.Fraction = Boss->TotalHp > 0.0 ? std::clamp(F.BossHp / Boss->TotalHp, 0.0, 1.0) : 0.0;
		Panel.Phase = F.BossPhase;
		Panel.bShielded = F.bBossShielded;
		const double Limit = Boss->TimeLimitSec;
		Panel.bTimerVisible = Limit > 0.0 && F.bBossEngagedOnce;
		Panel.Remaining = std::max(0.0, Limit - F.BossFightElapsed);
		Panel.bTimerDanger = Panel.bTimerVisible && Panel.Remaining <= 10.0;
		return Panel;
	}

	bool AnyUpgradeAffordable(const PcGameData& D, const PcProgress& P)
	{
		// UPGRADE_PATHS: every upgrade, visible or not (Godot's loop does not ask), Kitten Power's lock excepted.
		for (const PcUpgrade& U : D.Upgrades)
		{
			if (IsUpgradeAtCap(D, P, U.Id))
			{
				continue;
			}
			if (U.Id == "kitten_power" && IsKittenPowerLocked(D, P))
			{
				continue;
			}
			if (P.Catnip >= UpgradeCostById(D, P, U.Id))
			{
				return true;
			}
		}
		return false;
	}

	bool IsRealmNew(const PcGameData& D, const PcProgress& P, int64_t Index)
	{
		if (Index <= P.DeepestBiomeIndex || Index < 0 || Index >= static_cast<int64_t>(D.Tiers.size()))
		{
			return false;
		}
		return IsBiomeUnlocked(D, P, D.Tiers[static_cast<size_t>(Index)]);
	}

	PcTabState TabState(const PcField& F, const PcGameData& D, const PcProgress& P, PcDockTab Tab, PcDockTab Active, int64_t JobsSeen)
	{
		(void)F;
		PcTabState S;
		bool bAction = false;
		switch (Tab)
		{
		case PcDockTab::Upgrades:
			bAction = AnyUpgradeAffordable(D, P);
			break;
		case PcDockTab::Reawaken:
			bAction = IsReawakenSuggested(D, P);
			break;
		case PcDockTab::Realms:
			for (int64_t I = 0; I < static_cast<int64_t>(D.Tiers.size()) && !bAction; ++I)
			{
				bAction = IsRealmNew(D, P, I);
			}
			break;
		case PcDockTab::Catches:
			bAction = !P.Hand.empty();
			break;
		case PcDockTab::Achievements:
			// A count, shown even while the tab is open; the star with it (designer, 2026-10-01).
			S.Count = UnclaimedAchievementCount(D, P);
			S.bBadge = S.Count > 0;
			return S;
		case PcDockTab::Jobs:
			S.bVisible = !P.JobsHeld.empty();
			bAction = static_cast<int64_t>(P.JobsHeld.size()) > JobsSeen;
			break;
		case PcDockTab::Count:
			S.bVisible = false;
			return S;
		}
		S.bBadge = S.bVisible && Tab != Active && bAction;
		return S;
	}

	PcRealmRow RealmRow(const PcGameData& D, const PcProgress& P, int64_t Index)
	{
		PcRealmRow Row;
		if (Index < 0 || Index >= static_cast<int64_t>(D.Tiers.size()))
		{
			return Row;
		}
		const PcTier& Tier = D.Tiers[static_cast<size_t>(Index)];
		// PASSAGE, not just the permanent stage (F22, designer 2026-11-03): a biome behind a boss not beaten THIS run is
		// locked, as auto-travel and the zone bar have it.
		const bool bPassage = IsPassageOpen(D, P, Tier.RequiredEvolutionStage);
		Row.bNew = IsRealmNew(D, P, Index);
		// The gate names what is missing: the Catnip (here), or the boss (the stage line above the list).
		const double Owed = CatnipUntilUnlocked(P, Tier);
		if (Owed > 0.0 && bPassage)
		{
			Row.Kind = PcRealmRowKind::NeedsCatnip;
			Row.Owed = Owed;
			return Row;
		}
		// You are here: disabled rather than hidden, a map that drops your square is a worse map.
		if (Tier.Id == P.CurrentBiomeId)
		{
			Row.Kind = PcRealmRowKind::Here;
			return Row;
		}
		if (!bPassage)
		{
			return Row; // Locked
		}
		Row.bEnabled = true;
		if (!Tier.BossData.empty())
		{
			Row.Kind = PcRealmRowKind::TravelBoss;
			Row.BossId = Tier.BossData;
			Row.bCleared = IsBossDefeated(P, Tier.Id);
		}
		else
		{
			Row.Kind = PcRealmRowKind::Travel;
		}
		// CARTOGRAPHY: what the place pays, appended so a boss biome still names its boss.
		Row.bShowReward = BranchEffect(D, P, "cartography") > 0.0;
		Row.RewardMultiplier = Tier.RewardMultiplier;
		return Row;
	}

	PcStageLine StageLine(const PcGameData& D, const PcProgress& P)
	{
		PcStageLine Line;
		Line.Stage = EvolutionStage(D, P);
		if (Line.Stage >= D.Code.EvolutionStageCount)
		{
			Line.bFullyEvolved = true;
			return Line;
		}
		// _boss_gating_stage(stage + 1): stage N comes from the Nth boss, BOSS_REALMS[N - 2] in clear order.
		const int64_t Index = Line.Stage - 1;
		if (Index >= 0 && Index < static_cast<int64_t>(D.Code.BossRealms.size()))
		{
			const std::string& Wanted = D.Code.BossRealms[static_cast<size_t>(Index)];
			const PcTier* Tier = TierById(D, Wanted);
			if (Tier && !Tier->BossData.empty())
			{
				Line.NextBossTier = Wanted;
			}
		}
		return Line;
	}

	PcBaitButton BaitButton(const PcField& F, const PcGameData& D, const PcProgress& P)
	{
		PcBaitButton B;
		const PcTier* Here = F.TierIndex >= 0 && F.TierIndex < static_cast<int64_t>(D.Tiers.size())
			? &D.Tiers[static_cast<size_t>(F.TierIndex)] : nullptr;
		const PcBoss* Boss = Here && !Here->BossData.empty() ? D.FindBoss(Here->BossData) : nullptr;
		if (!Boss)
		{
			// Shown in every biome: outside a boss biome it names where the next boss is (_next_boss_biome: the first on
			// the ladder not beaten THIS RUN); hidden once every boss has fallen.
			for (const std::string& Id : D.Code.BossRealms)
			{
				if (!IsBossClearedThisCycle(P, Id))
				{
					B.Kind = PcBaitKind::Elsewhere;
					B.NextBossTier = Id;
					break;
				}
			}
			return B;
		}
		B.BossId = Here->BossData;
		const bool bCanSummon = CanSummonFieldBoss(F, D, P);
		const bool bAfford = CanAffordBait(P, *Boss);
		const int64_t Owed = BaitShortfall(D, P, *Boss);
		if (!bCanSummon && bAfford)
		{
			B.Kind = PcBaitKind::BossHere; // "buy bait" while the boss stands there reads as the purchase having failed
		}
		else if (Owed > 0)
		{
			B.Kind = PcBaitKind::BaitOwed; // the hardest gate first
			B.BaitOwed = Owed;
		}
		else if (!bAfford)
		{
			B.Kind = PcBaitKind::CatnipShort;
			B.CatnipShort = Boss->BaitCostCatnip - P.Catnip;
		}
		else
		{
			B.Kind = PcBaitKind::Ready;
		}
		B.bEnabled = bCanSummon && IsBaitUnlocked(D, P, *Boss);
		B.bCalling = BaitIsCalling(F, D, P);
		return B;
	}

	PcReawakenPage ReawakenPage(const PcGameData& D, const PcProgress& P, bool bConfirmArmed)
	{
		PcReawakenPage Page;
		// Cycle-relative Catnip toward the point, the same gate the button reads (get_reawaken_bar_progress()).
		const PcBarProgress Bar = ReawakenBarProgress(D, P);
		Page.Earned = Bar.Earned;
		Page.Required = Bar.Required;
		Page.Fraction = std::clamp(Bar.Earned / std::max(Bar.Required, 1.0), 0.0, 1.0);
		Page.Count = P.ReawakenCount;
		Page.bFirst = P.ReawakenCount < 1;
		Page.Pending = PendingStardust(D, P);
		Page.TotalAfter = P.Stardust + Page.Pending;
		Page.MultiplierNow = ReawakenMultiplier(D.Reawaken, P.StardustGrantedEver);
		Page.MultiplierAfter = ProjectedReawakenMultiplier(D, P);
		Page.bReady = CanReawaken(D, P);
		if (bConfirmArmed)
		{
			Page.Button = PcReawakenButtonKind::Confirm;
		}
		else if (Page.bReady)
		{
			// Past the first, going early is allowed: the button states the price of going now.
			Page.Button = !Page.bFirst && !IsReawakenSuggested(D, P) ? PcReawakenButtonKind::Early : PcReawakenButtonKind::Reawaken;
		}
		Page.bUnicorn = IsUnicornOwned(P);
		return Page;
	}

	std::string CardNumber(double Value)
	{
		// is_equal_approx(v, roundf(v)) -> str(int(...)); else String.num(v, 2).trim_suffix("0"): two decimals at most,
		// ONE trailing zero trimmed (String.num already drops zeros, so "0.10" never reaches the trim; 0.1 -> "0.1").
		const double Rounded = std::round(Value);
		if (std::fabs(Value - Rounded) <= std::max(1e-5 * std::fabs(Value), 1e-5))
		{
			return std::to_string(static_cast<long long>(Rounded));
		}
		char Buf[64];
		std::snprintf(Buf, sizeof(Buf), "%.2f", Value);
		std::string S = Buf;
		while (!S.empty() && S.back() == '0')
		{
			S.pop_back();
		}
		if (!S.empty() && S.back() == '.')
		{
			S.pop_back();
		}
		return S;
	}

	PcCatchesPage CatchesPage(const PcGameData& D, const PcProgress& P, bool bAutoPickSetting)
	{
		PcCatchesPage Page;
		for (const PcCard& Card : D.Cards)
		{
			PcCatchesRow Row;
			Row.CardId = Card.Id;
			Row.Level = CardLevel(P, Card.Id);
			Row.MaxLevel = CardMaxLevel(Card);
			Row.bHeld = Row.Level > 0;
			if (Row.bHeld)
			{
				++Page.Taken;
				Row.Value = CardValueAt(Card, Row.Level);
				Row.Cost = CardCostAt(Card, Row.Level);
			}
			Row.Hint = CardUnlockHint(D, P, Card);
			Page.Rows.push_back(Row);
		}
		Page.bPickingAuto = bAutoPickSetting;
		Page.bAutoPickShown = BranchEffect(D, P, "auto_card") > 0.0;
		return Page;
	}

	PcAchievementsPage AchievementsPage(const PcGameData& D, const PcProgress& P)
	{
		PcAchievementsPage Page;
		for (const PcAchievement& A : D.Achievements)
		{
			PcAchievementRow Row;
			Row.Id = A.Id;
			Row.Reward = A.StardustReward;
			Row.bClaimable = IsAchievementClaimable(D, P, A.Id);
			if (IsAchievementEarned(P, A.Id))
			{
				Row.Kind = IsAchievementClaimed(P, A.Id) ? PcAchievementRowKind::Claimed : PcAchievementRowKind::Earned;
			}
			else if (A.bSecret)
			{
				Row.Kind = PcAchievementRowKind::Secret;
			}
			else
			{
				Row.Kind = PcAchievementRowKind::Progress;
				Row.Value = AchievementProgress(D, P, A);
				Row.Threshold = A.Threshold;
			}
			Page.Rows.push_back(Row);
		}
		Page.Earned = EarnedAchievementCount(D, P);
		Page.Total = static_cast<int64_t>(D.Achievements.size());
		Page.Waiting = UnclaimedAchievementCount(D, P);
		Page.WaitingStardust = UnclaimedStardust(D, P);
		return Page;
	}

	PcCardPick CardPick(const PcGameData& D, const PcProgress& P)
	{
		PcCardPick Pick;
		Pick.bOpen = !P.Hand.empty();
		Pick.Level = P.CatchesLevel;
		for (const std::string& Id : P.Hand)
		{
			const PcCard* Card = CardById(D, Id);
			if (!Card)
			{
				continue;
			}
			PcPickCard C;
			C.CardId = Id;
			C.Flavour = Card->Flavour;
			C.Level = CardLevel(P, Id) + 1;
			C.MaxLevel = CardMaxLevel(*Card);
			C.bNew = C.Level <= 1;
			C.Value = CardValueAt(*Card, C.Level);
			C.Cost = CardCostAt(*Card, C.Level);
			C.bCosts = Card->Cost != CardCost::NONE;
			Pick.Cards.push_back(C);
		}
		Pick.Foresight = P.PromisedCards;
		Pick.Rerolls = std::max<int64_t>(0, P.Rerolls);
		Pick.bBanish = P.Banishes > 0;
		return Pick;
	}

	std::vector<PcJobRow> JobRows(const PcGameData& D, const PcProgress& P)
	{
		std::vector<PcJobRow> Rows;
		for (const PcJob* J : AllJobs(D))
		{
			PcJobRow Row;
			Row.JobId = J->Id;
			if (HoldsJob(P, J->Id))
			{
				// WORN = locked in for this run (run_job), not the standing preference (current_job).
				Row.Kind = P.RunJob == J->Id ? PcJobRowKind::Worn : PcJobRowKind::Owned;
				Rows.push_back(Row);
				continue;
			}
			// The gates in SkillTreeManager's order: the step named is the step actually blocking.
			const std::string BranchId = BranchForJob(D, J->Id);
			if (BranchId.empty())
			{
				Row.Kind = PcJobRowKind::NoNode;
				Rows.push_back(Row);
				continue;
			}
			Row.Kind = PcJobRowKind::Ready;
			if (const PcSkillBranch* B = D.FindBranch(BranchId))
			{
				const std::string Rival = BlockingRival(D, P, BranchId);
				if (!Rival.empty())
				{
					Row.Kind = PcJobRowKind::Closed;
					Row.RivalBranch = Rival;
				}
				else if (!B->GatedByBoss.empty() && !P.BossesDefeated.count(B->GatedByBoss))
				{
					Row.Kind = PcJobRowKind::BossGate;
					Row.BossTier = B->GatedByBoss;
				}
				else if (P.ReawakenCount < B->RequiredReawakens)
				{
					Row.Kind = PcJobRowKind::ReawakenGate;
					Row.Reawakens = B->RequiredReawakens;
				}
			}
			if (Row.Kind == PcJobRowKind::Ready)
			{
				Row.Cost = std::trunc(NextBranchCost(D, P, BranchId)); // int(cost)
			}
			Rows.push_back(Row);
		}
		return Rows;
	}

	PcJobPicker JobPicker(const PcGameData& D, const PcProgress& P, bool bSecond)
	{
		PcJobPicker Picker;
		Picker.bSecond = bSecond;
		Picker.Focus = -1;
		auto Add = [&](const std::string& Id)
		{
			PcJobPickRow Row;
			Row.JobId = Id;
			// The previous choice is offered as the default (the baseline too, when the last run went bare-necked).
			Row.bLastRun = Id == P.CurrentJob;
			if (Row.bLastRun)
			{
				Picker.Focus = static_cast<int64_t>(Picker.Rows.size());
			}
			Picker.Rows.push_back(Row);
		};
		// JobsHeld is ordered: Godot's owned.sort().
		for (const std::string& Id : P.JobsHeld)
		{
			// The primary is not offered twice; a held id with no job data is not offered at all.
			if ((bSecond && Id == P.RunJob) || !D.FindJob(Id))
			{
				continue;
			}
			Add(Id);
		}
		Add(std::string());
		if (Picker.Focus < 0)
		{
			Picker.Focus = 0;
		}
		Picker.SharePercent = bSecond ? static_cast<int64_t>(std::llround(UnionRulesShare(D, P) * 100.0)) : 0;
		return Picker;
	}

	bool PickJob(const PcGameData& D, PcProgress& P, bool bSecond, const std::string& Id)
	{
		if (bSecond)
		{
			LockRunJobSecond(D, P, Id);
			P.bRunJobChosen = true;
			return false;
		}
		LockRunJob(P, Id);
		// UNION RULES: ask again only when something is left to ask about.
		if (HasUnionRules(D, P) && OtherJobsThan(P, Id) > 0)
		{
			return true;
		}
		LockRunJobSecond(D, P, std::string());
		P.bRunJobChosen = true;
		return false;
	}

	PcDuration DurationParts(double Seconds)
	{
		// int(maxf(seconds, 0.0)): GDScript's int() truncates; a NaN reads 0 (maxf keeps the 0.0).
		const double Clamped = Seconds > 0.0 ? Seconds : 0.0;
		const int64_t Total = Clamped >= 9.2e18 ? INT64_MAX : static_cast<int64_t>(Clamped);
		PcDuration Out;
		Out.H = Total / 3600;
		Out.M = (Total % 3600) / 60;
		Out.S = Total % 60;
		Out.bHours = Out.H > 0;
		return Out;
	}

	int64_t DockTickNav(PcDockRepeat& R, bool bUpHeld, bool bDownHeld, double Dt)
	{
		int64_t Dir = 0;
		if (bUpHeld)
		{
			Dir -= 1;
		}
		if (bDownHeld)
		{
			Dir += 1;
		}
		if (Dir == 0)
		{
			R.NavDir = 0;
			return 0;
		}
		if (Dir != R.NavDir)
		{
			R.NavDir = Dir;
			R.NavHeld = 0.0;
			R.NavNext = DockNavRepeatDelay;
			return Dir;
		}
		R.NavHeld += Dt;
		if (R.NavHeld >= R.NavNext)
		{
			R.NavNext = R.NavHeld + 1.0 / DockNavRepeatHz;
			return Dir;
		}
		return 0;
	}

	PcDockPress DockTickPress(PcDockRepeat& R, bool bJustPressed, bool bHeld, bool bSelection, bool bRepeatRow, double Dt)
	{
		if (bJustPressed)
		{
			R.PressHeld = 0.0;
			R.PressNext = DockPressRepeatDelay;
			return bSelection ? PcDockPress::Press : PcDockPress::ShowSelection;
		}
		if (!bHeld || !bSelection || !bRepeatRow)
		{
			return PcDockPress::None;
		}
		R.PressHeld += Dt;
		if (R.PressHeld < R.PressNext)
		{
			return PcDockPress::None;
		}
		// lerpf(start, max, ramp), the ramp held to 0 .. 1.
		const double Ramp = std::clamp((R.PressHeld - DockPressRepeatDelay) / DockPressRampSec, 0.0, 1.0);
		R.PressNext = R.PressHeld + 1.0 / (DockPressStartHz + (DockPressMaxHz - DockPressStartHz) * Ramp);
		return PcDockPress::Press;
	}
}
