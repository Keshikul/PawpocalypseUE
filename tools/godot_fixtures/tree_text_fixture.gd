extends Node

# Ground truth for PawCore's PcTreeText: the REAL skill_tree_ui.gd _effect_text() for every branch at every level (rows
# "fx"), and each of the number expressions _describe() uses applied to random values (rows "num": the code names are
# tools/gen_tree_tables.py's NUMBERS). English, the source strings (no translation loaded). Scratch copy of the project,
# sandboxed APPDATA (tools/make_godot_fixtures.py).

const NUMS := 2000


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func _ready() -> void:
	TranslationServer.set_locale("en")
	var ui := SkillTreeUI.new()
	var out := FileAccess.open("res://tree_text_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["kind", "id", "level", "value", "text"])))
	var ids: Array = SkillTreeManager.branches.keys()
	ids.sort()
	for id in ids:
		var data: SkillBranchData = SkillTreeManager.branches[id]
		for level in range(0, data.max_level() + 2):
			out.store_line("\t".join(PackedStringArray(["fx", id, str(level), "-", ui._effect_text(id, level)])))
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261103
	for i in range(NUMS):
		# Every scale the tree's ladders live on, ties at .5 / .05 / .005 on purpose, and some negatives.
		var value: float
		match i % 5:
			0: value = rng.randf_range(0.0, 2.0)
			1: value = rng.randf_range(0.0, 120.0)
			2: value = float(rng.randi_range(0, 4000)) / 1000.0 + 0.0005
			3: value = float(rng.randi_range(0, 400)) / 200.0
			_: value = rng.randf_range(-3.0, 3.0)
		var v := value
		out.store_line("\t".join(PackedStringArray(["num", "R100", "-", h(v), str(roundi(value * 100.0))])))
		out.store_line("\t".join(PackedStringArray(["num", "Int", "-", h(v), str(int(value))])))
		out.store_line("\t".join(PackedStringArray(["num", "F0", "-", h(v), "%.0f" % (value)])))
		out.store_line("\t".join(PackedStringArray(["num", "F1", "-", h(v), "%.1f" % (value)])))
		out.store_line("\t".join(PackedStringArray(["num", "F2", "-", h(v), "%.2f" % (value)])))
		out.store_line("\t".join(PackedStringArray(["num", "F3", "-", h(v), "%.3f" % (value)])))
		out.store_line("\t".join(PackedStringArray(["num", "F1x100", "-", h(v), "%.1f" % (value * 100.0)])))
		out.store_line("\t".join(PackedStringArray(["num", "F0PerMinute", "-", h(v), "%.0f" % (60.0 / maxf(value, 0.001))])))
		out.store_line("\t".join(PackedStringArray(["num", "RInverse", "-", h(v), str(roundi(1.0 / maxf(value, 0.001)))])))
	out.close()
	ui.free()
	print("TREE_TEXT_FIXTURE done")
	get_tree().quit()
