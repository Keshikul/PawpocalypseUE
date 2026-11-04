extends Node

# Ground truth for PawCore's AURA and ZONE rules (phase 2 slice 3), run on the REAL Godot code (scratch copy of the project,
# sandboxed APPDATA; tools/make_godot_fixtures.py --only aura).
#
# Per row: a random progress state (upgrades, branches biased to the aura, Snare and Unicorn nodes, cards biased to the
# aura cards, jobs, Reawakens and depth, the stillness clocks) and probes:
#   - GameState: is_aura_owned(), get_aura_radius_multiplier() (Carry reads player_still_seconds), get_carry_radius_bonus(),
#     get_aura_reach_multiplier(), get_snare_multiplier(), get_gridlock_multiplier(), get_aura_crowd_scaling(),
#     get_aura_echo_power(), get_aura_beat_seconds(), is_poop_ability_unlocked(), get_long_lunch_payout() (lunch_seconds)
#   - CardManager: effect_best_low() of AURA_SLOW and POOP_TRAIL, effect_value() of AURA_TRAIL, AURA_PULSE, EDGE_BURN,
#     POOP_DURATION
#   - CombatResolver: aura_dps(), poop_dps(), player_dps()
#   - a REAL Enemy node (a ground mouse or a flier): up to four apply_slow() calls (random multiplier, Gridlock's
#     reaches_fliers flag, the frame's freshness reset in between as _process does it) -> the slow it keeps.
# The zones, the pulse, the trail, the echo and edge burn are frame-driven: pinned by PawCore's rule tests.

const ROWS := 3000
const ENEMY_SCENE: PackedScene = preload("res://scenes/enemies/enemy.tscn")


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
	rng.seed = 20261101
	var job_ids: Array = GameState.jobs.keys()
	job_ids.sort()
	var biomes := []
	for i in range(1, 13):
		biomes.append("tier_%d" % i)
	var focus := ["unlock_aura", "aura_reach", "aura_damage", "carry", "unlock_snare", "deadweight", "gridlock",
		"crowd_work", "echo", "standing_wave", "unlock_unicorn", "unlock_poop", "long_lunch", "traps"]
	var aura_effects := [CardData.Effect.AURA_SLOW, CardData.Effect.POOP_TRAIL, CardData.Effect.AURA_TRAIL,
		CardData.Effect.AURA_PULSE, CardData.Effect.EDGE_BURN, CardData.Effect.POOP_DURATION]
	var ground: Array[EnemyData] = []
	var fliers: Array[EnemyData] = []
	for id in biomes:
		var realm := GameState.realm_data("res://data/%s.tres" % id)
		for e in realm.spawn_pool:
			if e.movement == EnemyData.MovementKind.FLIER:
				if not fliers.has(e):
					fliers.append(e)
			elif not ground.has(e):
				ground.append(e)
	var bounds := Rect2(0.0, 0.0, 1920.0, 1080.0)
	var enemy: Enemy = ENEMY_SCENE.instantiate()
	add_child(enemy)

	var out := FileAccess.open("res://aura_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["upgrades", "branches", "cards", "run_job", "run_job_second",
		"reawaken_count", "deepest", "current_biome", "stardust", "still", "lunch",
		"aura_owned", "radius_mult", "carry", "reach", "snare", "gridlock", "crowd", "echo", "beat", "poop_unlocked",
		"lunch_payout", "aura_slow", "poop_trail", "aura_trail", "aura_pulse", "edge_burn", "poop_duration",
		"aura_dps", "poop_dps", "player_dps", "enemy", "slows", "slow_after"])))
	for i in range(ROWS):
		# --- the state ---
		var up := {}
		for id in GameState.upgrades:
			up[id] = rng.randi_range(0, 40) if rng.randf() < 0.6 else 0
		var br := {}
		for id in SkillTreeManager.branches:
			var data: SkillBranchData = SkillTreeManager.branches[id]
			var p := 0.6 if focus.has(id) else 0.2
			if rng.randf() < p:
				br[id] = rng.randi_range(1, data.max_level())
		var cards := {}
		for card in CardManager.deck:
			var p := 0.5 if aura_effects.has(card.effect) else 0.2
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
		GameState.player_still_seconds = 0.0 if rng.randf() < 0.3 else rng.randf_range(0.0, 6.0)
		GameState.lunch_seconds = 0.0 if rng.randf() < 0.3 else rng.randf_range(0.0, 20.0)
		GameState._rebuild_stats()
		var stats := GameState.get_stats()

		# --- one enemy taking slows ---
		var flier := rng.randf() < 0.4
		var edata: EnemyData = fliers[rng.randi_range(0, fliers.size() - 1)] if flier \
			else ground[rng.randi_range(0, ground.size() - 1)]
		enemy.activate(edata, bounds.get_center(), Vector2.RIGHT, bounds)
		var slows := PackedStringArray()
		for k in range(rng.randi_range(1, 4)):
			# Between calls, sometimes the frame turns over: _process's reset of the slow's freshness.
			var turn := k > 0 and rng.randf() < 0.3
			if turn:
				if not enemy._slow_fresh:
					enemy._slow_multiplier = 1.0
				enemy._slow_fresh = false
			var m: float = rng.randf_range(0.0, 1.0) if rng.randf() < 0.85 else [0.0, 1.0, 1.5, -0.5][rng.randi_range(0, 3)]
			var reach := rng.randf() < 0.3
			enemy.apply_slow(m, reach)
			slows.append("%s,%s,%s" % [flag(turn), h(m), flag(reach)])
		var slow_after := "%s,%s" % [h(enemy._slow_multiplier), flag(enemy._slow_fresh)]
		enemy.deactivate()

		var cols := PackedStringArray([pairs(up), pairs(br), pairs(cards), GameState.run_job, GameState.run_job_second,
			str(GameState.reawaken_count), str(GameState.deepest_biome_index), GameState.current_biome_id,
			h(GameState.stardust_granted_ever), h(GameState.player_still_seconds), h(GameState.lunch_seconds),
			flag(GameState.is_aura_owned()), h(GameState.get_aura_radius_multiplier()), h(GameState.get_carry_radius_bonus()),
			h(GameState.get_aura_reach_multiplier()), h(GameState.get_snare_multiplier()), h(GameState.get_gridlock_multiplier()),
			h(GameState.get_aura_crowd_scaling()), h(GameState.get_aura_echo_power()), h(GameState.get_aura_beat_seconds()),
			flag(GameState.is_poop_ability_unlocked()), h(GameState.get_long_lunch_payout()),
			h(CardManager.effect_best_low(CardData.Effect.AURA_SLOW)), h(CardManager.effect_best_low(CardData.Effect.POOP_TRAIL)),
			h(CardManager.effect_value(CardData.Effect.AURA_TRAIL)), h(CardManager.effect_value(CardData.Effect.AURA_PULSE)),
			h(CardManager.effect_value(CardData.Effect.EDGE_BURN)), h(CardManager.effect_value(CardData.Effect.POOP_DURATION)),
			h(CombatResolver.aura_dps(stats)), h(CombatResolver.poop_dps(stats)), h(CombatResolver.player_dps(stats)),
			edata.id, "|".join(slows), slow_after])
		out.store_line("\t".join(cols))
	out.close()
	print("AURA_FIXTURE wrote %d rows" % ROWS)
	get_tree().quit(0)
