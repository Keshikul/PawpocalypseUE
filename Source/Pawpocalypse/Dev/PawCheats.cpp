// The cheat overlay's actions as console commands (cheat_overlay.gd), development builds only. The rules are PawCore's
// (PcCheats); here each command also does what Godot's signals did after its button (kittens re-counted, stats rebuilt,
// the realm's Reawaken handling). The F1 panel behind a password is phase 3's (Common UI); the commands work today.
//
//   paw.Cheat.Catnip [n]       debug_add_currency (1000 by default)
//   paw.Cheat.Stardust [n]     (500 by default)
//   paw.Cheat.MaxUpgrades      debug_max_all_upgrades
//   paw.Cheat.Unicorn          debug_set_reawaken_count(unicorn_unlock_reawaken_count)
//   paw.Cheat.Reawaken         reawaken(true)
//   paw.Cheat.Reset            debug_reset_progress (a new game, the realm restarted)
//   paw.Cheat.AdvanceStage     debug_advance_stage
//   paw.Cheat.UnlockTiers      debug_unlock_all_tiers
//   paw.Cheat.UnlockStages     debug_unlock_all_stages: every biome open, all three gates (F18 fixed 2026-11-03)
//   paw.Cheat.KillAll          kill_all_requested
//   paw.Cheat.Speed <x>        Engine.time_scale (the overlay offers 1, 2, 5)
//   paw.Travel <biome>         travel to biome 1..12
//   paw.Summon                 the Summon button
//   paw.Spawn <enemy id>       one mouse at the cat

#include "Core/PawGameSubsystem.h"
#include "Core/PawLog.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "PcCheats.h"
#include "PcField.h"
#include "PcProgress.h"
#include "Realm/PawRealmSubsystem.h"

#if !UE_BUILD_SHIPPING

namespace
{
	// The realm and the game of the world the command was typed in; null (logged) when there is no running realm.
	UPawRealmSubsystem* RealmOf(UWorld* World, UPawGameSubsystem*& OutGame)
	{
		UPawRealmSubsystem* R = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
		OutGame = R ? R->GetGame() : nullptr;
		if (!R || !OutGame || !R->IsRunning())
		{
			UE_LOG(LogPaw, Warning, TEXT("Cheat: no running realm in this world"));
			return nullptr;
		}
		return R;
	}

	double Arg(const TArray<FString>& Args, double Default)
	{
		return Args.Num() > 0 ? FCString::Atod(*Args[0]) : Default;
	}

	using FCommand = TFunction<void(UPawRealmSubsystem&, UPawGameSubsystem&, const TArray<FString>&)>;

	struct FCheat
	{
		FAutoConsoleCommandWithWorldAndArgs Command;
		FCheat(const TCHAR* Name, const TCHAR* Help, FCommand Body)
			: Command(Name, Help, FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
				[Body = MoveTemp(Body)](const TArray<FString>& Args, UWorld* World)
				{
					UPawGameSubsystem* Game = nullptr;
					if (UPawRealmSubsystem* R = RealmOf(World, Game))
					{
						Body(*R, *Game, Args);
					}
				}))
		{
		}
	};

	FCheat Catnip(TEXT("paw.Cheat.Catnip"), TEXT("Grant Catnip (default 1000)."),
		[](UPawRealmSubsystem& R, UPawGameSubsystem& G, const TArray<FString>& A)
		{
			Pc::CheatAddCurrency(G.Progress(), Arg(A, Pc::CheatCatnipGrant), 0.0);
		});
	FCheat Stardust(TEXT("paw.Cheat.Stardust"), TEXT("Grant Stardust (default 500)."),
		[](UPawRealmSubsystem& R, UPawGameSubsystem& G, const TArray<FString>& A)
		{
			Pc::CheatAddCurrency(G.Progress(), 0.0, Arg(A, Pc::CheatStardustGrant));
		});
	FCheat MaxUpgrades(TEXT("paw.Cheat.MaxUpgrades"), TEXT("Every capped upgrade to its cap, every uncapped one +25."),
		[](UPawRealmSubsystem& R, UPawGameSubsystem& G, const TArray<FString>&)
		{
			Pc::CheatMaxAllUpgrades(G.Data(), G.Progress());
			R.OnProgressChanged(true); // upgrade_purchased: the kitten count may have moved
		});
	FCheat Unicorn(TEXT("paw.Cheat.Unicorn"), TEXT("Set the Reawaken count to the Unicorn's unlock."),
		[](UPawRealmSubsystem& R, UPawGameSubsystem& G, const TArray<FString>&)
		{
			Pc::CheatSetReawakenCount(G.Progress(), G.Data().Reawaken.UnicornUnlockReawakenCount);
			R.OnReawakened(); // Godot re-used the `reawakened` signal
		});
	FCheat Reawaken(TEXT("paw.Cheat.Reawaken"), TEXT("Reawaken now, whatever the requirement."),
		[](UPawRealmSubsystem& R, UPawGameSubsystem& G, const TArray<FString>&)
		{
			if (Pc::Reawaken(G.Data(), G.Progress(), true).bDone)
			{
				R.OnReawakened();
			}
		});
	FCheat Reset(TEXT("paw.Cheat.Reset"), TEXT("A brand-new game; the realm starts over."),
		[](UPawRealmSubsystem& R, UPawGameSubsystem& G, const TArray<FString>&)
		{
			G.NewGame();
			R.StartRealm();
		});
	FCheat AdvanceStage(TEXT("paw.Cheat.AdvanceStage"), TEXT("Beat the boss holding up the next evolution stage."),
		[](UPawRealmSubsystem& R, UPawGameSubsystem& G, const TArray<FString>&)
		{
			Pc::CheatAdvanceStage(G.Data(), G.Progress());
			R.OnProgressChanged(false);
		});
	FCheat UnlockTiers(TEXT("paw.Cheat.UnlockTiers"), TEXT("Every boss defeated (the stage gate only)."),
		[](UPawRealmSubsystem& R, UPawGameSubsystem& G, const TArray<FString>&)
		{
			Pc::CheatUnlockAllTiers(G.Data(), G.Progress());
			R.OnProgressChanged(false);
		});
	FCheat UnlockStages(TEXT("paw.Cheat.UnlockStages"), TEXT("Every boss defeated, this run's passage and the Catnip gates met: every biome open."),
		[](UPawRealmSubsystem& R, UPawGameSubsystem& G, const TArray<FString>&)
		{
			Pc::CheatUnlockAllStages(G.Data(), G.Progress());
			R.OnProgressChanged(false);
		});
	FCheat KillAll(TEXT("paw.Cheat.KillAll"), TEXT("Kill every mouse (it still pays and counts)."),
		[](UPawRealmSubsystem& R, UPawGameSubsystem& G, const TArray<FString>&)
		{
			FPawRealmSim& Sim = R.GetSim();
			Pc::KillAllEnemies(Sim.Field, G.Data(), G.Progress(), Sim.Input);
		});
	FCheat Speed(TEXT("paw.Cheat.Speed"), TEXT("The realm's speed (1, 2, 5; never World Time Dilation)."),
		[](UPawRealmSubsystem& R, UPawGameSubsystem&, const TArray<FString>& A)
		{
			R.SetGameSpeed(Arg(A, 1.0));
		});
	FCheat Travel(TEXT("paw.Travel"), TEXT("Travel to biome N (1-based)."),
		[](UPawRealmSubsystem& R, UPawGameSubsystem& G, const TArray<FString>& A)
		{
			if (!R.Travel(static_cast<int32>(Arg(A, 1.0)) - 1))
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Travel: biome 1..%d"), static_cast<int32>(G.Data().Tiers.size()));
			}
		});
	FCheat Summon(TEXT("paw.Summon"), TEXT("Summon this biome's boss (the bait is bought, all or nothing)."),
		[](UPawRealmSubsystem& R, UPawGameSubsystem&, const TArray<FString>&)
		{
			if (!R.SummonBoss())
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Summon: refused (no boss here, already up, or the bait is short)"));
			}
		});
	FCheat Spawn(TEXT("paw.Spawn"), TEXT("Spawn one mouse of an enemy id at the cat."),
		[](UPawRealmSubsystem& R, UPawGameSubsystem& G, const TArray<FString>& A)
		{
			FPawRealmSim& Sim = R.GetSim();
			const std::string Id = A.Num() > 0 ? std::string(TCHAR_TO_UTF8(*A[0].ToLower())) : std::string("mouse");
			if (Pc::SpawnEnemyAt(Sim.Field, G.Data(), G.Progress(), Id, Sim.Field.CatX + 120.0, Sim.Field.CatY, -1.0, 0.0) < 0)
			{
				UE_LOG(LogPaw, Warning, TEXT("paw.Spawn: unknown enemy id or a full field"));
			}
		});
}

#endif
