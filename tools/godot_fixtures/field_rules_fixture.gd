extends Node

# Ground truth for PawCore's FIELD rules (phase 2): the parts of the realm that are pure functions of the state, run on the
# REAL Godot code (scratch copy of the project, sandboxed APPDATA; tools/make_godot_fixtures.py --only field_rules).
#
# Per row: a random progress state (upgrades, branches biased to the field's nodes, cards, jobs, Infestation) and probes:
#   - GameState: spawn-rate multiplier, Swarm Logic x Last Stand at a live-enemy count, Traps, Play With Food, Funnel,
#     Misdirection, Spectrum; the TierController spawn rate (per sec, timer batch and interval)
#   - TierData: the weighted pool pick at a roll and a signature weight, the pool's total weight
#   - TierController.playfield_rect at a random view, dock and side
#   - a REAL pooled Enemy node: activate() (HP, max HP, wave reward multiplier, Funnel's whole bounces), then up to four
#     blows through take_damage() with random shield / snare / Shield Pierce / threshold / beam states: what each returns,
#     the HP after, and on the kill the reward the `killed` signal carries and whether it was toyed with.
# Movement and timers are frame-driven and roll the global RNG, so they are pinned by PawCore's rule tests, not here.

const ROWS := 3000
const ENEMY_SCENE: PackedScene = preload("res://scenes/enemies/enemy.tscn")

var _reward := -1.0


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


func realm_ids() -> Array:
	var ids := []
	for i in range(1, 13):
		ids.append("tier_%d" % i)
	return ids


func _on_killed(reward: float) -> void:
	_reward = reward


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261030
	var job_ids: Array = GameState.jobs.keys()
	job_ids.sort()
	var ids := realm_ids()
	var focus := ["swarm", "swarm_logic", "last_stand", "traps", "play_with_food", "funnel", "misdirection", "spectrum",
		"unlock_snare", "unlock_prism", "job_ratcatcher"]
	var enemy: Enemy = ENEMY_SCENE.instantiate()
	add_child(enemy)
	enemy.killed.connect(_on_killed)
	var out := FileAccess.open("res://field_rules_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["upgrades", "branches", "cards", "run_job", "run_job_second",
		"reawaken_count", "deepest", "current_biome", "wave_index", "infestation", "live", "probe_realm", "roll", "sig_weight",
		"view_x", "view_y", "dock", "dock_left", "enemy", "blows",
		"spawn_mult", "spawn_per_sec", "spawn_batch", "spawn_interval", "threshold", "snared", "play_with_food",
		"funnel", "drift", "prism_kill", "pick", "total_weight", "playfield",
		"hp", "max_hp", "wave_reward", "bounces_whole", "results"])))
	for i in range(ROWS):
		# --- the state ---
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
			if rng.randf() < 0.25:
				cards[card.id] = rng.randi_range(1, card.level_values.size())
		GameState.upgrade_levels = up
		SkillTreeManager.levels = br
		CardManager.owned = cards
		GameState.run_job = "" if rng.randf() < 0.4 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		GameState.run_job_second = "" if rng.randf() < 0.7 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		# Set, never inherited: the line caps grow with both, and a scratch project's GameState carries state between rows.
		GameState.reawaken_count = rng.randi_range(0, 15)
		GameState.deepest_biome_index = rng.randi_range(0, 11)
		GameState.current_biome_id = ids[rng.randi_range(0, ids.size() - 1)]
		GameState.current_wave_index = rng.randi_range(0, 64) if rng.randf() < 0.7 else 0
		GameState.infestation_bonus = rng.randf_range(0.0, 2.0) if rng.randf() < 0.4 else 0.0
		var live := rng.randi_range(0, 320) if rng.randf() < 0.6 else rng.randi_range(0, 8)
		var probe: String = ids[rng.randi_range(0, ids.size() - 1)]
		var realm := GameState.realm_data("res://data/%s.tres" % probe)
		var edges: Array[float] = [0.0, 0.9999, 1.0, -0.5, 1.5]
		var roll: float = rng.randf() if rng.randf() < 0.9 else edges[rng.randi_range(0, 4)]
		var sig_weight := 1.0 if rng.randf() < 0.4 else rng.randf_range(1.0, 4.0)
		var view := Vector2(rng.randf_range(100.0, 3840.0), rng.randf_range(100.0, 2160.0))
		var dock := Rect2()
		if rng.randf() < 0.7:
			var w := rng.randf_range(0.0, 900.0)
			var x := rng.randf_range(-200.0, view.x + 100.0)
			dock = Rect2(x, 0.0, w, view.y)
		var dock_left := rng.randf() < 0.5
		# The enemy: from the probe biome's pool or its golden.
		var pool: Array = realm.spawn_pool
		var edata: EnemyData = realm.golden_enemy if realm.golden_enemy != null and rng.randf() < 0.15 \
			else pool[rng.randi_range(0, pool.size() - 1)]

		# --- the answers ---
		var per_sec := maxf(realm.spawn_rate_per_min * GameState.get_spawn_rate_multiplier() / 60.0, 0.0001)
		var batch := maxi(1, int(ceil(per_sec * TierController.MIN_SPAWN_INTERVAL)))
		var interval := maxf(TierController.MIN_SPAWN_INTERVAL, float(batch) / per_sec)
		var picked: EnemyData = realm.pick_enemy(roll, sig_weight)
		var field := TierController.playfield_rect(view, dock, 600.0, dock_left)
		var threshold := GameState.get_threshold_damage_multiplier(live)

		var bounds := Rect2(0.0, 0.0, 1920.0, 1080.0)
		seed(i)
		enemy.activate(edata, bounds.get_center(), Vector2.RIGHT, bounds)
		var hp0 := enemy.hp
		var max0 := enemy.max_hp
		var wave_reward := enemy.wave_reward_multiplier
		var whole := floorf(GameState.get_funnel_bounces())
		var blows := PackedStringArray()
		var results := PackedStringArray()
		for b in range(rng.randi_range(1, 4)):
			var amount := enemy.hp * rng.randf_range(0.0, 1.6) if rng.randf() < 0.85 else rng.randf_range(0.0, 1.0e6)
			var shielded := rng.randf() < 0.25
			var slowed := rng.randf() < 0.3
			var pierce := 0.0 if rng.randf() < 0.6 else rng.randf_range(0.0, 1.0)
			var beam := rng.randf() < 0.2
			var blow_threshold := threshold if rng.randf() < 0.7 else rng.randf_range(1.0, 3.0)
			enemy._invuln_active = shielded
			enemy._slow_multiplier = 1.0
			enemy._slow_fresh = false
			if slowed:
				enemy.apply_slow(0.5)
			enemy.killed_by_beam = beam
			GameState.threshold_damage_now = blow_threshold
			_reward = -1.0
			var ret := enemy.take_damage(amount, pierce)
			blows.append("%s,%s,%s,%s,%s,%s" % [h(amount), flag(shielded), flag(slowed), h(pierce), flag(beam), h(blow_threshold)])
			results.append("%s,%s,%s,%s" % [h(ret), h(enemy.hp), h(_reward), flag(enemy.toyed_with) if _reward >= 0.0 else "-"])
			if not enemy.is_active():
				break
		if enemy.is_active():
			enemy.deactivate()
		GameState.threshold_damage_now = 1.0

		var cols := PackedStringArray([pairs(up), pairs(br), pairs(cards), GameState.run_job, GameState.run_job_second,
			str(GameState.reawaken_count), str(GameState.deepest_biome_index), GameState.current_biome_id, str(GameState.current_wave_index), h(GameState.infestation_bonus), str(live), probe,
			h(roll), h(sig_weight), h(view.x), h(view.y),
			"" if dock.size.x <= 0.0 else "%s,%s,%s,%s" % [h(dock.position.x), h(dock.position.y), h(dock.size.x), h(dock.size.y)],
			flag(dock_left), edata.id, "|".join(blows),
			h(GameState.get_spawn_rate_multiplier()), h(per_sec), str(batch), h(interval), h(threshold),
			h(GameState.get_snared_damage_multiplier()), h(GameState.get_play_with_food_multiplier()),
			h(GameState.get_funnel_bounces()), h(GameState.get_enemy_drift()), h(GameState.get_prism_kill_multiplier()),
			picked.id if picked != null else "", h(realm.total_spawn_weight(sig_weight)),
			"%s,%s,%s,%s" % [h(field.position.x), h(field.position.y), h(field.size.x), h(field.size.y)],
			h(hp0), h(max0), h(wave_reward), h(whole), "|".join(results)])
		out.store_line("\t".join(cols))
	out.close()
	print("FIELD_RULES_FIXTURE wrote %d rows" % ROWS)
	get_tree().quit(0)
