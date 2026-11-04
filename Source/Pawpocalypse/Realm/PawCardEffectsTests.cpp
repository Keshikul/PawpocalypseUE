// Every card effect, run in a live realm (docs/09 §3, the port of Godot's ci/check_card_effects.gd; docs/11 phase 2 exit).
//
// PawCore's rule tests pin each effect's arithmetic against Godot. These prove the WIRING the game puts around it: a real game
// instance and its subsystems, the card taken through the game's own entry (UPawRealmSubsystem::ChooseCard, what the pick UI
// will call), the realm advanced through UPawRealmSubsystem::AdvanceFrame, and the effect's CONSUMER observed. Godot's gate
// exists because a handler can compile and then throw, or run where nobody can see it; "the effect ran" is not the assertion,
// what it did is.
//
// Pawpocalypse.Realm.CardEffects  one case per card effect: the same seed, the same progress, the same input, run twice, once
//                     without the card (the control) and once with it at its maximum level. The control must NOT show the
//                     consumer's mark and the subject must, so each assertion also proves its own test is not vacuous. A
//                     coverage check fails on any CardEffect without a case (or with a card, when it is listed as cardless).
//                     Taking a card rebuilds the cached stats (Grudge Ledger).
// Pawpocalypse.Realm.EveryCard   every card in the deck at its maximum level at once (Godot's "maximally hostile" build):
//                     the realm runs, kills, keeps its invariants (caps, finite positions), and every cost lands where it
//                     is read.
//
// Negative tests: tools/sabotage_realm.py (`cards_*`): each effect's consumer broken in PawCore, and the game's ChooseCard.

#include "Core/PawGameSubsystem.h"
#include "Core/PawLog.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "PcCardDraw.h"
#include "PcCards.h"
#include "PcData.h"
#include "PcEnums.h"
#include "PcField.h"
#include "PcFieldRules.h"
#include "PcProgress.h"
#include "PcStats.h"
#include "Realm/PawRealmSim.h"
#include "Realm/PawRealmSubsystem.h"

#include <cmath>
#include <set>

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr uint64 CaseSeed = 4242;
	constexpr double Fat = 1e12; // a mouse that survives the case, so the damage it took is measurable

	using FMetrics = TMap<FString, double>;

	// One game instance for the whole test: its own Game world and subsystems (the game data loads from the DataTables).
	struct FLiveGame
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;
		UPawGameSubsystem* Game = nullptr;
		UPawRealmSubsystem* Realm = nullptr;

		bool Open(FAutomationTestBase& T)
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			World = GI->GetWorld();
			Realm = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
			Game = GI->GetSubsystem<UPawGameSubsystem>();
			return T.TestNotNull(TEXT("the realm subsystem exists in a game world"), Realm)
				&& T.TestNotNull(TEXT("the game subsystem exists"), Game) && T.TestTrue(TEXT("the game data is ready"), Game->IsDataReady());
		}

		void Close()
		{
			if (GI)
			{
				GI->Shutdown();
			}
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}
	};

	// What a case sees while it runs.
	struct FCardWorld
	{
		UPawRealmSubsystem* Realm;
		const Pc::PcGameData& D;
		Pc::PcProgress& P;
		Pc::PcField& F;
		const Pc::PcCard* Card;   // null in the control run
		TArray<int32> FatSlots;   // the case's placed mice
		TArray<uint64> FatSerials;

		const std::vector<Pc::PcFieldEvent>& Events() const { return Realm->GetSim().FrameEvents; }
		int32 Count(Pc::PcFieldEventKind Kind) const
		{
			int32 N = 0;
			for (const Pc::PcFieldEvent& E : Events())
			{
				N += E.Kind == Kind ? 1 : 0;
			}
			return N;
		}
		const Pc::PcTier& Tier() const { return D.Tiers[static_cast<size_t>(F.TierIndex)]; }

		// A mouse of the given kind placed at (X, Y); bFat: 1e12 HP, frozen in place, never ageing out.
		int32 Place(const std::string& EnemyId, double X, double Y, double DirX, double DirY, bool bFat)
		{
			const int32 Slot = Pc::SpawnEnemyAt(F, D, P, EnemyId, X, Y, DirX, DirY);
			if (Slot >= 0 && bFat)
			{
				const size_t I = static_cast<size_t>(Slot);
				F.Hp[I] = Fat;
				F.MaxHp[I] = Fat;
				F.SpeedMult[I] = 0.0;
				F.LifetimeLeft[I] = 1e9;
				FatSlots.Add(Slot);
				FatSerials.Add(F.Serial[I]);
			}
			return Slot;
		}
		// The damage the placed fat mice have taken (all of them).
		double FatDamage() const
		{
			double Sum = 0.0;
			for (int32 K = 0; K < FatSlots.Num(); ++K)
			{
				const size_t I = static_cast<size_t>(FatSlots[K]);
				Sum += F.Active[I] && F.Serial[I] == FatSerials[K] ? Fat - F.Hp[I] : Fat;
			}
			return Sum;
		}
		// The cat on the nearest live mouse (the soak's hunter), or where it is.
		void Hunt()
		{
			double TX = F.CatX, TY = F.CatY, Best = 1e30;
			for (const int32 S : F.Live)
			{
				const size_t I = static_cast<size_t>(S);
				const double Dist = (F.X[I] - F.CatX) * (F.X[I] - F.CatX) + (F.Y[I] - F.CatY) * (F.Y[I] - F.CatY);
				if (Dist < Best)
				{
					Best = Dist;
					TX = F.X[I];
					TY = F.Y[I];
				}
			}
			Realm->SetCat(TX, TY);
		}
		// One short of the next Catches level: the next common kill levels (and the realm deals a hand).
		void PrimeLevel()
		{
			P.CatchesPoints = Pc::TotalCatchesForLevel(D, P.CatchesLevel + 1) - 0.5;
		}
		// A plain mouse at the cat with almost no HP: the paw's first swipe kills it.
		void PlaceVictim()
		{
			const int32 Slot = Place(Tier().SpawnPool.front(), F.CatX, F.CatY, 1.0, 0.0, false);
			if (Slot >= 0)
			{
				F.Hp[static_cast<size_t>(Slot)] = 1e-9;
				F.SpeedMult[static_cast<size_t>(Slot)] = 0.0;
			}
		}
	};

	// The progress both runs of a case start from: what the effect needs to have something to act on.
	struct FPrereq
	{
		bool bAura = false, bPounce = false, bOverkill = false, bUnicorn = false;
		int64 Kittens = 0;
		int64 PawDamage = 0;
		double Infestation = 0.0;

		void Apply(Pc::PcProgress& P) const
		{
			P.BranchLevels["unlock_aura"] = bAura ? 1 : 0;
			P.BranchLevels["unlock_pounce"] = bPounce ? 1 : 0;
			P.BranchLevels["unlock_overkill"] = bOverkill ? 1 : 0;
			// The tree's chain: Unicorn -> Rainbow Poop (the puddles) -> Rainbow Gut (what the poop cards ask for).
			P.BranchLevels["unlock_unicorn"] = bUnicorn ? 1 : 0;
			P.BranchLevels["unlock_poop"] = bUnicorn ? 1 : 0;
			P.BranchLevels["unicorn"] = bUnicorn ? 1 : 0;
			P.UpgradeLevels["kitten_count"] = Kittens;
			P.UpgradeLevels["paw_damage"] = PawDamage;
			P.InfestationBonus = Infestation;
		}
	};

	struct FCardCase
	{
		Pc::CardEffect Effect = Pc::CardEffect::AURA_TRAIL;
		FPrereq Prereq;
		int32 Frames = 120;
		TFunction<void(FCardWorld&)> Place;                      // after the realm starts (and the card is taken)
		TFunction<void(FCardWorld&, FMetrics&)> Before;          // after Place, before the first frame
		TFunction<void(FCardWorld&, int32)> Drive;               // before each frame: the cat, Pounce
		TFunction<void(FCardWorld&, int32, FMetrics&)> Observe;  // after each frame
		// The verdict on the two runs; CardValue = the card's value at its maximum level.
		TFunction<void(FAutomationTestBase&, const FString&, const FMetrics&, const FMetrics&, double)> Judge;
	};

	double Get(const FMetrics& M, const TCHAR* Key)
	{
		const double* V = M.Find(Key);
		return V ? *V : 0.0;
	}
	void Max(FMetrics& M, const TCHAR* Key, double V)
	{
		M.FindOrAdd(Key) = FMath::Max(Get(M, Key), V);
	}
	void Add(FMetrics& M, const TCHAR* Key, double V)
	{
		M.FindOrAdd(Key) += V;
	}

	// The card a case takes: the first of its effect with no cost (so only the effect differs), else the first.
	const Pc::PcCard* CardFor(const Pc::PcGameData& D, Pc::CardEffect Effect)
	{
		const Pc::PcCard* First = nullptr;
		for (const Pc::PcCard& C : D.Cards)
		{
			if (C.Effect != Effect)
			{
				continue;
			}
			if (C.Cost == Pc::CardCost::NONE)
			{
				return &C;
			}
			First = First ? First : &C;
		}
		return First;
	}

	// Takes the card to its maximum level the way the game will: dealt into the hand, chosen through the realm.
	bool TakeToMax(FAutomationTestBase& T, UPawRealmSubsystem* Realm, const Pc::PcGameData& D, Pc::PcProgress& P,
		const Pc::PcCard& Card, bool bCheckEligible)
	{
		const FString Id(UTF8_TO_TCHAR(Card.Id.c_str()));
		if (bCheckEligible && !T.TestTrue(FString::Printf(TEXT("%s is offerable with the case's progress"), *Id), Pc::IsCardEligible(D, P, Card)))
		{
			return false;
		}
		for (int64 Guard = 0; Pc::CardLevel(P, Card.Id) < Pc::CardMaxLevel(Card) && Guard < 16; ++Guard)
		{
			P.Hand = { Card.Id };
			if (!T.TestTrue(FString::Printf(TEXT("%s is taken from the hand"), *Id), Realm->ChooseCard(Id)))
			{
				return false;
			}
		}
		return T.TestEqual(FString::Printf(TEXT("%s reached its maximum level"), *Id), Pc::CardLevel(P, Card.Id), Pc::CardMaxLevel(Card));
	}

	FMetrics RunCase(FAutomationTestBase& T, FLiveGame& G, const FCardCase& C, const Pc::PcCard* Card)
	{
		FMetrics M;
		G.Game->NewGame();
		const Pc::PcGameData& D = G.Game->Data();
		Pc::PcProgress& P = G.Game->Progress();
		C.Prereq.Apply(P);
		if (!T.TestTrue(TEXT("the realm starts"), G.Realm->StartRealm(CaseSeed)))
		{
			return M;
		}
		FCardWorld W{ G.Realm, D, P, G.Realm->GetSim().Field, Card, {}, {} };
		if (Card && !TakeToMax(T, G.Realm, D, P, *Card, true))
		{
			return M;
		}
		if (C.Place)
		{
			C.Place(W);
		}
		if (C.Before)
		{
			C.Before(W, M);
		}
		for (int32 Frame = 0; Frame < C.Frames; ++Frame)
		{
			if (C.Drive)
			{
				C.Drive(W, Frame);
			}
			G.Realm->AdvanceFrame(FPawRealmSim::Step);
			C.Observe(W, Frame, M);
		}
		return M;
	}

	// A press on one frame, released on the next (the press edge is what fires).
	void PressOn(FCardWorld& W, int32 Frame, int32 At)
	{
		W.Realm->SetPounceDown(Frame == At);
	}

	// Subject shows the mark, the control does not.
	void JudgeOnlyWithCard(FAutomationTestBase& T, const FString& Name, const FMetrics& C, const FMetrics& S, const TCHAR* Key)
	{
		T.TestEqual(FString::Printf(TEXT("%s: without the card, no %s"), *Name, Key), Get(C, Key), 0.0);
		T.TestTrue(FString::Printf(TEXT("%s: with the card, %s (%g)"), *Name, Key, Get(S, Key)), Get(S, Key) > 0.0);
	}
	void JudgeMore(FAutomationTestBase& T, const FString& Name, const FMetrics& C, const FMetrics& S, const TCHAR* Key)
	{
		T.TestTrue(FString::Printf(TEXT("%s: more %s with the card (%g vs %g)"), *Name, Key, Get(S, Key), Get(C, Key)),
			Get(S, Key) > Get(C, Key));
	}

	TArray<FCardCase> MakeCases()
	{
		using K = Pc::PcFieldEventKind;
		TArray<FCardCase> Cases;
		auto Hunt = [](FCardWorld& W, int32) { W.Hunt(); };

		// --- the aura ------------------------------------------------------------------------------------------------
		{
			FCardCase C;
			C.Effect = Pc::CardEffect::AURA_TRAIL;
			C.Prereq.bAura = true;
			C.Drive = [](FCardWorld& W, int32 Frame)
			{
				W.Realm->SetCat(W.F.Bounds.CentreX() + 300.0 * FMath::Cos(Frame * 0.05), W.F.Bounds.CentreY() + 300.0 * FMath::Sin(Frame * 0.05));
			};
			C.Observe = [](FCardWorld& W, int32, FMetrics& M)
			{
				int32 N = 0;
				for (const Pc::PcDamageZone& Z : W.F.Zones)
				{
					N += Z.Kind == Pc::PcZoneKind::Trail ? 1 : 0;
				}
				Max(M, TEXT("trail zones"), N);
			};
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double) { JudgeOnlyWithCard(T, N, Ctl, S, TEXT("trail zones")); };
			Cases.Add(C);
		}
		{
			FCardCase C;
			C.Effect = Pc::CardEffect::AURA_PULSE;
			C.Prereq.bAura = true;
			C.Prereq.Infestation = 50.0;
			C.Frames = 420; // past the 6 s interval
			C.Drive = Hunt;
			C.Observe = [](FCardWorld& W, int32, FMetrics& M) { Add(M, TEXT("pulses"), W.Count(K::AuraPulse)); };
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double) { JudgeOnlyWithCard(T, N, Ctl, S, TEXT("pulses")); };
			Cases.Add(C);
		}
		{
			FCardCase C;
			C.Effect = Pc::CardEffect::AURA_SLOW;
			C.Prereq.bAura = true;
			C.Prereq.Infestation = 200.0;
			C.Frames = 240;
			C.Drive = Hunt;
			C.Observe = [](FCardWorld& W, int32, FMetrics& M)
			{
				bool bSlowed = false;
				for (const int32 S : W.F.Live)
				{
					bSlowed = bSlowed || W.F.SlowMult[static_cast<size_t>(S)] < 1.0;
				}
				Add(M, TEXT("frames with a slowed mouse"), bSlowed ? 1.0 : 0.0);
			};
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double) { JudgeOnlyWithCard(T, N, Ctl, S, TEXT("frames with a slowed mouse")); };
			Cases.Add(C);
		}
		{
			// A fat mouse frozen in the rim's band, the cat in the far corner: nothing else can reach it.
			FCardCase C;
			C.Effect = Pc::CardEffect::EDGE_BURN;
			C.Prereq.bAura = true;
			C.Frames = 60;
			C.Place = [](FCardWorld& W)
			{
				W.Realm->SetCat(W.F.Bounds.X + W.F.Bounds.W - 100.0, W.F.Bounds.Y + W.F.Bounds.H - 100.0);
				W.Place(W.Tier().SpawnPool.front(), W.F.Bounds.X + 45.0, W.F.Bounds.CentreY(), 0.0, 1.0, true);
			};
			C.Observe = [](FCardWorld& W, int32, FMetrics& M) { M.FindOrAdd(TEXT("damage at the rim")) = W.FatDamage(); };
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double) { JudgeOnlyWithCard(T, N, Ctl, S, TEXT("damage at the rim")); };
			Cases.Add(C);
		}

		// --- the kittens ---------------------------------------------------------------------------------------------
		{
			FCardCase C;
			C.Effect = Pc::CardEffect::KITTEN_KILL_EXPLODES;
			C.Prereq.Kittens = 6;
			C.Prereq.Infestation = 200.0;
			C.Frames = 600;
			C.Place = [](FCardWorld& W) { W.Realm->SetCat(W.F.Bounds.X + 60.0, W.F.Bounds.Y + 60.0); }; // the kittens kill, not the paw
			C.Observe = [](FCardWorld& W, int32, FMetrics& M)
			{
				for (const Pc::PcFieldEvent& E : W.Events())
				{
					if (E.Kind == K::KittenBlast)
					{
						Add(M, TEXT("kitten blasts"), 1.0);
						Add(M, TEXT("blasts on the field"), W.F.Bounds.HasPoint(E.X, E.Y) ? 1.0 : 0.0);
					}
				}
			};
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double)
			{
				JudgeOnlyWithCard(T, N, Ctl, S, TEXT("kitten blasts"));
				// F13 (fixed 2026-11-03, Godot first): the blast goes off where the victim died, so on the field. Asserting the
				// POSITION, not the call (Godot's lesson); it used to go off at the parking spot and hurt nothing.
				T.TestTrue(N + TEXT(": every blast on the field (F13)"), Get(S, TEXT("blasts on the field")) == Get(S, TEXT("kitten blasts"))
					&& Get(S, TEXT("kitten blasts")) > 0.0);
			};
			Cases.Add(C);
		}
		{
			FCardCase C;
			C.Effect = Pc::CardEffect::GOLDEN_KITTEN;
			C.Prereq.Kittens = 6;
			C.Frames = 30;
			C.Observe = [](FCardWorld& W, int32, FMetrics& M)
			{
				int32 N = 0;
				for (size_t I = 0; I < W.F.KActive.size(); ++I)
				{
					N += W.F.KActive[I] && W.F.KGolden[I] && W.F.KDamageMult[I] > 1.0 ? 1 : 0;
				}
				M.FindOrAdd(TEXT("gilded kittens")) = N;
			};
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double)
			{
				T.TestEqual(N + TEXT(": no gilded kitten without the card"), Get(Ctl, TEXT("gilded kittens")), 0.0);
				T.TestEqual(N + TEXT(": exactly one with it"), Get(S, TEXT("gilded kittens")), 1.0);
			};
			Cases.Add(C);
		}
		{
			// Its cost (kittens) lands the moment the card is taken (F14, 2026-11-03: it used to wait for the next re-count):
			// nothing here re-counts but the take itself.
			FCardCase C;
			C.Effect = Pc::CardEffect::LONE_HUNTER;
			C.Prereq.Kittens = 8;
			C.Frames = 30;
			C.Observe = [](FCardWorld& W, int32, FMetrics& M)
			{
				int32 Lone = 0;
				for (size_t I = 0; I < W.F.KActive.size(); ++I)
				{
					Lone += W.F.KActive[I] && W.F.KLone[I] && W.F.KDamageMult[I] > 1.0 ? 1 : 0;
				}
				M.FindOrAdd(TEXT("lone hunters")) = Lone;
				M.FindOrAdd(TEXT("awake kittens")) = static_cast<double>(W.F.AwakeKittens());
				M.FindOrAdd(TEXT("kitten cost")) = Pc::CardCostTotal(W.D, W.P, Pc::CardCost::KITTEN_COUNT);
			};
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double)
			{
				T.TestEqual(N + TEXT(": no lone hunter without the card"), Get(Ctl, TEXT("lone hunters")), 0.0);
				T.TestEqual(N + TEXT(": exactly one with it"), Get(S, TEXT("lone hunters")), 1.0);
				T.TestTrue(N + TEXT(": the card costs kittens"), Get(S, TEXT("kitten cost")) > 0.0);
				T.TestEqual(N + TEXT(": the others wander off"), Get(S, TEXT("awake kittens")),
					Get(Ctl, TEXT("awake kittens")) - Get(S, TEXT("kitten cost")));
			};
			Cases.Add(C);
		}

		// --- Pounce --------------------------------------------------------------------------------------------------
		{
			// One fat mouse under the cat, two beyond the blast's rim: only the chain reaches them.
			FCardCase C;
			C.Effect = Pc::CardEffect::POUNCE_CHAIN;
			C.Prereq.bPounce = true;
			C.Frames = 6;
			C.Place = [](FCardWorld& W)
			{
				const double CX = W.F.Bounds.CentreX(), CY = W.F.Bounds.CentreY();
				W.Realm->SetCat(CX, CY);
				W.F.CatX = CX;
				W.F.CatY = CY;
				W.Place(W.Tier().SpawnPool.front(), CX, CY, 1.0, 0.0, true); // FatSlots[0]: under the cat
				W.Place(W.Tier().SpawnPool.front(), CX + 400.0, CY, 1.0, 0.0, true);
				W.Place(W.Tier().SpawnPool.front(), CX - 400.0, CY, 1.0, 0.0, true);
			};
			C.Drive = [](FCardWorld& W, int32 Frame) { PressOn(W, Frame, 2); };
			C.Observe = [](FCardWorld& W, int32, FMetrics& M)
			{
				double Beyond = 0.0;
				for (int32 Kx = 1; Kx < W.FatSlots.Num(); ++Kx)
				{
					const size_t I = static_cast<size_t>(W.FatSlots[Kx]);
					Beyond += Fat - W.F.Hp[I];
				}
				M.FindOrAdd(TEXT("damage beyond the rim")) = Beyond;
				Add(M, TEXT("pounces"), W.Count(K::Pounce));
			};
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double)
			{
				T.TestEqual(N + TEXT(": both runs pounced once"), Get(Ctl, TEXT("pounces")) + Get(S, TEXT("pounces")), 2.0);
				JudgeOnlyWithCard(T, N, Ctl, S, TEXT("damage beyond the rim"));
			};
			Cases.Add(C);
		}
		{
			FCardCase C;
			C.Effect = Pc::CardEffect::POUNCE_CRATER;
			C.Prereq.bPounce = true;
			C.Frames = 10;
			C.Drive = [](FCardWorld& W, int32 Frame) { PressOn(W, Frame, 2); };
			C.Observe = [](FCardWorld& W, int32, FMetrics& M)
			{
				int32 N = 0;
				for (const Pc::PcDamageZone& Z : W.F.Zones)
				{
					N += Z.Kind == Pc::PcZoneKind::Crater ? 1 : 0;
				}
				Max(M, TEXT("craters"), N);
				Add(M, TEXT("pounces"), W.Count(K::Pounce));
			};
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double)
			{
				T.TestEqual(N + TEXT(": both runs pounced once"), Get(Ctl, TEXT("pounces")) + Get(S, TEXT("pounces")), 2.0);
				JudgeOnlyWithCard(T, N, Ctl, S, TEXT("craters"));
			};
			Cases.Add(C);
		}
		{
			// A frame with a kill (and no new Pounce) takes more off the cooldown than its steps' time.
			FCardCase C;
			C.Effect = Pc::CardEffect::POUNCE_KILL_REFUND;
			C.Prereq.bPounce = true;
			C.Prereq.bAura = true;
			C.Prereq.PawDamage = 30;
			C.Prereq.Infestation = 200.0;
			C.Frames = 280; // inside the 5 s cooldown
			C.Drive = [](FCardWorld& W, int32 Frame)
			{
				W.Hunt();
				PressOn(W, Frame, 3);
			};
			C.Observe = [](FCardWorld& W, int32, FMetrics& M)
			{
				const double Before = Get(M, TEXT("_left"));
				const double Now = W.F.PounceCooldownLeft;
				M.FindOrAdd(TEXT("_left")) = Now;
				const double Ticked = W.Realm->GetSim().StepsLastFrame * FPawRealmSim::Step;
				if (W.Count(K::Killed) > 0 && W.Count(K::Pounce) == 0 && Before > 0.0 && Before - Now > Ticked * 1.5 + 1e-12)
				{
					Add(M, TEXT("refunded frames"), 1.0);
				}
				Add(M, TEXT("pounces"), W.Count(K::Pounce));
			};
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double)
			{
				T.TestTrue(N + TEXT(": both runs pounced"), Get(Ctl, TEXT("pounces")) > 0.0 && Get(S, TEXT("pounces")) > 0.0);
				JudgeOnlyWithCard(T, N, Ctl, S, TEXT("refunded frames"));
			};
			Cases.Add(C);
		}
		{
			// Six fat mice in a ring outside the blast, the kittens on them; the Pounce frame hurts them more with the card.
			// Both runs are the same, step for step, until that frame.
			FCardCase C;
			C.Effect = Pc::CardEffect::KITTEN_MIMIC_POUNCE;
			C.Prereq.bPounce = true;
			C.Prereq.Kittens = 6;
			C.Frames = 122;
			C.Place = [](FCardWorld& W)
			{
				const double CX = W.F.Bounds.CentreX(), CY = W.F.Bounds.CentreY();
				W.Realm->SetCat(CX, CY);
				for (int32 I = 0; I < 6; ++I)
				{
					const double A = I * PI / 3.0;
					W.Place(W.Tier().SpawnPool.front(), CX + 400.0 * FMath::Cos(A), CY + 400.0 * FMath::Sin(A), 1.0, 0.0, true);
				}
			};
			C.Drive = [](FCardWorld& W, int32 Frame) { PressOn(W, Frame, 120); };
			C.Observe = [](FCardWorld& W, int32 Frame, FMetrics& M)
			{
				if (Frame == 119)
				{
					M.FindOrAdd(TEXT("_before")) = W.FatDamage();
				}
				if (Frame == 120)
				{
					M.FindOrAdd(TEXT("damage on the Pounce frame")) = W.FatDamage() - Get(M, TEXT("_before"));
					M.FindOrAdd(TEXT("damage before it")) = Get(M, TEXT("_before"));
					Add(M, TEXT("pounces"), W.Count(K::Pounce));
				}
			};
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double)
			{
				T.TestEqual(N + TEXT(": both runs pounced on the frame"), Get(Ctl, TEXT("pounces")) + Get(S, TEXT("pounces")), 2.0);
				T.TestEqual(N + TEXT(": the runs are identical before the Pounce"), Get(S, TEXT("damage before it")), Get(Ctl, TEXT("damage before it")));
				T.TestTrue(N + TEXT(": the kittens were on the ring"), Get(Ctl, TEXT("damage before it")) > 0.0);
				JudgeMore(T, N, Ctl, S, TEXT("damage on the Pounce frame"));
			};
			Cases.Add(C);
		}

		// --- the swarm -----------------------------------------------------------------------------------------------
		auto Turnover = [](FCardCase& C)
		{
			C.Prereq.bAura = true;
			C.Prereq.PawDamage = 30;
			C.Prereq.Kittens = 6;
			C.Prereq.Infestation = 1000.0;
			C.Frames = 1800;
			C.Drive = [](FCardWorld& W, int32) { W.Hunt(); };
		};
		{
			FCardCase C;
			C.Effect = Pc::CardEffect::GOLDEN_FREQUENCY;
			Turnover(C);
			C.Observe = [](FCardWorld& W, int32, FMetrics& M)
			{
				for (const Pc::PcFieldEvent& E : W.Events())
				{
					if (E.Kind == K::Spawned)
					{
						Add(M, TEXT("spawns"), 1.0);
						const Pc::PcEnemy& En = W.D.Enemies[static_cast<size_t>(W.F.Kind[static_cast<size_t>(E.Slot)])];
						Add(M, TEXT("golden spawns"), En.Id == W.Tier().GoldenEnemy ? 1.0 : 0.0);
					}
				}
			};
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double)
			{
				T.TestTrue(FString::Printf(TEXT("%s: the swarm turned over (%g spawns)"), *N, Get(Ctl, TEXT("spawns"))), Get(Ctl, TEXT("spawns")) > 500.0);
				JudgeMore(T, N, Ctl, S, TEXT("golden spawns"));
			};
			Cases.Add(C);
		}
		{
			FCardCase C;
			C.Effect = Pc::CardEffect::SIGNATURE_FREQUENCY;
			Turnover(C);
			C.Observe = [](FCardWorld& W, int32, FMetrics& M)
			{
				for (const Pc::PcFieldEvent& E : W.Events())
				{
					if (E.Kind == K::Spawned)
					{
						Add(M, TEXT("spawns"), 1.0);
						const Pc::PcEnemy& En = W.D.Enemies[static_cast<size_t>(W.F.Kind[static_cast<size_t>(E.Slot)])];
						Add(M, TEXT("signature spawns"), En.bIsSignature ? 1.0 : 0.0);
					}
				}
			};
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double)
			{
				T.TestTrue(FString::Printf(TEXT("%s: the swarm turned over (%g spawns)"), *N, Get(Ctl, TEXT("spawns"))), Get(Ctl, TEXT("spawns")) > 500.0);
				JudgeMore(T, N, Ctl, S, TEXT("signature spawns"));
			};
			Cases.Add(C);
		}
		{
			FCardCase C;
			C.Effect = Pc::CardEffect::CATNIP_BOMB_EVERY;
			Turnover(C);
			C.Frames = 900;
			C.Observe = [](FCardWorld& W, int32, FMetrics& M) { Add(M, TEXT("catnip bombs"), W.Count(K::CatnipBomb)); };
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double) { JudgeOnlyWithCard(T, N, Ctl, S, TEXT("catnip bombs")); };
			Cases.Add(C);
		}

		// --- Rainbow Poop --------------------------------------------------------------------------------------------
		{
			FCardCase C;
			C.Effect = Pc::CardEffect::POOP_DURATION;
			C.Prereq.bUnicorn = true;
			C.Frames = 600;
			C.Observe = [](FCardWorld& W, int32, FMetrics& M)
			{
				for (const Pc::PcFieldEvent& E : W.Events())
				{
					if (E.Kind == K::PoopDropped && !M.Contains(TEXT("first puddle lasts")))
					{
						M.Add(TEXT("first puddle lasts"), E.ToX);
					}
				}
			};
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double Value)
			{
				T.TestTrue(N + TEXT(": a puddle was dropped"), Ctl.Contains(TEXT("first puddle lasts")) && S.Contains(TEXT("first puddle lasts")));
				T.TestTrue(FString::Printf(TEXT("%s: the puddle lasts %g s longer (%g vs %g)"), *N, Value, Get(S, TEXT("first puddle lasts")),
					Get(Ctl, TEXT("first puddle lasts"))), FMath::IsNearlyEqual(Get(S, TEXT("first puddle lasts")) - Get(Ctl, TEXT("first puddle lasts")), Value, 1e-9));
			};
			Cases.Add(C);
		}
		{
			FCardCase C;
			C.Effect = Pc::CardEffect::POOP_TRAIL;
			C.Prereq.bUnicorn = true;
			// 25 s: the PoopTimer restarts on its old wait before it reads the new interval, so the card's pace starts on
			// the second puddle (8 s, 16 s, then every interval).
			C.Frames = 1500;
			C.Observe = [](FCardWorld& W, int32, FMetrics& M) { Add(M, TEXT("puddles"), W.Count(K::PoopDropped)); };
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double)
			{
				T.TestTrue(N + TEXT(": the control drops its own puddles"), Get(Ctl, TEXT("puddles")) > 0.0);
				JudgeMore(T, N, Ctl, S, TEXT("puddles"));
			};
			Cases.Add(C);
		}

		// --- fortune -------------------------------------------------------------------------------------------------
		{
			// One signature at the cat, one swipe from death: its kill pays the card's multiple of the control's. Currency
			// Find makes the payout large, so the whole-Catnip rounding (half a Catnip) cannot hide or fake the multiple.
			FCardCase C;
			C.Effect = Pc::CardEffect::SIGNATURE_BOUNTY;
			C.Frames = 90;
			C.Place = [](FCardWorld& W)
			{
				W.P.UpgradeLevels["currency_find"] = 25;
				for (const std::string& Id : W.Tier().SpawnPool)
				{
					const Pc::PcEnemy* E = W.D.FindEnemy(Id);
					if (E && E->bIsSignature)
					{
						const int32 Slot = W.Place(Id, W.F.CatX, W.F.CatY, 1.0, 0.0, false);
						if (Slot >= 0)
						{
							W.F.Hp[static_cast<size_t>(Slot)] = 1e-9;
							W.F.SpeedMult[static_cast<size_t>(Slot)] = 0.0;
							W.FatSlots.Add(Slot);
							W.FatSerials.Add(W.F.Serial[static_cast<size_t>(Slot)]);
						}
						break;
					}
				}
			};
			C.Observe = [](FCardWorld& W, int32, FMetrics& M)
			{
				for (const Pc::PcFieldEvent& E : W.Events())
				{
					if (E.Kind == K::Killed && W.FatSlots.Num() > 0 && E.Slot == W.FatSlots[0] && !M.Contains(TEXT("signature pays")))
					{
						M.Add(TEXT("signature pays"), E.Value);
					}
				}
			};
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double Value)
			{
				const double Base = Get(Ctl, TEXT("signature pays"));
				T.TestTrue(FString::Printf(TEXT("%s: the signature died and paid enough to measure (%.0f)"), *N, Base), Base >= 50.0);
				T.TestTrue(FString::Printf(TEXT("%s: it pays %gx with the card (%.0f vs %.0f)"), *N, Value, Get(S, TEXT("signature pays")), Base),
					FMath::Abs(Get(S, TEXT("signature pays")) - Base * Value) <= Value);
			};
			Cases.Add(C);
		}
		{
			// The level a kill reaches pays the card's share of the bank (outside Currency Find), before the hand.
			FCardCase C;
			C.Effect = Pc::CardEffect::CATNIP_INTEREST;
			C.Frames = 90;
			C.Place = [](FCardWorld& W)
			{
				W.P.Catnip = 1e6;
				W.PrimeLevel();
				W.PlaceVictim();
			};
			C.Observe = [](FCardWorld& W, int32, FMetrics& M)
			{
				const double Before = M.Contains(TEXT("_bank")) ? Get(M, TEXT("_bank")) : 1e6;
				M.FindOrAdd(TEXT("_bank")) = W.P.Catnip;
				if (W.Count(K::Levelled) == 0 || M.Contains(TEXT("interest")))
				{
					return;
				}
				double Kills = 0.0;
				for (const Pc::PcFieldEvent& E : W.Events())
				{
					Kills += E.Kind == K::Killed ? E.Value : 0.0;
				}
				M.Add(TEXT("interest"), W.P.Catnip - Before - Kills);
				M.Add(TEXT("bank before the interest"), Before + Kills);
				M.Add(TEXT("kills on the frame"), W.Count(K::Killed));
			};
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double Value)
			{
				T.TestTrue(N + TEXT(": both runs levelled"), Ctl.Contains(TEXT("interest")) && S.Contains(TEXT("interest")));
				T.TestTrue(FString::Printf(TEXT("%s: no interest without the card (%g)"), *N, Get(Ctl, TEXT("interest"))), FMath::Abs(Get(Ctl, TEXT("interest"))) < 1e-6);
				const double Want = std::floor(Value * Get(S, TEXT("bank before the interest"))); // credited floored
				T.TestTrue(FString::Printf(TEXT("%s: %g of the bank paid (%.2f, want %.2f)"), *N, Value, Get(S, TEXT("interest")), Want),
					Want > 0.0 && Get(S, TEXT("interest")) == Want);
			};
			Cases.Add(C);
		}
		{
			// Mice walking off the near edge, the cat far away: a despawn pays only with the card.
			FCardCase C;
			C.Effect = Pc::CardEffect::SCAVENGE;
			C.Frames = 180;
			C.Place = [](FCardWorld& W)
			{
				W.Realm->SetCat(W.F.Bounds.X + W.F.Bounds.W - 80.0, W.F.Bounds.Y + W.F.Bounds.H - 80.0);
				for (int32 I = 0; I < 5; ++I)
				{
					W.Place(W.Tier().SpawnPool.front(), W.F.Bounds.X + 3.0, W.F.Bounds.Y + 200.0 + 120.0 * I, -1.0, 0.0, false);
				}
			};
			C.Observe = [](FCardWorld& W, int32, FMetrics& M)
			{
				const double Before = M.Contains(TEXT("_bank")) ? Get(M, TEXT("_bank")) : 0.0;
				M.FindOrAdd(TEXT("_bank")) = W.P.Catnip;
				if (W.Count(K::Despawned) > 0)
				{
					Add(M, TEXT("despawns"), W.Count(K::Despawned));
					if (W.Count(K::Killed) == 0)
					{
						Add(M, TEXT("catnip from strays"), W.P.Catnip - Before);
					}
				}
			};
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double)
			{
				T.TestTrue(N + TEXT(": mice walked off in the control"), Get(Ctl, TEXT("despawns")) > 0.0);
				JudgeOnlyWithCard(T, N, Ctl, S, TEXT("catnip from strays"));
			};
			Cases.Add(C);
		}

		// --- the deck: applied when the card is taken, consumed by the hand the realm deals on its next level ----------
		auto LevelSoon = [](FCardWorld& W)
		{
			W.PrimeLevel();
			W.PlaceVictim();
		};
		{
			FCardCase C;
			C.Effect = Pc::CardEffect::EXTRA_CHOICE;
			C.Prereq = FPrereq{ true, true, true, true, 4, 0, 0.0 }; // a deep deck: the bigger hand can fill
			C.Frames = 90;
			C.Place = LevelSoon;
			C.Observe = [](FCardWorld& W, int32, FMetrics& M) { Max(M, TEXT("hand"), static_cast<double>(W.P.Hand.size())); };
			C.Judge = [](FAutomationTestBase& T, const FString& N, const FMetrics& Ctl, const FMetrics& S, double Value)
			{
				T.TestTrue(N + TEXT(": the realm dealt a hand"), Get(Ctl, TEXT("hand")) > 0.0);
				T.TestEqual(FString::Printf(TEXT("%s: %g more cards in the hand"), *N, Value), Get(S, TEXT("hand")), Get(Ctl, TEXT("hand")) + Value);
			};
			Cases.Add(C);
		}
		// Reroll, Banish, Fixation: what the takes granted is held before the first frame; the hand the realm deals on its
		// level spends one (a reroll and a banish by the pick UI's calls, Fixation by the deal itself). The verdict is in
		// RunTest: it needs the sum over the takes.
		auto Held = [](Pc::CardEffect E, const Pc::PcProgress& P)
		{
			return static_cast<double>(E == Pc::CardEffect::BANISH ? P.Banishes : E == Pc::CardEffect::REROLL ? P.Rerolls : P.FixationHands);
		};
		for (const Pc::CardEffect E : { Pc::CardEffect::REROLL, Pc::CardEffect::BANISH, Pc::CardEffect::FIXATION_HANDS })
		{
			FCardCase C;
			C.Effect = E;
			C.Frames = 90;
			C.Place = LevelSoon;
			C.Before = [E, Held](FCardWorld& W, FMetrics& M) { M.Add(TEXT("held"), Held(E, W.P)); };
			C.Observe = [E, Held](FCardWorld& W, int32, FMetrics& M)
			{
				if (M.Contains(TEXT("_dealt")) || W.P.Hand.empty())
				{
					return;
				}
				M.Add(TEXT("_dealt"), 1.0);
				bool bOk = true;
				if (E != Pc::CardEffect::FIXATION_HANDS)
				{
					Pc::PcRngCardRolls Rolls(W.F.Seed, ++W.F.Deals);
					bOk = E == Pc::CardEffect::BANISH ? Pc::BanishFromHand(W.D, W.P, W.P.Hand.front(), Rolls) : Pc::RerollHand(W.D, W.P, Rolls);
				}
				M.Add(TEXT("spent"), bOk ? Get(M, TEXT("held")) - Held(E, W.P) : 0.0);
			};
			Cases.Add(C);
		}
		return Cases;
	}

	// The sum a deck card adds over every take to its maximum (choose() adds value_at(new level) each time; Dilated SETS).
	double GrantedByTakes(const Pc::PcCard& Card)
	{
		double Sum = 0.0;
		for (int64 L = 1; L <= Pc::CardMaxLevel(Card); ++L)
		{
			Sum += static_cast<int64>(Pc::CardValueAt(Card, L));
		}
		return Sum;
	}

	// Effects no card in the deck carries (doc 01 §3 "the three card effects with no card", F15; open with the designer).
	const Pc::CardEffect Cardless[] = { Pc::CardEffect::KITTEN_DRAG, Pc::CardEffect::KITTEN_FROM_SIGNATURE, Pc::CardEffect::OVERKILL_TARGETS };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawCardEffectsTest, "Pawpocalypse.Realm.CardEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawCardEffectsTest::RunTest(const FString& Parameters)
{
	FLiveGame G;
	if (!G.Open(*this))
	{
		G.Close();
		return false;
	}
	const Pc::PcGameData& D = G.Game->Data();
	TArray<FCardCase> Cases = MakeCases();

	// Coverage: every CardEffect is a case here or carries no card; nothing is both, nothing is neither.
	std::set<int32_t> Covered;
	for (const FCardCase& C : Cases)
	{
		TestTrue(FString::Printf(TEXT("%hs has one case"), Pc::ToName(C.Effect)), Covered.insert(static_cast<int32_t>(C.Effect)).second);
	}
	for (const Pc::CardEffect E : Cardless)
	{
		TestTrue(FString::Printf(TEXT("%hs is not both a case and cardless"), Pc::ToName(E)), Covered.insert(static_cast<int32_t>(E)).second);
		TestNull(FString::Printf(TEXT("%hs carries no card (give it a case when one does)"), Pc::ToName(E)), CardFor(D, E));
	}
	int32 Declared = 0;
	for (int32_t V = 0; V < 256; ++V)
	{
		if (*Pc::ToName(static_cast<Pc::CardEffect>(V)) != '\0')
		{
			++Declared;
			TestTrue(FString::Printf(TEXT("%hs is covered"), Pc::ToName(static_cast<Pc::CardEffect>(V))), Covered.count(V) == 1);
		}
	}
	TestEqual(TEXT("every declared effect counted"), static_cast<int32>(Covered.size()), Declared);
	for (const Pc::PcCard& Card : D.Cards)
	{
		TestTrue(FString::Printf(TEXT("card %hs's effect has a case"), Card.Id.c_str()), Covered.count(static_cast<int32_t>(Card.Effect)) == 1);
	}

	// The control/subject comparison stands on this: a fixed seed replays the realm exactly (a second start must not
	// inherit the first one's travel count, which seeds each biome).
	{
		auto Replay = [this, &G]()
		{
			G.Game->NewGame();
			FPrereq{ true, false, false, false, 4, 10, 200.0 }.Apply(G.Game->Progress());
			G.Realm->StartRealm(CaseSeed);
			FCardWorld W{ G.Realm, G.Game->Data(), G.Game->Progress(), G.Realm->GetSim().Field, nullptr, {}, {} };
			for (int32 Frame = 0; Frame < 240; ++Frame)
			{
				W.Hunt();
				G.Realm->AdvanceFrame(FPawRealmSim::Step);
			}
			double Sum = static_cast<double>(W.F.Spawns) + 1e3 * static_cast<double>(W.F.LiveCount());
			for (const int32 S : W.F.Live)
			{
				Sum += W.F.X[static_cast<size_t>(S)] * 1.7 + W.F.Y[static_cast<size_t>(S)];
			}
			return Sum;
		};
		const double First = Replay();
		const double Second = Replay();
		TestEqual(TEXT("a fixed seed replays the realm exactly"), Second, First);
	}

	for (FCardCase& C : Cases)
	{
		const FString Name(Pc::ToName(C.Effect));
		const Pc::PcCard* Card = CardFor(D, C.Effect);
		if (!TestNotNull(FString::Printf(TEXT("%s has a card"), *Name), Card))
		{
			continue;
		}
		const FMetrics Control = RunCase(*this, G, C, nullptr);
		const FMetrics Subject = RunCase(*this, G, C, Card);
		const bool bDeck = C.Effect == Pc::CardEffect::REROLL || C.Effect == Pc::CardEffect::BANISH || C.Effect == Pc::CardEffect::FIXATION_HANDS;
		if (bDeck)
		{
			// choose() adds value_at(new level) on every take: the held count is the sum over the takes.
			const double Granted = GrantedByTakes(*Card);
			TestEqual(FString::Printf(TEXT("%s: every take of the card added its value (%g)"), *Name, Granted), Get(Subject, TEXT("held")), Granted);
			TestEqual(Name + TEXT(": nothing held without the card"), Get(Control, TEXT("held")), 0.0);
			TestTrue(Name + TEXT(": the realm dealt a hand in both runs"), Control.Contains(TEXT("_dealt")) && Subject.Contains(TEXT("_dealt")));
			TestEqual(Name + TEXT(": nothing spent without the card"), Get(Control, TEXT("spent")), 0.0);
			TestEqual(Name + TEXT(": one spent on the dealt hand"), Get(Subject, TEXT("spent")), 1.0);
		}
		else
		{
			C.Judge(*this, FString::Printf(TEXT("%s (%hs)"), *Name, Card->Id.c_str()), Control, Subject, Pc::CardValueAt(*Card, Pc::CardMaxLevel(*Card)));
		}
		UE_LOG(LogPaw, Display, TEXT("PAW_CARD %s (%hs): control %s | card %s"), *Name, Card->Id.c_str(),
			*FString::JoinBy(Control, TEXT(", "), [](const TPair<FString, double>& KV) { return FString::Printf(TEXT("%s=%g"), *KV.Key, KV.Value); }),
			*FString::JoinBy(Subject, TEXT(", "), [](const TPair<FString, double>& KV) { return FString::Printf(TEXT("%s=%g"), *KV.Key, KV.Value); }));
	}

	// Taking a card rebuilds the cached stats (CardManager.choose() -> GameState.refresh_stats()): with Grudge Ledger the
	// damage product grows with every card taken, at once, not at the next kill.
	{
		G.Game->NewGame();
		Pc::PcProgress& P = G.Game->Progress();
		P.BranchLevels["grudge_ledger"] = 1;
		TestTrue(TEXT("the realm starts"), G.Realm->StartRealm(CaseSeed));
		const Pc::PcField& F = G.Realm->GetSim().Field;
		const double Before = F.Stats.MultiplicativeDamageProduct;
		const Pc::PcCard* Card = nullptr;
		for (const Pc::PcCard& C : D.Cards)
		{
			if (Pc::IsCardEligible(D, P, C))
			{
				Card = &C;
				break;
			}
		}
		if (TestNotNull(TEXT("a card is offerable to a new game"), Card))
		{
			P.Hand = { Card->Id };
			TestTrue(TEXT("the card is taken"), G.Realm->ChooseCard(FString(UTF8_TO_TCHAR(Card->Id.c_str()))));
			TestTrue(FString::Printf(TEXT("Grudge Ledger: the field's stats grew on the take (%g -> %g)"), Before, F.Stats.MultiplicativeDamageProduct),
				F.Stats.MultiplicativeDamageProduct > Before);
			TestFalse(TEXT("a card not in the hand is refused"), G.Realm->ChooseCard(FString(UTF8_TO_TCHAR(Card->Id.c_str()))));
		}
	}
	G.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawEveryCardTest, "Pawpocalypse.Realm.EveryCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawEveryCardTest::RunTest(const FString& Parameters)
{
	FLiveGame G;
	if (!G.Open(*this))
	{
		G.Close();
		return false;
	}
	const Pc::PcGameData& D = G.Game->Data();
	G.Game->NewGame();
	Pc::PcProgress& P = G.Game->Progress();
	FPrereq{ true, true, true, true, 10, 30, 1000.0 }.Apply(P);
	const Pc::PcProgress Bare = P;
	TestTrue(TEXT("the realm starts"), G.Realm->StartRealm(CaseSeed));
	for (const Pc::PcCard& Card : D.Cards)
	{
		TakeToMax(*this, G.Realm, D, P, Card, false);
	}
	// No re-count here: each take re-counted the kittens itself (F14).
	TestEqual(TEXT("every card is owned"), static_cast<int32>(P.CardLevels.size()), static_cast<int32>(D.Cards.size()));

	FCardWorld W{ G.Realm, D, P, G.Realm->GetSim().Field, nullptr, {}, {} };
	const Pc::PcField& F = W.F;
	const double SpeedCost = Pc::CardCostMultiplier(D, P, Pc::CardCost::MOUSE_SPEED);
	const int64 KillsBefore = P.LifetimeKills;
	int32 Pulses = 0, Bombs = 0, Pounces = 0, Puddles = 0, Spawns = 0, WrongSpeed = 0, Broken = 0;
	for (int32 Frame = 0; Frame < 900; ++Frame)
	{
		W.Hunt();
		G.Realm->SetPounceDown(Frame % 30 == 0);
		G.Realm->AdvanceFrame(FPawRealmSim::Step);
		Pulses += W.Count(Pc::PcFieldEventKind::AuraPulse);
		Bombs += W.Count(Pc::PcFieldEventKind::CatnipBomb);
		Pounces += W.Count(Pc::PcFieldEventKind::Pounce);
		Puddles += W.Count(Pc::PcFieldEventKind::PoopDropped);
		for (const Pc::PcFieldEvent& E : W.Events())
		{
			if (E.Kind == Pc::PcFieldEventKind::Spawned)
			{
				++Spawns;
				WrongSpeed += F.SpeedMult[static_cast<size_t>(E.Slot)] == SpeedCost ? 0 : 1;
			}
		}
		bool bFinite = std::isfinite(F.CatX) && std::isfinite(F.CatY);
		for (const int32 S : F.Live)
		{
			const size_t I = static_cast<size_t>(S);
			bFinite = bFinite && std::isfinite(F.X[I]) && std::isfinite(F.Y[I]) && std::isfinite(F.Hp[I]);
		}
		const bool bCaps = F.LiveCount() <= D.Code.MaxLiveEnemies && static_cast<int64>(F.Zones.size()) <= D.Code.MaxDamageZones;
		Broken += bFinite && bCaps ? 0 : 1;
	}
	UE_LOG(LogPaw, Display, TEXT("PAW_EVERYCARD %lld kills, %d spawns, %d pulses, %d bombs, %d pounces, %d puddles, %lld kittens"),
		P.LifetimeKills - KillsBefore, Spawns, Pulses, Bombs, Pounces, Puddles, F.AwakeKittens());
	TestEqual(TEXT("every frame kept its caps and finite positions"), Broken, 0);
	TestTrue(TEXT("mice spawned and died"), P.LifetimeKills > KillsBefore && Spawns > 0);
	TestTrue(TEXT("the aura pulsed"), Pulses > 0);
	TestTrue(TEXT("catnip bombs went off"), Bombs > 0);
	TestTrue(TEXT("the cat pounced"), Pounces > 0);
	TestTrue(TEXT("puddles dropped"), Puddles > 0);

	// Every cost lands where it is read.
	TestTrue(FString::Printf(TEXT("MOUSE_SPEED: the cost is live (%g)"), SpeedCost), SpeedCost > 1.0);
	TestEqual(TEXT("MOUSE_SPEED: every mouse spawned at the cost's speed"), WrongSpeed, 0);
	TestTrue(TEXT("AURA_RADIUS: the aura shrank"), Pc::AuraRadius(D, P) < Pc::AuraRadius(D, Bare));
	TestTrue(TEXT("CATNIP_YIELD: the yield fell"), Pc::CardCostMultiplier(D, P, Pc::CardCost::CATNIP_YIELD) < 1.0);
	TestTrue(TEXT("SPAWN_RATE: the rate fell"), Pc::CardCostMultiplier(D, P, Pc::CardCost::SPAWN_RATE) < 1.0);
	TestTrue(TEXT("KITTEN_COUNT: kittens wandered off"), Pc::KittenCount(D, P) < Pc::KittenCount(D, Bare));
	TestEqual(TEXT("KITTEN_COUNT: the realm wakes what is left"), F.AwakeKittens(), static_cast<int64_t>(Pc::KittenCount(D, P)));
	G.Close();
	return true;
}

#endif
