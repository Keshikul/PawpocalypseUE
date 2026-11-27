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
//   paw.Title <press> [n]    a press on the title screen: new, load, continue, leaderboard, credits, back, slot <n>,
//                            language (the cluster's Language: its list opens or closes)
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
//                            in the text weight; `paw.FontSheet off` takes it away (tools/screen_tour.py --set fonts)
//   paw.Fx                   one of each effect round the cat: a burst, a slam, a swipe and a crit swipe, a beam, shards
//                            (the effect looks, phase 6 slice 3; the effects setting decides as in play)
//   paw.Fx look              the restyle's aura and strike (D3), held still for a shot: the field held, the aura owned, three
//                            swipes stepped to 25 %, 50 % (a crit) and 75 % of their sweep (a swipe lasts 0.24 s, shorter than
//                            the queue's beat); the first swipe from the cat. The field stays held (a dev shot; restart to play on).
// e.g. -ExecCmds="paw.Cheat.Catnip 300,paw.Tab 3,paw.Shot realms,paw.Tab 2,paw.Shot reawaken quit"
//
// Clips (the store's GIFs, clips and trailer: tools/store_clips.py, 2026-11-25). Run the game with -UseFixedTimeStep -FPS=60:
// every frame is then 1/60 s of game time however long its file takes to write, so the frames play back at real speed.
//   paw.Clip <name> <frames> [ui] [append]  record this many frames, one BMP each, into Saved/PawClips/<name>/f_00000.bmp ... while the
//                                  queue goes on (the steps after it play during the take). Without `ui` the field alone: no
//                                  widget, so no text (FScreenshotRequest's bShowUI); `ui`: the screen as the player sees it;
//                                  `append`: on after the take's last frame (a montage: set up, record, set up, record)
//   paw.Steer hunt|boss|off [px/s] the cat steered as a hand moves a mouse: eased toward the thickest crowd near it (hunt) or
//                                  the boss (boss), at most px/s (default 1100); the cursor and pad are not sampled meanwhile
//   paw.Pounce                     one Pounce press (down two frames, then up; the field fires on the press edge)
//   paw.Quit                       quit once the clip being recorded has every frame on disk
//   paw.AutoPick                   the tree's Auto Card owned and switched on: level ups take a card without holding the field
//                                  (a take without the widgets would otherwise stand still under a hand nobody sees)
//   paw.Camera <zoom>              the field's camera punched in this much (1 = the whole field, as the game draws it) and
//                                  easing after the cat, held inside the field; rendered, not cropped, so a close-up is as
//                                  sharp as the wide view
//   paw.Row <n>                    press row n (0-based) of the open dock page, as a click does (Upgrades: buy one)
//   paw.Ever <stardust>            the Stardust ever granted raised to this (the Reawaken multiplier: a cat that kills a boss in
//                                  seconds, for a take that shows the kill)
//   paw.Own <branch> [level]       a skill tree node owned at this level (default 1), the field's stats rebuilt (a take's aura)
//   paw.Zoom <delta>               the skill tree's zoom about the view's centre, as the wheel does (+0.3: closer)
//   paw.Dock off                   the playfield the whole window wide, as if there were no dock (a take without the widgets:
//                                  enemies, the cat and the kittens use the strip the dock covers); until the window's size changes

#include <algorithm>

#include "Camera/CameraComponent.h"
#include "Containers/Ticker.h"
#include "HAL/FileManager.h"
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
		Do,
		Clip,
		Steer,
		Pounce,
		Quit,
		Dock,
		Zoom,
		AutoPick,
		Own,
		Row,
		Ever,
		Camera,
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

	// The take being recorded (paw.Clip) and the hand on the cat (paw.Steer, paw.Pounce).
	struct FClip
	{
		FString Dir;
		int32 Left = 0;
		int32 Index = 0;
		bool bUi = false;
	};
	FClip GClip;
	enum class ESteerMode
	{
		Off,
		Hunt,
		Boss,
	};
	ESteerMode GSteer = ESteerMode::Off;
	double GSteerSpeed = 1100.0;
	FVector2D GSteerPos = FVector2D::ZeroVector, GSteerVel = FVector2D::ZeroVector, GSteerGoal = FVector2D::ZeroVector;
	int32 GSteerGoalIndex = -1;
	int32 GRetargetIn = 0;
	bool bSteerStarted = false;
	int32 GPounceFrames = 0;
	bool bPounceWasHeld = false;
	double GCamZoom = 1.0;
	bool bCamSet = false;
	FVector2D GCamPos = FVector2D::ZeroVector;

	UPawRealmSubsystem* RealmNow()
	{
		UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
		return World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
	}

	// The view samples the cursor and the pad every frame and would overwrite the steered cat and the held Pounce.
	void SampleInput(bool bOn)
	{
		UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
		for (TActorIterator<APawRealmView> It(World); It; ++It)
		{
			It->bSampleInput = bOn;
		}
	}

	// One frame of the take: the file numbered by the frame, the widgets in it only when asked. BMP, not PNG: the engine
	// writes by the extension (FImageUtils::SaveImageByExtension), and PNG's compression is what holds a take to ~3 frames a
	// second; tools/store_clips.py packs each take losslessly and deletes the frames.
	void RecordFrame()
	{
		if (GClip.Left <= 0)
		{
			return;
		}
		const FString Path = GClip.Dir / FString::Printf(TEXT("f_%05d.bmp"), GClip.Index++);
		FScreenshotRequest::RequestScreenshot(Path, GClip.bUi, false);
		if (--GClip.Left == 0)
		{
			UE_LOG(LogPaw, Display, TEXT("PAW_CLIP done %s (%d frames)"), *GClip.Dir, GClip.Index);
		}
	}

	// The goal a player's hand would pick: the live enemy with the most others round it, nearer ones first.
	int32 PickGoal(const Pc::PcField& F)
	{
		int32 Best = -1;
		double BestScore = -1.0;
		for (const int32 S : F.Live)
		{
			const size_t I = static_cast<size_t>(S);
			int32 Near = 0;
			for (const int32 T : F.Live)
			{
				const size_t J = static_cast<size_t>(T);
				if ((F.X[J] - F.X[I]) * (F.X[J] - F.X[I]) + (F.Y[J] - F.Y[I]) * (F.Y[J] - F.Y[I]) < 170.0 * 170.0)
				{
					++Near;
				}
			}
			const double Dist = FMath::Sqrt((F.X[I] - GSteerPos.X) * (F.X[I] - GSteerPos.X) + (F.Y[I] - GSteerPos.Y) * (F.Y[I] - GSteerPos.Y));
			const double Score = Near / (1.0 + Dist / 380.0);
			if (Score > BestScore)
			{
				BestScore = Score;
				Best = S;
			}
		}
		return Best;
	}

	// The punch-in. Measured 2026-11-25 (5.8.3): an ortho camera COMPONENT moved off its actor in X/Y does not pan: with the
	// automatic ortho planes on (the default), FSceneViewProjectionData::UpdateOrthoPlanes moves the view's origin onto the
	// camera's owner (ViewOrigin += CameraToViewTarget), so the view stays centred on the actor (the zoom takes, the pan
	// never does); and fixed planes (near 0, far 2000) dropped the cat's body (KB 2D/02: such planes clip above about Z 100).
	// So the punch-in is a rig of its own, built as the realm view's camera is (a root on the field's plane, the camera 1000
	// above it looking down, the same post-process: exact colours), with the auto planes left on, and the RIG is moved: the
	// engine centres the view on it. The auto planes shrink with the ortho width, and at 2x the cat's body (the highest
	// band) was cut while its shadow drew; the rig's planes are shifted toward the camera (measured at 2x: shift 0 cut it,
	// -300 and -800 drew everything, +300 drew nothing). Every sprite, the floor and the effects are world
	// geometry, so the close-up renders at the window's full resolution. The view stays inside the field: no edge of nothing.
	TWeakObjectPtr<AActor> GCamRig;

	void CameraFrame(double Dt)
	{
		if (!bCamSet)
		{
			return;
		}
		UPawRealmSubsystem* R = RealmNow();
		UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
		APawRealmView* View = nullptr;
		for (TActorIterator<APawRealmView> It(World); It && !View; ++It)
		{
			View = *It;
		}
		UCameraComponent* Game = View ? View->FindComponentByClass<UCameraComponent>() : nullptr;
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (!R || !R->IsRunning() || !Game || !PC)
		{
			return;
		}
		if (!GCamRig.IsValid())
		{
			AActor* Rig = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
			USceneComponent* Root = NewObject<USceneComponent>(Rig, TEXT("Root"));
			Root->SetMobility(EComponentMobility::Movable);
			Rig->SetRootComponent(Root);
			Root->RegisterComponent();
			UCameraComponent* Cam = NewObject<UCameraComponent>(Rig, TEXT("Camera"));
			Cam->SetupAttachment(Root);
			Cam->SetRelativeLocationAndRotation(Game->GetRelativeLocation(), Game->GetRelativeRotation());
			Cam->SetProjectionMode(Game->ProjectionMode);
			Cam->bOverrideAspectRatioAxisConstraint = Game->bOverrideAspectRatioAxisConstraint;
			Cam->SetAspectRatioAxisConstraint(Game->AspectRatioAxisConstraint);
			Cam->SetConstraintAspectRatio(Game->bConstrainAspectRatio);
			Cam->PostProcessSettings = Game->PostProcessSettings;
			Cam->PostProcessBlendWeight = Game->PostProcessBlendWeight;
			Cam->AutoPlaneShift = -600.0f;
			Cam->RegisterComponent();
			GCamRig = Rig;
		}
		AActor* Rig = GCamRig.Get();
		UCameraComponent* Cam = Rig->FindComponentByClass<UCameraComponent>();
		const double H = APawRealmView::LogicalHeight;
		const double W = View->GetLogicalWidth();
		const double Zoom = FMath::Max(GCamZoom, 1.0);
		const double HalfW = W * 0.5 / Zoom, HalfH = H * 0.5 / Zoom;
		const Pc::PcField& F = R->GetSim().Field;
		const FVector2D Goal(FMath::Clamp(F.CatX, HalfW, W - HalfW), FMath::Clamp(F.CatY, HalfH, H - HalfH));
		GCamPos = GCamPos.IsZero() ? Goal : GCamPos + (Goal - GCamPos) * (1.0 - FMath::Exp(-Dt * 2.5));
		Cam->SetOrthoWidth(static_cast<float>(Game->OrthoWidth / Zoom));
		// Field (x right, y down) -> world (screen up = +X, screen right = +Y), the field's centre at the view's origin
		// (UPawSpriteRenderer::FieldToWorld).
		Rig->SetActorLocation(View->GetActorLocation() + FVector(H * 0.5 - GCamPos.Y, GCamPos.X - W * 0.5, 0.0));
		if (PC->GetViewTarget() != Rig)
		{
			PC->SetViewTarget(Rig);
		}
	}

	// A hand on a mouse: a critically damped spring toward the goal, its speed capped, so the cat eases in and out of each
	// crowd instead of snapping to it (the cat IS the cursor: the field puts it wherever the input says).
	void SteerFrame(double Dt)
	{
		UPawRealmSubsystem* R = RealmNow();
		if (!R || !R->IsRunning())
		{
			return;
		}
		if (GPounceFrames > 0)
		{
			R->SetPounceDown(true);
			--GPounceFrames;
			bPounceWasHeld = true;
		}
		else if (bPounceWasHeld)
		{
			R->SetPounceDown(false);
			bPounceWasHeld = false;
		}
		if (GSteer == ESteerMode::Off)
		{
			return;
		}
		const Pc::PcField& F = R->GetSim().Field;
		// Once a game second: what the take shows (staging a take is tuning how strong the cat is against the field).
		static int32 FieldLogIn = 0;
		if (--FieldLogIn <= 0)
		{
			FieldLogIn = 60;
			UPawGameSubsystem* G = R->GetGame();
			UE_LOG(LogPaw, Display, TEXT("PAW_FIELD live %d kills %lld boss %s %.4g"), static_cast<int32>(F.Live.size()),
				G ? static_cast<long long>(G->Progress().LifetimeKills) : -1LL, F.bBossUp ? TEXT("up") : TEXT("-"), F.BossHp);
		}
		if (!bSteerStarted)
		{
			bSteerStarted = true;
			GSteerPos = FVector2D(F.CatX, F.CatY);
			GSteerVel = FVector2D::ZeroVector;
			GSteerGoal = GSteerPos;
			GSteerGoalIndex = -1;
			GRetargetIn = 0;
		}
		if (GSteer == ESteerMode::Boss && F.bBossUp)
		{
			// Beside the boss, not on it: a cat sitting on the boss hid it for the whole fight (2026-11-25).
			GSteerGoal = FVector2D(F.BossX - 95.0, F.BossY + 30.0);
		}
		else
		{
			const bool bGoalAlive = GSteerGoalIndex >= 0 && std::find(F.Live.begin(), F.Live.end(), GSteerGoalIndex) != F.Live.end();
			if (bGoalAlive)
			{
				const size_t I = static_cast<size_t>(GSteerGoalIndex);
				GSteerGoal = FVector2D(F.X[I], F.Y[I]);
			}
			if (!bGoalAlive || --GRetargetIn <= 0 || FVector2D::Distance(GSteerGoal, GSteerPos) < 24.0)
			{
				GSteerGoalIndex = PickGoal(F);
				GRetargetIn = 24;
				if (GSteerGoalIndex >= 0)
				{
					const size_t I = static_cast<size_t>(GSteerGoalIndex);
					GSteerGoal = FVector2D(F.X[I], F.Y[I]);
				}
			}
		}
		GSteerGoal.X = FMath::Clamp(GSteerGoal.X, F.Bounds.X + 40.0, F.Bounds.X + F.Bounds.W - 40.0);
		GSteerGoal.Y = FMath::Clamp(GSteerGoal.Y, F.Bounds.Y + 40.0, F.Bounds.Y + F.Bounds.H - 40.0);
		constexpr double K = 38.0;
		const double C = 2.0 * FMath::Sqrt(K);
		GSteerVel += (K * (GSteerGoal - GSteerPos) - C * GSteerVel) * Dt;
		if (GSteerVel.Size() > GSteerSpeed)
		{
			GSteerVel = GSteerVel.GetSafeNormal() * GSteerSpeed;
		}
		GSteerPos += GSteerVel * Dt;
		R->SetCat(GSteerPos.X, GSteerPos.Y);
	}

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
					.ColorAndOpacity(FSlateColor(bTitle ? PawTokens::UI_TEXT : PawTokens::UI_TEXT_MUTED))
					.AutoWrapText(true)
			];
		}
		GFontSheet = SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FSlateColor(PawTokens::UI_PANEL))
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

	bool Tick(float DeltaTime)
	{
		RecordFrame();
		SteerFrame(DeltaTime);
		CameraFrame(DeltaTime);
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
		// Every step in the log (a take that never starts is then plain to see); paw.Quit's wait for the take, once.
		if (Step.Kind != EStep::Quit || GClip.Left <= 0 || !Step.bQuit)
		{
			UE_LOG(LogPaw, Display, TEXT("PAW_STEP frame %llu: %d %s%s (%d left)"), static_cast<unsigned long long>(GFrameCounter), static_cast<int32>(Step.Kind), *Step.Name, *Step.Press, GQueue.Num());
		}
		if (Step.Kind == EStep::Wait)
		{
			GFramesLeft = FMath::Max(0, Step.Frames);
			return true;
		}
		if (Step.Kind == EStep::Clip)
		{
			// The steps after it are the take's action: no beat before the next one.
			const FString Dir = FPaths::ProjectSavedDir() / TEXT("PawClips") / Step.Name;
			if (!Step.bJobs || Dir != GClip.Dir)
			{
				IFileManager::Get().DeleteDirectory(*Dir, false, true); // never an old take's frames among the new
				IFileManager::Get().MakeDirectory(*Dir, true);
				GClip.Index = 0;
			}
			GClip.Dir = Dir;
			GClip.Left = FMath::Max(1, Step.Frames);
			GClip.bUi = Step.bSpend;
			GFramesLeft = 0;
			UE_LOG(LogPaw, Display, TEXT("PAW_CLIP start %s (%d frames%s)"), *GClip.Dir, GClip.Left, GClip.bUi ? TEXT(", the widgets too") : TEXT(""));
			return true;
		}
		if (Step.Kind == EStep::Steer)
		{
			GSteer = Step.Press == TEXT("hunt") ? ESteerMode::Hunt : Step.Press == TEXT("boss") ? ESteerMode::Boss : ESteerMode::Off;
			GSteerSpeed = Step.X > 0.0 ? Step.X : 1100.0;
			bSteerStarted = false;
			if (GSteer != ESteerMode::Off)
			{
				if (UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr)
				{
					if (UPawInputDeviceSubsystem* Device = World->GetGameInstance()->GetSubsystem<UPawInputDeviceSubsystem>())
					{
						Device->ClaimStick(); // when the view samples again, the stick holds the cat where the hand left it
					}
				}
			}
			SampleInput(GSteer == ESteerMode::Off);
			GFramesLeft = 0;
			return true;
		}
		if (Step.Kind == EStep::Pounce)
		{
			SampleInput(false); // the view would let go of the press on its next frame
			GPounceFrames = 2;
			GFramesLeft = 0;
			return true;
		}
		if (Step.Kind == EStep::Row)
		{
			if (UPawHudScreen* H = Hud())
			{
				H->PressRow(Step.Arg);
			}
			else
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Row: no HUD"));
			}
			return true;
		}
		if (Step.Kind == EStep::Camera)
		{
			GCamZoom = FMath::Max(Step.X, 1.0);
			bCamSet = true;
			SampleInput(false); // the view would take the view target back every frame it samples
			GCamPos = FVector2D::ZeroVector; // the first frame starts on the cat
			return true;
		}
		if (Step.Kind == EStep::Ever)
		{
			UPawRealmSubsystem* R = RealmNow();
			UPawGameSubsystem* G = R ? R->GetGame() : nullptr;
			if (!G || !G->IsDataReady())
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Ever: no realm"));
				return true;
			}
			G->Progress().StardustGrantedEver = FMath::Max(G->Progress().StardustGrantedEver, Step.X);
			R->OnProgressChanged(false);
			return true;
		}
		if (Step.Kind == EStep::Own)
		{
			UPawRealmSubsystem* R = RealmNow();
			UPawGameSubsystem* G = R ? R->GetGame() : nullptr;
			if (!G || !G->IsDataReady())
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Own: no realm"));
				return true;
			}
			G->Progress().BranchLevels[TCHAR_TO_UTF8(*Step.Press)] = FMath::Max(Step.Arg, 1);
			R->OnProgressChanged(true);
			return true;
		}
		if (Step.Kind == EStep::AutoPick)
		{
			UPawRealmSubsystem* R = RealmNow();
			UPawGameSubsystem* G = R ? R->GetGame() : nullptr;
			if (!G || !G->IsDataReady())
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.AutoPick: no realm"));
				return true;
			}
			int64_t& Level = G->Progress().BranchLevels["auto_card"];
			Level = FMath::Max<int64_t>(Level, 1);
			R->SetAutoPick(true);
			return true;
		}
		if (Step.Kind == EStep::Zoom)
		{
			UPawHudScreen* H = Hud();
			if (UPawSkillTreeScreen* Tree = H ? H->GetSkillTree() : nullptr; Tree && Tree->IsActivated())
			{
				Tree->ZoomBy(Step.X);
			}
			else
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Zoom: no tree up"));
			}
			return true;
		}
		if (Step.Kind == EStep::Dock)
		{
			// The HUD reports its dock only when the window's size changes (UPawHudScreen::ReportDockForSize).
			if (UPawRealmSubsystem* R = RealmNow())
			{
				R->SetDock(Pc::PcRect{}, false);
			}
			return true;
		}
		if (Step.Kind == EStep::Quit)
		{
			if (GClip.Left > 0)
			{
				FStep Again = Step;
				Again.bQuit = true; // logged once
				GQueue.Insert(Again, 0); // the take is still being written
				GFramesLeft = 0;
				return true;
			}
			GQuitIn = 30; // let the last file land
			return true;
		}
		if (Step.Kind == EStep::FontSheet)
		{
			ShowFontSheet(Step.Press);
			return true;
		}
		if (Step.Kind == EStep::Do)
		{
			// Any console command, in the queue's order (a cheat between two shots: tools/store_shots.py).
			UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
			if (!GEngine || !GEngine->Exec(World, *Step.Press))
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Do: '%s' was not run"), *Step.Press);
			}
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
			else if (P == TEXT("language")) { M->PressCluster(2); }
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
			if (Step.Arg == 1)
			{
				UPawRealmSubsystem* R = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
				UPawGameSubsystem* G = R ? R->GetGame() : nullptr;
				if (!G)
				{
					UE_LOG(LogPaw, Warning, TEXT("paw.Fx look: no realm"));
					return true;
				}
				R->SetPaused(true);
				G->Progress().BranchLevels["unlock_aura"] = 1;
				auto Advance = [&V, View](double Seconds)
				{
					FPawVfx::FContext T;
					View->MakeVfxContext(T, Seconds);
					V.Tick(T);
				};
				Advance(0.2); // the aura's poll
				V.Swipe(C, Cat + FVector2D(-420.0, 180.0), Cat + FVector2D(-360.0, 160.0), 62.0, false, false);
				Advance(0.06);
				V.Swipe(C, Cat + FVector2D(380.0, 200.0), Cat + FVector2D(320.0, 180.0), 62.0, true, false);
				Advance(0.06);
				V.Swipe(C, Cat, Cat + FVector2D(-55.0, -20.0), 62.0, false, false);
				Advance(0.06);
				return true;
			}
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
					// D17 E2: the Bait is a bank per biome (BaitKillsByBiome), not the wave's kill count (found 2026-11-18:
					// topping up TierKillCount left the shortfall as it was, and the boss never came out).
					P.BaitKillsByBiome[P.CurrentBiomeId] += Pc::BaitShortfall(D, P, *B) * B->KillsPerBait;
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
				T->AnswerLeave(true); // P4.7: the leave question's Leave
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

	FAutoConsoleCommand FxCommand(TEXT("paw.Fx"), TEXT("paw.Fx [look]: one of each effect round the cat; look: the aura and three strikes held still (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Fx;
			Step.Arg = Args.Num() > 0 && Args[0] == TEXT("look") ? 1 : 0;
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

	FAutoConsoleCommand DoCommand(TEXT("paw.Do"), TEXT("paw.Do <command...>: run a console command when the tour queue reaches it (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Do;
			Step.Press = FString::Join(Args, TEXT(" "));
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

	FAutoConsoleCommand ClipCommand(TEXT("paw.Clip"), TEXT("paw.Clip <name> <frames> [ui]: record frames to Saved/PawClips/<name>/ (no widgets unless ui; queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Clip;
			Step.Name = Args.Num() > 0 ? Args[0] : TEXT("clip");
			Step.Frames = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 600;
			Step.bSpend = Args.Contains(TEXT("ui"));
			Step.bJobs = Args.Contains(TEXT("append"));
			Queue(Step);
		}));

	FAutoConsoleCommand SteerCommand(TEXT("paw.Steer"), TEXT("paw.Steer hunt|boss|off [px/s]: steer the cat as a hand would (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Steer;
			Step.Press = Args.Num() > 0 ? Args[0] : FString(TEXT("hunt"));
			Step.X = Args.Num() > 1 ? FCString::Atod(*Args[1]) : 0.0;
			Queue(Step);
		}));

	FAutoConsoleCommand PounceCommand(TEXT("paw.Pounce"), TEXT("paw.Pounce: one Pounce press (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>&)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Pounce;
			Queue(Step);
		}));

	FAutoConsoleCommand RowCommand(TEXT("paw.Row"), TEXT("paw.Row <n>: press a row of the open dock page (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Row;
			Step.Arg = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
			Queue(Step);
		}));

	FAutoConsoleCommand CameraCommand(TEXT("paw.Camera"), TEXT("paw.Camera <zoom>: the field's camera punched in, after the cat (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Camera;
			Step.X = Args.Num() > 0 ? FCString::Atod(*Args[0]) : 1.6;
			Queue(Step);
		}));

	FAutoConsoleCommand EverCommand(TEXT("paw.Ever"), TEXT("paw.Ever <stardust>: the Stardust ever granted raised to this (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Ever;
			Step.X = Args.Num() > 0 ? FCString::Atod(*Args[0]) : 1e6;
			Queue(Step);
		}));

	FAutoConsoleCommand OwnCommand(TEXT("paw.Own"), TEXT("paw.Own <branch> [level]: a tree node owned, the field's stats rebuilt (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Own;
			Step.Press = Args.Num() > 0 ? Args[0] : FString(TEXT("unlock_aura"));
			Step.Arg = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 1;
			Queue(Step);
		}));

	FAutoConsoleCommand AutoPickCommand(TEXT("paw.AutoPick"), TEXT("paw.AutoPick: Auto Card owned and on (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>&)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::AutoPick;
			Queue(Step);
		}));

	FAutoConsoleCommand ZoomCommand(TEXT("paw.Zoom"), TEXT("paw.Zoom <delta>: the skill tree's zoom (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Zoom;
			Step.X = Args.Num() > 0 ? FCString::Atod(*Args[0]) : 0.3;
			Queue(Step);
		}));

	FAutoConsoleCommand DockCommand(TEXT("paw.Dock"), TEXT("paw.Dock off: the playfield the whole window wide (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>&)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Dock;
			Queue(Step);
		}));

	FAutoConsoleCommand QuitCommand(TEXT("paw.Quit"), TEXT("paw.Quit: quit once the clip being recorded is on disk (queued with paw.Shot)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>&)
		{
			FStep Step;
			Step.bShot = false;
			Step.Kind = EStep::Quit;
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
