#include "Dev/PawDebugHud.h"

#include "Core/PawGameSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "PcField.h"
#include "Realm/PawRealmSubsystem.h"
#include "Realm/PawRealmView.h"
#include "EngineUtils.h"
#include "Render/PawSpriteRenderer.h"

namespace
{
	TAutoConsoleVariable<int32> CVarDebugHud(TEXT("paw.DebugHud"), 1,
		TEXT("The realm's debug readout (development builds): 1 shows it, 0 hides it."));
}

void APawDebugHud::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas || CVarDebugHud.GetValueOnGameThread() == 0)
	{
		return;
	}
	const UPawRealmSubsystem* R = GetWorld() ? GetWorld()->GetSubsystem<UPawRealmSubsystem>() : nullptr;
	const UPawGameSubsystem* Game = R ? R->GetGame() : nullptr;
	if (!R || !Game || !R->IsRunning())
	{
		return;
	}
	const FPawRealmSim& Sim = R->GetSim();
	const Pc::PcField& F = Sim.Field;
	const Pc::PcProgress& P = Game->Progress();
	const double FrameMs = FApp::GetDeltaTime() * 1000.0;
	SmoothedFrameMs = SmoothedFrameMs <= 0.0 ? FrameMs : SmoothedFrameMs * 0.95 + FrameMs * 0.05;

	const APawRealmView* View = nullptr;
	for (TActorIterator<APawRealmView> It(GetWorld()); It; ++It)
	{
		View = *It;
		break;
	}

	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("frame %.2f ms (%.0f fps)  steps %d  dropped %lld  sim %.3f ms  draw %.3f ms"), SmoothedFrameMs,
		SmoothedFrameMs > 0.0 ? 1000.0 / SmoothedFrameMs : 0.0, Sim.StepsLastFrame, Sim.StepsDropped, Sim.LastStepsMs,
		View ? View->LastDrawMs() : 0.0));
	Lines.Add(FString::Printf(TEXT("%hs (biome %lld)  step %llu  seed %llu  speed %.0fx%s"), P.CurrentBiomeId.c_str(),
		F.TierIndex + 1, static_cast<unsigned long long>(F.StepCount), static_cast<unsigned long long>(Sim.BaseSeed),
		Sim.GameSpeed, Sim.bPaused ? TEXT("  PAUSED") : TEXT("")));
	Lines.Add(FString::Printf(TEXT("mice %lld  kittens %lld  zones %d  effects %d  boss %s"), F.LiveCount(), F.AwakeKittens(),
		static_cast<int32>(F.Zones.size()), View ? View->LiveEffects() : 0,
		F.bBossUp ? *FString::Printf(TEXT("phase %lld, %.4g HP, %d adds"), F.BossPhase, F.BossHp, static_cast<int32>(F.Adds.size())) : TEXT("-")));
	Lines.Add(FString::Printf(TEXT("Catnip %.4g  Stardust %.4g  kills %lld  Reawakens %lld"), P.Catnip, P.Stardust, P.LifetimeKills,
		P.ReawakenCount));
	Lines.Add(FString::Printf(TEXT("travel %.2f ms (worst %.2f)  Pounce %.0f%%"), Sim.LastTravelMs, Sim.WorstTravelMs,
		Pc::PounceCooldownFraction(F) * 100.0));

	const UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	float Y = 8.0f;
	for (const FString& L : Lines)
	{
		Canvas->SetDrawColor(FColor(25, 25, 25));
		Canvas->DrawText(Font, L, 9.0f, Y + 1.0f);
		Canvas->SetDrawColor(FColor::White);
		Canvas->DrawText(Font, L, 8.0f, Y);
		Y += 14.0f;
	}
}
