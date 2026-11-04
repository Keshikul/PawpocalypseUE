extends Node

# Ground truth for PawCore's KITTEN rules (phase 2 slice 2), run on the REAL Godot code (scratch copy of the project,
# sandboxed APPDATA; tools/make_godot_fixtures.py --only kittens).
#
# Per row: a random progress state (upgrades, branches biased to the kitten nodes, cards, jobs, Reawakens and depth for
# the line caps) and probes:
#   - GameState: get_kitten_count(), kittens_seek_clusters(), get_kitten_illusion_pull(); CombatResolver.kitten_dps() and
#     the kitten speed multiplier; TierController's base pool size (Kitten Count cap + max Vitality + MAX_TEMP_KITTENS)
#   - a REAL Kitten node: configure_orbit_slot() at a random index/total; _pick_target() and _is_overcrowded() over up to
#     24 REAL Enemy nodes at integer positions (so float32 squared distances are exact and ties are real ties) with random
#     claims, some of them deactivated (parked off the field, as a mouse killed earlier in the frame is); then ONE hunt()
#     call: the target it ends on, the claims after, where the kitten stands, the victim's HP and position (Kitten Drag).
#     The roam path is probed with the heading timer far from expiry (its re-roll draws the global RNG).
# Kittens are frame-driven and roll the global RNG, so whole-field behaviour is pinned by PawCore's rule tests, not here.

const ROWS := 3000
const MAX_MICE := 24
const ENEMY_SCENE: PackedScene = preload("res://scenes/enemies/enemy.tscn")
const KITTEN_SCENE: PackedScene = preload("res://scenes/cats/kitten.tscn")


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
	rng.seed = 20261031
	var job_ids: Array = GameState.jobs.keys()
	job_ids.sort()
	var biomes := []
	for i in range(1, 13):
		biomes.append("tier_%d" % i)
	var focus := ["vitality", "kitten_foreman", "grand_finale", "instinct", "whisker_trade", "job_magician"]
	var mouse_data: Array[EnemyData] = []
	for id in biomes:
		var realm := GameState.realm_data("res://data/%s.tres" % id)
		for e in realm.spawn_pool:
			if not mouse_data.has(e):
				mouse_data.append(e)
	var bounds := Rect2(0.0, 0.0, 1920.0, 1080.0)
	var mice: Array[Enemy] = []
	for i in range(MAX_MICE):
		var e: Enemy = ENEMY_SCENE.instantiate()
		add_child(e)
		mice.append(e)
	var kitten: Kitten = KITTEN_SCENE.instantiate()
	add_child(kitten)
	kitten.configure_roam_bounds(bounds)

	var out := FileAccess.open("res://kittens_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["upgrades", "branches", "cards", "run_job", "run_job_second",
		"reawaken_count", "deepest", "current_biome", "stardust",
		"kitten_count", "pool_base", "foreman", "illusion_pull", "kitten_dps", "kitten_speed",
		"orbit_index", "orbit_total", "orbit",
		"origin", "mice", "pick", "crowd_target", "overcrowded",
		"hunt_target", "damage_mult", "drag", "anchor", "delta", "roam_dir",
		"after_target", "after_claims", "after_pos", "after_victim"])))
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
		# Set, never inherited: the line caps grow with both, and the scratch project's GameState carries state.
		GameState.reawaken_count = rng.randi_range(0, 15)
		GameState.deepest_biome_index = rng.randi_range(0, 11)
		GameState.current_biome_id = biomes[rng.randi_range(0, biomes.size() - 1)]
		# Every other input of _rebuild_stats() set too: the sandbox's GameState keeps what an earlier fixture left
		# (found 2026-10-31: an inherited stardust_granted_ever scaled every kitten DPS by 8.28).
		GameState.bosses_defeated = {}
		GameState.final_boss_defeated = false
		GameState.stardust_granted_ever = pow(10.0, rng.randf_range(0.0, 5.0)) if rng.randf() < 0.7 else 0.0
		GameState.waves_here = 0
		GameState.cards_taken_this_run = 0
		GameState.kills_this_cycle = 0
		GameState.threshold_damage_now = 1.0
		GameState._rebuild_stats()
		var stats := GameState.get_stats()

		# --- the getters ---
		var pool_base := int(GameState.get_upgrade_data("kitten_count").cap) \
			+ SkillTreeManager.get_max_possible_vitality_bonus() + TierController.MAX_TEMP_KITTENS
		var orbit_total := rng.randi_range(1, 40)
		var orbit_index := rng.randi_range(0, orbit_total - 1)
		kitten.configure_orbit_slot(orbit_index, orbit_total)
		var orbit: Vector2 = kitten._orbit_offset

		# --- the mice: integer positions around an integer origin, random claims, some parked ---
		var origin := Vector2(float(rng.randi_range(0, 1920)), float(rng.randi_range(0, 1080)))
		var n := rng.randi_range(0, MAX_MICE) if rng.randf() < 0.8 else rng.randi_range(0, 3)
		var spread := 700 if rng.randf() < 0.7 else 60
		var cands: Array[Enemy] = []
		var claims := {}
		var mice_cells := PackedStringArray()
		var last := Vector2.ZERO
		for k in range(n):
			var e := mice[k]
			var pos := Vector2(origin.x + float(rng.randi_range(-spread, spread)),
				origin.y + float(rng.randi_range(-spread, spread)))
			# Real ties: the same spot as the previous mouse, or the mirror image of it about the origin.
			if k > 0 and rng.randf() < 0.15:
				pos = last if rng.randf() < 0.5 else origin * 2.0 - last
			last = pos
			e.activate(mouse_data[rng.randi_range(0, mouse_data.size() - 1)], pos, Vector2.RIGHT, bounds)
			e._invuln_active = rng.randf() < 0.15
			var parked := rng.randf() < 0.1
			if parked:
				e.deactivate()
			var c := 0 if rng.randf() < 0.5 else rng.randi_range(0, 5)
			if c > 0:
				claims[e] = c
			cands.append(e)
			mice_cells.append("%d,%d,%d,%s,%s,%s,%s" % [int(pos.x), int(pos.y), c, flag(parked), h(e.hp), flag(e._invuln_active),
				e.data.id])
		var foreman := GameState.kittens_seek_clusters()

		kitten.activate(origin)
		kitten.global_position = origin
		var picked := kitten._pick_target(cands, claims)
		var crowd_target := rng.randi_range(0, n - 1) if n > 0 else -1
		var overcrowded := false
		if crowd_target >= 0:
			kitten._target = cands[crowd_target]
			overcrowded = kitten._is_overcrowded(cands, claims)

		# --- one hunt() ---
		var hunt_target := -1
		if n > 0 and rng.randf() < 0.6:
			hunt_target = rng.randi_range(0, n - 1)
		kitten._target = cands[hunt_target] if hunt_target >= 0 else null
		kitten.damage_multiplier = 1.0 if rng.randf() < 0.6 else rng.randf_range(1.0, 20.0)
		kitten.drag_speed = 0.0 if rng.randf() < 0.6 else rng.randf_range(10.0, 400.0)
		kitten.drag_anchor = Vector2(float(rng.randi_range(0, 1920)), float(rng.randi_range(0, 1080)))
		var delta := rng.randf_range(0.0, 0.05) if rng.randf() < 0.9 else rng.randf_range(0.5, 4.0)
		var roam_angle := rng.randf_range(-PI, PI)
		kitten._roam_direction = Vector2.RIGHT.rotated(roam_angle)
		kitten._roam_timer = 1000.0
		var roam_dir: Vector2 = kitten._roam_direction
		kitten.hunt(delta, cands, stats, claims)
		var after: Enemy = kitten._target
		var after_index := cands.find(after) if after != null else -1
		var after_claims := PackedStringArray()
		for e in cands:
			after_claims.append(str(int(claims.get(e, 0))))
		var victim := ""
		if hunt_target >= 0 or after_index >= 0:
			var v: Enemy = cands[after_index] if after_index >= 0 else cands[hunt_target]
			victim = "%s,%s,%s,%s" % [h(v.hp), flag(v.is_active()), h(v.global_position.x), h(v.global_position.y)]
		for e in cands:
			if e.is_active():
				e.deactivate()
		kitten.deactivate()

		var cols := PackedStringArray([pairs(up), pairs(br), pairs(cards), GameState.run_job, GameState.run_job_second,
			str(GameState.reawaken_count), str(GameState.deepest_biome_index), GameState.current_biome_id,
			h(GameState.stardust_granted_ever),
			str(GameState.get_kitten_count()), str(pool_base), flag(foreman), h(GameState.get_kitten_illusion_pull()),
			h(CombatResolver.kitten_dps(stats)), h(stats.kitten_speed_multiplier),
			str(orbit_index), str(orbit_total), "%s,%s" % [h(orbit.x), h(orbit.y)],
			"%d,%d" % [int(origin.x), int(origin.y)], "|".join(mice_cells), str(picked_index(cands, picked)),
			str(crowd_target), flag(overcrowded),
			str(hunt_target), h(kitten.damage_multiplier), h(kitten.drag_speed),
			"%d,%d" % [int(kitten.drag_anchor.x), int(kitten.drag_anchor.y)], h(delta),
			"%s,%s" % [h(roam_dir.x), h(roam_dir.y)],
			str(after_index), ";".join(after_claims),
			"%s,%s" % [h(kitten.global_position.x), h(kitten.global_position.y)], victim])
		out.store_line("\t".join(cols))
	out.close()
	print("KITTENS_FIXTURE wrote %d rows" % ROWS)
	get_tree().quit(0)


func picked_index(cands: Array[Enemy], picked: Enemy) -> int:
	return cands.find(picked) if picked != null else -1
