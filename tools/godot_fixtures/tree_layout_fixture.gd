extends Node

# Ground truth for PawCore's PcTreeLayout: the REAL SkillTreeUI layout code (skill_tree_ui.gd: branch_order(), the hex
# seating of _compute_layout(), _separate_layout(), _normalise_layout(), _layout_bounds(), canvas_size_for() and nav_pick())
# on the shipping tree (row 0) and on random trees (rows 1..): a random subset of the trunks, a random subset of the other
# branches, each hung off a random branch already in the tree. The script is instanced without its scene: the layout reads
# only constants, SkillTreeManager.branches and its own _positions (scratch copy of the project, sandboxed APPDATA;
# tools/make_godot_fixtures.py).

const TREES := 401


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261102
	var real: Dictionary = SkillTreeManager.branches.duplicate()
	var real_ids: Array = real.keys()
	real_ids.sort()
	var wheel_trunks: Array = []
	for id in SkillTreeUI.TRUNK_WHEEL:
		if real.has(id):
			wheel_trunks.append(id)
	var others: Array = []
	for id in real_ids:
		if not wheel_trunks.has(id):
			others.append(id)
	var ui := SkillTreeUI.new()
	var out := FileAccess.open("res://tree_layout_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["tree", "ids", "parents", "order", "hub_x", "hub_y", "xs", "ys",
		"bounds", "canvas", "nav_up", "nav_down", "nav_left", "nav_right"])))
	for t in range(TREES):
		var synth: Dictionary = {}
		if t == 0:
			synth = real
		else:
			# Trunks: a random non-empty subset of the wheel, and now and then a trunk the wheel does not name (appended
			# after the wheel by trunk_order()).
			var chosen: Array = []
			for id in wheel_trunks:
				if rng.randf() < 0.7:
					chosen.append(id)
			if chosen.is_empty():
				chosen.append(wheel_trunks[rng.randi_range(0, wheel_trunks.size() - 1)])
			var pool: Array = others.duplicate()
			for i in range(pool.size() - 1, 0, -1):
				var j := rng.randi_range(0, i)
				var tmp = pool[i]
				pool[i] = pool[j]
				pool[j] = tmp
			var extra_trunk := ""
			if rng.randf() < 0.15 and not pool.is_empty():
				extra_trunk = pool.pop_back()
			for id in chosen:
				var d: SkillBranchData = real[id].duplicate()
				d.parent = ""
				synth[id] = d
			if extra_trunk != "":
				var d: SkillBranchData = real[extra_trunk].duplicate()
				d.parent = ""
				synth[extra_trunk] = d
			var hung: Array = synth.keys()
			var n := rng.randi_range(0, pool.size())
			for k in range(n):
				var id: String = pool[k]
				var d: SkillBranchData = real[id].duplicate()
				# Mostly off a recent branch (chains), sometimes off any (fans).
				var lo := 0 if rng.randf() < 0.5 else maxi(0, hung.size() - 4)
				d.parent = hung[rng.randi_range(lo, hung.size() - 1)]
				synth[id] = d
				hung.append(id)
		SkillTreeManager.branches = synth

		var ids: Array = synth.keys()
		ids.sort()
		var parents := PackedStringArray()
		for id in ids:
			var p: String = (synth[id] as SkillBranchData).parent
			parents.append(p if p != "" else "-")
		var order := SkillTreeUI.branch_order()

		ui._positions = ui._compute_layout()
		ui._separate_layout()
		ui._normalise_layout()
		var pos: Dictionary = ui._positions
		var xs := PackedStringArray()
		var ys := PackedStringArray()
		var node_positions := {}
		for id in order:
			var p: Vector2 = pos[id]
			xs.append(h(p.x))
			ys.append(h(p.y))
			node_positions[id] = p
		var hub: Vector2 = pos["hub"]
		var b: Rect2 = ui._layout_bounds()
		var c: Vector2 = SkillTreeUI.canvas_size_for(b)
		var nav := [PackedStringArray(), PackedStringArray(), PackedStringArray(), PackedStringArray()]
		var dirs := [Vector2.UP, Vector2.DOWN, Vector2.LEFT, Vector2.RIGHT]
		for id in order:
			for k in 4:
				var pick := SkillTreeUI.nav_pick(id, dirs[k], node_positions)
				nav[k].append(pick if pick != "" else "-")
		out.store_line("\t".join(PackedStringArray([str(t), ";".join(PackedStringArray(ids)), ";".join(parents),
			";".join(PackedStringArray(order)), h(hub.x), h(hub.y), ";".join(xs), ";".join(ys),
			";".join(PackedStringArray([h(b.position.x), h(b.position.y), h(b.size.x), h(b.size.y)])),
			";".join(PackedStringArray([h(c.x), h(c.y)])),
			";".join(nav[0]), ";".join(nav[1]), ";".join(nav[2]), ";".join(nav[3])])))
	out.close()
	SkillTreeManager.branches = real
	ui.free()
	print("TREE_LAYOUT_FIXTURE done")
	get_tree().quit()
