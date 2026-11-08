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
//   paw.Title <press> [n]    a press on the title screen: new, load, continue, leaderboard, credits, back, slot <n>
//                            (0-based; an empty slot starts a game there, an occupied one loads it; both open L_Realm)
//   paw.Press <key> [device] a key or pad button pressed and let go THROUGH SLATE (the device subsystem sees it, focus moves
//                            on it): Gamepad_DPad_Down 1 is pad 1's D-pad (the prompts turn to the pad's)
//   paw.Prompts <style>      Settings > Controls > Button prompts (auto, keyboard, xbox, playstation, steam_deck)
//   paw.Settings [section]   open the settings dialog on its page: audio, video, controls (the title or the HUD's Modal stack)
//   paw.Cat <x> <y>          put the cat at field pixel (x, y), the stick claiming the cursor so the OS pointer does not move it
//   paw.Boss                 bring the biome's boss out now (as the bait does; the look tours, phase 6)
//   paw.Travel <n>           travel to biome n (1-based, tier_n), as the Realms page does (the backdrop tours, phase 6)
//   paw.Buy <branch>         in the spend step, press a node as a click does (the Unicorn's ceremony tour, phase 6)
//   paw.FontSheet <culture>  the font check (restyle R2): the culture made current, and over the screen the lines of
//                            Saved/PawShots/fontsheet_<culture>.txt (UTF-8) in F_Paw, bone on plum: the first in Black, the rest
//                            in the text weight; `paw.FontSheet off` takes it away (tools/godot_tour.py --set fonts)
//   paw.Fx                   one of each effect round the cat: a burst, a slam, a swipe and a crit swipe, a beam, shards
//                            (the effect looks, phase 6 slice 3; the effects setting decides as in play)
// e.g. -ExecCmds="paw.Cheat.Catnip 300,paw.Tab 3,paw.Shot realms,paw.Tab 2,paw.Shot reawaken quit"

#include "Containers/Ticker.h"
#include "Core/PawLog.h"
#include "Core/PawPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/UserInterfaceSettings.h"
#include "Save/PawSettingsSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "Core/PawGameSubsystem.h"
#include "PcCardDraw.h"
#include "PcData.h"
#include "PcHud.h"
#include "Realm/PawRealmSubsystem.h"
#include "Realm/PawRealmView.h"
#include "EngineUtils.h"
#include "Input/PawInputDeviceSubsystem.h"
#include "PcField.h"
#include "PcProgress.h"
#include "PcFieldRules.h"
#include "PcEconomy.h"
#include "UI/PawCardPickScreen.h"
#include "UI/PawHudScreen.h"
#include "UI/PawMainMenu.h"
#include "UI/PawRootLayout.h"
#include "UI/PawSettingsDialog.h"
#include "UI/PawSkillTreeScreen.h"
#include "UnrealClient.h"
#include "Internationalization/Internationalization.h"
#include "Engine/Font.h"
#include "Misc/FileHelper.h"
#include "Styling/CoreStyle.h"
#include "UI/PawTokens.h"
#include "UI/PawWidgets.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

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
		Title,
		Press,
		Prompts,
		Settings,
		Cat,
		Boss,
		Travel,
		Fx,
		Buy,
		FontSheet,
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
		FString Press;
		int32 Arg = 0;
		double X = 0.0, Y = 0.0;
	};
	TArray<FStep> GQueue;
	int32 GFramesLeft = 0;
	int32 GQuitIn = -1;
	FTSTicker::FDelegateHandle GHandle;
	TSharedPtr<SWidget> GFontSheet;

	void ShowFontSheet(const FString& Culture)
	{
		UGameViewportClient* Viewport = GEngine ? GEngine->GameViewport : nullptr;
		if (!Viewport)
		{
			return;
		}
		if (GFontSheet.IsValid())
		{
			Viewport->RemoveViewportWidgetContent(GFontSheet.ToSharedRef());
			GFontSheet.Reset();
		}
		if (Culture == TEXT("off"))
		{
			return;
		}
		if (!FInternationalization::Get().SetCurrentCulture(Culture))
		{
			UE_LOG(LogPaw, Warning, TEXT("paw.FontSheet: no culture %s"), *Culture);
		}
		TArray<FString> Lines;
		FFileHelper::LoadFileToStringArray(Lines, *(FPaths::ProjectSavedDir() / TEXT("PawShots") / (TEXT("fontsheet_") + Culture + TEXT(".txt"))));
		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
		for (int32 I = 0; I < Lines.Num(); ++I)
		{
			const bool bTitle = I == 0;
			Box->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, bTitle ? 14.0f : 6.0f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(Lines[I]))
					.Font(FSlateFontInfo(PawUi::PawFont(), bTitle ? 28 : 17, bTitle ? TEXT("Black") : TEXT("Regular")))
					.ColorAndOpacity(FSlateColor(bTitle ? PawTokens::PLUM_BUTTER : PawTokens::PLUM_BONE))
					.AutoWrapText(true)
			];
		}
		GFontSheet = SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FSlateColor(PawTokens::PLUM_BASE))
			.Padding(FMargin(48.0f, 40.0f))
			[
				Box
			];
		Viewport->AddViewportWidgetContent(GFontSheet.ToSharedRef(), 1000);
	}

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
		if (Step.Kind == EStep::FontSheet)
		{
			ShowFontSheet(Step.Press);
			return true;
		}
		if (Step.Kind == EStep::Press)
		{
			const FKey Key(*Step.Press);
			if (!Key.IsValid() || !FSlateApplication::IsInitialized())
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Press: no such key '%s'"), *Step.Press);
				return true;
			}
			const FKeyEvent Event(Key, FModifierKeysState(), FInputDeviceId::CreateFromInternalId(Step.Arg), false, 0, 0, TOptional<int32>(0)); // Slate user 0: the one player
			FSlateApplication::Get().ProcessKeyDownEvent(Event);
			FSlateApplication::Get().ProcessKeyUpEvent(Event);
			return true;
		}
		if (Step.Kind == EStep::Prompts)
		{
			UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
			UPawSettingsSubsystem* S = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
			if (S)
			{
				S->SetPromptStyle(Step.Press);
			}
			return true;
		}
		if (Step.Kind == EStep::Settings)
		{
			UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
			APawPlayerController* PC = World ? Cast<APawPlayerController>(World->GetFirstPlayerController()) : nullptr;
			UPawRootLayout* Root = PC ? PC->GetRootLayout() : nullptr;
			if (UPawSettingsDialog* D = Root ? UPawSettingsDialog::Open(Root->ModalStack, nullptr) : nullptr)
			{
				D->PressSection(Step.Press == TEXT("video") ? Pc::PcSettingsSection::Video
					: Step.Press == TEXT("controls") ? Pc::PcSettingsSection::Controls : Pc::PcSettingsSection::Audio);
			}
			return true;
		}
		if (Step.Kind == EStep::Title)
		{
			UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
			APawPlayerController* PC = World ? Cast<APawPlayerController>(World->GetFirstPlayerController()) : nullptr;
			UPawMainMenu* M = PC ? PC->GetMainMenu() : nullptr;
			if (!M)
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Title: no title screen"));
				return true;
			}
			const FString& P = Step.Press;
			if (P == TEXT("new")) { M->PressNewGame(); }
			else if (P == TEXT("load")) { M->PressLoad(); }
			else if (P == TEXT("continue")) { M->PressContinue(); }
			else if (P == TEXT("leaderboard")) { M->PressLeaderboard(); }
			else if (P == TEXT("credits")) { M->PressCredits(); }
			else if (P == TEXT("back")) { M->Back(); }
			else if (P == TEXT("slot")) { M->PressSlot(Step.Arg); }
			else { UE_LOG(LogPaw, Warning, TEXT("paw.Title: no such press '%s'"), *P); }
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
		if (Step.Kind == EStep::Buy)
		{
			UPawHudScreen* H = Hud();
			UPawSkillTreeScreen* Tree = H ? H->GetSkillTree() : nullptr;
			if (!Tree || !Tree->PressNode(Step.Press))
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Buy %s: nothing bought"), *Step.Press);
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
		if (Step.Kind == EStep::Fx)
		{
			UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
			APawRealmView* View = nullptr;
			for (TActorIterator<APawRealmView> It(World); It && !View; ++It)
			{
				View = *It;
			}
			FPawVfx::FContext C;
			if (!View || !View->MakeVfxContext(C, 0.0))
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Fx: no realm view"));
				return true;
			}
			FPawVfx& V = View->GetVfxForTest();
			const FVector2D Cat = C.Cat;
			V.Burst(C, Cat + FVector2D(-260.0, -160.0), FLinearColor(0.55f, 0.82f, 1.0f, 0.9f), 120.0, 1.2, 4.0);
			V.Slam(C, Cat + FVector2D(0.0, 200.0), 160.0, false);
			V.Swipe(C, Cat, Cat + FVector2D(140.0, -60.0), 62.0, false, false);
			V.Swipe(C, Cat + FVector2D(-300.0, 120.0), Cat + FVector2D(-180.0, 140.0), 62.0, true, true);
			V.Beam(C, Cat + FVector2D(200.0, 100.0), Cat + FVector2D(420.0, -60.0), FLinearColor(0.95f, 0.75f, 1.0f, 0.95f), 7.0, 1.2);
			V.Shards(C, Cat + FVector2D(260.0, -200.0), FLinearColor(1.0f, 0.86f, 0.32f, 1.0f), 26, 340.0, 1.1, 0.0, 16.0, true);
			return true;
		}
		if (Step.Kind == EStep::Travel)
		{
			UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
			if (UPawRealmSubsystem* R = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr; !R || !R->Travel(Step.Arg - 1))
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Travel %d: no such biome"), Step.Arg);
			}
			return true;
		}
		if (Step.Kind == EStep::Cat || Step.Kind == EStep::Boss)
		{
			UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
			UPawRealmSubsystem* R = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
			UPawGameSubsystem* G = R ? R->GetGame() : nullptr;
			if (!G || !G->IsDataReady())
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Cat / paw.Boss: no realm"));
				return true;
			}
			if (Step.Kind == EStep::Cat)
			{
				if (UPawInputDeviceSubsystem* Device = World->GetGameInstance()->GetSubsystem<UPawInputDeviceSubsystem>())
				{
					Device->ClaimStick();
				}
				R->SetCat(Step.X, Step.Y);
			}
			else
			{
				// The first boss realm when this biome has none; the bait topped up as the cheat overlay would, then a real
				// summon.
				const Pc::PcGameData& D = G->Data();
				Pc::PcProgress& P = G->Progress();
				const Pc::PcField& F = R->GetSim().Field;
				if (F.TierIndex < 0 || D.Tiers[static_cast<size_t>(F.TierIndex)].BossData.empty())
				{
					R->Travel(static_cast<int32>(Pc::RealmIndex(D, D.Code.BossRealms.front())));
				}
				const Pc::PcBoss* B = F.TierIndex >= 0 ? D.FindBoss(D.Tiers[static_cast<size_t>(F.TierIndex)].BossData) : nullptr;
				if (B)
				{
					P.TierKillCount += Pc::BaitShortfall(D, P, *B) * B->KillsPerBait;
					P.Catnip = FMath::Max(P.Catnip, 1e15);
				}
				if (!B || !Pc::SummonFieldBoss(R->GetSim().Field, D, P))
				{
					UE_LOG(LogPaw, Warning, TEXT("paw.Boss: the boss did not come out"));
				}
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

	FAutoConsoleCommand CatCommand(TEXT("paw.Cat"), TEXT("paw.Cat <x> <y>: put the cat at a field pixel (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Cat;
			Step.X = Args.Num() > 0 ? FCString::Atod(*Args[0]) : 1200.0;
			Step.Y = Args.Num() > 1 ? FCString::Atod(*Args[1]) : 540.0;
			Queue(Step);
		}));

	FAutoConsoleCommand FxCommand(TEXT("paw.Fx"), TEXT("paw.Fx: one of each effect round the cat (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>&)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Fx;
			Queue(Step);
		}));

	FAutoConsoleCommand TravelCommand(TEXT("paw.Travel"), TEXT("paw.Travel <n>: travel to biome n, 1-based (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Travel;
			Step.Arg = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1;
			Queue(Step);
		}));

	FAutoConsoleCommand BossCommand(TEXT("paw.Boss"), TEXT("paw.Boss: bring the boss out (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>&)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Boss;
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

	FAutoConsoleCommand TitleCommand(TEXT("paw.Title"), TEXT("paw.Title <press> [n]: a press on the title screen (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Title;
			Step.Press = Args.Num() > 0 ? Args[0] : FString();
			Step.Arg = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
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

	FAutoConsoleCommand BuyCommand(TEXT("paw.Buy"), TEXT("paw.Buy <branch>: press a node of the spend step's tree (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Buy;
			Step.Press = Args.Num() > 0 ? Args[0] : FString(TEXT("unlock_unicorn"));
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

	FAutoConsoleCommand PressCommand(TEXT("paw.Press"), TEXT("paw.Press <key> [device]: a key pressed and let go through Slate (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Press;
			Step.Press = Args.Num() > 0 ? Args[0] : FString();
			Step.Arg = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
			Queue(Step);
		}));

	FAutoConsoleCommand PromptsCommand(TEXT("paw.Prompts"), TEXT("paw.Prompts <style>: Settings > Controls > Button prompts (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Prompts;
			Step.Press = Args.Num() > 0 ? Args[0] : FString(TEXT("auto"));
			Queue(Step);
		}));

	FAutoConsoleCommand SettingsCommand(TEXT("paw.Settings"), TEXT("paw.Settings [audio|video|controls]: open the settings dialog (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Settings;
			Step.Press = Args.Num() > 0 ? Args[0] : FString(TEXT("audio"));
			Queue(Step);
		}));

	FAutoConsoleCommand FontSheetCommand(TEXT("paw.FontSheet"), TEXT("paw.FontSheet <culture|off>: the culture's font sheet over the screen (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::FontSheet;
			Step.Press = Args.Num() > 0 ? Args[0] : FString(TEXT("off"));
			Queue(Step);
		}));

	// Restyle R4h: the UI at a size the game has no option for yet (phase 7): UMG's ApplicationScale, on top of the DPI
	// curve (FSlateApplication::SetApplicationScale does not reach the game viewport: measured, the shots identical).
	FAutoConsoleCommand UiScaleCommand(TEXT("paw.UiScale"), TEXT("paw.UiScale <scale>: UMG's application scale (1.5 = the 150 % check)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			GetMutableDefault<UUserInterfaceSettings>()->ApplicationScale = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 1.0f;
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
