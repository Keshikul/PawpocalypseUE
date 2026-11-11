#include "PcField.h"

#include "PcCardDraw.h"
#include "PcCards.h"
#include "PcEconomy.h"
#include "PcJobs.h"
#include "PcProgress.h"
#include "PcRng.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Pc
{
	namespace
	{
		// RNG entity spaces for rolls that are not a spawn's or a swing's own: Anvil Paw's echo swing, Sixth Sense's
		// pre-roll of the next spawn.
		constexpr uint64_t AnvilSwingEntity = 1ull << 40;
		constexpr uint64_t ForesightEntity = 1ull << 41;
		constexpr uint64_t BossEntity = 1ull << 42;
		constexpr uint64_t AddEntity = 1ull << 43;

		// Kitten Kill Explodes and every Overkill splash start where the victim DIED (designer 2026-11-03: F13, F16, fixed in
		// Godot first with Enemy.death_position). Godot used to read the position after the killing blow, when deactivate()
		// had parked the mouse at (-10000, -10000).

		void Normalize(double& X, double& Y)
		{
			const double L = std::sqrt(X * X + Y * Y);
			if (L > 0.0)
			{
				X /= L;
				Y /= L;
			}
		}

		void Rotate(double& X, double& Y, double Angle)
		{
			const double C = std::cos(Angle);
			const double S = std::sin(Angle);
			const double RX = X * C - Y * S;
			Y = X * S + Y * C;
			X = RX;
		}

		// Vector2.lerp(to, w).normalized(): how every steer in Enemy._process turns the heading.
		void SteerToward(double& DX, double& DY, double TX, double TY, double Weight)
		{
			DX += (TX - DX) * Weight;
			DY += (TY - DY) * Weight;
			Normalize(DX, DY);
		}

		const PcTier& TierOf(const PcField& F, const PcGameData& D)
		{
			return D.Tiers[static_cast<size_t>(F.TierIndex)];
		}

		const PcEnemy& DataOf(const PcField& F, const PcGameData& D, int32_t Slot)
		{
			return D.Enemies[static_cast<size_t>(F.Kind[static_cast<size_t>(Slot)])];
		}

		double Unit(const PcField& F, uint64_t Entity, uint64_t Salt)
		{
			return RngUnit(F.Seed, F.StepCount, Entity, Salt);
		}

		// randf_range(-A, A).
		double Spread(const PcField& F, uint64_t Entity, uint64_t Salt, double A)
		{
			return -A + 2.0 * A * Unit(F, Entity, Salt);
		}

		void Push(PcField& F, PcFieldEventKind Kind, int32_t Slot, double X, double Y, double Value = 0.0, uint32_t Flags = 0,
			double ToX = 0.0, double ToY = 0.0)
		{
			PcFieldEvent E;
			E.Kind = Kind;
			E.Slot = Slot;
			E.X = X;
			E.Y = Y;
			E.ToX = ToX;
			E.ToY = ToY;
			E.Value = Value;
			E.Flags = Flags;
			F.Events.push_back(E);
		}

		// _get_free_enemy(): the first free slot in pool order, or a new one.
		int32_t FreeSlot(PcField& F)
		{
			for (size_t I = 0; I < F.Active.size(); ++I)
			{
				if (!F.Active[I])
				{
					return static_cast<int32_t>(I);
				}
			}
			const size_t N = F.Active.size() + 1;
			for (auto* V : { &F.X, &F.Y, &F.DirX, &F.DirY, &F.Hp, &F.MaxHp, &F.WaveReward, &F.SpeedMult, &F.LifetimeLeft,
					 &F.Bounces, &F.InvulnTimer, &F.IllusionPull })
			{
				V->resize(N, 0.0);
			}
			F.SlowMult.resize(N, 1.0);
			for (auto* V : { &F.Active, &F.Shielded, &F.SlowFresh, &F.HasFleeTarget, &F.BeamKill, &F.IllusionFresh })
			{
				V->resize(N, 0);
			}
			F.Claims.resize(N, 0);
			F.Kind.resize(N, -1);
			F.Serial.resize(N, 0);
			return static_cast<int32_t>(N - 1);
		}

		// deactivate() + the despawned/killed bookkeeping: the slot is free, it is swept from Live at the end of the step.
		void Retire(PcField& F, int32_t Slot)
		{
			F.Active[static_cast<size_t>(Slot)] = 0;
			F.DeadThisStep.push_back(Slot);
		}

		void PayDespawnRecovery(const PcGameData& D, PcProgress& P, const PcEnemy& E);

		void Despawn(PcField& F, const PcGameData& D, PcProgress& P, int32_t Slot)
		{
			const size_t I = static_cast<size_t>(Slot);
			Push(F, PcFieldEventKind::Despawned, Slot, F.X[I], F.Y[I]);
			Retire(F, Slot);
			PayDespawnRecovery(D, P, DataOf(F, D, Slot));
		}

		// _sweep_dead(): the single point where the live list shrinks (every resolve marks rather than erases).
		void SweepDead(PcField& F)
		{
			if (F.DeadThisStep.empty())
			{
				return;
			}
			F.Live.erase(std::remove_if(F.Live.begin(), F.Live.end(),
				[&F](int32_t S) { return !F.Active[static_cast<size_t>(S)]; }), F.Live.end());
			F.DeadThisStep.clear();
		}

		// --- the kill ----------------------------------------------------------------------------------------------

		void GrantTempKitten(PcField& F, const PcGameData& D, const PcProgress& P, double Seconds);
		void DetonateCatnipBomb(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double AtX, double AtY);
		void AdvanceCatnipBomb(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double AtX, double AtY);

		// TierController._on_enemy_killed(), every term, in its order.
		void OnKilled(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, int32_t Slot, const PcBlowResult& Blow)
		{
			const size_t I = static_cast<size_t>(Slot);
			const PcEnemy& E = DataOf(F, D, Slot);
			double Reward = Blow.Reward;
			// _pay_burglars_cut(): wasted damage pays, a share of THIS kill's reward scaled by how far the blow overshot (a
			// RATIO, capped: hit points are not comparable between biomes).
			const double Cut = OverkillCatnipShare(D, P);
			if (Cut > 0.0 && F.MaxHp[I] > 0.0 && Blow.Overkill > 0.0)
			{
				AddCurrency(D, P, Blow.Reward * Cut * std::min(Blow.Overkill / F.MaxHp[I], D.Code.CutMaxRatio));
			}
			// _resolve_overflow(): a kill that overshoots by OVERFLOW times the mouse's own health drops a Catnip bomb
			// where it died.
			const double Overflow = OverflowRatio(D, P);
			if (Overflow > 0.0 && F.MaxHp[I] > 0.0 && Blow.Overkill >= F.MaxHp[I] * Overflow)
			{
				DetonateCatnipBomb(F, D, P, In, F.X[I], F.Y[I]);
			}
			uint32_t Flags = 0;
			if (F.BeamKill[I] != 0)
			{
				Flags |= FieldFlag::Beam;
			}
			if (E.bSparkle)
			{
				Flags |= FieldFlag::Golden;
			}
			if (E.bIsSignature)
			{
				Flags |= FieldFlag::Signature;
				const double Bounty = CardEffectValue(D, P, CardEffect::SIGNATURE_BOUNTY);
				if (Bounty > 0.0)
				{
					Reward *= Bounty;
				}
			}
			if (Blow.bToyedWith)
			{
				Flags |= FieldFlag::Toyed;
			}
			// The Overkill bounty, priced in the victim's own (base) reward.
			const double Overshoot = OverkillBounty(D.Combat, F.Stats, Blow.Overkill, F.MaxHp[I], E.RewardCatnip);
			if (Overshoot > 0.0)
			{
				Reward += Overshoot;
				Flags |= FieldFlag::Bounty;
			}
			const double Before = P.Catnip;
			AddCurrency(D, P, Reward);
			const PcKillStep Step = IncrementKillCount(D, P);
			if (Step.bStatsDirty)
			{
				F.Stats = RebuildStats(D, P);
			}
			Push(F, PcFieldEventKind::Killed, Slot, F.X[I], F.Y[I], P.Catnip - Before, Flags);
			AdvanceCatnipBomb(F, D, P, In, F.X[I], F.Y[I]);
			// POUNCE_KILL_REFUND: kills shave the cooldown, floored at zero (a negative remaining would bank refunds against
			// future Pounces and, with Auto-Pounce, fire every frame).
			const double Refund = CardEffectValue(D, P, CardEffect::POUNCE_KILL_REFUND);
			if (Refund > 0.0)
			{
				F.PounceCooldownLeft = std::max(0.0, F.PounceCooldownLeft - Refund);
			}
			// KITTEN_FROM_SIGNATURE. Signatures only: the card is payment for taking on the harder target; on commons it
			// would just be a kitten-count upgrade, which the shop already sells.
			if (E.bIsSignature)
			{
				GrantTempKitten(F, D, P, CardEffectValue(D, P, CardEffect::KITTEN_FROM_SIGNATURE));
			}
			// Catches: a signature is worth two.
			PcRngCardRolls Rolls(F.Seed, ++F.Deals);
			const PcCatchesGain Gain = GainCatches(D, P, E.bIsSignature ? 2.0 : 1.0, Rolls, In.bAutoPickSetting);
			if (Gain.bLevelled)
			{
				F.Stats = RebuildStats(D, P); // an auto-picked card changes the stats
				// A card taken re-counts the kittens at once, the auto-picked one too (Godot: card_taken; F14, 2026-11-03).
				if (!Gain.Deal.AutoPicked.empty())
				{
					SyncFieldKittens(F, D, P);
				}
				Push(F, PcFieldEventKind::Levelled, -1, F.X[I], F.Y[I]);
			}
			// LONG LUNCH: the stillness bank pays out on this kill and empties, as a multiple of what this kill earned (a flat
			// sum would disappear by biome 3).
			const double Lunch = LongLunchPayout(D, P);
			if (Lunch > 0.0)
			{
				AddCurrency(D, P, Reward * Lunch);
				P.LunchSeconds = 0.0;
			}
			// LEVY: every LEVY_EVERY kills pay a lump (the count lives in the progress, so it survives travel).
			if (LevyMultiple(D, P) > 0.0 && P.KillsSinceTithe >= D.Code.LevyEvery)
			{
				P.KillsSinceTithe = 0;
				AddCurrency(D, P, Reward * LevyMultiple(D, P));
			}
			if (F.InsuranceWindow > 0.0)
			{
				++F.InsuranceKills;
			}
			// MOMENTUM: kills eat into the Cone of Shame (Player.shorten_pounce_lock()).
			const double LockRefund = PounceLockRefund(D, P);
			if (LockRefund > 0.0 && F.PounceLockLeft > 0.0)
			{
				F.PounceLockLeft = std::max(0.0, F.PounceLockLeft - LockRefund);
			}
		}

		// Enemy.take_damage() on one slot; pays the kill. Returns the blow's result (Overkill for the caller to splash).
		PcBlowResult Hit(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, int32_t Slot, double Amount,
			double ShieldPierce)
		{
			const size_t I = static_cast<size_t>(Slot);
			PcBlowTarget T;
			T.Data = &DataOf(F, D, Slot);
			T.Hp = F.Hp[I];
			T.WaveRewardMultiplier = F.WaveReward[I];
			T.bShielded = F.Shielded[I] != 0;
			T.bSnared = F.SlowMult[I] < 1.0;
			T.bKilledByBeam = F.BeamKill[I] != 0;
			const PcBlowResult R = ApplyBlow(D, P, T, Amount, ShieldPierce, F.ThresholdNow);
			F.Hp[I] = T.Hp;
			if (R.bKilled)
			{
				Retire(F, Slot);
				OnKilled(F, D, P, In, Slot, R);
			}
			return R;
		}

		// TierController.nearest_enemy(): strict `<`, so the FIRST of equally near mice in live order wins.
		int32_t NearestEnemy(const PcField& F, double OX, double OY, const std::vector<int32_t>& Exclude)
		{
			int32_t Best = -1;
			double BestDistance = std::numeric_limits<double>::infinity();
			for (const int32_t S : F.Live)
			{
				const size_t I = static_cast<size_t>(S);
				if (!F.Active[I] || std::find(Exclude.begin(), Exclude.end(), S) != Exclude.end())
				{
					continue;
				}
				const double Distance = std::hypot(F.X[I] - OX, F.Y[I] - OY);
				if (Distance < BestDistance)
				{
					Best = S;
					BestDistance = Distance;
				}
			}
			return Best;
		}

		// TierController.splash_overkill(): a killing blow's wasted damage lands on the nearest survivor(s) and NEVER chains
		// (late-game damage per frame dwarfs whole HP pools; a chain would clear the board every frame). The splash itself
		// takes no Shield Pierce (Godot's default argument).
		void SplashOverkill(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double OX, double OY, double Excess)
		{
			const double Damage = OverkillDamage(F.Stats, Excess);
			if (Damage <= 0.0)
			{
				return;
			}
			const int64_t Targets = 1 + static_cast<int64_t>(CardEffectValue(D, P, CardEffect::OVERKILL_TARGETS));
			std::vector<int32_t> HitSlots;
			for (int64_t K = 0; K < Targets; ++K)
			{
				const int32_t Nearest = NearestEnemy(F, OX, OY, HitSlots);
				if (Nearest < 0)
				{
					break;
				}
				HitSlots.push_back(Nearest);
				const size_t I = static_cast<size_t>(Nearest);
				Push(F, PcFieldEventKind::Splash, Nearest, OX, OY, Damage, 0, F.X[I], F.Y[I]);
				Hit(F, D, P, In, Nearest, Damage, 0.0);
			}
		}

		// --- the boss and its adds ------------------------------------------------------------------------------------

		const PcBoss* BossOf(const PcField& F, const PcGameData& D)
		{
			return F.TierIndex >= 0 ? D.FindBoss(TierOf(F, D).BossData) : nullptr;
		}

		// _is_player_engaging_boss(): the cat within its aura's radius of the boss (aura bought or not). The kittens follow
		// it in only then: on boss presence alone they would strand the mice unfarmed for the whole biome.
		bool EngagingBoss(const PcField& F, const PcGameData& D, const PcProgress& P)
		{
			return F.bBossUp && std::hypot(F.CatX - F.BossX, F.CatY - F.BossY) <= AuraRadius(D, P);
		}

		// The boss leaves the field without dying (the clock ran out, a Reawaken): nothing is credited, its adds go with it.
		void ClearBossAdds(PcField& F)
		{
			F.Adds.clear();
			F.AddTimer = 0.0;
		}

		void WithdrawBoss(PcField& F)
		{
			ClearBossAdds(F);
			F.bBossUp = false;
			F.bBossEngagedOnce = false;
			F.BossFightElapsed = 0.0;
		}

		// Boss._check_phase_transition(): phases by HP fraction; a new phase opens with its window (vulnerable first).
		void CheckBossPhase(PcField& F, const PcBoss& B)
		{
			const double Fraction = F.BossHp / B.TotalHp;
			int64_t Phase = 1;
			for (size_t K = 0; K < B.PhaseHpThresholds.size(); ++K)
			{
				if (Fraction <= B.PhaseHpThresholds[K])
				{
					Phase = static_cast<int64_t>(K) + 2;
				}
			}
			if (Phase != F.BossPhase)
			{
				F.BossPhase = Phase;
				F.bBossShielded = false;
				F.BossInvulnTimer = 0.0;
				Push(F, PcFieldEventKind::BossPhase, -1, F.BossX, F.BossY, static_cast<double>(Phase));
			}
		}

		// TierController._on_boss_defeated(): credited on the frame it dies (the death sequence is presentation; a sequence
		// that gated progression could strand the player if it ever failed). A first clear of a boss that is not the last
		// travels on by itself after the sequence when the Auto-Travel SETTING is on and the next biome is open.
		void BossDefeated(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, const PcBoss& B)
		{
			F.bBossUp = false;
			const PcTier& Tier = TierOf(F, D);
			const PcBossMark Mark = RecordBossDefeat(D, P, Tier.Id, B.RewardCatnip);
			F.bBossEngagedOnce = false;
			ClearBossAdds(F);
			const bool bFinal = !D.Code.BossRealms.empty() && Tier.Id == D.Code.BossRealms.back();
			Push(F, PcFieldEventKind::BossDefeated, -1, F.BossX, F.BossY, B.RewardCatnip,
				static_cast<uint32_t>((Mark.bNew ? FieldFlag::New : 0) | (bFinal ? FieldFlag::Final : 0)
					| (RaisesVictory(D, Tier.Id, Mark) ? FieldFlag::Victory : 0)));
			F.Stats = RebuildStats(D, P); // the evolution stage moved
			const size_t Next = static_cast<size_t>(F.TierIndex) + 1;
			const bool bMayTravel = In.bAutoTravelSetting && Next < D.Tiers.size() && IsBiomeUnlocked(D, P, D.Tiers[Next]);
			if (Mark.bNew && !bFinal && bMayTravel)
			{
				F.TravelDelay = D.Code.BossDeathSequenceSec;
			}
		}

		// Boss.take_damage(): nothing lands through the shield without Shield Pierce. No threshold or Traps multiplier (the
		// boss is not a mouse). Phases follow the HP; at zero it is defeated.
		void BossTakeDamage(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Amount, double ShieldPierce)
		{
			if (!F.bBossUp)
			{
				return;
			}
			if (F.bBossShielded)
			{
				if (ShieldPierce <= 0.0)
				{
					return;
				}
				Amount *= ShieldPierce;
			}
			const PcBoss* B = BossOf(F, D);
			F.BossHp = std::max(0.0, F.BossHp - Amount);
			CheckBossPhase(F, *B);
			if (F.BossHp <= 0.0)
			{
				BossDefeated(F, D, P, In, *B);
			}
		}

		// Boss.heal(): a leaked add heals it, capped at full (an HP bar past 100% reads as a bug, and the phases are
		// fractions of the total: going over would walk it backwards through them).
		void HealBoss(PcField& F, const PcBoss& B, double Amount)
		{
			if (!F.bBossUp || Amount <= 0.0)
			{
				return;
			}
			F.BossHp = std::min(B.TotalHp, F.BossHp + Amount);
			CheckBossPhase(F, B);
			Push(F, PcFieldEventKind::BossHealed, -1, F.BossX, F.BossY, Amount);
		}

		// BossAdd.take_damage(): adds have no shield; a kill intercepts it (the whole point is attention, not HP).
		void HitAdd(PcField& F, size_t A, double Amount)
		{
			if (A >= F.Adds.size())
			{
				return;
			}
			F.Adds[A].Hp -= Amount;
			if (F.Adds[A].Hp > 0.0)
			{
				return;
			}
			Push(F, PcFieldEventKind::AddIntercepted, -1, F.Adds[A].X, F.Adds[A].Y);
			F.Adds.erase(F.Adds.begin() + static_cast<std::ptrdiff_t>(A));
		}

		// Boss._pick_relocation(): a random spot inside the arena, inset from the edges (never half off-screen, never flush
		// against the dock).
		void PickBossRelocation(PcField& F, const PcGameData& D, double& OX, double& OY)
		{
			const PcRect& A = F.Bounds;
			const double Inset = std::min(D.Code.BossArenaInset, std::min(A.W, A.H) * 0.3);
			const uint64_t Roll = BossEntity + ++F.BossRolls;
			OX = A.X + Inset + (A.W - 2.0 * Inset) * Unit(F, Roll, Salt::BossPlaceX);
			OY = A.Y + Inset + (A.H - 2.0 * Inset) * Unit(F, Roll, Salt::BossPlaceY);
		}

		// Boss._process(): it drifts around its anchor so it never looks parked, races to a new anchor when it relocates
		// (fast but continuous: a snap would teleport it out from under the cursor), and turns its shield over. It
		// RELOCATES the instant its window opens: the window is the only time damage lands, so moving exactly then is
		// what makes the cursor travel. (The twitch, the anger and the death sequence are presentation: not simulated.)
		void TickBoss(PcField& F, const PcGameData& D, double Dt)
		{
			if (!F.bBossUp)
			{
				return;
			}
			const PcBoss* B = BossOf(F, D);
			const size_t Phase = static_cast<size_t>(F.BossPhase - 1);
			F.BossDriftTimer -= Dt;
			if (F.BossDriftTimer <= 0.0)
			{
				F.BossDriftTimer = D.Code.BossDriftInterval;
				const uint64_t Roll = BossEntity + ++F.BossRolls;
				F.BossDriftX = F.BossAnchorX + Spread(F, Roll, Salt::BossWobbleX, D.Code.BossWobbleXPx);
				F.BossDriftY = F.BossAnchorY + Spread(F, Roll, Salt::BossWobbleY, D.Code.BossWobbleYPx);
			}
			const double Speed = std::hypot(F.BossX - F.BossAnchorX, F.BossY - F.BossAnchorY) > D.Code.BossRelocateDistancePx
				? D.Code.BossRelocateSpeed : D.Code.BossDriftSpeed;
			const double ToX = F.BossDriftX - F.BossX;
			const double ToY = F.BossDriftY - F.BossY;
			const double Length = std::sqrt(ToX * ToX + ToY * ToY);
			const double Stride = Speed * Dt;
			if (Length <= Stride)
			{
				F.BossX = F.BossDriftX;
				F.BossY = F.BossDriftY;
			}
			else
			{
				F.BossX += ToX / Length * Stride;
				F.BossY += ToY / Length * Stride;
			}
			F.BossInvulnTimer += Dt;
			if (F.bBossShielded)
			{
				if (F.BossInvulnTimer >= B->PhaseShieldSec[Phase])
				{
					F.bBossShielded = false;
					F.BossInvulnTimer = 0.0;
					PickBossRelocation(F, D, F.BossAnchorX, F.BossAnchorY);
					F.BossDriftX = F.BossAnchorX;
					F.BossDriftY = F.BossAnchorY;
					Push(F, PcFieldEventKind::BossWindow, -1, F.BossX, F.BossY);
				}
			}
			else if (F.BossInvulnTimer >= B->PhaseVulnerableSec[Phase])
			{
				F.bBossShielded = true;
				F.BossInvulnTimer = 0.0;
				Push(F, PcFieldEventKind::BossWindow, -1, F.BossX, F.BossY, 0.0, FieldFlag::Shielded);
			}
		}

		void EdgePosition(const PcField& F, const PcGameData& D, uint64_t Entity, double& OX, double& OY);

		// _tick_boss_adds(): while the boss is alive, a void-kitten add walks in from an edge every add_interval_sec,
		// re-aimed at the boss every step (it relocates; an add aimed where it used to be would never arrive). The cat's
		// WHOLE damage (paw included: the paw's loop never reaches an add, so this double-counts nothing) and the poop zone
		// wear them down.
		void TickBossAdds(PcField& F, const PcGameData& D, const PcProgress& P, double Dt)
		{
			if (!F.bBossUp)
			{
				return;
			}
			const PcBoss* B = BossOf(F, D);
			if (B->AddIntervalSec <= 0.0)
			{
				return;
			}
			F.AddTimer -= Dt;
			if (F.AddTimer <= 0.0)
			{
				F.AddTimer = B->AddIntervalSec;
				PcBossAdd A;
				EdgePosition(F, D, AddEntity + ++F.AddSpawns, A.X, A.Y);
				// Its identity: the kittens chase the add NEAREST the cat by serial (Godot passes the node itself). Left at 0,
				// every add matched and the pool chased the oldest one, through every add in reach.
				A.Serial = F.AddSpawns;
				A.Hp = B->TotalHp * B->AddHpFraction;
				A.Heal = B->TotalHp * B->AddHealFraction;
				A.Speed = B->AddSpeedPxPerSec;
				F.Adds.push_back(A);
				Push(F, PcFieldEventKind::AddSpawned, -1, A.X, A.Y);
			}
			for (PcBossAdd& A : F.Adds)
			{
				A.TargetX = F.BossX;
				A.TargetY = F.BossY;
			}
			// _resolve_add_contact()
			const double Radius = AuraRadius(D, P);
			const double Contact = PlayerDps(D.Combat, F.Stats) * Dt;
			const double Poop = PoopDps(D.Combat, F.Stats) * Dt;
			for (size_t K = 0; K < F.Adds.size();)
			{
				const size_t Before = F.Adds.size();
				if (std::hypot(F.Adds[K].X - F.CatX, F.Adds[K].Y - F.CatY) <= Radius)
				{
					HitAdd(F, K, Contact);
				}
				if (F.Adds.size() == Before && F.bPoopActive
					&& std::hypot(F.Adds[K].X - F.PoopX, F.Adds[K].Y - F.PoopY) <= D.Combat.PoopRadiusPx)
				{
					HitAdd(F, K, Poop);
				}
				if (F.Adds.size() == Before)
				{
					++K;
				}
			}
		}

		// BossAdd._process(): walks at the boss; on arrival it heals it and is gone.
		void MoveBossAdds(PcField& F, const PcGameData& D, double Dt)
		{
			const PcBoss* B = BossOf(F, D);
			for (size_t K = 0; K < F.Adds.size();)
			{
				PcBossAdd& A = F.Adds[K];
				const double ToX = A.TargetX - A.X;
				const double ToY = A.TargetY - A.Y;
				const double Length = std::sqrt(ToX * ToX + ToY * ToY);
				if (Length <= D.Code.BossAddArrivePx)
				{
					const double Heal = A.Heal;
					Push(F, PcFieldEventKind::AddArrived, -1, A.X, A.Y, Heal);
					F.Adds.erase(F.Adds.begin() + static_cast<std::ptrdiff_t>(K));
					if (B)
					{
						HealBoss(F, *B, Heal);
					}
					continue;
				}
				A.X += ToX / Length * A.Speed * Dt;
				A.Y += ToY / Length * A.Speed * Dt;
				++K;
			}
		}

		// _resolve_boss_contact(): the AURA's damage while the cat engages (zero until the Aura is bought); the fight's
		// clock starts at the first engagement (a countdown from arrival would time the whole biome), and when it runs out
		// the boss leaves: the bait is simply gone, which is the whole cost of losing (v4 Phase 2).
		void ResolveBossContact(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Dt)
		{
			if (!F.bBossUp)
			{
				return;
			}
			if (EngagingBoss(F, D, P))
			{
				BossTakeDamage(F, D, P, In, AuraDps(D.Combat, F.Stats) * Dt, F.Stats.ShieldPierceFraction);
				F.bBossEngagedOnce = true;
			}
			if (!F.bBossUp)
			{
				return; // it died to that
			}
			if (F.bBossEngagedOnce)
			{
				F.BossFightElapsed += Dt;
				const double Limit = BossOf(F, D)->TimeLimitSec;
				if (Limit > 0.0 && F.BossFightElapsed >= Limit)
				{
					WithdrawBoss(F);
					Push(F, PcFieldEventKind::BossTimeout, -1, F.BossX, F.BossY);
				}
			}
		}

		// _tick_auto_travel() / _tick_auto_bait(): owned AND switched on, polled every auto_travel_poll_sec, never while
		// the boss is up. Travel is the game's to do: the field asks for it.
		void TickAutoTravel(PcField& F, const PcGameData& D, const PcProgress& P, const PcFieldInput& In, double Dt)
		{
			if (!(OwnsAutoTravel(D, P) && In.bAutoTravelSetting))
			{
				return;
			}
			F.AutoTravelTimer -= Dt;
			if (F.AutoTravelTimer > 0.0)
			{
				return;
			}
			F.AutoTravelTimer = D.Code.AutoTravelPollSec;
			if (F.bBossUp)
			{
				return;
			}
			const size_t Next = static_cast<size_t>(F.TierIndex) + 1;
			if (Next >= D.Tiers.size() || !IsBiomeUnlocked(D, P, D.Tiers[Next]))
			{
				return;
			}
			Push(F, PcFieldEventKind::TravelRequested, static_cast<int32_t>(Next), F.CatX, F.CatY);
		}

		void TickAutoBait(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Dt)
		{
			if (!(OwnsAutoBait(D, P) && In.bAutoBaitSetting))
			{
				return;
			}
			F.AutoBaitTimer -= Dt;
			if (F.AutoBaitTimer > 0.0)
			{
				return;
			}
			F.AutoBaitTimer = D.Code.AutoTravelPollSec;
			if (F.bBossUp || !CanSummonFieldBoss(F, D, P))
			{
				return;
			}
			SummonFieldBoss(F, D, P);
		}

		// --- the paw -----------------------------------------------------------------------------------------------

		// _nearest_in_reach(): `<=`, so the LAST of equally near mice in live order wins (Godot's loop, kept).
		int32_t NearestInReach(const PcField& F, double Reach)
		{
			int32_t Best = -1;
			double BestDistance = Reach;
			for (const int32_t S : F.Live)
			{
				const size_t I = static_cast<size_t>(S);
				if (!F.Active[I])
				{
					continue;
				}
				const double Distance = std::hypot(F.X[I] - F.CatX, F.Y[I] - F.CatY);
				if (Distance <= BestDistance)
				{
					BestDistance = Distance;
					Best = S;
				}
			}
			return Best;
		}

		enum class PcCrit : uint8_t
		{
			None,
			Lucky,  // the dice
			Earned, // a counter decided it
		};

		// _roll_crit(): BUTCHER'S RHYTHM first (a share of swings crit unasked, its count never resets), then INSCRUTABLE (the
		// same crit chance spent as a cadence: identical in expectation, free of variance; it RETURNS, because rolling as
		// well would make the keystone strictly better than not taking it), then the ordinary roll.
		PcCrit RollCrit(PcField& F, const PcGameData& D, const PcProgress& P, uint64_t Swing)
		{
			const double Rhythm = GuaranteedCritShare(D, P);
			if (Rhythm > 0.0)
			{
				F.RhythmCharge += Rhythm;
				if (F.RhythmCharge >= 1.0)
				{
					F.RhythmCharge -= 1.0;
					return PcCrit::Earned;
				}
			}
			if (F.Stats.CritChance <= 0.0)
			{
				return PcCrit::None;
			}
			if (DeterministicCrit(D, P))
			{
				F.InscrutableCharge += F.Stats.CritChance;
				if (F.InscrutableCharge >= 1.0)
				{
					F.InscrutableCharge -= 1.0;
					return PcCrit::Earned;
				}
				return PcCrit::None;
			}
			return RngUnit(F.Seed, F.StepCount, Swing, Salt::Crit) < F.Stats.CritChance ? PcCrit::Lucky : PcCrit::None;
		}

		// _resolve_hammerfall(): the swipe carries through what stands BEHIND the target, along the line from the cat
		// through it (a lane, not a second aura: the aura is what the Blacksmith gave up for this).
		void ResolveHammerfall(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double StruckX,
			double StruckY, int32_t Target, double Damage)
		{
			const int64_t Count = PawCleaveCount(D, P);
			if (Count <= 0)
			{
				return;
			}
			double LX = StruckX - F.CatX;
			double LY = StruckY - F.CatY;
			if (std::hypot(LX, LY) < 0.001)
			{
				return;
			}
			Normalize(LX, LY);
			const double AX = -LY;
			const double AY = LX;
			int64_t Hits = 0;
			double Furthest = 0.0;
			const size_t Total = F.Live.size();
			for (size_t K = 0; K < Total; ++K)
			{
				if (Hits >= Count)
				{
					break;
				}
				const int32_t S = F.Live[K];
				const size_t I = static_cast<size_t>(S);
				if (!F.Active[I] || S == Target)
				{
					continue;
				}
				const double OX = F.X[I] - StruckX;
				const double OY = F.Y[I] - StruckY;
				const double Along = OX * LX + OY * LY;
				// BEHIND the target, not around it: a negative `along` is between the cat and the target, already swept.
				if (Along <= 0.0 || Along > D.Code.HammerfallRangePx)
				{
					continue;
				}
				if (std::fabs(OX * AX + OY * AY) > D.Code.HammerfallLanePx)
				{
					continue;
				}
				const PcBlowResult R = Hit(F, D, P, In, S, Damage, F.Stats.ShieldPierceFraction);
				if (R.Overkill > 0.0)
				{
					// Hammerfall's kill: from where the victim died (F16, 2026-11-03).
					SplashOverkill(F, D, P, In, F.X[I], F.Y[I], R.Overkill);
				}
				++Hits;
				Furthest = std::max(Furthest, Along);
			}
			if (Hits > 0 && Furthest > 0.0)
			{
				Push(F, PcFieldEventKind::Cleave, Target, StruckX, StruckY, 0.0, 0, StruckX + LX * Furthest, StruckY + LY * Furthest);
			}
		}

		// _strike_boss(): Shred applies (the boss is the one target that cannot wander off mid-streak); Execute and Riddle
		// do not (a finisher that deleted the last tenth of a boss would collapse the fight the phases exist to pace).
		void StrikeBoss(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, uint64_t Swing)
		{
			if (!F.bBossUp)
			{
				return;
			}
			// "Shredding the boss" and "shredding nothing" are not both spelled ShredSlot == -1.
			if (!F.bShredOnBoss)
			{
				F.ShredSlot = -1;
				F.bShredOnBoss = true;
				F.ShredStacks = 0;
			}
			else
			{
				F.ShredStacks = std::min(D.Combat.ShredMaxStacks, F.ShredStacks + 1);
			}
			F.ShredExpires = D.Combat.ShredStackSeconds;
			double Damage = PawStrikeDamage(D.Combat, F.Stats, F.ShredStacks);
			const PcCrit Crit = RollCrit(F, D, P, Swing);
			if (Crit != PcCrit::None)
			{
				Damage = Damage * F.Stats.CritMultiplier;
			}
			const uint32_t Flags = static_cast<uint32_t>(FieldFlag::Boss | (Crit != PcCrit::None ? FieldFlag::Crit : 0)
				| (Crit == PcCrit::Earned ? FieldFlag::Earned : 0));
			Push(F, PcFieldEventKind::Strike, -1, F.CatX, F.CatY, Damage, Flags, F.BossX, F.BossY);
			BossTakeDamage(F, D, P, In, Damage, F.Stats.ShieldPierceFraction);
		}

		// _strike_once(): the nearest mouse in reach; nothing small in reach, the boss if it stands here (checked second, so
		// a screen full of mice still gets cleared: the boss is the thing that does not wander off).
		void StrikeOnce(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, uint64_t Swing, bool bEcho = false)
		{
			const int32_t Target = NearestInReach(F, D.Combat.BasePawReach);
			if (Target < 0)
			{
				if (F.bBossUp && std::hypot(F.BossX - F.CatX, F.BossY - F.CatY) <= D.Combat.BasePawReach)
				{
					StrikeBoss(F, D, P, In, Swing);
				}
				return;
			}
			const size_t I = static_cast<size_t>(Target);
			// Switching victims resets the ramp: the aura rewards a crowd, the paw rewards finishing what you started.
			if (Target != F.ShredSlot || F.Serial[I] != F.ShredSerial || F.bShredOnBoss)
			{
				F.ShredSlot = Target;
				F.ShredSerial = F.Serial[I];
				F.bShredOnBoss = false;
				F.ShredStacks = 0;
			}
			else
			{
				F.ShredStacks = std::min(F.ShredStacks + 1, D.Combat.ShredMaxStacks);
			}
			F.ShredExpires = D.Combat.ShredStackSeconds;

			double Damage = PawStrikeDamage(D.Combat, F.Stats, F.ShredStacks);
			// Rolled per swipe, for real (PawDps folds the same crit in as an expected value: the balance model's view).
			const PcCrit Crit = RollCrit(F, D, P, Swing);
			const bool bCrit = Crit != PcCrit::None;
			bool bFinisher = false;
			bool bThreshold = false;
			// Only an arming from an EARLIER swing may be spent now (F17, designer 2026-11-03, fixed in Godot first: the
			// code used to arm and spend on the same swing, against its own comment).
			const bool bRiddleReady = F.bRiddleArmed;
			bool bRiddleArmedNow = false;
			if (bCrit)
			{
				Damage *= F.Stats.CritMultiplier;
				// RIDDLE arms on the crit that just landed, and the NEXT swing spends it.
				const double Riddle = CritExecuteChance(D, P);
				if (Riddle > 0.0 && RngUnit(F.Seed, F.StepCount, Swing, Salt::Riddle) < Riddle)
				{
					bRiddleArmedNow = true;
				}
			}
			// RIDDLE answered before Execute, so the two cannot both claim one mouse; never through a shield (a swipe that
			// lands nothing on a shield must not kill through one).
			if (bRiddleReady && !F.Shielded[I])
			{
				F.bRiddleArmed = false;
				Damage = std::max(Damage, F.Hp[I]);
				bFinisher = true;
			}
			if (bRiddleArmedNow)
			{
				F.bRiddleArmed = true; // for the next swing
			}
			// Execute: below the line a strike kills outright; never through a shield.
			if (F.Stats.ExecuteThreshold > 0.0 && !F.Shielded[I] && F.MaxHp[I] > 0.0 && F.Hp[I] <= F.MaxHp[I] * F.Stats.ExecuteThreshold)
			{
				Damage = std::max(Damage, F.Hp[I]);
				bFinisher = true;
				bThreshold = true;
			}
			const double StruckX = F.X[I];
			const double StruckY = F.Y[I];
			const uint32_t Flags = static_cast<uint32_t>((bCrit ? FieldFlag::Crit : 0) | (Crit == PcCrit::Earned ? FieldFlag::Earned : 0)
				| (bEcho ? FieldFlag::Echo : 0) | (bFinisher ? FieldFlag::Execute : 0) | (bRiddleArmedNow ? FieldFlag::Riddle : 0)
				| (bThreshold ? FieldFlag::Threshold : 0));
			Push(F, PcFieldEventKind::Strike, Target, F.CatX, F.CatY, Damage, Flags, StruckX, StruckY);
			// Where the blow landed, captured BEFORE it lands (a dead mouse is pooled; reading its position afterwards gave
			// Hammerfall a stale point and it almost never fired).
			const PcBlowResult R = Hit(F, D, P, In, Target, Damage, F.Stats.ShieldPierceFraction);
			if (R.Overkill > 0.0)
			{
				SplashOverkill(F, D, P, In, StruckX, StruckY, R.Overkill);
			}
			ResolveHammerfall(F, D, P, In, StruckX, StruckY, Target, Damage);
		}

		// _resolve_paw_strikes(): attack_speed swipes a second, at most max_strikes_per_frame per step (a hitch must not
		// dump a hundred swipes at once). ANVIL PAW strikes again on an accumulated share of swings (a fraction rising
		// smoothly with its level, not a jump between "every 5th" and "every 4th").
		void ResolvePawStrikes(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Dt)
		{
			const double Rate = StrikesPerSec(D.Combat, F.Stats);
			if (Rate <= 0.0)
			{
				return;
			}
			F.ShredExpires -= Dt;
			if (F.ShredExpires <= 0.0)
			{
				F.ShredSlot = -1;
				F.bShredOnBoss = false;
				F.ShredStacks = 0;
			}
			F.StrikeTimer += Dt;
			const double Interval = 1.0 / Rate;
			int64_t Swings = 0;
			while (F.StrikeTimer >= Interval && Swings < D.Code.MaxStrikesPerFrame)
			{
				F.StrikeTimer -= Interval;
				StrikeOnce(F, D, P, In, static_cast<uint64_t>(Swings));
				++Swings;
				const double Anvil = DoubleStrikeShare(D, P);
				if (Anvil > 0.0)
				{
					F.AnvilCharge += Anvil;
					if (F.AnvilCharge >= 1.0)
					{
						F.AnvilCharge -= 1.0;
						StrikeOnce(F, D, P, In, static_cast<uint64_t>(Swings) + AnvilSwingEntity, true);
					}
				}
			}
			if (Swings >= D.Code.MaxStrikesPerFrame)
			{
				F.StrikeTimer = 0.0;
			}
		}

		// --- the kittens -------------------------------------------------------------------------------------------

		bool KittenTargetAlive(const PcField& F, int32_t K)
		{
			const int32_t T = F.KTarget[static_cast<size_t>(K)];
			return T >= 0 && F.Active[static_cast<size_t>(T)] && F.Serial[static_cast<size_t>(T)] == F.KTargetSerial[static_cast<size_t>(K)];
		}

		void SetKittenTarget(PcField& F, int32_t K, int32_t Slot)
		{
			F.KTarget[static_cast<size_t>(K)] = Slot;
			F.KTargetSerial[static_cast<size_t>(K)] = Slot >= 0 ? F.Serial[static_cast<size_t>(Slot)] : 0;
		}

		// _grow_kitten_pool(): kittens are added, never freed, and every orbit slot is re-spread over the new size so a
		// bigger pool still rings its target. (It was once sized only from Kitten Count's BASE cap; the cap rises per
		// biome and Reawaken, so deep runs owned kittens the pool could not hold, paid for and never seen.)
		void GrowKittenPool(PcField& F, const PcGameData& D, int64_t Size)
		{
			if (Size <= static_cast<int64_t>(F.KActive.size()))
			{
				return;
			}
			const size_t N = static_cast<size_t>(Size);
			for (auto* V : { &F.KX, &F.KY, &F.KOrbitX, &F.KOrbitY, &F.KRoamDirX, &F.KRoamDirY, &F.KRoamTimer })
			{
				V->resize(N, 0.0);
			}
			F.KDamageMult.resize(N, 1.0);
			for (auto* V : { &F.KActive, &F.KGolden, &F.KLone, &F.KIllusory })
			{
				V->resize(N, 0);
			}
			F.KTarget.resize(N, -1);
			F.KTargetSerial.resize(N, 0);
			for (size_t I = 0; I < N; ++I)
			{
				KittenOrbitSlot(D, static_cast<int64_t>(I), Size, F.KOrbitX[I], F.KOrbitY[I]);
			}
		}

		// Kitten.activate(): on the cat. They all wake on the cat, so without a spread they would roam as one block: the
		// first heading is the kitten's own orbit angle, its timer a random share of a heading.
		void WakeKitten(PcField& F, const PcGameData& D, int32_t K)
		{
			const size_t I = static_cast<size_t>(K);
			F.KActive[I] = 1;
			F.KX[I] = F.CatX;
			F.KY[I] = F.CatY;
			double DX = F.KOrbitX[I];
			double DY = F.KOrbitY[I];
			if (DX == 0.0 && DY == 0.0)
			{
				DX = 1.0;
			}
			Normalize(DX, DY);
			F.KRoamDirX[I] = DX;
			F.KRoamDirY[I] = DY;
			F.KRoamTimer[I] = Unit(F, ++F.KittenWakes, Salt::KittenWake) * D.Code.KittenRoamHeadingSec;
		}

		// Kitten.deactivate(): asleep, and its commitment dropped.
		void SleepKitten(PcField& F, int32_t K)
		{
			F.KActive[static_cast<size_t>(K)] = 0;
			SetKittenTarget(F, K, -1);
		}

		// _grant_temp_kitten(): bounded, because signature kills are frequent and the pool keeps exactly this much headroom.
		void GrantTempKitten(PcField& F, const PcGameData& D, const PcProgress& P, double Seconds)
		{
			if (Seconds <= 0.0 || static_cast<int64_t>(F.TempKittens.size()) >= D.Code.MaxTempKittens)
			{
				return;
			}
			F.TempKittens.push_back(Seconds);
			SyncFieldKittens(F, D, P);
		}

		// _on_kitten_contact_kill(): KITTEN_KILL_EXPLODES. A fraction of the VICTIM's max HP (what the card says, and what
		// makes it scale: enemy HP is the curve everything else is pinned to), on every live mouse in the radius, with
		// the cat's Shield Pierce. The blast's own kills are not contact kills: they do not explode again.
		void KittenBlast(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double AtX, double AtY,
			double VictimMaxHp)
		{
			const double Fraction = CardEffectValue(D, P, CardEffect::KITTEN_KILL_EXPLODES);
			if (Fraction <= 0.0 || VictimMaxHp <= 0.0)
			{
				return;
			}
			const double Damage = VictimMaxHp * Fraction;
			const double Radius = D.Code.KittenExplosionRadius;
			const std::vector<int32_t> Snapshot = F.Live;
			for (const int32_t S : Snapshot)
			{
				const size_t I = static_cast<size_t>(S);
				if (F.Active[I] && std::hypot(F.X[I] - AtX, F.Y[I] - AtY) <= Radius)
				{
					Hit(F, D, P, In, S, Damage, F.Stats.ShieldPierceFraction);
				}
			}
			Push(F, PcFieldEventKind::KittenBlast, -1, AtX, AtY, Radius);
		}

		// Kitten._roam(): a slowly curving heading, turned back toward the middle at the edges. Not a per-step random walk:
		// that averages to standing still, which is the look roaming replaces. Slower than the hunt, so "spotted
		// something" reads with no other cue.
		void RoamKitten(PcField& F, const PcGameData& D, int32_t K, double Dt)
		{
			const size_t I = static_cast<size_t>(K);
			F.KRoamTimer[I] -= Dt;
			if (F.KRoamTimer[I] <= 0.0)
			{
				F.KRoamTimer[I] = D.Code.KittenRoamHeadingSec;
				// Re-rolled within an arc of the current heading, so the path curves instead of teleport-turning.
				const double A = D.Code.KittenRoamTurnSpread;
				Rotate(F.KRoamDirX[I], F.KRoamDirY[I], -A + 2.0 * A * Unit(F, static_cast<uint64_t>(K), Salt::KittenRoam));
			}
			const double Step = D.Code.KittenSpeedPxPerSec * D.Code.KittenRoamSpeedRatio * F.Stats.KittenSpeedMultiplier * Dt;
			double NX = F.KX[I] + F.KRoamDirX[I] * Step;
			double NY = F.KY[I] + F.KRoamDirY[I] * Step;
			if (!F.Bounds.HasPoint(NX, NY))
			{
				// Back toward the middle instead of sliding along the wall.
				F.KRoamDirX[I] = F.Bounds.CentreX() - F.KX[I];
				F.KRoamDirY[I] = F.Bounds.CentreY() - F.KY[I];
				Normalize(F.KRoamDirX[I], F.KRoamDirY[I]);
				F.KRoamTimer[I] = D.Code.KittenRoamHeadingSec;
				NX = F.KX[I] + F.KRoamDirX[I] * Step;
				NY = F.KY[I] + F.KRoamDirY[I] * Step;
			}
			F.KX[I] = NX;
			F.KY[I] = NY;
		}

		// Kitten._approach(): keeps closing on the kitten's OWN slot around the target even while it already deals
		// damage (stopping at the contact radius froze every kitten where it first touched, a clump on the side they all
		// came from). The slot sits inside the contact radius, so settling into it never costs uptime.
		bool ApproachKitten(PcField& F, const PcGameData& D, int32_t K, double TX, double TY, double Dt)
		{
			const size_t I = static_cast<size_t>(K);
			const double DestX = TX + F.KOrbitX[I];
			const double DestY = TY + F.KOrbitY[I];
			const double ToX = DestX - F.KX[I];
			const double ToY = DestY - F.KY[I];
			const double Step = D.Code.KittenSpeedPxPerSec * F.Stats.KittenSpeedMultiplier * Dt;
			const double Length = std::sqrt(ToX * ToX + ToY * ToY);
			if (Length <= Step)
			{
				F.KX[I] = DestX;
				F.KY[I] = DestY;
			}
			else
			{
				F.KX[I] += ToX / Length * Step;
				F.KY[I] += ToY / Length * Step;
			}
			return std::hypot(F.KX[I] - TX, F.KY[I] - TY) <= D.Combat.BaseKittenContactRadius;
		}

		// TierController._apply_kitten_cards(): Gilded Kitten, Lone Hunter and Kitten Drag every step (the pool is resized
		// by purchases, Reawaken and travel: one cheap loop beats four signals to remember), then the temporary kittens
		// age and the count is re-synced while any are out.
		void ApplyKittenCards(PcField& F, const PcGameData& D, const PcProgress& P, double Dt)
		{
			const double Golden = CardEffectValue(D, P, CardEffect::GOLDEN_KITTEN);
			const double Lone = CardEffectValue(D, P, CardEffect::LONE_HUNTER);
			F.KDragSpeed = CardEffectValue(D, P, CardEffect::KITTEN_DRAG);
			// Exactly ONE kitten wears each mark (marking every kitten would make either card a flat kitten-damage
			// multiplier, which is a shop stat), and not the same one: Gilded takes the first awake kitten, Lone Hunter
			// the last. With one kitten awake Gilded wins it (designer, 2026-08-23).
			std::vector<int32_t> Awake;
			for (size_t I = 0; I < F.KActive.size(); ++I)
			{
				if (F.KActive[I])
				{
					Awake.push_back(static_cast<int32_t>(I));
				}
			}
			const int64_t GildedAt = Golden > 1.0 && !Awake.empty() ? 0 : -1;
			int64_t LoneAt = -1;
			if (Lone > 1.0 && !Awake.empty())
			{
				LoneAt = static_cast<int64_t>(Awake.size()) - 1;
				if (LoneAt == GildedAt)
				{
					LoneAt = -1;
				}
			}
			for (size_t A = 0; A < Awake.size(); ++A)
			{
				const size_t I = static_cast<size_t>(Awake[A]);
				const bool bGolden = static_cast<int64_t>(A) == GildedAt;
				const bool bLone = static_cast<int64_t>(A) == LoneAt;
				F.KDamageMult[I] = bGolden ? Golden : (bLone ? Lone : 1.0);
				F.KGolden[I] = bGolden ? 1 : 0;
				F.KLone[I] = bLone ? 1 : 0;
			}
			if (F.TempKittens.empty())
			{
				return;
			}
			std::vector<double> StillAlive;
			for (const double Left : F.TempKittens)
			{
				if (Left - Dt > 0.0)
				{
					StillAlive.push_back(Left - Dt);
				}
			}
			F.TempKittens.swap(StillAlive);
			SyncFieldKittens(F, D, P);
		}

		// _nearest_active_add(): to the cat, strict `<`.
		int32_t NearestAdd(const PcField& F)
		{
			int32_t Best = -1;
			double BestD = std::numeric_limits<double>::infinity();
			for (size_t K = 0; K < F.Adds.size(); ++K)
			{
				const double Dist = std::hypot(F.Adds[K].X - F.CatX, F.Adds[K].Y - F.CatY);
				if (Dist < BestD)
				{
					BestD = Dist;
					Best = static_cast<int32_t>(K);
				}
			}
			return Best;
		}

		// Kitten.hunt_add() / hunt_boss(): the mouse dropped (it is re-acquired the instant the cat disengages); the same
		// contact shape as everything else a kitten attacks. An add already intercepted this step is a no-op.
		void HuntKittenAdd(PcField& F, const PcGameData& D, int32_t K, uint64_t AddSerial, double Dt)
		{
			const auto It = std::find_if(F.Adds.begin(), F.Adds.end(), [AddSerial](const PcBossAdd& A) { return A.Serial == AddSerial; });
			if (It == F.Adds.end())
			{
				return;
			}
			const size_t A = static_cast<size_t>(It - F.Adds.begin());
			SetKittenTarget(F, K, -1);
			if (ApproachKitten(F, D, K, F.Adds[A].X, F.Adds[A].Y, Dt))
			{
				HitAdd(F, A, KittenDps(D.Combat, F.Stats) * F.KDamageMult[static_cast<size_t>(K)] * Dt);
			}
		}

		void HuntKittenBoss(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, int32_t K, double Dt)
		{
			if (!F.bBossUp)
			{
				return;
			}
			SetKittenTarget(F, K, -1);
			if (ApproachKitten(F, D, K, F.BossX, F.BossY, Dt))
			{
				BossTakeDamage(F, D, P, In, KittenDps(D.Combat, F.Stats) * F.KDamageMult[static_cast<size_t>(K)] * Dt,
					F.Stats.ShieldPierceFraction);
			}
		}

		// TierController._resolve_kittens(). Kittens join the boss only while the cat is personally on it, and the adds
		// outrank the boss (the pool is worth more on the boss, but an add that lands undoes more than the pool deals in
		// the same seconds). Otherwise two passes: first the claims the pool is ALREADY committed to, then each kitten
		// decides against that complete picture (a single pass made early kittens see an empty board and dogpile).
		void ResolveKittens(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Dt)
		{
			ApplyKittenCards(F, D, P, Dt);
			if (EngagingBoss(F, D, P))
			{
				const int32_t Add = NearestAdd(F);
				const uint64_t AddSerial = Add >= 0 ? F.Adds[static_cast<size_t>(Add)].Serial : 0;
				for (size_t K = 0; K < F.KActive.size(); ++K)
				{
					if (!F.KActive[K])
					{
						continue;
					}
					if (Add >= 0)
					{
						HuntKittenAdd(F, D, static_cast<int32_t>(K), AddSerial, Dt);
					}
					else
					{
						HuntKittenBoss(F, D, P, In, static_cast<int32_t>(K), Dt);
					}
				}
				return;
			}
			std::fill(F.Claims.begin(), F.Claims.end(), int64_t(0));
			for (size_t K = 0; K < F.KActive.size(); ++K)
			{
				if (F.KActive[K] && KittenTargetAlive(F, static_cast<int32_t>(K)))
				{
					++F.Claims[static_cast<size_t>(F.KTarget[K])];
				}
			}
			// By index: a kill inside a hunt can grant a temporary kitten, which wakes (and may grow) the pool mid-loop;
			// Godot's loop then reached the woken kitten too.
			for (size_t K = 0; K < F.KActive.size(); ++K)
			{
				if (F.KActive[K])
				{
					HuntKitten(F, D, P, In, static_cast<int32_t>(K), Dt);
				}
			}
		}

		// TierController._resolve_grand_finale(): the kittens the Magician switched off come back as illusions that drag
		// rather than bite, reaching only mice near a LIVE kitten (the kittens doing the work, not a second Misdirection).
		// They LOOK like illusions exactly when they are: the node owned AND the worn job has zeroed their damage.
		void ResolveGrandFinale(PcField& F, const PcGameData& D, const PcProgress& P)
		{
			const double Pull = KittenIllusionPull(D, P);
			// is_zero_approx(): within Godot's CMP_EPSILON.
			const bool bIllusory = Pull > 0.0 && std::fabs(F.Stats.KittenMultiplicativeDamageProduct) < 0.00001;
			std::fill(F.KIllusory.begin(), F.KIllusory.end(), static_cast<uint8_t>(bIllusory ? 1 : 0));
			if (Pull <= 0.0)
			{
				return;
			}
			for (size_t K = 0; K < F.KActive.size(); ++K)
			{
				if (!F.KActive[K])
				{
					continue;
				}
				for (const int32_t S : F.Live)
				{
					const size_t I = static_cast<size_t>(S);
					if (F.Active[I] && std::hypot(F.X[I] - F.KX[K], F.Y[I] - F.KY[K]) <= D.Code.FinaleReachPx)
					{
						// Enemy.apply_illusion_pull(): the strongest pull of the step, fresh for this step.
						F.IllusionPull[I] = std::max(F.IllusionPull[I], Pull);
						F.IllusionFresh[I] = 1;
					}
				}
			}
		}

		// --- the aura, the Snare and the zones ------------------------------------------------------------------------

		bool IsFlier(const PcField& F, const PcGameData& D, int32_t Slot)
		{
			return DataOf(F, D, Slot).Movement == EnemyMovementKind::FLIER;
		}

		void SlowMouse(PcField& F, const PcGameData& D, int32_t Slot, double Multiplier, bool bReachesFliers = false)
		{
			const size_t I = static_cast<size_t>(Slot);
			ApplySlow(F.SlowMult[I], F.SlowFresh[I], IsFlier(F, D, Slot), Multiplier, bReachesFliers);
		}

		// _resolve_gridlock(): the snared drag down whatever strays over them. Fliers are the ONLY population this reaches
		// (everything on the ground is already inside the arena-wide Snare), so it is "fliers near snared ground mice".
		void ResolveGridlock(PcField& F, const PcGameData& D, const PcProgress& P, double Snare)
		{
			const double Pull = GridlockMultiplier(D, P);
			if (Pull >= 1.0 || Snare >= 1.0)
			{
				return;
			}
			for (const int32_t A : F.Live)
			{
				const size_t I = static_cast<size_t>(A);
				if (!F.Active[I] || !IsFlier(F, D, A))
				{
					continue;
				}
				for (const int32_t G : F.Live)
				{
					const size_t J = static_cast<size_t>(G);
					if (!F.Active[J] || IsFlier(F, D, G))
					{
						continue;
					}
					if (std::hypot(F.X[I] - F.X[J], F.Y[I] - F.Y[J]) <= D.Code.GridlockReachPx)
					{
						SlowMouse(F, D, A, Pull, true);
						break;
					}
				}
			}
		}

		// _resolve_aura_contact() after the flee update: nothing (not even the slow) before the aura is bought.
		void ResolveAuraContact(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Dt)
		{
			if (!F.Stats.bAuraOwned)
			{
				return;
			}
			const double Radius = AuraRadius(D, P);
			double Dps = AuraDps(D.Combat, F.Stats);
			// AURA_SLOW is a speed multiplier that DESCENDS with level: summing it would make a second card speed mice up.
			const double Slow = CardEffectBestLow(D, P, CardEffect::AURA_SLOW);
			// The Snare is arena-wide, so it cannot ride the aura loop (which skips exactly the mice it exists to catch).
			const double Snare = SnareMultiplier(D, P);
			if (Snare < 1.0)
			{
				for (const int32_t S : F.Live)
				{
					if (F.Active[static_cast<size_t>(S)])
					{
						SlowMouse(F, D, S, Snare);
					}
				}
			}
			ResolveGridlock(F, D, P, Snare);
			// CROWD WORK counts the aura's crowd in its own pass BEFORE any damage lands: scaling inside the loop would hit
			// the first mouse reached for a crowd of one and the last for a crowd of all of them.
			const double CrowdPer = AuraCrowdScaling(D, P);
			if (CrowdPer > 0.0)
			{
				int64_t Inside = 0;
				for (const int32_t S : F.Live)
				{
					const size_t I = static_cast<size_t>(S);
					if (F.Active[I] && std::hypot(F.X[I] - F.CatX, F.Y[I] - F.CatY) <= Radius)
					{
						++Inside;
					}
				}
				Dps *= 1.0 + CrowdPer * static_cast<double>(Inside);
			}
			const size_t Count = F.Live.size();
			for (size_t K = 0; K < Count; ++K)
			{
				const int32_t S = F.Live[K];
				const size_t I = static_cast<size_t>(S);
				if (!F.Active[I] || std::hypot(F.X[I] - F.CatX, F.Y[I] - F.CatY) > Radius)
				{
					continue;
				}
				if (Slow > 0.0)
				{
					SlowMouse(F, D, S, Slow);
				}
				const double AtX = F.X[I];
				const double AtY = F.Y[I];
				const PcBlowResult R = Hit(F, D, P, In, S, Dps * Dt, F.Stats.ShieldPierceFraction);
				if (R.Overkill > 0.0)
				{
					// From where the victim died (F16, 2026-11-03; Godot read the parking spot after the blow).
					SplashOverkill(F, D, P, In, AtX, AtY,
						R.Overkill);
				}
			}
		}

		// _add_damage_zone(): one primitive for every "leave damage on the floor" effect. Bounded (oldest dropped first)
		// so a badly retuned cadence cannot leak zones.
		void AddDamageZone(PcField& F, const PcGameData& D, double X, double Y, double Radius, double PowerFraction, double Seconds,
			PcZoneKind Kind)
		{
			if (PowerFraction <= 0.0 || Seconds <= 0.0 || Radius <= 0.0)
			{
				return;
			}
			if (static_cast<int64_t>(F.Zones.size()) >= D.Code.MaxDamageZones)
			{
				F.Zones.erase(F.Zones.begin());
			}
			PcDamageZone Z;
			Z.X = X;
			Z.Y = Y;
			Z.Radius = Radius;
			Z.PowerFraction = PowerFraction;
			Z.Remaining = Seconds;
			Z.Duration = std::max(Seconds, 0.001);
			Z.Kind = Kind;
			F.Zones.push_back(Z);
		}

		// _resolve_damage_zones(): priced live against the WHOLE player (never snapshotted: a zone made in biome 1 and
		// resolved in biome 8 is worth biome-8 damage). Fliers take nothing from the ground. The boss is hit too (the poop
		// zone once shipped without that and the Unicorn did nothing in the fight that mattered most).
		void ResolveDamageZones(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Dt)
		{
			if (F.Zones.empty())
			{
				return;
			}
			const double Power = PlayerDps(D.Combat, F.Stats);
			std::vector<PcDamageZone> Survivors;
			for (PcDamageZone& Z : F.Zones)
			{
				const double Dps = Power * Z.PowerFraction;
				const size_t Count = F.Live.size();
				for (size_t K = 0; K < Count; ++K)
				{
					const int32_t S = F.Live[K];
					const size_t I = static_cast<size_t>(S);
					if (!F.Active[I] || IsFlier(F, D, S))
					{
						continue;
					}
					if (std::hypot(F.X[I] - Z.X, F.Y[I] - Z.Y) <= Z.Radius)
					{
						Hit(F, D, P, In, S, Dps * Dt, F.Stats.ShieldPierceFraction);
					}
				}
				if (F.bBossUp && std::hypot(F.BossX - Z.X, F.BossY - Z.Y) <= Z.Radius)
				{
					BossTakeDamage(F, D, P, In, Dps * Dt, F.Stats.ShieldPierceFraction);
				}
				Z.Remaining -= Dt;
				if (Z.Remaining > 0.0)
				{
					Survivors.push_back(Z);
				}
			}
			F.Zones.swap(Survivors);
		}

		// _resolve_aura_trail(): AURA_TRAIL drops a zone on a cadence, and only when the cat has MOVED (a parked cursor would
		// stack every drop in one place and make the card a second aura). Half the aura's radius: a trail as wide as the
		// aura reads as smearing the aura around.
		void ResolveAuraTrail(PcField& F, const PcGameData& D, const PcProgress& P, double Dt)
		{
			const double Fraction = CardEffectValue(D, P, CardEffect::AURA_TRAIL);
			if (Fraction <= 0.0)
			{
				return;
			}
			F.TrailElapsed += Dt;
			if (F.TrailElapsed < D.Code.AuraTrailDropSec)
			{
				return;
			}
			F.TrailElapsed = 0.0;
			if (F.bTrailDropped && std::hypot(F.CatX - F.TrailLastX, F.CatY - F.TrailLastY) < D.Code.AuraTrailMinMovePx)
			{
				return;
			}
			F.bTrailDropped = true;
			F.TrailLastX = F.CatX;
			F.TrailLastY = F.CatY;
			AddDamageZone(F, D, F.CatX, F.CatY, AuraRadius(D, P) * 0.5, Fraction, D.Code.AuraTrailLifetimeSec, PcZoneKind::Trail);
		}

		// _resolve_aura_echo(): ECHO, a second weaker aura a beat behind the cat. ONE zone, living exactly as long as the
		// cadence, so there is never more than one (a trail smears; an echo lags).
		void ResolveAuraEcho(PcField& F, const PcGameData& D, const PcProgress& P, double Dt)
		{
			const double Power = AuraEchoPower(D, P);
			if (Power <= 0.0)
			{
				F.EchoElapsed = 0.0;
				return;
			}
			F.EchoElapsed += Dt;
			if (F.EchoElapsed < D.Code.EchoDropSec)
			{
				return;
			}
			F.EchoElapsed = 0.0;
			AddDamageZone(F, D, F.CatX, F.CatY, AuraRadius(D, P), Power, D.Code.EchoDropSec, PcZoneKind::Echo);
		}

		// _resolve_edge_burn(): EDGE_BURN, mice in the band along the rim burn so the ones wandering off are not simply
		// lost. Priced off the AURA (the card's text; the whole player's damage on every stray was the strongest effect in
		// the game: "all the mice die very very fast").
		void ResolveEdgeBurn(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Dt)
		{
			const double Fraction = CardEffectValue(D, P, CardEffect::EDGE_BURN);
			if (Fraction <= 0.0)
			{
				return;
			}
			// Rect2.grow(-band): the inner rectangle; a mouse outside it is in the band.
			const double Band = D.Code.EdgeBurnBandPx;
			const PcRect Inner{ F.Bounds.X + Band, F.Bounds.Y + Band, F.Bounds.W - 2.0 * Band, F.Bounds.H - 2.0 * Band };
			const double Dps = AuraDps(D.Combat, F.Stats) * Fraction;
			const size_t Count = F.Live.size();
			for (size_t K = 0; K < Count; ++K)
			{
				const int32_t S = F.Live[K];
				const size_t I = static_cast<size_t>(S);
				if (!F.Active[I] || Inner.HasPoint(F.X[I], F.Y[I]))
				{
					continue;
				}
				Hit(F, D, P, In, S, Dps * Dt, F.Stats.ShieldPierceFraction);
			}
		}

		// _resolve_aura_pulse(): AURA_PULSE detonates the aura for a burst every interval; Standing Wave beats on the SAME
		// clock and adds to it (two pulses on two timers would double-hit when they coincided). The boss too.
		void ResolveAuraPulse(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Dt)
		{
			const double Seconds = CardEffectValue(D, P, CardEffect::AURA_PULSE) + AuraBeatSeconds(D, P);
			if (Seconds <= 0.0)
			{
				F.AuraPulseElapsed = 0.0;
				return;
			}
			F.AuraPulseElapsed += Dt;
			if (F.AuraPulseElapsed < D.Code.AuraPulseIntervalSec)
			{
				return;
			}
			F.AuraPulseElapsed = 0.0;
			const double Radius = AuraRadius(D, P);
			const double Damage = AuraDps(D.Combat, F.Stats) * Seconds;
			const size_t Count = F.Live.size();
			for (size_t K = 0; K < Count; ++K)
			{
				const int32_t S = F.Live[K];
				const size_t I = static_cast<size_t>(S);
				if (F.Active[I] && std::hypot(F.X[I] - F.CatX, F.Y[I] - F.CatY) <= Radius)
				{
					Hit(F, D, P, In, S, Damage, F.Stats.ShieldPierceFraction);
				}
			}
			if (F.bBossUp && std::hypot(F.BossX - F.CatX, F.BossY - F.CatY) <= Radius)
			{
				BossTakeDamage(F, D, P, In, Damage, F.Stats.ShieldPierceFraction);
			}
			Push(F, PcFieldEventKind::AuraPulse, -1, F.CatX, F.CatY, Radius);
		}

		// _refresh_poop_timer(): POOP_TRAIL's value IS the interval and descends with level (best-low, never summed),
		// floored at the spawn floor so it cannot fire every frame.
		double PoopInterval(const PcGameData& D, const PcProgress& P)
		{
			const double Interval = CardEffectBestLow(D, P, CardEffect::POOP_TRAIL);
			return Interval > 0.0 ? std::max(Interval, D.Code.MinSpawnInterval) : D.Combat.PoopCooldownSec;
		}

		// The PoopTimer's timeout (_on_poop_timer_timeout): the timer is refreshed on every drop (a card taken mid-run
		// takes effect this run; the new wait applies from the NEXT cycle, as a Godot Timer restarts before it signals),
		// then, with the Unicorn's Rainbow Poop, a zone opens under the cat. POOP_DURATION adds seconds.
		void TickPoopTimer(PcField& F, const PcGameData& D, const PcProgress& P, double Dt)
		{
			F.PoopTimeLeft -= Dt;
			if (F.PoopTimeLeft >= 0.0)
			{
				return;
			}
			F.PoopTimeLeft += F.PoopWait;
			F.PoopWait = PoopInterval(D, P);
			if (!PoopUnlocked(P))
			{
				return;
			}
			F.bPoopActive = true;
			F.PoopX = F.CatX;
			F.PoopY = F.CatY;
			F.PoopRemaining = D.Combat.PoopDurationSec + CardEffectValue(D, P, CardEffect::POOP_DURATION);
			Push(F, PcFieldEventKind::PoopDropped, -1, F.PoopX, F.PoopY, D.Combat.PoopRadiusPx, 0, F.PoopRemaining, 0.0);
		}

		// _resolve_poop_zone(): continuous DPS while the zone lasts (a mouse that wanders in partway takes a share), and
		// no Shield Pierce (Godot's default argument). The boss too (its adds take the poop in their own pass).
		void ResolvePoopZone(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Dt)
		{
			if (!F.bPoopActive)
			{
				return;
			}
			const double Dps = PoopDps(D.Combat, F.Stats);
			const double Radius = D.Combat.PoopRadiusPx;
			const size_t Count = F.Live.size();
			for (size_t K = 0; K < Count; ++K)
			{
				const int32_t S = F.Live[K];
				const size_t I = static_cast<size_t>(S);
				if (F.Active[I] && std::hypot(F.X[I] - F.PoopX, F.Y[I] - F.PoopY) <= Radius)
				{
					Hit(F, D, P, In, S, Dps * Dt, 0.0);
				}
			}
			if (F.bBossUp && std::hypot(F.BossX - F.PoopX, F.BossY - F.PoopY) <= Radius)
			{
				BossTakeDamage(F, D, P, In, Dps * Dt, 0.0);
			}
			F.PoopRemaining -= Dt;
			if (F.PoopRemaining <= 0.0)
			{
				F.bPoopActive = false;
			}
		}

		// --- Pounce ----------------------------------------------------------------------------------------------------

		// _fire_pounce(): the burst on everything in the radius (the vulnerable-window bonus only on a target whose window
		// is open: a shielded one takes nothing anyway), then the chain beyond the rim, the kittens' mimic, the crater.
		// The Cone of Shame roots the cat (auto pounces too: a cost Auto-Pounce could dodge would make the job a free x4),
		// with Nine Lives Insurance's window and Crash Landing's crater the length of the root. The adds and the boss are
		// hit too; the boss always has phase windows, so timing a Pounce into one is the best thing a player can do.
		void FirePounce(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, bool bAutomatic)
		{
			F.PounceCooldownLeft = PounceCooldown(D.Combat, F.Stats);
			F.PounceCooldownFull = F.PounceCooldownLeft;
			const double Scale = bAutomatic ? AutoPounceMultiplier(D, P) : 1.0;
			const double LockSec = JobPounceLockSec(D, P);
			if (LockSec > 0.0)
			{
				F.PounceLockLeft = LockSec; // Player.lock_after_pounce()
			}
			if (HasPounceInsurance(D, P))
			{
				F.InsuranceWindow = LockSec;
				F.InsuranceKills = 0;
			}
			const double Crater = PounceCraterPower(D, P);
			if (Crater > 0.0 && LockSec > 0.0)
			{
				AddDamageZone(F, D, F.CatX, F.CatY, AuraRadius(D, P), Crater, LockSec, PcZoneKind::Crater);
			}
			const double Radius = D.Combat.PounceRadiusPx;
			const double OX = F.CatX;
			const double OY = F.CatY;
			Push(F, PcFieldEventKind::Pounce, -1, OX, OY, Radius, bAutomatic ? FieldFlag::Auto : 0);

			const size_t Count = F.Live.size();
			for (size_t K = 0; K < Count; ++K)
			{
				const int32_t S = F.Live[K];
				const size_t I = static_cast<size_t>(S);
				if (!F.Active[I] || std::hypot(F.X[I] - OX, F.Y[I] - OY) > Radius)
				{
					continue;
				}
				const bool bVulnerable = DataOf(F, D, S).InvulnVulnerableSec > 0.0 && !F.Shielded[I];
				const PcBlowResult R = Hit(F, D, P, In, S, PounceDamage(D.Combat, F.Stats, bVulnerable, Scale), F.Stats.ShieldPierceFraction);
				if (R.Overkill > 0.0)
				{
					// The Pounce blast's kill: from where the victim died (F16, 2026-11-03).
					SplashOverkill(F, D, P, In, F.X[I], F.Y[I], R.Overkill);
				}
			}

			// The adds in range (no window bonus: an add has no shield to time against).
			for (size_t K = 0; K < F.Adds.size();)
			{
				const size_t Before = F.Adds.size();
				if (std::hypot(F.Adds[K].X - OX, F.Adds[K].Y - OY) <= Radius)
				{
					HitAdd(F, K, PounceDamage(D.Combat, F.Stats, false, Scale));
				}
				if (F.Adds.size() == Before)
				{
					++K;
				}
			}
			if (F.bBossUp && std::hypot(F.BossX - OX, F.BossY - OY) <= Radius)
			{
				BossTakeDamage(F, D, P, In, PounceDamage(D.Combat, F.Stats, !F.bBossShielded, Scale), 0.0);
				F.bBossEngagedOnce = true;
			}

			// _resolve_pounce_chain(): POUNCE_CHAIN reaches OUTSIDE the blast (inside it Pounce already hits everything), the
			// N nearest beyond the rim, nearest first. No vulnerable bonus: the bonus is payment for aiming. (Godot's sort is
			// not stable; equally near mice keep live order here.)
			const int64_t Extra = static_cast<int64_t>(CardEffectValue(D, P, CardEffect::POUNCE_CHAIN));
			if (Extra > 0)
			{
				std::vector<int32_t> Outside;
				for (const int32_t S : F.Live)
				{
					const size_t I = static_cast<size_t>(S);
					if (F.Active[I] && std::hypot(F.X[I] - OX, F.Y[I] - OY) > Radius)
					{
						Outside.push_back(S);
					}
				}
				std::stable_sort(Outside.begin(), Outside.end(), [&F, OX, OY](int32_t A, int32_t B)
				{
					const size_t I = static_cast<size_t>(A);
					const size_t J = static_cast<size_t>(B);
					return (F.X[I] - OX) * (F.X[I] - OX) + (F.Y[I] - OY) * (F.Y[I] - OY)
						< (F.X[J] - OX) * (F.X[J] - OX) + (F.Y[J] - OY) * (F.Y[J] - OY);
				});
				const double Damage = PounceDamage(D.Combat, F.Stats, false, Scale);
				for (int64_t K = 0; K < std::min<int64_t>(Extra, static_cast<int64_t>(Outside.size())); ++K)
				{
					const int32_t S = Outside[static_cast<size_t>(K)];
					const size_t I = static_cast<size_t>(S);
					const PcBlowResult R = Hit(F, D, P, In, S, Damage, F.Stats.ShieldPierceFraction);
					if (R.Overkill > 0.0)
					{
						SplashOverkill(F, D, P, In, F.X[I], F.Y[I], R.Overkill);
					}
				}
			}

			// _resolve_kitten_mimic_pounce(): every kitten pounces what it is ALREADY hunting for a share (so its value
			// depends on how well the pool is spread, not a flat Pounce multiplier). No vulnerable bonus: no aiming.
			const double Share = CardEffectValue(D, P, CardEffect::KITTEN_MIMIC_POUNCE);
			if (Share > 0.0)
			{
				const double Damage = PounceDamage(D.Combat, F.Stats, false, Scale) * Share;
				for (size_t K = 0; K < F.KActive.size(); ++K)
				{
					if (!F.KActive[K] || !KittenTargetAlive(F, static_cast<int32_t>(K)))
					{
						continue;
					}
					const int32_t S = F.KTarget[K];
					const size_t I = static_cast<size_t>(S);
					const PcBlowResult R = Hit(F, D, P, In, S, Damage, F.Stats.ShieldPierceFraction);
					if (R.Overkill > 0.0)
					{
						SplashOverkill(F, D, P, In, F.X[I], F.Y[I], R.Overkill);
					}
				}
			}

			// POUNCE_CRATER: a burning patch where it landed, on the shared zone primitive.
			AddDamageZone(F, D, OX, OY, Radius, CardEffectValue(D, P, CardEffect::POUNCE_CRATER), D.Code.PounceCraterLifetimeSec,
				PcZoneKind::Crater);
		}

		// _unhandled_input(): a press fires when Pounce is owned and ready. Only the crossing from released to pressed
		// counts (a pad's trigger is an axis: every motion past the dead zone reads "pressed", so a held trigger would
		// re-fire on jitter the moment the cooldown ended); a press during the cooldown is spent.
		void HandlePounceInput(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In)
		{
			const bool bWasDown = F.bPounceWasDown;
			F.bPounceWasDown = In.bPounceDown;
			if (!In.bPounceDown || bWasDown)
			{
				return;
			}
			if (!IsPounceOwned(P) || F.PounceCooldownLeft > 0.0)
			{
				return;
			}
			FirePounce(F, D, P, In, false);
		}

		// _tick_pounce(): the cooldown runs only once Pounce is owned; Reflex fires the instant it is ready.
		void TickPounce(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Dt)
		{
			if (!IsPounceOwned(P))
			{
				return;
			}
			F.PounceCooldownLeft = std::max(0.0, F.PounceCooldownLeft - Dt);
			if (F.PounceCooldownLeft <= 0.0 && AutoPounceUnlocked(P))
			{
				FirePounce(F, D, P, In, true);
			}
		}

		// _tick_pounce_insurance(): a window the length of the root; if it closes with nothing killed, the Pounce did not
		// happen as far as the cooldown is concerned (the Cone of Shame's cost is that it can be wasted; this buys it back).
		void TickPounceInsurance(PcField& F, double Dt)
		{
			if (F.InsuranceWindow <= 0.0)
			{
				return;
			}
			F.InsuranceWindow -= Dt;
			if (F.InsuranceWindow > 0.0)
			{
				return;
			}
			F.InsuranceWindow = 0.0;
			if (F.InsuranceKills == 0)
			{
				F.PounceCooldownLeft = 0.0;
			}
		}

		// --- the kill's extras --------------------------------------------------------------------------------------

		// _detonate_catnip_bomb(): seconds of AURA DPS (a flat bomb would be worthless by biome 3), on every live mouse in
		// the radius, with the cat's Shield Pierce. Split out so Overflow can drop one without touching the card's count.
		void DetonateCatnipBomb(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double AtX, double AtY)
		{
			const double Damage = AuraDps(D.Combat, F.Stats) * D.Code.CatnipBombDamageSeconds;
			const size_t Count = F.Live.size();
			for (size_t K = 0; K < Count; ++K)
			{
				const int32_t S = F.Live[K];
				const size_t I = static_cast<size_t>(S);
				if (F.Active[I] && std::hypot(F.X[I] - AtX, F.Y[I] - AtY) <= D.Code.CatnipBombRadius)
				{
					Hit(F, D, P, In, S, Damage, F.Stats.ShieldPierceFraction);
				}
			}
			Push(F, PcFieldEventKind::CatnipBomb, -1, AtX, AtY, D.Code.CatnipBombRadius);
		}

		// _advance_catnip_bomb(): CATNIP_BOMB_EVERY detonates on every Nth kill where the last mouse died (best-low: the
		// value is a count that falls with level).
		void AdvanceCatnipBomb(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double AtX, double AtY)
		{
			const double Every = CardEffectBestLow(D, P, CardEffect::CATNIP_BOMB_EVERY);
			if (Every <= 0.0)
			{
				return;
			}
			++F.KillsSinceBomb;
			if (F.KillsSinceBomb < static_cast<int64_t>(Every))
			{
				return;
			}
			F.KillsSinceBomb = 0;
			DetonateCatnipBomb(F, D, P, In, AtX, AtY);
		}

		// _on_enemy_despawned(): a mouse that wandered off pays nothing, unless Scavenge (the card, a run's luck) and
		// Pickpocket (the node, the job's trade) recover a share of its BASE reward. Deliberately not a kill: no kill
		// count, no XP, no boss progress.
		void PayDespawnRecovery(const PcGameData& D, PcProgress& P, const PcEnemy& E)
		{
			const double Recovered = CardEffectValue(D, P, CardEffect::SCAVENGE) + PickpocketShare(D, P);
			if (Recovered <= 0.0 || E.bRewardExempt)
			{
				return;
			}
			AddCurrency(D, P, E.RewardCatnip * std::min(Recovered, 1.0));
		}

		// _tick_sleight_of_hand(): every so often the first eligible mouse simply stops existing, killed through the damage
		// path (with full Shield Pierce) so it counts, pays and feeds every per-kill node. Reward-exempt mice are skipped.
		// Bosses are exempt (a node that deleted the boss would delete the fight); here it is not in the pool at all.
		void TickSleightOfHand(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Dt)
		{
			const double Interval = VanishInterval(D, P);
			if (Interval <= 0.0)
			{
				F.VanishTimer = 0.0;
				return;
			}
			F.VanishTimer += Dt;
			if (F.VanishTimer < Interval)
			{
				return;
			}
			F.VanishTimer = 0.0;
			for (const int32_t S : F.Live)
			{
				const size_t I = static_cast<size_t>(S);
				if (!F.Active[I] || DataOf(F, D, S).bRewardExempt)
				{
					continue;
				}
				Hit(F, D, P, In, S, F.Hp[I] * 1000.0, 1.0);
				return;
			}
		}

		// --- the Prism -----------------------------------------------------------------------------------------------

		// _nearest_enemy_to(): squared distance, strict `<` (the FIRST of equally near mice in live order wins).
		int32_t NearestTo(const PcField& F, double PX, double PY, const std::vector<int32_t>& Taken)
		{
			int32_t Best = -1;
			double BestD = std::numeric_limits<double>::infinity();
			for (const int32_t S : F.Live)
			{
				const size_t I = static_cast<size_t>(S);
				if (!F.Active[I] || std::find(Taken.begin(), Taken.end(), S) != Taken.end())
				{
					continue;
				}
				const double DSq = (F.X[I] - PX) * (F.X[I] - PX) + (F.Y[I] - PY) * (F.Y[I] - PY);
				if (DSq < BestD)
				{
					BestD = DSq;
					Best = S;
				}
			}
			return Best;
		}

		// _tick_prism(): the beam's DPS split across its targets: the nearest mouse and Refraction's chain walking outward
		// from the LAST link (so it walks the crowd instead of firing a fan), or, with Full Spectrum, everything in the
		// aura's reach (the sweep cannot focus). A kill by the beam is a Spectrum kill (killed_by_beam for that one blow)
		// and a shard; every PRISM_SHARDS_PER_DETONATION shards, Critical Mass detonates on EVERY live mouse.
		void TickPrism(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Dt)
		{
			const double Dps = PrismBeamDps(D, P, F.Stats);
			if (Dps <= 0.0)
			{
				return;
			}
			std::vector<int32_t> Targets;
			if (PrismSweeps(P))
			{
				const double Radius = AuraRadius(D, P);
				for (const int32_t S : F.Live)
				{
					const size_t I = static_cast<size_t>(S);
					if (F.Active[I] && std::hypot(F.X[I] - F.CatX, F.Y[I] - F.CatY) <= Radius)
					{
						Targets.push_back(S);
					}
				}
			}
			else
			{
				const int32_t Nearest = NearestTo(F, F.CatX, F.CatY, Targets);
				if (Nearest >= 0)
				{
					Targets.push_back(Nearest);
					double PX = F.X[static_cast<size_t>(Nearest)];
					double PY = F.Y[static_cast<size_t>(Nearest)];
					const int64_t Chain = PrismChainCount(D, P);
					for (int64_t K = 0; K < Chain; ++K)
					{
						const int32_t Next = NearestTo(F, PX, PY, Targets);
						if (Next < 0)
						{
							break;
						}
						Targets.push_back(Next);
						PX = F.X[static_cast<size_t>(Next)];
						PY = F.Y[static_cast<size_t>(Next)];
					}
				}
			}
			const double Share = Dps * Dt / std::max(static_cast<double>(Targets.size()), 1.0);
			double FromX = F.CatX;
			double FromY = F.CatY;
			for (const int32_t S : Targets)
			{
				const size_t I = static_cast<size_t>(S);
				if (!F.Active[I])
				{
					continue;
				}
				const double AtX = F.X[I];
				const double AtY = F.Y[I];
				const double Before = F.Hp[I];
				F.BeamKill[I] = 1;
				Hit(F, D, P, In, S, Share, F.Stats.ShieldPierceFraction);
				F.BeamKill[I] = 0;
				if (Before > 0.0 && F.Hp[I] <= 0.0)
				{
					F.PrismShards += 1.0;
				}
				// Every step; the renderer draws at PRISM_BEAM_DRAW_INTERVAL (presentation, appendix C).
				Push(F, PcFieldEventKind::PrismBeam, S, FromX, FromY, Share, 0, AtX, AtY);
				FromX = AtX;
				FromY = AtY;
			}
			if (F.PrismShards >= D.Code.PrismShardsPerDetonation)
			{
				F.PrismShards -= D.Code.PrismShardsPerDetonation;
				// _detonate_prism_shards()
				const double Payload = PrismShardPayload(D, P);
				if (Payload > 0.0)
				{
					const double Damage = AuraDps(D.Combat, F.Stats) * Payload;
					Push(F, PcFieldEventKind::PrismBurst, -1, F.CatX, F.CatY, AuraRadius(D, P) * 1.6);
					const size_t Count = F.Live.size();
					for (size_t K = 0; K < Count; ++K)
					{
						const int32_t S = F.Live[K];
						if (F.Active[static_cast<size_t>(S)])
						{
							Hit(F, D, P, In, S, Damage, F.Stats.ShieldPierceFraction);
						}
					}
				}
			}
		}

		// --- the spawner -------------------------------------------------------------------------------------------

		// _random_edge_position(): one of four edges, inset (Rect2.has_point excludes the far edges, so a mouse born on one
		// would read as already out). The spawner's and the boss's adds'.
		void EdgePosition(const PcField& F, const PcGameData& D, uint64_t Entity, double& OX, double& OY)
		{
			const PcRect& B = F.Bounds;
			const uint64_t Edge = RngBelow(4, F.Seed, F.StepCount, Entity, Salt::SpawnEdge);
			const double Along = Unit(F, Entity, Salt::SpawnAlong);
			const double Inset = D.Code.SpawnEdgeInset;
			switch (Edge)
			{
			case 0: OX = B.X + Along * B.W; OY = B.Y + Inset; break;
			case 1: OX = B.X + Along * B.W; OY = B.Bottom() - Inset; break;
			case 2: OX = B.X + Inset; OY = B.Y + Along * B.H; break;
			default: OX = B.Right() - Inset; OY = B.Y + Along * B.H; break;
			}
		}

		// _spawn_position(): THE PIED PIPER summons mice on a ring around the cat (clamped to the field, far edges
		// included, as Godot's Vector2.clamp does); otherwise an edge.
		void SpawnPosition(const PcField& F, const PcGameData& D, const PcProgress& P, uint64_t Entity, double& OX, double& OY)
		{
			const double Piper = PiedPiperRadius(D, P);
			if (Piper > 0.0)
			{
				const PcRect& B = F.Bounds;
				const double Angle = Unit(F, Entity, Salt::PiedPiper) * 6.283185307179586;
				OX = std::clamp(F.CatX + std::cos(Angle) * Piper, B.X, B.Right());
				OY = std::clamp(F.CatY + std::sin(Angle) * Piper, B.Y, B.Bottom());
				return;
			}
			EdgePosition(F, D, Entity, OX, OY);
		}

		int32_t Activate(PcField& F, const PcGameData& D, const PcProgress& P, const PcEnemy& Data, uint64_t Entity, double SX,
			double SY, double DX, double DY);

		// _foresee_next_spawn(): SIXTH SENSE pre-rolls the next spawn (kind and place) so there is something to point at
		// before it arrives. Its rolls are their own RNG entity: the spawn after this one, seen early.
		void ForeseeNextSpawn(PcField& F, const PcGameData& D, const PcProgress& P, const PcTier& Tier, double GoldenChance,
			double SignatureWeight, uint64_t Entity)
		{
			const uint64_t Seer = Entity + ForesightEntity;
			const PcEnemy* Next = nullptr;
			if (!Tier.GoldenEnemy.empty() && Unit(F, Seer, Salt::Golden) < std::min(GoldenChance, 0.5))
			{
				Next = D.FindEnemy(Tier.GoldenEnemy);
			}
			else
			{
				Next = PickEnemy(D, Tier, Unit(F, Seer, Salt::SpawnPick), SignatureWeight);
			}
			F.bForeseen = Next != nullptr;
			F.ForeseenKind = Next ? static_cast<int32_t>(Next - D.Enemies.data()) : -1;
			SpawnPosition(F, D, P, Seer, F.ForeseenX, F.ForeseenY);
		}

		// _spawn_one(): golden first (it wins outright, so a golden is never also a signature), else the weighted pool. A
		// foreseen spawn replaces this one's kind and place (both still rolled, as Godot's were).
		void SpawnOne(PcField& F, const PcGameData& D, const PcProgress& P)
		{
			const PcTier& Tier = TierOf(F, D);
			const uint64_t Entity = ++F.Spawns;
			const double GoldenChance = GoldenChanceFor(D, P, Tier) * std::max(CardEffectValue(D, P, CardEffect::GOLDEN_FREQUENCY), 1.0);
			const double SignatureWeight = std::max(CardEffectValue(D, P, CardEffect::SIGNATURE_FREQUENCY), 1.0);
			const PcEnemy* Data = nullptr;
			if (!Tier.GoldenEnemy.empty() && Unit(F, Entity, Salt::Golden) < std::min(GoldenChance, 0.5))
			{
				Data = D.FindEnemy(Tier.GoldenEnemy);
			}
			else
			{
				Data = PickEnemy(D, Tier, Unit(F, Entity, Salt::SpawnPick), SignatureWeight);
			}
			if (!Data)
			{
				return;
			}
			double SX = 0.0, SY = 0.0;
			SpawnPosition(F, D, P, Entity, SX, SY);
			if (F.bForeseen)
			{
				Data = &D.Enemies[static_cast<size_t>(F.ForeseenKind)];
				SX = F.ForeseenX;
				SY = F.ForeseenY;
				F.bForeseen = false;
			}
			// _inward_direction(): toward the middle, spread so a mouse does not beeline back out the edge it came in by.
			double DX = F.Bounds.CentreX() - SX;
			double DY = F.Bounds.CentreY() - SY;
			Normalize(DX, DY);
			Rotate(DX, DY, Spread(F, Entity, Salt::SpawnTurn, D.Code.SpawnDirectionSpread));
			Activate(F, D, P, *Data, Entity, SX, SY, DX, DY);
			if (SpawnForesight(D, P))
			{
				ForeseeNextSpawn(F, D, P, Tier, GoldenChance, SignatureWeight, Entity);
			}
		}

		// activate(): costs and the wave sampled now, so a mouse on screen is never retroactively buffed.
		int32_t Activate(PcField& F, const PcGameData& D, const PcProgress& P, const PcEnemy& Data, uint64_t Entity, double SX,
			double SY, double DX, double DY)
		{
			const int32_t Slot = FreeSlot(F);
			const size_t I = static_cast<size_t>(Slot);
			const PcEnemySpawnStats S = EnemySpawnStats(D, P, Data);
			F.Kind[I] = static_cast<int32_t>(&Data - D.Enemies.data());
			F.Active[I] = 1;
			F.Serial[I] = Entity;
			F.X[I] = SX;
			F.Y[I] = SY;
			F.DirX[I] = DX;
			F.DirY[I] = DY;
			F.Hp[I] = S.Hp;
			F.MaxHp[I] = S.MaxHp;
			F.WaveReward[I] = S.WaveRewardMultiplier;
			F.SpeedMult[I] = S.SpeedMultiplier;
			// Whole bounces, plus a CHANCE at the fraction, so no rung of Funnel is dead weight.
			F.Bounces[I] = std::floor(S.FunnelBounces) + (Unit(F, Entity, Salt::Funnel) < S.FunnelBounces - std::floor(S.FunnelBounces) ? 1.0 : 0.0);
			F.LifetimeLeft[I] = Data.LifetimeSec;
			F.InvulnTimer[I] = 0.0;
			F.Shielded[I] = 0;
			F.SlowMult[I] = 1.0;
			F.SlowFresh[I] = 0;
			F.HasFleeTarget[I] = 0;
			F.BeamKill[I] = 0;
			F.Live.push_back(Slot);
			Push(F, PcFieldEventKind::Spawned, Slot, SX, SY);
			return Slot;
		}

		// The SpawnTimer's timeout: the rate re-read every tick (Infestation decays between kills, no signal says so), then
		// a batch, never past max_live_enemies.
		void TickSpawner(PcField& F, const PcGameData& D, const PcProgress& P, double Dt)
		{
			F.SpawnTimeLeft -= Dt;
			if (F.SpawnTimeLeft >= 0.0)
			{
				return;
			}
			const PcSpawnCadence Cadence = SpawnCadence(D, SpawnRatePerSec(D, P, TierOf(F, D)));
			F.SpawnTimeLeft += Cadence.Interval;
			for (int64_t K = 0; K < Cadence.Batch; ++K)
			{
				if (F.LiveCount() >= D.Code.MaxLiveEnemies)
				{
					return;
				}
				SpawnOne(F, D, P);
			}
		}

		// --- one mouse moving (Enemy._process) ---------------------------------------------------------------------

		void MoveEnemy(PcField& F, const PcGameData& D, PcProgress& P, int32_t Slot, double Dt)
		{
			const size_t I = static_cast<size_t>(Slot);
			const PcEnemy& E = DataOf(F, D, Slot);
			const bool bFlier = E.Movement == EnemyMovementKind::FLIER;
			// A fleeing enemy that never left could be cornered and farmed forever.
			if (E.LifetimeSec > 0.0)
			{
				F.LifetimeLeft[I] -= Dt;
				if (F.LifetimeLeft[I] <= 0.0)
				{
					Despawn(F, D, P, Slot);
					return;
				}
			}
			const double CX = F.Bounds.CentreX();
			const double CY = F.Bounds.CentreY();
			if (E.bFleesPlayer && F.HasFleeTarget[I])
			{
				// Steers away from the cat with a pull back toward the middle, so it ARCS instead of leaving by the edge.
				double AX = F.X[I] - F.CatX;
				double AY = F.Y[I] - F.CatY;
				if (std::hypot(AX, AY) > 0.001)
				{
					Normalize(AX, AY);
					double IX = CX - F.X[I];
					double IY = CY - F.Y[I];
					if (std::hypot(IX, IY) > 0.001)
					{
						Normalize(IX, IY);
						AX += IX * D.Code.EnemyFleeCentreBias;
						AY += IY * D.Code.EnemyFleeCentreBias;
						Normalize(AX, AY);
					}
					SteerToward(F.DirX[I], F.DirY[I], AX, AY, 1.0 - std::exp(-6.0 * Dt));
				}
			}
			else if (bFlier)
			{
				// Straight across: a flier that wandered would linger, and lingering is what makes a mouse farmable.
			}
			else if (E.bSeeksPlayer)
			{
				// Gristle walks toward the cat, steered so it lumbers rather than homes.
				const double TX = F.CatX - F.X[I];
				const double TY = F.CatY - F.Y[I];
				const double L = std::hypot(TX, TY);
				if (L > 0.001)
				{
					SteerToward(F.DirX[I], F.DirY[I], TX / L, TY / L, 1.0 - std::exp(-2.0 * Dt));
				}
			}
			else
			{
				// Misdirection blends the wander toward the cat instead of replacing it (a mouse that homed exactly would
				// queue in a line, and a line is not a crowd). Grand Finale's illusions fold into the same steer: two
				// steerers would fight each other on alternate frames.
				const double Pull = EnemyDrift(D, P) + F.IllusionPull[I];
				if (Pull > 0.0)
				{
					const double TX = F.CatX - F.X[I];
					const double TY = F.CatY - F.Y[I];
					const double L = std::hypot(TX, TY);
					if (L > 0.001)
					{
						SteerToward(F.DirX[I], F.DirY[I], TX / L, TY / L, 1.0 - std::exp(-Pull * Dt));
					}
				}
				else
				{
					Rotate(F.DirX[I], F.DirY[I], Spread(F, F.Serial[I], Salt::Wander, D.Code.EnemyWanderTurnRate) * Dt);
				}
			}
			// A slow lasts until this move: the aura sets it again every step the mouse is still in range; the step it is
			// not, it clears here and the mouse speeds back up.
			if (!F.SlowFresh[I])
			{
				F.SlowMult[I] = 1.0;
			}
			F.SlowFresh[I] = 0;
			// Same one-step life for the illusion pull: set again every step a kitten stands near, cleared the step none does.
			if (!F.IllusionFresh[I])
			{
				F.IllusionPull[I] = 0.0;
			}
			F.IllusionFresh[I] = 0;
			double Pace = D.Code.EnemySpeedPxPerSec * F.SpeedMult[I] * F.SlowMult[I] * E.SpeedMultiplier;
			if (bFlier)
			{
				Pace *= D.Code.EnemyFlierSpeedMultiplier;
			}
			F.X[I] += F.DirX[I] * Pace * Dt;
			F.Y[I] += F.DirY[I] * Pace * Dt;
			if (!F.Bounds.HasPoint(F.X[I], F.Y[I]))
			{
				// Funnel turns a mouse that would leave back toward the middle; a fleeing one is ALWAYS turned back (its
				// lifetime, not the edge, is the chase's clock: playtest 2026-09-15).
				const bool bFunnel = F.Bounces[I] >= 1.0;
				if (bFunnel || E.bFleesPlayer)
				{
					if (bFunnel)
					{
						F.Bounces[I] -= 1.0;
					}
					F.X[I] = CX + (F.X[I] - CX) * 0.92;
					F.Y[I] = CY + (F.Y[I] - CY) * 0.92;
					F.DirX[I] = CX - F.X[I];
					F.DirY[I] = CY - F.Y[I];
					Normalize(F.DirX[I], F.DirY[I]);
					return;
				}
				Despawn(F, D, P, Slot);
				return;
			}
			// The shield cycle: vulnerable for invuln_vulnerable_sec, then shielded for invuln_shield_sec.
			if (E.InvulnVulnerableSec > 0.0)
			{
				F.InvulnTimer[I] += Dt;
				const bool bWas = F.Shielded[I] != 0;
				if (bWas ? F.InvulnTimer[I] >= E.InvulnShieldSec : F.InvulnTimer[I] >= E.InvulnVulnerableSec)
				{
					F.Shielded[I] = bWas ? 0 : 1;
					F.InvulnTimer[I] = 0.0;
					Push(F, PcFieldEventKind::Shield, Slot, F.X[I], F.Y[I], 0.0, bWas ? 0 : FieldFlag::Shielded);
				}
			}
		}
	}

	void BeginField(PcField& F, const PcGameData& D, const PcProgress& P, const std::string& TierId, const PcRect& Bounds,
		uint64_t Seed)
	{
		F = PcField();
		F.TierIndex = RealmIndex(D, TierId);
		F.Bounds = Bounds;
		F.Seed = Seed;
		F.CatX = Bounds.CentreX();
		F.CatY = Bounds.CentreY();
		F.Stats = RebuildStats(D, P);
		if (F.TierIndex >= 0)
		{
			F.SpawnTimeLeft = SpawnCadence(D, SpawnRatePerSec(D, P, TierOf(F, D))).Interval;
		}
		// The PoopTimer starts at the cooldown (_ready); POOP_TRAIL reaches it from its first timeout.
		F.PoopWait = D.Combat.PoopCooldownSec;
		F.PoopTimeLeft = F.PoopWait;
		// The kittens wake on the cat (here: the middle of the field, where the game puts the cat on arrival).
		GrowKittenPool(F, D, KittenPoolBaseSize(D));
		SyncFieldKittens(F, D, P);
	}

	void SyncFieldKittens(PcField& F, const PcGameData& D, const PcProgress& P)
	{
		const int64_t Target = KittenCount(D, P) + static_cast<int64_t>(F.TempKittens.size());
		GrowKittenPool(F, D, Target);
		int64_t Awake = F.AwakeKittens();
		for (size_t K = 0; K < F.KActive.size() && Awake < Target; ++K)
		{
			if (!F.KActive[K])
			{
				WakeKitten(F, D, static_cast<int32_t>(K));
				++Awake;
			}
		}
		for (size_t K = 0; K < F.KActive.size() && Awake > Target; ++K)
		{
			if (F.KActive[K])
			{
				SleepKitten(F, static_cast<int32_t>(K));
				--Awake;
			}
		}
	}

	void HuntKitten(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, int32_t K, double Dt)
	{
		const size_t I = static_cast<size_t>(K);
		if (!F.KActive[I])
		{
			return;
		}
		// The target is sticky: kept until it dies or leaves (re-picking "nearest" every step made kittens flicker
		// between mice and never commit to a kill). The one exception is a badly over-subscribed mouse.
		if (!KittenTargetAlive(F, K))
		{
			// No commitment to honour: this kitten was not in the tally, so claiming is a pure increment.
			const int32_t Picked = PickKittenTarget(D, F.KX[I], F.KY[I], F.Live, F.Active, F.X, F.Y, F.Claims, KittensSeekClusters(D, P));
			SetKittenTarget(F, K, Picked);
			if (Picked >= 0)
			{
				++F.Claims[static_cast<size_t>(Picked)];
			}
		}
		else if (KittenOvercrowded(D, F.KX[I], F.KY[I], F.KTarget[I], F.Live, F.Active, F.X, F.Y, F.Claims))
		{
			const int32_t Replacement = PickKittenTarget(D, F.KX[I], F.KY[I], F.Live, F.Active, F.X, F.Y, F.Claims, KittensSeekClusters(D, P));
			if (Replacement >= 0 && Replacement != F.KTarget[I])
			{
				int64_t& Old = F.Claims[static_cast<size_t>(F.KTarget[I])];
				Old = std::max<int64_t>(0, Old - 1);
				++F.Claims[static_cast<size_t>(Replacement)];
				SetKittenTarget(F, K, Replacement);
			}
		}
		const int32_t T = F.KTarget[I];
		if (T < 0)
		{
			// Nothing in range: wander rather than freeze.
			RoamKitten(F, D, K, Dt);
			return;
		}
		const size_t V = static_cast<size_t>(T);
		if (ApproachKitten(F, D, K, F.X[V], F.Y[V], Dt))
		{
			const double VictimMaxHp = F.MaxHp[V];
			Hit(F, D, P, In, T, KittenDps(D.Combat, F.Stats) * F.KDamageMult[I] * Dt, F.Stats.ShieldPierceFraction);
			if (!F.Active[V])
			{
				// The contact kill, where the victim stood (F13, 2026-11-03: Godot read the parking spot).
				KittenBlast(F, D, P, In, F.X[V], F.Y[V],
					VictimMaxHp);
			}
			else if (F.KDragSpeed > 0.0)
			{
				// KITTEN_DRAG: the living victim is hauled toward the cat, MOVED rather than steered (a mouse being
				// hauled has no say in it; its own wander fighting the drag would make the card look broken).
				const double ToX = F.CatX - F.X[V];
				const double ToY = F.CatY - F.Y[V];
				const double Length = std::sqrt(ToX * ToX + ToY * ToY);
				const double Step = F.KDragSpeed * Dt;
				if (Length <= Step)
				{
					F.X[V] = F.CatX;
					F.Y[V] = F.CatY;
				}
				else
				{
					F.X[V] += ToX / Length * Step;
					F.Y[V] += ToY / Length * Step;
				}
			}
		}
	}

	bool CanSummonFieldBoss(const PcField& F, const PcGameData& D, const PcProgress& P)
	{
		const PcBoss* B = BossOf(F, D);
		return B && !F.bBossUp && CanAffordBait(P, *B);
	}

	bool SummonFieldBoss(PcField& F, const PcGameData& D, PcProgress& P)
	{
		const PcBoss* B = BossOf(F, D);
		if (!B || F.bBossUp)
		{
			return false;
		}
		// What BuyBait takes is measured, not recomputed: the Bait (none under Audit) and the Catnip, for F36's refund.
		const std::string Biome = P.CurrentBiomeId;
		const auto Spent = [&P, &Biome]()
		{
			const auto It = P.BaitSpentByBiome.find(Biome);
			return It == P.BaitSpentByBiome.end() ? int64_t(0) : It->second;
		};
		const int64_t BaitBefore = Spent();
		const double CatnipBefore = P.Catnip;
		if (!BuyBait(D, P, *B))
		{
			return false;
		}
		F.SummonPaid.BiomeId = Biome;
		F.SummonPaid.Bait = Spent() - BaitBefore;
		F.SummonPaid.Catnip = CatnipBefore - P.Catnip;
		// Boss.activate() in the middle of the field; _resummon_boss()'s fresh fight.
		F.bBossUp = true;
		F.BossHp = B->TotalHp;
		F.BossPhase = 1;
		F.bBossShielded = false;
		F.BossInvulnTimer = 0.0;
		F.BossX = F.Bounds.CentreX();
		F.BossY = F.Bounds.CentreY();
		F.BossAnchorX = F.BossX;
		F.BossAnchorY = F.BossY;
		F.BossDriftX = F.BossX;
		F.BossDriftY = F.BossY;
		F.bBossEngagedOnce = false;
		F.BossFightElapsed = 0.0;
		Push(F, PcFieldEventKind::BossSummoned, -1, F.BossX, F.BossY);
		return true;
	}

	void DamageFieldBoss(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Amount, double ShieldPierce)
	{
		BossTakeDamage(F, D, P, In, Amount, ShieldPierce);
	}

	void HealFieldBoss(PcField& F, const PcGameData& D, double Amount)
	{
		const PcBoss* B = BossOf(F, D);
		if (B)
		{
			HealBoss(F, *B, Amount);
		}
	}

	void TickFieldBoss(PcField& F, const PcGameData& D, double Dt)
	{
		TickBoss(F, D, Dt);
	}

	void DismissFieldBoss(PcField& F)
	{
		if (F.bBossUp)
		{
			WithdrawBoss(F);
		}
	}

	const PcSummonPayment* FightPayment(const PcField& F)
	{
		// Only while the fight is on: one that ended (won, timed out, dismissed) keeps its payment spent, as in Godot.
		return F.bBossUp ? &F.SummonPaid : nullptr;
	}

	double PounceCooldownFraction(const PcField& F)
	{
		if (F.PounceCooldownLeft <= 0.0 || F.PounceCooldownFull <= 0.0)
		{
			return 1.0;
		}
		return std::clamp(1.0 - F.PounceCooldownLeft / F.PounceCooldownFull, 0.0, 1.0);
	}

	void SetFieldBounds(PcField& F, const PcRect& Bounds)
	{
		F.Bounds = Bounds;
	}

	void RefreshFieldStats(PcField& F, const PcGameData& D, const PcProgress& P)
	{
		F.Stats = RebuildStats(D, P);
	}

	void StepField(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In, double Dt)
	{
		if (F.TierIndex < 0 || Dt <= 0.0)
		{
			return;
		}
		++F.StepCount;
		// The cat (Player._process). Rooted by the Cone of Shame it neither moves nor counts as still: the root is a cost,
		// not a free Carry. Otherwise its stillness is measured from where it actually is (a gamepad and a mouse both count).
		if (F.PounceLockLeft > 0.0)
		{
			F.PounceLockLeft -= Dt;
		}
		else
		{
			if (std::hypot(In.CatX - F.CatX, In.CatY - F.CatY) <= D.Code.PlayerStillEpsilonPx)
			{
				P.PlayerStillSeconds += Dt;
				P.LunchSeconds += Dt;
			}
			else
			{
				P.PlayerStillSeconds = 0.0; // Long Lunch's bank does NOT reset on moving: only a kill spends it
			}
			F.CatX = In.CatX;
			F.CatY = In.CatY;
		}
		// The Pounce button: input is handled before the frame's process (Godot's _unhandled_input).
		HandlePounceInput(F, D, P, In);

		// TierController._process, in its order.
		TickBossAdds(F, D, P, Dt);
		TickPounce(F, D, P, In, Dt);
		TickPrism(F, D, P, In, Dt);
		ResolvePawStrikes(F, D, P, In, Dt);
		// _resolve_aura_contact() begins with _update_fleeing_enemies(), aura or not: the golden runs BEFORE you reach it.
		for (const int32_t S : F.Live)
		{
			const size_t I = static_cast<size_t>(S);
			if (F.Active[I] && DataOf(F, D, S).bFleesPlayer)
			{
				F.HasFleeTarget[I] = 1;
			}
		}
		ResolveAuraContact(F, D, P, In, Dt);
		ResolveAuraPulse(F, D, P, In, Dt);
		ResolveAuraTrail(F, D, P, Dt);
		ResolveKittens(F, D, P, In, Dt);
		ResolvePoopZone(F, D, P, In, Dt);
		ResolveDamageZones(F, D, P, In, Dt);
		ResolveEdgeBurn(F, D, P, In, Dt);
		ResolveBossContact(F, D, P, In, Dt);
		TickAutoTravel(F, D, P, In, Dt);
		TickAutoBait(F, D, P, In, Dt);
		// Swarm Logic and Last Stand read the screen as it stands NOW (the dead of this step still counted, as Godot's
		// active_enemies did before the sweep) and apply from the next step's blows.
		F.ThresholdNow = ThresholdDamageMultiplier(D, P, F.LiveCount());
		ResolveAuraEcho(F, D, P, Dt);
		TickPounceInsurance(F, Dt);
		ResolveGrandFinale(F, D, P);
		TickRealtimeInterest(D, P, Dt);
		TickInfestation(D, P, Dt);
		TickSleightOfHand(F, D, P, In, Dt);
		SweepDead(F);

		// The children, in tree order: the spawn timer, then every mouse moves. A mouse spawned this step first moves
		// on the next one.
		const size_t Movers = F.Live.size();
		TickSpawner(F, D, P, Dt);
		TickPoopTimer(F, D, P, Dt);
		for (size_t K = 0; K < Movers; ++K)
		{
			const int32_t S = F.Live[K];
			if (F.Active[static_cast<size_t>(S)])
			{
				MoveEnemy(F, D, P, S, Dt);
			}
		}
		SweepDead(F);
		// The boss and its adds are children added after the pool: they move last.
		TickBoss(F, D, Dt);
		MoveBossAdds(F, D, Dt);
		// The travel after a first clear waits out the death sequence (a scene swap on the kill would read as a glitch).
		if (F.TravelDelay >= 0.0)
		{
			F.TravelDelay -= Dt;
			if (F.TravelDelay < 0.0)
			{
				Push(F, PcFieldEventKind::TravelRequested, static_cast<int32_t>(F.TierIndex) + 1, F.CatX, F.CatY);
			}
		}
	}

	int32_t SpawnEnemyAt(PcField& F, const PcGameData& D, const PcProgress& P, const std::string& EnemyId, double X, double Y,
		double DirX, double DirY)
	{
		const PcEnemy* Data = D.FindEnemy(EnemyId);
		if (!Data || F.LiveCount() >= D.Code.MaxLiveEnemies)
		{
			return -1;
		}
		Normalize(DirX, DirY);
		return Activate(F, D, P, *Data, ++F.Spawns, X, Y, DirX, DirY);
	}

	void KillAllEnemies(PcField& F, const PcGameData& D, PcProgress& P, const PcFieldInput& In)
	{
		const std::vector<int32_t> Snapshot = F.Live;
		for (const int32_t S : Snapshot)
		{
			const size_t I = static_cast<size_t>(S);
			if (F.Active[I])
			{
				Hit(F, D, P, In, S, F.Hp[I] + 1.0, 0.0);
			}
		}
		SweepDead(F);
	}

	std::vector<PcFieldEvent> DrainFieldEvents(PcField& F)
	{
		std::vector<PcFieldEvent> Out;
		Out.swap(F.Events);
		return Out;
	}
}
