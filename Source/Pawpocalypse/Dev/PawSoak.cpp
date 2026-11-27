// The soak (docs/09 Â§5, L6; phase 8): `paw.Soak [game-minutes] [speed]`, development builds only.
//
// Plays a whole game in the running realm as a player would, through the UI's own presses (never the model): the cat hunts
// the nearest enemy; every half second the Upgrades page's affordable rows are pressed; a dealt hand takes its first card;
// the job picker takes its first job; every 2 s the Realms page's deepest open row is travelled to (the game's auto-travel
// too, once owned) and its Bait summons a boss when it can;
// the Reawaken page reawakens when the game SUGGESTS it (the button reads "Reawaken": the payout at least the requirement,
// pawsim's v4 player), never an early cash-out (after the first Reawaken the button allows one from 1 Stardust: designer
// 2026-08-20, the player's choice; the soak's first version took every one, 14 Reawakens in an hour; found by the
// designer 2026-11-23), and the tree's spend step buys every node it can (waiting out the Unicorn ceremony), then leaves;
// the Achievements are claimed. It stops at the victory (the first final-boss clear), or at the game-minute limit (240 by
// default), at `speed` x (5 by default; 20 for stability: docs/09 Â§5, "trust 5x for pace, 20x for stability").
//
// It logs, per game minute, the biome, Catnip, Stardust, Reawakens and the frame (p50 / p99 / worst), the memory and the
// object count; and over the run every Error and Warning line, the worst frames with what was happening, and any time the
// field stood held by a screen it does not know. Report: Saved/PawSoak/report.txt; log lines PAW_SOAK; the verdict
// "PAW_SOAK PASS" (the victory, no Error line, no stall) or "PAW_SOAK FAIL <reasons>". tools/packaged_soak.py runs it in a
// packaged Development build.

#include "Containers/Ticker.h"
#include "Core/PawGameSubsystem.h"
#include "Core/PawLog.h"
#include "Core/PawPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMemory.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDevice.h"
#include "Misc/Paths.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcField.h"
#include "PcHud.h"
#include "PcProgress.h"
#include "Realm/PawRealmSubsystem.h"
#include "Realm/PawRealmView.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawCardPickScreen.h"
#include "UI/PawHudScreen.h"
#include "UI/PawJobPicker.h"
#include "UI/PawReawakenCeremony.h"
#include "UI/PawSkillTreeScreen.h"
#include "UI/PawVictoryScreen.h"
#include "UObject/UObjectArray.h"

#if !UE_BUILD_SHIPPING

namespace
{
	double Percentile(TArray<double> V, double Q)
	{
		if (V.Num() == 0)
		{
			return 0.0;
		}
		V.Sort();
		return V[FMath::Clamp(static_cast<int32>(FMath::CeilToDouble(Q * V.Num())) - 1, 0, V.Num() - 1)];
	}

	// Every Error and Warning line the run logs (any thread; kept under a lock).
	class FSoakLog : public FOutputDevice
	{
	public:
		FCriticalSection Lock;
		int64 Errors = 0, Warnings = 0;
		TArray<FString> FirstErrors, FirstWarnings;

		virtual bool CanBeUsedOnAnyThread() const override { return true; }
		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			const ELogVerbosity::Type Level = static_cast<ELogVerbosity::Type>(Verbosity & ELogVerbosity::VerbosityMask);
			if (Level != ELogVerbosity::Error && Level != ELogVerbosity::Fatal && Level != ELogVerbosity::Warning)
			{
				return;
			}
			const FString Line = FString::Printf(TEXT("%s: %s"), *Category.ToString(), V);
			if (Line.Contains(TEXT("PAW_SOAK")))
			{
				return;
			}
			FScopeLock Held(&Lock);
			const bool bError = Level != ELogVerbosity::Warning;
			(bError ? Errors : Warnings) += 1;
			TArray<FString>& Keep = bError ? FirstErrors : FirstWarnings;
			if (Keep.Num() < 30 && !Keep.Contains(Line))
			{
				Keep.Add(Line);
			}
		}
	};

	struct FWorst
	{
		double Ms = 0.0;
		double GameMin = 0.0;
		FString What;
	};

	// "id:level id:level" of a level map (the levels above zero), for the build lines.
	FString Levels(const std::map<std::string, int64_t>& Map)
	{
		FString Out;
		for (const auto& [Id, Level] : Map)
		{
			if (Level > 0)
			{
				Out += FString::Printf(TEXT("%s%hs:%lld"), Out.IsEmpty() ? TEXT("") : TEXT(" "), Id.c_str(), static_cast<long long>(Level));
			}
		}
		return Out.IsEmpty() ? FString(TEXT("-")) : Out;
	}

	// The boss's HP left, 0..1 (1 when none is up).
	double BossHpLeft(const Pc::PcField& F, const Pc::PcGameData& D)
	{
		if (!F.bBossUp || F.TierIndex < 0 || F.TierIndex >= static_cast<int64_t>(D.Tiers.size()))
		{
			return 1.0;
		}
		const Pc::PcBoss* B = D.FindBoss(D.Tiers[static_cast<size_t>(F.TierIndex)].BossData);
		return B && B->TotalHp > 0.0 ? F.BossHp / B->TotalHp : 1.0;
	}

	struct FSoak
	{
		double LimitGameMin = 240.0;
		double Speed = 5.0;
		bool bStarted = false, bDone = false;
		int32 QuitIn = -1;
		double RealSec = 0.0, NextBuy = 0.0, NextBait = 0.0, NextClaim = 0.0, NextReawakenLook = 0.0, NextTravel = 0.0;
		double HeldSec = 0.0, LongestHeldSec = 0.0;
		FString Doing = TEXT("start");
		int64 Bought = 0, Cards = 0, Jobs = 0, Baits = 0, Reawakens = 0, TreeBuys = 0, TreeLeaves = 0, TreeRefused = 0, BiomeChanges = 0, Travels = 0;
		FString LastBiome;
		bool bBossWasUp = false;
		double LastBossHpLeft = 1.0;
		int32 LastLoggedMinute = -1;
		int64 KillsAtMinute = 0;
		TArray<double> FrameMs, MinuteMs;
		TArray<FWorst> Worst;
		uint64 MemStart = 0, MemPeak = 0;
		int32 ObjectsStart = 0;
		TArray<FString> Lines;
		FSoakLog Log;
		FTSTicker::FDelegateHandle Handle;

		static UWorld* GameWorld()
		{
			return GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
		}

		void Note(const FString& Line)
		{
			Lines.Add(Line);
			UE_LOG(LogPaw, Display, TEXT("PAW_SOAK %s"), *Line);
		}

		// What the cat is made of: the tree, the run's upgrades and cards, and what it puts out (the stats the field uses).
		void NoteBuild(const TCHAR* When, const Pc::PcGameData& D, const Pc::PcProgress& P, const Pc::PcField& F)
		{
			Note(FString::Printf(TEXT("build %s: paw hit %.4g x %.3g/s, paw dps %.4g, aura dps %.4g, kitten dps %.4g x %lld; tree [%s]; upgrades [%s]; cards [%s]; job %hs"),
				When, Pc::PawStrikeDamage(D.Combat, F.Stats), Pc::StrikesPerSec(D.Combat, F.Stats), Pc::PawDps(D.Combat, F.Stats),
				Pc::AuraDps(D.Combat, F.Stats), Pc::KittenDps(D.Combat, F.Stats), static_cast<long long>(F.AwakeKittens()),
				*Levels(P.BranchLevels), *Levels(P.UpgradeLevels), *Levels(P.CardLevels), P.RunJob.empty() ? "-" : P.RunJob.c_str()));
		}

		void KeepWorst(double Ms, double GameMin)
		{
			if (Worst.Num() < 10 || Ms > Worst.Last().Ms)
			{
				Worst.Add({ Ms, GameMin, Doing });
				Worst.Sort([](const FWorst& A, const FWorst& B) { return A.Ms > B.Ms; });
				if (Worst.Num() > 10)
				{
					Worst.Pop();
				}
			}
		}

		bool Tick(float DeltaSeconds)
		{
			if (QuitIn >= 0)
			{
				if (QuitIn-- == 0)
				{
					FPlatformMisc::RequestExit(false, TEXT("paw.Soak"));
				}
				return true;
			}
			UWorld* World = GameWorld();
			UPawRealmSubsystem* Realm = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
			UPawGameSubsystem* Game = Realm ? Realm->GetGame() : nullptr;
			APawPlayerController* PC = World ? Cast<APawPlayerController>(World->GetFirstPlayerController()) : nullptr;
			UPawHudScreen* H = PC ? PC->GetHud() : nullptr;
			if (!Realm || !Game || !H || !Realm->IsRunning())
			{
				return true; // the map is still loading
			}
			Pc::PcProgress& P = Game->Progress();
			const Pc::PcField& F = Realm->GetSim().Field;
			if (!bStarted)
			{
				bStarted = true;
				if (IConsoleVariable* Overlay = IConsoleManager::Get().FindConsoleVariable(TEXT("paw.DebugHud")))
				{
					Overlay->Set(0, ECVF_SetByCode);
				}
				// The real frame, whatever window is in front (Q10's background cap off, as the bench does).
				if (IConsoleVariable* Background = IConsoleManager::Get().FindConsoleVariable(TEXT("paw.BackgroundFps")))
				{
					Background->Set(0, ECVF_SetByCode);
					if (UPawSettingsSubsystem* Settings = World->GetGameInstance()->GetSubsystem<UPawSettingsSubsystem>())
					{
						Settings->RefreshBackgroundCap();
					}
				}
				for (TActorIterator<APawRealmView> It(World); It; ++It)
				{
					It->bSampleInput = false; // the soak drives the cat
				}
				Realm->SetGameSpeed(Speed);
				Realm->SetAutoTravel(true); // the game's own auto-travel moves on when a biome is cleared
				MemStart = MemPeak = FPlatformMemory::GetStats().UsedPhysical;
				ObjectsStart = GUObjectArray.GetObjectArrayNumMinusAvailable();
				KillsAtMinute = P.LifetimeKills;
				GLog->AddOutputDevice(&Log);
				Note(FString::Printf(TEXT("started: %.0fx, at most %.0f game minutes"), Speed, LimitGameMin));
			}
			RealSec += DeltaSeconds;
			const double GameMin = P.TotalPlaySeconds / 60.0;
			const double Ms = DeltaSeconds * 1000.0;
			FrameMs.Add(Ms);
			MinuteMs.Add(Ms);
			KeepWorst(Ms, GameMin);
			MemPeak = FMath::Max<uint64>(MemPeak, FPlatformMemory::GetStats().UsedPhysical);

			// The cat on the boss while one is up (a player fights it: the bench's hunter only knows the field's enemies,
			// and the soak's first version lost every boss fight to the clock), else on the nearest enemy.
			double TX = F.CatX, TY = F.CatY, Best = F.bBossUp ? -1.0 : 1e30;
			if (F.bBossUp)
			{
				TX = F.BossX;
				TY = F.BossY;
			}
			for (const int32 S : F.bBossUp ? decltype(F.Live)() : F.Live)
			{
				const size_t I = static_cast<size_t>(S);
				const double D2 = (F.X[I] - F.CatX) * (F.X[I] - F.CatX) + (F.Y[I] - F.CatY) * (F.Y[I] - F.CatY);
				if (D2 < Best)
				{
					Best = D2;
					TX = F.X[I];
					TY = F.Y[I];
				}
			}
			Realm->SetCat(TX, TY);

			const FString Biome = Realm->CurrentTierId();
			if (F.bBossUp != bBossWasUp)
			{
				// Gone: beaten (its HP ran out), or it left (the fight clock, a Reawaken) with HP left.
				Note(FString::Printf(TEXT("boss %s in %s at game minute %.1f%s"), F.bBossUp ? TEXT("up") : TEXT("gone"), *Biome, GameMin,
					F.bBossUp ? TEXT("") : *FString::Printf(TEXT(" (%.1f%% of its HP left when last seen)"), 100.0 * LastBossHpLeft)));
				bBossWasUp = F.bBossUp;
				if (F.bBossUp)
				{
					NoteBuild(TEXT("at the boss"), Game->Data(), P, F);
				}
			}
			if (F.bBossUp)
			{
				LastBossHpLeft = BossHpLeft(F, Game->Data());
			}
			if (Biome != LastBiome)
			{
				BiomeChanges += LastBiome.IsEmpty() ? 0 : 1;
				Doing = TEXT("biome ") + Biome;
				LastBiome = Biome;
			}

			// The end: the first final-boss clear is the game won (the victory screen follows it).
			const UPawVictoryScreen* Victory = H->GetVictory();
			if (P.FinalBossSeconds > 0.0 || (Victory && Victory->IsActivated()))
			{
				Finish(true, TEXT(""), P, Realm);
				return true;
			}
			if (GameMin >= LimitGameMin)
			{
				Finish(false, FString::Printf(TEXT("no victory in %.0f game minutes"), LimitGameMin), P, Realm);
				return true;
			}

			// The screens that hold the field, as a player answers them.
			bool bHandled = false;
			if (UPawCardPickScreen* Pick = H->IsCardPickUp() ? H->GetCardPick() : nullptr)
			{
				Doing = TEXT("card pick");
				Cards += Pick->Take(0) ? 1 : 0;
				bHandled = true;
			}
			else if (UPawJobPicker* Jobs_ = H->GetJobPicker(); Jobs_ && Jobs_->IsActivated())
			{
				Doing = TEXT("job picker");
				Jobs += Jobs_->Pick(0) ? 1 : 0;
				bHandled = true;
			}
			else if (UPawReawakenCeremony* C = H->GetCeremony(); C && C->IsPlaying())
			{
				Doing = TEXT("Reawaken ceremony");
				bHandled = true;
			}
			else if (UPawSkillTreeScreen* T = H->GetSkillTree(); T && T->IsActivated())
			{
				Doing = TEXT("the tree's spend step");
				if (T->IsCeremonyPlaying())
				{
					// The Unicorn arrives over the tree: a player waits for it (every key and click goes to it). The soak's
					// first version took the refused press for "nothing left" and closed the tree under it; the ceremony
					// ticks with the tree, so it never finished and the tree refused every purchase for the rest of the game
					// (found 2026-11-24: the cat's tree frozen from Reawaken ~6 of 12, millions of Stardust unspent).
					Doing = TEXT("the Unicorn ceremony");
				}
				else if (T->IsSpendMode())
				{
					const FString Id = T->JumpToAffordable();
					if (Id.IsEmpty())
					{
						T->PressClose();
						T->AnswerLeave(true); // P4.7: the leave question's Leave
						++TreeLeaves;
					}
					else if (T->PressNode(Id))
					{
						++TreeBuys;
					}
					else
					{
						// The tree offered a node its own press refuses: a fault of the game, reported; then it leaves.
						Note(FString::Printf(TEXT("the tree offered %s and refused to buy it (%.4g Stardust) at game minute %.1f"), *Id, P.Stardust, GameMin));
						++TreeRefused;
						T->PressClose();
						T->AnswerLeave(true); // P4.7: the leave question's Leave
						++TreeLeaves;
					}
				}
				else
				{
					T->PressClose();
				}
				bHandled = true;
			}
			else if (H->IsSpendStepPending())
			{
				Doing = TEXT("before the spend step");
				bHandled = true;
			}
			if (Realm->IsPaused() && !bHandled)
			{
				HeldSec += DeltaSeconds;
				LongestHeldSec = FMath::Max(LongestHeldSec, HeldSec);
			}
			else
			{
				HeldSec = 0.0;
			}
			if (HeldSec > 30.0)
			{
				Finish(false, TEXT("the field held 30 s by a screen the soak does not know"), P, Realm);
				return true;
			}

			if (!Realm->IsPaused() && !bHandled)
			{
				Doing = TEXT("play");
				// The Reawaken page: as soon as its button opens (pawsim's v4 player), its two presses.
				if (RealSec >= NextReawakenLook)
				{
					NextReawakenLook = RealSec + 1.0;
					// When the game suggests it (the page's "Reawaken" label, not "Reawaken early"); the press checks the
					// permission itself. The dock stays on Upgrades (a page shown for a moment has not rebuilt its rows).
					if (Pc::CanReawaken(Game->Data(), P) && Pc::IsReawakenSuggested(Game->Data(), P))
					{
						const double Payout = Pc::PendingStardust(Game->Data(), P);
						const double Requirement = Pc::ReawakenRequirement(Game->Data(), P);
						Note(FString::Printf(TEXT("Reawaken %lld at game minute %.1f in %s: +%.4g Stardust (requirement %.4g, ever granted %.4g)"),
							static_cast<long long>(P.ReawakenCount + 1), GameMin, *Biome, Payout, Requirement, P.StardustGrantedEver));
						NoteBuild(TEXT("before the Reawaken"), Game->Data(), P, F);
						H->PressReawaken();
						H->PressReawaken();
						++Reawakens;
						Doing = TEXT("Reawaken");
						return true;
					}
				}
				// The Upgrades page: every affordable row, top to bottom.
				if (RealSec >= NextBuy)
				{
					NextBuy = RealSec + 0.5;
					if (H->GetActiveTab() != Pc::PcDockTab::Upgrades)
					{
						H->SelectTab(Pc::PcDockTab::Upgrades);
						H->RefreshNow();
					}
					for (int32 Row = 0; Row < 64; ++Row)
					{
						const UPawButton* B = H->GetRowButton(Row);
						if (!B)
						{
							break;
						}
						if (B->GetIsEnabled() && !B->IsLocked() && B->IsVisible())
						{
							H->PressRow(Row);
							++Bought;
						}
					}
				}
				// The Realms page: travel to the deepest biome open (as the rows say it: Travel, or a boss's), when deeper.
				if (RealSec >= NextTravel)
				{
					NextTravel = RealSec + 2.0;
					const Pc::PcGameData& D = Game->Data();
					int32 Here = -1, Deepest = -1;
					for (int32 I = 0; I < static_cast<int32>(D.Tiers.size()); ++I)
					{
						const Pc::PcRealmRow Row = Pc::RealmRow(D, P, I);
						Here = Row.Kind == Pc::PcRealmRowKind::Here ? I : Here;
						if (Row.bEnabled && (Row.Kind == Pc::PcRealmRowKind::Travel || Row.Kind == Pc::PcRealmRowKind::TravelBoss))
						{
							Deepest = I;
						}
					}
					if (Deepest > Here)
					{
						H->PressRealm(Deepest);
						++Travels;
						Doing = TEXT("travel");
						return true;
					}
				}
				// The Realms page's Bait: a boss summoned whenever it can be.
				if (RealSec >= NextBait)
				{
					NextBait = RealSec + 2.0;
					if (Pc::BaitButton(F, Game->Data(), P).bEnabled) // the rule the Bait button reads
					{
						H->PressBait();
						++Baits;
					}
				}
				if (RealSec >= NextClaim)
				{
					NextClaim = RealSec + 10.0;
					H->PressClaimAll();
				}
			}

			// A line a game minute.
			const int32 Minute = static_cast<int32>(GameMin);
			if (Minute != LastLoggedMinute)
			{
				LastLoggedMinute = Minute;
				// The field's toughness against the cat: the live enemies' mean max HP, in paw hits.
				double HpSum = 0.0;
				for (const int32 S : F.Live)
				{
					HpSum += F.MaxHp[static_cast<size_t>(S)];
				}
				const double MeanHp = F.Live.size() ? HpSum / static_cast<double>(F.Live.size()) : 0.0;
				const double Hit = Pc::PawStrikeDamage(Game->Data().Combat, F.Stats);
				Note(FString::Printf(TEXT("min %d biome=%s catnip=%.3g stardust=%.3g reawakens=%lld live=%lld kills=%lld wave=%lld hp=%.3g hits=%.3g aura=%.3g boss=%s frame p50=%.2f p99=%.2f max=%.1f ms mem=%.0f MB objects=%d"),
					Minute, *Biome, P.Catnip, P.Stardust, static_cast<long long>(P.ReawakenCount), static_cast<long long>(F.LiveCount()),
					static_cast<long long>(P.LifetimeKills - KillsAtMinute), static_cast<long long>(P.CurrentWaveIndex), MeanHp,
					Hit > 0.0 ? MeanHp / Hit : 0.0, Pc::AuraDps(Game->Data().Combat, F.Stats),
					F.bBossUp ? *FString::Printf(TEXT("%.1f%%"), 100.0 * BossHpLeft(F, Game->Data())) : TEXT("-"),
					Percentile(MinuteMs, 0.5), Percentile(MinuteMs, 0.99), Percentile(MinuteMs, 1.0),
					FPlatformMemory::GetStats().UsedPhysical / 1048576.0, GUObjectArray.GetObjectArrayNumMinusAvailable()));
				KillsAtMinute = P.LifetimeKills;
				MinuteMs.Reset();
			}
			return true;
		}

		void Finish(bool bWon, const FString& Why, const Pc::PcProgress& P, UPawRealmSubsystem* Realm)
		{
			if (bDone)
			{
				return;
			}
			bDone = true;
			GLog->RemoveOutputDevice(&Log);
			TArray<FString> Fail;
			if (!bWon)
			{
				Fail.Add(Why);
			}
			if (TreeRefused > 0)
			{
				Fail.Add(FString::Printf(TEXT("the tree refused %lld purchase(s) it offered"), TreeRefused));
			}
			{
				FScopeLock Held(&Log.Lock);
				if (Log.Errors > 0)
				{
					Fail.Add(FString::Printf(TEXT("%lld Error line(s)"), Log.Errors));
				}
				Note(FString::Printf(TEXT("errors=%lld warnings=%lld"), Log.Errors, Log.Warnings));
				for (const FString& L : Log.FirstErrors)
				{
					Note(TEXT("error: ") + L.Left(300));
				}
				for (const FString& L : Log.FirstWarnings)
				{
					Note(TEXT("warning: ") + L.Left(300));
				}
			}
			const double GameMin = P.TotalPlaySeconds / 60.0;
			if (Realm && Realm->GetGame())
			{
				NoteBuild(TEXT("at the end"), Realm->GetGame()->Data(), P, Realm->GetSim().Field);
			}
			Note(FString::Printf(TEXT("%s after %.1f game minutes (%.1f real), victory at %.1f game minutes; %lld Reawakens, %lld biome changes, %lld travels"),
				bWon ? TEXT("won") : TEXT("stopped"), GameMin, RealSec / 60.0, P.FinalBossSeconds / 60.0, static_cast<long long>(P.ReawakenCount),
				BiomeChanges, Realm ? Realm->GetSim().Travels : -1));
			Note(FString::Printf(TEXT("presses: %lld upgrade rows, %lld cards, %lld jobs, %lld travels, %lld baits, %lld Reawakens, %lld tree nodes, %lld tree leaves, %lld refused; %.4g Stardust unspent"),
				Bought, Cards, Jobs, Travels, Baits, Reawakens, TreeBuys, TreeLeaves, TreeRefused, P.Stardust));
			Note(FString::Printf(TEXT("frame_ms p50=%.2f p95=%.2f p99=%.2f max=%.1f (%d frames); the field held by an unknown screen at most %.1f s"),
				Percentile(FrameMs, 0.5), Percentile(FrameMs, 0.95), Percentile(FrameMs, 0.99), Percentile(FrameMs, 1.0), FrameMs.Num(), LongestHeldSec));
			for (const FWorst& W : Worst)
			{
				Note(FString::Printf(TEXT("worst frame %.1f ms at game minute %.1f: %s"), W.Ms, W.GameMin, *W.What));
			}
			Note(FString::Printf(TEXT("memory %.0f -> peak %.0f MB; objects %d -> %d; steps dropped %lld"),
				MemStart / 1048576.0, MemPeak / 1048576.0, ObjectsStart, GUObjectArray.GetObjectArrayNumMinusAvailable(),
				Realm ? Realm->GetSim().StepsDropped : -1));
			const FString Verdict = Fail.Num() ? TEXT("FAIL ") + FString::Join(Fail, TEXT("; ")) : FString(TEXT("PASS"));
			Lines.Add(Verdict);
			UE_LOG(LogPaw, Display, TEXT("PAW_SOAK %s"), *Verdict);
			FFileHelper::SaveStringArrayToFile(Lines, *(FPaths::ProjectSavedDir() / TEXT("PawSoak") / TEXT("report.txt")));
			QuitIn = 30;
		}
	};

	TUniquePtr<FSoak> GSoak;

	FAutoConsoleCommand SoakCommand(TEXT("paw.Soak"),
		TEXT("paw.Soak [game-minutes] [speed]: play a whole game through the UI's presses at speed x (5), report to Saved/PawSoak, quit."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			if (GSoak)
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Soak: already running"));
				return;
			}
			GSoak = MakeUnique<FSoak>();
			GSoak->LimitGameMin = Args.Num() > 0 ? FMath::Max(1.0, FCString::Atod(*Args[0])) : 240.0;
			GSoak->Speed = Args.Num() > 1 ? FMath::Clamp(FCString::Atod(*Args[1]), 1.0, 50.0) : 5.0;
			GSoak->Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float Dt) { return GSoak->Tick(Dt); }));
		}));
}

#endif
