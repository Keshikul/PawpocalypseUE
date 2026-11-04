extends Node

# Ground truth for PawCore's paw and kill EXTRAS and the Prism (phase 2 slice 5), run on the REAL Godot code (scratch copy
# of the project, sandboxed APPDATA; tools/make_godot_fixtures.py --only extras).
#
# Per row: a random progress state (upgrades, branches biased to the nodes of this slice, cards biased to its cards, jobs,
# Reawakens and depth) and probes:
#   - GameState: get_guaranteed_crit_share() (Butcher's Rhythm), has_deterministic_crit() (Inscrutable),
#     get_double_strike_share() (Anvil Paw), get_paw_cleave_count() (Hammerfall), get_crit_execute_chance() (Riddle),
#     get_overkill_catnip_share() (Cat Burglar's Cut), get_overflow_ratio(), get_levy_multiple(), get_pied_piper_radius(),
#     has_spawn_foresight() (Sixth Sense), get_vanish_interval() (Sleight of Hand), get_pickpocket_share(),
#     prism_owned(), get_prism_beam_dps(), prism_sweeps() (Full Spectrum), get_prism_chain_count() (Refraction),
#     get_prism_shard_payload() (Critical Mass)
#   - CardManager: effect_best_low(CATNIP_BOMB_EVERY), effect_value(SCAVENGE)
# The swings, the kills and the beam themselves are frame-driven and roll the global RNG: pinned by PawCore's rule tests.

const ROWS := 3000


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func flag(b: bool) -> String:
	return "1" if b else "0"


func pairs(d: Dictionary) -> String:
	var keys := d.keys()
	keys.sort()
	var parts := PackedStringArray()
	for k in keys:
		parts.append("%s=%d" % [k, int(d[k])])
	return ";".join(parts)


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261103
	var job_ids: Array = GameState.jobs.keys()
	job_ids.sort()
	var biomes := []
	for i in range(1, 13):
		biomes.append("tier_%d" % i)
	var focus := ["anvil_paw", "butchers_rhythm", "inscrutable", "riddle", "hammerfall", "cat_burglars_cut", "overflow",
		"levy", "long_lunch", "pied_piper", "sixth_sense", "sleight_of_hand", "pickpocket", "unlock_prism", "refraction",
		"unlock_full_spectrum", "critical_mass", "spectrum", "unlock_aura", "unlock_unicorn"]
	var effects := [CardData.Effect.CATNIP_BOMB_EVERY, CardData.Effect.SCAVENGE]

	var out := FileAccess.open("res://extras_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["upgrades", "branches", "cards", "run_job", "run_job_second",
		"reawaken_count", "deepest", "current_biome", "stardust",
		"rhythm", "inscrutable", "anvil", "cleave", "riddle", "cut", "overflow", "levy", "piper", "foresight", "vanish",
		"pickpocket", "prism_owned", "beam_dps", "sweeps", "chain", "payload", "bomb_every", "scavenge"])))
	for i in range(ROWS):
		var up := {}
		for id in GameState.upgrades:
			up[id] = rng.randi_range(0, 40) if rng.randf() < 0.6 else 0
		var br := {}
		for id in SkillTreeManager.branches:
			var data: SkillBranchData = SkillTreeManager.branches[id]
			var p := 0.55 if focus.has(id) else 0.2
			if rng.randf() < p:
				br[id] = rng.randi_range(1, data.max_level())
		var cards := {}
		for card in CardManager.deck:
			var p := 0.5 if effects.has(card.effect) else 0.2
			if rng.randf() < p:
				cards[card.id] = rng.randi_range(1, card.level_values.size())
		GameState.upgrade_levels = up
		SkillTreeManager.levels = br
		CardManager.owned = cards
		GameState.run_job = "" if rng.randf() < 0.4 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		GameState.run_job_second = "" if rng.randf() < 0.7 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		# Every input set, never inherited: the sandbox's GameState keeps what an earlier fixture left.
		GameState.reawaken_count = rng.randi_range(0, 15)
		GameState.deepest_biome_index = rng.randi_range(0, 11)
		GameState.current_biome_id = biomes[rng.randi_range(0, biomes.size() - 1)]
		GameState.bosses_defeated = {}
		GameState.final_boss_defeated = false
		GameState.stardust_granted_ever = pow(10.0, rng.randf_range(0.0, 5.0)) if rng.randf() < 0.7 else 0.0
		GameState.waves_here = 0
		GameState.cards_taken_this_run = 0
		GameState.kills_this_cycle = 0
		GameState.player_still_seconds = 0.0
		GameState.lunch_seconds = 0.0
		GameState._rebuild_stats()

		var cols := PackedStringArray([pairs(up), pairs(br), pairs(cards), GameState.run_job, GameState.run_job_second,
			str(GameState.reawaken_count), str(GameState.deepest_biome_index), GameState.current_biome_id,
			h(GameState.stardust_granted_ever),
			h(GameState.get_guaranteed_crit_share()), flag(GameState.has_deterministic_crit()),
			h(GameState.get_double_strike_share()), str(GameState.get_paw_cleave_count()),
			h(GameState.get_crit_execute_chance()), h(GameState.get_overkill_catnip_share()),
			h(GameState.get_overflow_ratio()), h(GameState.get_levy_multiple()), h(GameState.get_pied_piper_radius()),
			flag(GameState.has_spawn_foresight()), h(GameState.get_vanish_interval()), h(GameState.get_pickpocket_share()),
			flag(GameState.prism_owned()), h(GameState.get_prism_beam_dps()), flag(GameState.prism_sweeps()),
			str(GameState.get_prism_chain_count()), h(GameState.get_prism_shard_payload()),
			h(CardManager.effect_best_low(CardData.Effect.CATNIP_BOMB_EVERY)),
			h(CardManager.effect_value(CardData.Effect.SCAVENGE))])
		out.store_line("\t".join(cols))
	out.close()
	print("EXTRAS_FIXTURE wrote %d rows" % ROWS)
	get_tree().quit(0)
