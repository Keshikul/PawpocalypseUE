// The packaged-build frame gate (docs/09 L5, docs/11 phase 2 exit): `paw.Bench [seconds]`, development builds only.
//
// Fills the running realm to the live cap (the flood: Infestation held at 1000, 20 kittens), lets the cat hunt the nearest
// mouse, and measures for N seconds (20 by default): the real frame, the game thread, the render thread, the GPU, the draw
// calls, and the view's own steps + draw. It checks the cooked L_Realm draws what is live (every body, its shadow, the floor,
// the four materials loaded), takes a screenshot halfway, writes Saved/PawBench/report.txt, logs PAW_BENCH lines, and quits.
// Exit verdict in the log: "PAW_BENCH PASS" or "PAW_BENCH FAIL <reasons>".
//
// Waits for a running realm, so it can be given on the command line before the map has loaded:
//   Pawpocalypse.exe /Game/Pawpocalypse/Maps/L_Realm -windowed -ResX=1280 -ResY=800 -ExecCmds="r.VSync 0,t.MaxFPS 0,paw.Bench 20"
//
// Budgets (docs/04 §9): the game thread under 4 ms at peak on the dev PC (judged at p99), under 50 draw calls for the field.
// The GPU is reported, not judged (the Deck-class budget is phase 8's, on a limited GPU).

#include "Containers/Ticker.h"
#include "Core/PawGameSubsystem.h"
#include "Core/PawLog.h"
#include "DynamicRHI.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PcData.h"
#include "PcField.h"
#include "PcProgress.h"
#include "RHIStats.h"
#include "Realm/PawRealmSubsystem.h"
#include "Realm/PawRealmView.h"
#include "Render/PawSpriteRenderer.h"
#include "RenderCore.h"
#include "UnrealClient.h"

#if !UE_BUILD_SHIPPING

namespace
{
	constexpr double GameThreadBudgetMs = 4.0; // docs/04 §9, at peak on the dev PC
	constexpr double DrawCallBudget = 50.0;    // docs/04 §9, the field at peak
	constexpr double WarmupLimitSec = 30.0;    // the field must fill by then

	double Mean(const TArray<double>& V)
	{
		double S = 0.0;
		for (const double X : V)
		{
			S += X;
		}
		return V.Num() ? S / V.Num() : 0.0;
	}

	double Percentile(TArray<double> V, double Q)
	{
		if (V.Num() == 0)
		{
			return 0.0;
		}
		V.Sort();
		return V[FMath::Clamp(static_cast<int32>(FMath::CeilToDouble(Q * V.Num())) - 1, 0, V.Num() - 1)];
	}

	struct FBench
	{
		double Seconds = 20.0;
		double Warmup = 0.0, Measured = 0.0;
		bool bMeasuring = false, bShot = false, bDone = false;
		int32 QuitIn = -1;
		int64 MinLive = MAX_int64, DrawMismatches = 0, ShadowMismatches = 0, FloorMismatches = 0;
		int64 HeldFrames = 0; // measured frames the field was held (a screen up): a held field measures nothing
		TArray<double> FrameMs, GameMs, RenderMs, GpuMs, DrawCalls, StepsMs, DrawMs;
		FTSTicker::FDelegateHandle Handle;

		static UWorld* GameWorld()
		{
			return GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
		}

		bool Tick(float DeltaSeconds)
		{
			if (QuitIn >= 0)
			{
				if (QuitIn-- == 0)
				{
					FPlatformMisc::RequestExit(false, TEXT("paw.Bench"));
				}
				return true;
			}
			UWorld* World = GameWorld();
			UPawRealmSubsystem* Realm = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
			UPawGameSubsystem* Game = Realm ? Realm->GetGame() : nullptr;
			APawRealmView* View = nullptr;
			if (World)
			{
				for (TActorIterator<APawRealmView> It(World); It; ++It)
				{
					View = *It;
					break;
				}
			}
			if (!Realm || !Game || !View || !Realm->IsRunning())
			{
				return true; // the map is still loading
			}
			const Pc::PcGameData& D = Game->Data();
			Pc::PcProgress& P = Game->Progress();
			const Pc::PcField& F = Realm->GetSim().Field;
			if (Warmup == 0.0)
			{
				View->bSampleInput = false; // the bench drives the cat
				P.UpgradeLevels["kitten_count"] = 20;
				P.UpgradeLevels["paw_damage"] = 30;
				// Level-ups deal card hands, and a hand on screen holds the field (the card pick, phase 3): Auto-Card takes
				// each at once, as a player with the node and the switch on has it, so the field never stops.
				P.BranchLevels["auto_card"] = FMath::Max<int64>(P.BranchLevels["auto_card"], 1);
				Realm->SetAutoPick(true);
				Realm->OnProgressChanged(true);
				UE_LOG(LogPaw, Display, TEXT("PAW_BENCH started: %s, %.0f s after the field fills"), *GRHIAdapterName, Seconds);
			}
			P.InfestationBonus = 1000.0; // held: it decays between kills

			// The cat on the nearest mouse (the soak's hunter): the paw strikes, kills come, effects follow.
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

			if (!bMeasuring)
			{
				Warmup += DeltaSeconds;
				if (F.LiveCount() >= D.Code.MaxLiveEnemies - 5 || Warmup > WarmupLimitSec)
				{
					bMeasuring = true;
					UE_LOG(LogPaw, Display, TEXT("PAW_BENCH measuring from %lld mice after %.1f s"), F.LiveCount(), Warmup);
				}
				return true;
			}

			// The previous frame's numbers (the engine's thread and GPU times trail by a frame).
			FrameMs.Add(DeltaSeconds * 1000.0);
			GameMs.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));
			RenderMs.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime));
			GpuMs.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));
			DrawCalls.Add(GNumDrawCallsRHI[0]);
			StepsMs.Add(Realm->GetSim().LastStepsMs);
			DrawMs.Add(View->LastDrawMs());
			MinLive = FMath::Min<int64>(MinLive, F.LiveCount());
			HeldFrames += Realm->IsPaused() ? 1 : 0;

			// What is drawn is what is live (the view drew this frame's field before this ticker ran).
			const UPawSpriteRenderer* R = View->GetRenderer();
			const int32 Bodies = static_cast<int32>(F.LiveCount() + F.AwakeKittens() + static_cast<int64>(F.Adds.size())) + (F.bBossUp ? 1 : 0) + 1;
			DrawMismatches += R && R->DrawnCount(EPawLayer::Bodies) == Bodies ? 0 : 1;
			// A shadow under each that stands: not the boss, not its adds (boss.tscn, boss_add.gd).
			ShadowMismatches += R && R->DrawnCount(EPawLayer::Shadows) == static_cast<int32>(F.LiveCount() + F.AwakeKittens()) + 1 ? 0 : 1;
			FloorMismatches += R && R->DrawnCount(EPawLayer::Floor) == 1 ? 0 : 1;

			Measured += DeltaSeconds;
			if (!bShot && Measured > Seconds * 0.5)
			{
				bShot = true;
				FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("PawBench/bench.png"), true, false);
			}
			if (Measured >= Seconds)
			{
				Report(*View, F, D);
				QuitIn = 30; // let the screenshot land
			}
			return true;
		}

		void Report(const APawRealmView& View, const Pc::PcField& F, const Pc::PcGameData& D)
		{
			TArray<FString> Fail;
			if (GameMs.Num() == 0)
			{
				Fail.Add(TEXT("nothing measured"));
			}
			if (MinLive < D.Code.MaxLiveEnemies - 40)
			{
				Fail.Add(FString::Printf(TEXT("the field was not full (min %lld mice)"), MinLive));
			}
			if (DrawMismatches || ShadowMismatches || FloorMismatches)
			{
				Fail.Add(FString::Printf(TEXT("drawn != live on %lld body / %lld shadow / %lld floor frames"), DrawMismatches, ShadowMismatches, FloorMismatches));
			}
			if (!View.FlatMaterial || !View.SoftMaterial || !View.DiscMaterial || !View.RingMaterial || !View.QuadMesh)
			{
				Fail.Add(TEXT("a field material or the quad did not cook"));
			}
			if (!View.GetRenderer() || !View.GetRenderer()->HasSprites())
			{
				Fail.Add(TEXT("the field atlas or a sprite material did not cook: the creatures were not drawn"));
			}
			const double GameP99 = Percentile(GameMs, 0.99);
			if (GameP99 >= GameThreadBudgetMs)
			{
				Fail.Add(FString::Printf(TEXT("game thread p99 %.3f ms >= %.1f ms"), GameP99, GameThreadBudgetMs));
			}
			if (HeldFrames * 100 > static_cast<int64>(GameMs.Num()))
			{
				Fail.Add(FString::Printf(TEXT("the field was held on %lld of %d frames (a screen up): nothing was measured"), HeldFrames, GameMs.Num()));
			}
			if (Mean(DrawCalls) >= DrawCallBudget)
			{
				Fail.Add(FString::Printf(TEXT("%.1f draw calls >= %.0f"), Mean(DrawCalls), DrawCallBudget));
			}
			const FIntPoint Size = GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport ? GEngine->GameViewport->Viewport->GetSizeXY() : FIntPoint::ZeroValue;
			auto Line = [](const TCHAR* Name, const TArray<double>& V)
			{
				return FString::Printf(TEXT("%-10s avg %8.3f  p99 %8.3f  max %8.3f"), Name, Mean(V), Percentile(V, 0.99), Percentile(V, 1.0));
			};
			TArray<FString> Out;
			Out.Add(FString::Printf(TEXT("viewport=%dx%d frames=%d adapter=%s mice(min)=%lld kittens=%lld held=%lld"), Size.X, Size.Y, FrameMs.Num(), *GRHIAdapterName, MinLive, F.AwakeKittens(), HeldFrames));
			Out.Add(Line(TEXT("frame_ms"), FrameMs));
			Out.Add(Line(TEXT("game_ms"), GameMs));
			Out.Add(Line(TEXT("render_ms"), RenderMs));
			Out.Add(Line(TEXT("gpu_ms"), GpuMs));
			Out.Add(Line(TEXT("steps_ms"), StepsMs));
			Out.Add(Line(TEXT("draw_ms"), DrawMs));
			Out.Add(Line(TEXT("draws"), DrawCalls));
			Out.Add(Fail.Num() ? TEXT("FAIL ") + FString::Join(Fail, TEXT("; ")) : FString(TEXT("PASS")));
			FFileHelper::SaveStringArrayToFile(Out, *(FPaths::ProjectSavedDir() / TEXT("PawBench/report.txt")));
			for (const FString& L : Out)
			{
				UE_LOG(LogPaw, Display, TEXT("PAW_BENCH %s"), *L);
			}
			bDone = true;
		}
	};

	TUniquePtr<FBench> GBench;

	FAutoConsoleCommand BenchCommand(TEXT("paw.Bench"),
		TEXT("Fill the realm to the live cap, measure N seconds (default 20), check the drawing, report to Saved/PawBench, quit."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			if (GBench)
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Bench: already running"));
				return;
			}
			GBench = MakeUnique<FBench>();
			GBench->Seconds = Args.Num() > 0 ? FMath::Max(1.0, FCString::Atod(*Args[0])) : 20.0;
			GBench->Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float Dt) { return GBench->Tick(Dt); }));
		}));
}

#endif
