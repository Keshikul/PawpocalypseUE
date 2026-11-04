// Screenshot tours (docs/11 phase 3 exit: every tab against the Godot shots). Development builds only. Queued in order,
// the first step waiting for the map to settle, each later one a short beat after the one before:
//   paw.Shot <name> [quit]   a screenshot into Saved/PawShots/<name>.png (then quit, once it has landed)
//   paw.Tab <1-6>            open a dock tab on the HUD
//   paw.Tree [spend]         open the skill tree (a browse, or the spend step)
//   paw.Hand                 deal a card hand from the field's rolls (the card pick opens on it)
//   paw.Reawaken [jobs]      the HUD's Reawaken (its two presses) on a run made worth it: the ceremony, then the spend
//                            step; `jobs`: hold three jobs first, so leaving the spend step opens the job picker
//   paw.Leave                leave the spend step (its two presses)
//   paw.Wait <frames>        hold the queue this many frames (a step otherwise waits 30)
//   paw.Take                 take the first card of the hand (as its button does)
//   paw.Victory [seconds]    the ending, as the last boss's kill raises it (the time shown: 4200 s unless given)
// e.g. -ExecCmds="paw.Cheat.Catnip 300,paw.Tab 3,paw.Shot realms,paw.Tab 2,paw.Shot reawaken quit"

#include "Containers/Ticker.h"
#include "Core/PawLog.h"
#include "Core/PawPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "Core/PawGameSubsystem.h"
#include "PcCardDraw.h"
#include "PcData.h"
#include "PcHud.h"
#include "Realm/PawRealmSubsystem.h"
#include "UI/PawCardPickScreen.h"
#include "UI/PawHudScreen.h"
#include "UI/PawSkillTreeScreen.h"
#include "UnrealClient.h"

#if !UE_BUILD_SHIPPING

namespace
{
	enum class EStep
	{
		Shot,
		Tab,
		Tree,
		Hand,
		Reawaken,
		Leave,
		Victory,
		Wait,
		Take,
	};
	struct FStep
	{
		bool bShot = true;
		FString Name;
		int32 Tab = 0;
		bool bQuit = false;
		EStep Kind = EStep::Shot;
		bool bSpend = false;
		bool bJobs = false;
		double Seconds = 4200.0;
		int32 Frames = 30;
	};
	TArray<FStep> GQueue;
	int32 GFramesLeft = 0;
	int32 GQuitIn = -1;
	FTSTicker::FDelegateHandle GHandle;

	UPawHudScreen* Hud()
	{
		UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
		APawPlayerController* PC = World ? Cast<APawPlayerController>(World->GetFirstPlayerController()) : nullptr;
		return PC ? PC->GetHud() : nullptr;
	}

	bool Tick(float)
	{
		if (GQuitIn >= 0)
		{
			if (GQuitIn-- == 0)
			{
				FPlatformMisc::RequestExit(false, TEXT("paw.Shot"));
			}
			return true;
		}
		if (GQueue.Num() == 0 || GFramesLeft-- > 0)
		{
			return true;
		}
		const FStep Step = GQueue[0];
		GQueue.RemoveAt(0);
		GFramesLeft = 30;
		if (Step.Kind == EStep::Wait)
		{
			GFramesLeft = FMath::Max(0, Step.Frames);
			return true;
		}
		if (Step.Kind == EStep::Take)
		{
			// THROUGH THE SCREEN, as its button does: the realm's ChooseCard alone empties the hand and leaves the screen up
			// (holding the field), the trap Godot's own probe notes for CatchesPickUI.
			UPawHudScreen* H = Hud();
			UPawCardPickScreen* Pick = H ? H->GetCardPick() : nullptr;
			if (!Pick || !H->IsCardPickUp() || !Pick->Take(0))
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Take: no card taken"));
			}
			return true;
		}
		if (Step.Kind == EStep::Tree)
		{
			if (UPawHudScreen* H = Hud(); !H || !H->OpenSkillTree(Step.bSpend))
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Tree: the tree did not open"));
			}
			return true;
		}
		if (Step.Kind == EStep::Hand)
		{
			UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
			UPawRealmSubsystem* R = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
			UPawGameSubsystem* G = R ? R->GetGame() : nullptr;
			if (G && G->IsDataReady())
			{
				Pc::PcRngCardRolls Rolls(R->GetSim().Field.Seed, ++R->GetSim().Field.Deals);
				Pc::OfferHand(G->Data(), G->Progress(), Rolls, false);
			}
			return true;
		}
		if (Step.Kind == EStep::Reawaken)
		{
			UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
			UPawRealmSubsystem* R = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
			UPawGameSubsystem* G = R ? R->GetGame() : nullptr;
			UPawHudScreen* H = Hud();
			if (G && G->IsDataReady() && H)
			{
				Pc::PcProgress& P = G->Progress();
				P.LifetimeCatnipEver = FMath::Max(P.LifetimeCatnipEver, 1e12);
				P.LifetimeCatnipThisCycle = FMath::Max(P.LifetimeCatnipThisCycle, 1e12);
				if (Step.bJobs)
				{
					for (const Pc::PcJob& J : G->Data().Jobs)
					{
						if (P.JobsHeld.size() < 3)
						{
							P.JobsHeld.insert(J.Id);
							P.CurrentJob = J.Id;
						}
					}
				}
				H->RefreshNow();
				H->PressReawaken();
				H->PressReawaken(); // the confirm, when "Ask before Reawakening" is on; nothing otherwise
			}
			return true;
		}
		if (Step.Kind == EStep::Victory)
		{
			if (UPawHudScreen* H = Hud())
			{
				H->ShowVictory(Step.Seconds);
			}
			else
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Victory: no HUD"));
			}
			return true;
		}
		if (Step.Kind == EStep::Leave)
		{
			UPawHudScreen* H = Hud();
			if (UPawSkillTreeScreen* T = H ? H->GetSkillTree() : nullptr; T && T->IsActivated())
			{
				T->PressClose();
				T->PressClose();
			}
			else
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Leave: no tree up"));
			}
			return true;
		}
		if (!Step.bShot)
		{
			if (UPawHudScreen* H = Hud())
			{
				H->SelectTab(static_cast<Pc::PcDockTab>(FMath::Clamp(Step.Tab - 1, 0, static_cast<int32>(Pc::PcDockTab::Count) - 1)));
			}
			else
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Tab: no HUD"));
			}
			return true;
		}
		const FString Path = FPaths::ProjectSavedDir() / TEXT("PawShots") / Step.Name + TEXT(".png");
		FScreenshotRequest::RequestScreenshot(Path, true, false);
		UE_LOG(LogPaw, Display, TEXT("PAW_SHOT %s"), *Path);
		if (Step.bQuit)
		{
			GQuitIn = 30; // let the file land
		}
		return true;
	}

	void Queue(const FStep& Step)
	{
		if (GQueue.Num() == 0 && GFramesLeft <= 0)
		{
			GFramesLeft = 90; // the first step waits for the map to settle
		}
		GQueue.Add(Step);
		if (!GHandle.IsValid())
		{
			GHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick));
		}
	}

	FAutoConsoleCommand ShotCommand(TEXT("paw.Shot"), TEXT("paw.Shot <name> [quit]: screenshot to Saved/PawShots/<name>.png (queued)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.Name = Args.Num() > 0 ? Args[0] : TEXT("shot");
			Step.bQuit = Args.Num() > 1 && Args[1] == TEXT("quit");
			Queue(Step);
		}));

	FAutoConsoleCommand TreeCommand(TEXT("paw.Tree"), TEXT("paw.Tree [spend]: open the skill tree (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Tree;
			Step.bSpend = Args.Num() > 0 && Args[0] == TEXT("spend");
			Queue(Step);
		}));

	FAutoConsoleCommand HandCommand(TEXT("paw.Hand"), TEXT("paw.Hand: deal a card hand (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>&)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Hand;
			Queue(Step);
		}));

	FAutoConsoleCommand ReawakenCommand(TEXT("paw.Reawaken"), TEXT("paw.Reawaken [jobs]: the HUD's Reawaken on a run made worth it (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Reawaken;
			Step.bJobs = Args.Num() > 0 && Args[0] == TEXT("jobs");
			Queue(Step);
		}));

	FAutoConsoleCommand LeaveCommand(TEXT("paw.Leave"), TEXT("paw.Leave: leave the spend step (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>&)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Leave;
			Queue(Step);
		}));

	FAutoConsoleCommand VictoryCommand(TEXT("paw.Victory"), TEXT("paw.Victory [seconds]: the victory screen (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Victory;
			Step.Seconds = Args.Num() > 0 ? FCString::Atod(*Args[0]) : 4200.0;
			Queue(Step);
		}));

	FAutoConsoleCommand WaitCommand(TEXT("paw.Wait"), TEXT("paw.Wait <frames>: hold the tour queue (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Wait;
			Step.Frames = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 30;
			Queue(Step);
		}));

	FAutoConsoleCommand TakeCommand(TEXT("paw.Take"), TEXT("paw.Take: take the hand's first card (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>&)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Take;
			Queue(Step);
		}));

	FAutoConsoleCommand TabCommand(TEXT("paw.Tab"), TEXT("paw.Tab <1-6>: open a dock tab on the HUD (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Tab = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1;
			Queue(Step);
		}));
}

#endif
