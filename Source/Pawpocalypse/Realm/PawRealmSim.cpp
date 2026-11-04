#include "Realm/PawRealmSim.h"

#include "HAL/PlatformTime.h"
#include "PcAchievements.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcProgress.h"

void FPawRealmSim::BeginHere(const Pc::PcGameData& D, Pc::PcProgress& P, const std::string& TierId)
{
	const double T0 = FPlatformTime::Seconds();
	Pc::BeginBiome(D, P, TierId);
	Pc::BeginField(Field, D, P, TierId, Bounds, BaseSeed + static_cast<uint64>(Travels));
	++Travels;
	Snapshot();
	LastTravelMs = (FPlatformTime::Seconds() - T0) * 1000.0;
	WorstTravelMs = FMath::Max(WorstTravelMs, LastTravelMs);
}

void FPawRealmSim::Start(const Pc::PcGameData& D, Pc::PcProgress& P)
{
	const int64 Index = P.CurrentBiomeId.empty() ? -1 : Pc::RealmIndex(D, P.CurrentBiomeId);
	BeginHere(D, P, Index >= 0 ? P.CurrentBiomeId : D.Tiers.front().Id);
	Accumulator = 0.0;
}

bool FPawRealmSim::Travel(const Pc::PcGameData& D, Pc::PcProgress& P, int32 Index)
{
	if (Index < 0 || Index >= static_cast<int32>(D.Tiers.size()))
	{
		return false;
	}
	BeginHere(D, P, D.Tiers[static_cast<size_t>(Index)].Id);
	return true;
}

void FPawRealmSim::SetBounds(const Pc::PcRect& NewBounds)
{
	Bounds = NewBounds;
	if (Field.TierIndex >= 0)
	{
		Pc::SetFieldBounds(Field, Bounds);
	}
}

void FPawRealmSim::EvaluateAchievements(const Pc::PcGameData& D, Pc::PcProgress& P)
{
	for (std::string& Id : Pc::EvaluateAchievements(D, P))
	{
		EarnedAchievements.push_back(std::move(Id));
	}
}

void FPawRealmSim::OnReawakened(const Pc::PcGameData& D, Pc::PcProgress& P)
{
	Pc::SyncFieldKittens(Field, D, P);
	Pc::RefreshFieldStats(Field, D, P);
	Pc::DismissFieldBoss(Field);
	EvaluateAchievements(D, P); // the reawakened signal
	// Territory no longer travels from here (designer 2026-11-01, "Territory wins"): Godot's timer ran during the ceremony
	// and swapped the scene out from under it, so the spend step never opened. The HUD travels to the starting biome,
	// Territory's included, at the end of the spend step (UPawHudScreen::DoReawaken, hud.gd _return_to_starting_biome).
}

int32 FPawRealmSim::Advance(const Pc::PcGameData& D, Pc::PcProgress& P, double RealDelta)
{
	FrameEvents.clear();
	StepsLastFrame = 0;
	LastStepsMs = 0.0;
	if (bPaused || Field.TierIndex < 0)
	{
		return 0;
	}
	const double T0 = FPlatformTime::Seconds();
	Accumulator += FMath::Max(RealDelta, 0.0) * GameSpeed;
	while (Accumulator >= Step && StepsLastFrame < MaxStepsPerFrame)
	{
		Snapshot();
		Pc::StepField(Field, D, P, Input, Step);
		Pc::TickPlayClock(P, Step);
		EvaluateAchievements(D, P);
		Accumulator -= Step;
		++StepsLastFrame;

		int32 Travel = -1;
		for (const Pc::PcFieldEvent& E : Pc::DrainFieldEvents(Field))
		{
			FrameEvents.push_back(E);
			if (E.Kind == Pc::PcFieldEventKind::TravelRequested)
			{
				Travel = E.Slot;
			}
		}
		if (Travel >= 0)
		{
			this->Travel(D, P, Travel);
		}
	}
	// Only when time was really dropped: a frame that needed exactly the cap keeps its fraction of a step.
	if (StepsLastFrame == MaxStepsPerFrame && Accumulator >= Step)
	{
		StepsDropped += static_cast<int64>(FMath::FloorToDouble(Accumulator / Step + 1e-9)); // 52.000 not 51.999
		Accumulator = 0.0;
	}
	LastStepsMs = (FPlatformTime::Seconds() - T0) * 1000.0;
	return StepsLastFrame;
}

void FPawRealmSim::Snapshot()
{
	PrevX = Field.X;
	PrevY = Field.Y;
	PrevSerial = Field.Serial;
	PrevActive = Field.Active;
	PrevKX = Field.KX;
	PrevKY = Field.KY;
	PrevKActive = Field.KActive;
	PrevCatX = Field.CatX;
	PrevCatY = Field.CatY;
	PrevBossX = Field.BossX;
	PrevBossY = Field.BossY;
	bPrevBossUp = Field.bBossUp;
}

void FPawRealmSim::MousePos(int32 Slot, double& OutX, double& OutY) const
{
	const size_t I = static_cast<size_t>(Slot);
	OutX = Field.X[I];
	OutY = Field.Y[I];
	if (I < PrevX.size() && PrevActive[I] && PrevSerial[I] == Field.Serial[I])
	{
		const double A = Alpha();
		OutX = PrevX[I] + (OutX - PrevX[I]) * A;
		OutY = PrevY[I] + (OutY - PrevY[I]) * A;
	}
}

void FPawRealmSim::KittenPos(int32 K, double& OutX, double& OutY) const
{
	const size_t I = static_cast<size_t>(K);
	OutX = Field.KX[I];
	OutY = Field.KY[I];
	if (I < PrevKX.size() && PrevKActive[I])
	{
		const double A = Alpha();
		OutX = PrevKX[I] + (OutX - PrevKX[I]) * A;
		OutY = PrevKY[I] + (OutY - PrevKY[I]) * A;
	}
}

void FPawRealmSim::CatPos(double& OutX, double& OutY) const
{
	const double A = Alpha();
	OutX = PrevCatX + (Field.CatX - PrevCatX) * A;
	OutY = PrevCatY + (Field.CatY - PrevCatY) * A;
}

void FPawRealmSim::BossPos(double& OutX, double& OutY) const
{
	OutX = Field.BossX;
	OutY = Field.BossY;
	if (bPrevBossUp)
	{
		const double A = Alpha();
		OutX = PrevBossX + (OutX - PrevBossX) * A;
		OutY = PrevBossY + (OutY - PrevBossY) * A;
	}
}
