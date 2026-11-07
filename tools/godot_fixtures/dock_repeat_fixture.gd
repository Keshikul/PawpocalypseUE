extends Node

# Ground truth for PawCore's PcDockRepeat (the dock's held keys, controller plan 6.2): the REAL DockNavigator (dock_navigator.gd)
# of the real HUD (hud.tscn), its _tick_nav and _tick_press driven frame by frame with Godot's own action state
# (Input.action_press / action_release) and chosen frame lengths (scratch copy, sandboxed user://; tools/make_godot_fixtures.py).
#
# dock_repeat_rows.tsv     the Upgrades page's rows: whether each repeats (META_REPEAT: the shop rows)
# dock_repeat_fixture.tsv  SEQS sequences, each from a chosen row with nothing held, then STEPS frames: what is held (press,
#                          menu_up, menu_down), the frame's delta; after the frame: the presses made (counted on the rows'
#                          pressed signals), the selected row, and the navigator's counters.

const SEQS := 600
const STEPS := 40


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261107
	GameState.catnip = 1.0e300   # every shop row live: a press always presses
	var hud := load("res://scenes/ui/hud.tscn").instantiate() as HUD
	add_child(hud)
	await get_tree().process_frame
	await get_tree().process_frame
	var nav: DockNavigator = hud.dock_nav
	nav.set_process(false)       # the frames are the fixture's
	hud.show_tab("upgrades")
	hud._refresh_upgrade_buttons()   # the rows priced against the Catnip set above (check_padplay.gd does the same)
	await get_tree().process_frame
	await get_tree().process_frame
	var rows: Array[Control] = nav.rows()
	var presses := [0]
	var r := FileAccess.open("res://dock_repeat_rows.tsv", FileAccess.WRITE)
	r.store_line("# row\trepeats")
	for i in rows.size():
		r.store_line("%d\t%s" % [i, "1" if rows[i].has_meta(DockNavigator.META_REPEAT) else "0"])
		(rows[i] as BaseButton).pressed.connect(func() -> void: presses[0] += 1)
	r.close()

	var out := FileAccess.open("res://dock_repeat_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["seq", "step", "press", "up", "down", "dt", "sel_nav", "sel_press", "disabled", "presses", "row", "press_held",
		"press_next", "nav_dir", "nav_held", "nav_next"])))
	var actions := ["press", "menu_up", "menu_down"]
	var counts := {"first": 0, "repeat": 0, "steps": 0}
	for s in range(SEQS):
		for a in actions:
			Input.action_release(a)
		await get_tree().process_frame
		# Every line back to level 0 and priced: a line bought to its maximum is disabled, and would stay so.
		GameState.upgrade_levels = {}
		hud._refresh_upgrade_buttons()
		var start := rng.randi_range(0, rows.size() - 1)
		nav.select(rows[start], true)
		nav._press_held = 0.0
		nav._press_next = 0.0
		nav._nav_dir = 0
		nav._nav_held = 0.0
		nav._nav_next = 0.0
		# A quarter of the sequences begin with nothing selected (the focus on a tab, as a mouse player leaves it): the first
		# press or step selects the remembered row instead.
		if rng.randf() < 0.25:
			(hud._tab_buttons.values()[0] as Control).grab_focus()
		await get_tree().process_frame
		out.store_line("\t".join(PackedStringArray([str(s), "start", "-", "-", "-", "-", "-", "-", "-", "0", str(start), h(0.0), h(0.0), "0", h(0.0), h(0.0)])))
		var held := {"press": false, "menu_up": false, "menu_down": false}
		for k in range(STEPS):
			# Held keys change now and then; a press is held longest (hold-to-buy is what is measured).
			if rng.randf() < 0.15:
				held["press"] = not held["press"]
			if rng.randf() < 0.08:
				held["menu_up"] = not held["menu_up"]
			if rng.randf() < 0.08:
				held["menu_down"] = not held["menu_down"]
			for a in actions:
				if held[a]:
					if not Input.is_action_pressed(a):
						Input.action_press(a)
				else:
					Input.action_release(a)
			var dt: float = [1.0 / 60.0, 1.0 / 30.0, 0.05, 0.1, 0.2, 0.35, rng.randf_range(0.001, 0.3)][rng.randi_range(0, 6)]
			presses[0] = 0
			var sel_nav := nav.has_selection()
			nav._tick_nav(dt)
			var sel_press := nav.has_selection()
			# press() does nothing on a disabled row (a line at its maximum, Kitten Power before a kitten): recorded.
			var disabled := (nav.selected as BaseButton).disabled
			nav._tick_press(dt)
			if presses[0] > 0:
				counts["first" if Input.is_action_just_pressed("press") else "repeat"] += presses[0]
			out.store_line("\t".join(PackedStringArray([str(s), str(k), "1" if held["press"] else "0", "1" if held["menu_up"] else "0",
				"1" if held["menu_down"] else "0", h(dt), "1" if sel_nav else "0", "1" if sel_press else "0", "1" if disabled else "0", str(presses[0]), str(rows.find(nav.selected)), h(nav._press_held),
				h(nav._press_next), str(nav._nav_dir), h(nav._nav_held), h(nav._nav_next)])))
			counts["steps"] += 1
			await get_tree().process_frame
	out.close()
	print("DOCK_REPEAT_FIXTURE rows=%d counts=%s" % [rows.size(), counts])
	print("DOCK_REPEAT_FIXTURE done")
	get_tree().quit(0)
