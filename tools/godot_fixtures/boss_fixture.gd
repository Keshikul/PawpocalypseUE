extends Node

# Ground truth for PawCore's BOSS rules (phase 2 slice 6), run on the REAL Godot code (scratch copy of the project,
# sandboxed APPDATA; tools/make_godot_fixtures.py --only boss).
#
# Per row: one of the five bosses, activated on a REAL Boss node, then up to 12 random operations, each followed by its
# state (HP, phase, shielded, the shield timer, alive):
#   d  take_damage(amount, shield_pierce): through the shield or not, phase thresholds crossed, the kill
#   h  heal(amount): capped at the total, phases walked back
#   p  _process(delta): the shield cycle of the current phase (its movement rolls the global RNG: positions are not
#      compared, the shield is)
# The adds' arrival is a distance check against a moving boss: pinned by PawCore's rule tests.

const ROWS := 3000
const BOSS_SCENE: PackedScene = preload("res://scenes/enemies/boss.tscn")


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func flag(b: bool) -> String:
	return "1" if b else "0"


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261104
	var bosses: Array[BossData] = []
	for i in range(1, 13):
		var realm := GameState.realm_data("res://data/tier_%d.tres" % i)
		if realm.boss_data != null and not bosses.has(realm.boss_data):
			bosses.append(realm.boss_data)
	var boss: Boss = BOSS_SCENE.instantiate()
	add_child(boss)

	var out := FileAccess.open("res://boss_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["boss", "ops", "states"])))
	for i in range(ROWS):
		var data: BossData = bosses[rng.randi_range(0, bosses.size() - 1)]
		seed(i)
		boss.configure_arena(Rect2(0.0, 0.0, 1920.0, 1080.0))
		boss.activate(data, Vector2(960.0, 540.0))
		var ops := PackedStringArray()
		var states := PackedStringArray()
		for k in range(rng.randi_range(1, 12)):
			var roll := rng.randf()
			if roll < 0.45:
				var amount: float = data.total_hp * rng.randf_range(0.0, 0.45) if rng.randf() < 0.9 else data.total_hp * 2.0
				var pierce := 0.0 if rng.randf() < 0.6 else rng.randf_range(0.0, 1.0)
				boss.take_damage(amount, pierce)
				ops.append("d,%s,%s" % [h(amount), h(pierce)])
			elif roll < 0.65:
				var amount := data.total_hp * rng.randf_range(0.0, 0.35)
				boss.heal(amount)
				ops.append("h,%s" % h(amount))
			else:
				var delta: float = rng.randf_range(0.0, 2.5) if rng.randf() < 0.8 else rng.randf_range(0.0, 0.05)
				if boss.is_active():
					boss._process(delta)
				ops.append("p,%s" % h(delta))
			states.append("%s,%d,%s,%s,%s" % [h(boss.hp), boss.current_phase, flag(boss._invuln_active),
				h(boss._invuln_timer), flag(boss.is_active())])
		boss.withdraw()
		out.store_line("\t".join(PackedStringArray([data.id, "|".join(ops), "|".join(states)])))
	out.close()
	print("BOSS_FIXTURE wrote %d rows" % ROWS)
	get_tree().quit(0)
