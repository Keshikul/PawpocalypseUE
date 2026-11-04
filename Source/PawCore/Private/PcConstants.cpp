#include "PcConstants.h"

#include <map>

namespace Pc
{
	namespace
	{
		// One entry per field: the CSV id and where it goes. Exactly one of the three pointers is set.
		struct FieldRef
		{
			const char* Id;
			double PcCodeConstants::* Double;
			int64_t PcCodeConstants::* Int;
			std::vector<std::string> PcCodeConstants::* List;
		};

		constexpr FieldRef D(const char* Id, double PcCodeConstants::* M) { return { Id, M, nullptr, nullptr }; }
		constexpr FieldRef I(const char* Id, int64_t PcCodeConstants::* M) { return { Id, nullptr, M, nullptr }; }
		constexpr FieldRef L(const char* Id, std::vector<std::string> PcCodeConstants::* M) { return { Id, nullptr, nullptr, M }; }

		using C = PcCodeConstants;
		const FieldRef Fields[] = {
			L("boss_realms", &C::BossRealms),
			I("evolution_stage_count", &C::EvolutionStageCount),
			I("max_wave", &C::MaxWave),
			D("max_golden_chance", &C::MaxGoldenChance),
			D("currency_epsilon", &C::CurrencyEpsilon),
			I("bulk_max_steps", &C::BulkMaxSteps),
			I("mounted_stage", &C::MountedStage),
			I("levy_every", &C::LevyEvery),
			D("infestation_decay_per_sec", &C::InfestationDecayPerSec),
			D("infestation_ceiling", &C::InfestationCeiling),
			D("realtime_interest_period_sec", &C::RealtimeInterestPeriodSec),
			I("second_helping_biome_index", &C::SecondHelpingBiomeIndex),
			I("fence_max_kills", &C::FenceMaxKills),
			I("crowd_threshold", &C::CrowdThreshold),
			I("last_stand_threshold", &C::LastStandThreshold),
			D("lunch_full_seconds", &C::LunchFullSeconds),
			D("carry_full_seconds", &C::CarryFullSeconds),
			D("card_level_cost_base", &C::CardLevelCostBase),
			D("card_level_cost_growth", &C::CardLevelCostGrowth),
			I("base_hand_size", &C::BaseHandSize),
			I("max_live_enemies", &C::MaxLiveEnemies),
			D("min_spawn_interval", &C::MinSpawnInterval),
			D("min_playfield_width", &C::MinPlayfieldWidth),
			D("spawn_edge_inset", &C::SpawnEdgeInset),
			D("spawn_direction_spread", &C::SpawnDirectionSpread),
			I("max_strikes_per_frame", &C::MaxStrikesPerFrame),
			D("hammerfall_lane_px", &C::HammerfallLanePx),
			D("hammerfall_range_px", &C::HammerfallRangePx),
			I("max_damage_zones", &C::MaxDamageZones),
			D("aura_trail_drop_sec", &C::AuraTrailDropSec),
			D("aura_trail_lifetime_sec", &C::AuraTrailLifetimeSec),
			D("aura_trail_min_move_px", &C::AuraTrailMinMovePx),
			D("finale_reach_px", &C::FinaleReachPx),
			D("gridlock_reach_px", &C::GridlockReachPx),
			D("echo_drop_sec", &C::EchoDropSec),
			D("edge_burn_band_px", &C::EdgeBurnBandPx),
			D("aura_pulse_interval_sec", &C::AuraPulseIntervalSec),
			D("prism_shards_per_detonation", &C::PrismShardsPerDetonation),
			D("pounce_crater_lifetime_sec", &C::PounceCraterLifetimeSec),
			D("boss_death_sequence_sec", &C::BossDeathSequenceSec),
			D("auto_travel_poll_sec", &C::AutoTravelPollSec),
			D("cut_max_ratio", &C::CutMaxRatio),
			D("catnip_bomb_damage_seconds", &C::CatnipBombDamageSeconds),
			D("catnip_bomb_radius", &C::CatnipBombRadius),
			I("max_temp_kittens", &C::MaxTempKittens),
			D("kitten_explosion_radius", &C::KittenExplosionRadius),
			D("boss_relocate_speed", &C::BossRelocateSpeed),
			D("boss_drift_speed", &C::BossDriftSpeed),
			D("boss_drift_interval", &C::BossDriftInterval),
			D("boss_arena_inset", &C::BossArenaInset),
			D("boss_add_arrive_px", &C::BossAddArrivePx),
			D("boss_relocate_distance_px", &C::BossRelocateDistancePx),
			D("boss_wobble_x_px", &C::BossWobbleXPx),
			D("boss_wobble_y_px", &C::BossWobbleYPx),
			D("play_with_food_hp_fraction", &C::PlayWithFoodHpFraction),
			D("kitten_roam_heading_sec", &C::KittenRoamHeadingSec),
			D("kitten_roam_turn_spread", &C::KittenRoamTurnSpread),
			I("kitten_rebalance_margin", &C::KittenRebalanceMargin),
			D("player_still_epsilon_px", &C::PlayerStillEpsilonPx),
			D("enemy_speed_px_per_sec", &C::EnemySpeedPxPerSec),
			D("enemy_wander_turn_rate", &C::EnemyWanderTurnRate),
			D("enemy_flee_centre_bias", &C::EnemyFleeCentreBias),
			D("enemy_flier_speed_multiplier", &C::EnemyFlierSpeedMultiplier),
			D("kitten_speed_px_per_sec", &C::KittenSpeedPxPerSec),
			D("kitten_orbit_radius_px", &C::KittenOrbitRadiusPx),
			D("kitten_roam_speed_ratio", &C::KittenRoamSpeedRatio),
		};
	}

	bool ReadCodeConstants(const CsvTable& Table, PcCodeConstants& Out, std::vector<std::string>& Errors)
	{
		const size_t Before = Errors.size();
		const int IdColumn = Table.Column("id");
		const int ValueColumn = Table.Column("value");
		if (IdColumn < 0 || ValueColumn < 0)
		{
			Errors.push_back("CodeConstants.csv: needs id and value columns");
			return false;
		}
		std::map<std::string, const FieldRef*> ById;
		for (const FieldRef& F : Fields)
		{
			ById.emplace(F.Id, &F);
		}
		std::map<std::string, int> Seen;
		for (const std::vector<std::string>& Row : Table.Rows)
		{
			const std::string& Id = Row[static_cast<size_t>(IdColumn)];
			const std::string& Value = Row[static_cast<size_t>(ValueColumn)];
			const auto It = ById.find(Id);
			if (It == ById.end())
			{
				Errors.push_back("CodeConstants.csv: unknown id '" + Id + "' (no PcCodeConstants field reads it)");
				continue;
			}
			if (++Seen[Id] > 1)
			{
				Errors.push_back("CodeConstants.csv: id '" + Id + "' appears twice");
				continue;
			}
			const FieldRef& F = *It->second;
			bool bOk = true;
			if (F.Double)
			{
				bOk = ParseDouble(Value, Out.*F.Double);
			}
			else if (F.Int)
			{
				bOk = ParseInt(Value, Out.*F.Int);
			}
			else
			{
				std::vector<std::string>& List = Out.*F.List;
				List.clear();
				size_t Start = 0;
				while (bOk)
				{
					const size_t End = Value.find(';', Start);
					List.push_back(Value.substr(Start, End == std::string::npos ? std::string::npos : End - Start));
					bOk = !List.back().empty();
					if (End == std::string::npos)
					{
						break;
					}
					Start = End + 1;
				}
			}
			if (!bOk)
			{
				Errors.push_back("CodeConstants.csv: " + Id + " = '" + Value + "' is not a valid " +
					(F.Double ? "number" : F.Int ? "integer" : "';'-joined id list"));
			}
		}
		for (const FieldRef& F : Fields)
		{
			if (!Seen.count(F.Id))
			{
				Errors.push_back(std::string("CodeConstants.csv: no row for '") + F.Id + "'");
			}
		}
		return Errors.size() == Before;
	}

	std::vector<std::pair<std::string, double>> CodeConstantNumbers(const PcCodeConstants& C)
	{
		std::vector<std::pair<std::string, double>> Out;
		for (const FieldRef& F : Fields)
		{
			if (F.Double)
			{
				Out.emplace_back(F.Id, C.*F.Double);
			}
			else if (F.Int)
			{
				Out.emplace_back(F.Id, static_cast<double>(C.*F.Int));
			}
		}
		return Out;
	}
}
