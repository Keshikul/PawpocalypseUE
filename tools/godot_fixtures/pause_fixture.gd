extends Node

# Ground truth for PawCore's PcPause and UPawPauseMenu: the REAL pause menu (pause_menu.tscn / pause_menu.gd) with the real
# SaveManager and its slot files (scratch copy of the project, sandboxed APPDATA; tools/make_godot_fixtures.py).
#
# Each row writes a random set of the five slot files, sets the active slot and the game in memory (what a save writes),
# then runs 12 random presses on the live menu: open, close, the Save page, back to the main page, a slot (the two-press
# overwrite), the first press of "Back to Main Menu" (its second press and the discard leave the scene, so they are left to
# the port's engine tests). After every press it records what the menu shows: open or not, the page, the status line, the
# quit button's caption, the discard button, the five slot captions, the active slot, which slot files exist, and the
# armed overwrite and quit flags.

const ROWS := 2000
const OPS := 12
const SLOTS := 5


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func flag(b: bool) -> String:
	return "1" if b else "0"


func esc(s: String) -> String:
	return s.replace("\\", "\\\\").replace("\n", "\\n").replace("\t", "\\t")


func write_slot(slot: int, rng: RandomNumberGenerator) -> void:
	var d := PawpocalypseSaveData.new()
	d.version = PawpocalypseSaveData.CURRENT_VERSION
	d.unicorn_split = true
	d.reawaken_count = rng.randi_range(0, 40)
	d.lifetime_kills = rng.randi_range(0, 3000000) if rng.randf() < 0.7 else rng.randi_range(0, 999)
	d.lifetime_catnip_ever = 10.0
	d.total_play_seconds = rng.randf_range(0.0, 400000.0) if rng.randf() < 0.8 else rng.randf_range(0.0, 59.0)
	d.last_played_unix = 1.8e9
	ResourceSaver.save(d, SaveManager.slot_path(slot))


func state(menu: PauseMenu) -> PackedStringArray:
	var present := ""
	for s in range(SLOTS):
		if SaveManager.has_slot(s):
			present += str(s)
	var cols := PackedStringArray([flag(menu.visible), flag(menu.main_page.visible), flag(menu.slot_page.visible),
		esc(menu.status_label.text), esc(menu.main_menu_button.text), flag(menu.discard_button.visible),
		str(menu._pending_overwrite), flag(menu._exit_pending), str(SaveManager.active_slot), present, flag(get_tree().paused)])
	for s in range(SLOTS):
		cols.append(esc(menu._slot_buttons[s].text))
	return cols


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261105
	var menu := load("res://scenes/ui/pause_menu.tscn").instantiate() as PauseMenu
	add_child(menu)
	await get_tree().process_frame
	menu.set_process(false)  # no input polling: the presses are the fixture's
	var header := ["row", "op", "arg", "game_reawakens", "game_kills", "game_play"]
	for c in ["open", "main_page", "slot_page", "status", "quit_caption", "discard", "pending", "exit_pending", "active",
			"present", "paused"]:
		header.append(c)
	for s in range(SLOTS):
		header.append("s%d" % s)
	var out := FileAccess.open("res://pause_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(header)))
	var counts := {}
	for r in range(ROWS):
		if menu.visible:
			menu.close()
		get_tree().paused = false
		for s in range(SLOTS):
			SaveManager.delete_slot(s)
			if rng.randf() < 0.5:
				write_slot(s, rng)
		if rng.randf() < 0.05:
			var junk := rng.randi_range(0, SLOTS - 1)
			var f := FileAccess.open(SaveManager.slot_path(junk), FileAccess.WRITE)
			f.store_string("not a save")
			f.close()
		SaveManager.active_slot = rng.randi_range(0, SLOTS - 1)
		# The game a save writes.
		GameState.reawaken_count = rng.randi_range(0, 40)
		GameState.lifetime_kills = rng.randi_range(0, 3000000)
		GameState.lifetime_catnip_ever = 50.0
		GameState.total_play_seconds = rng.randf_range(0.0, 400000.0)
		var game := [str(GameState.reawaken_count), str(GameState.lifetime_kills), h(GameState.total_play_seconds)]
		# The starting row: the menu opened.
		menu.open()
		var row := PackedStringArray([str(r), "open", "-"])
		row.append_array(game)
		row.append_array(state(menu))
		out.store_line("\t".join(row))
		for k in range(OPS):
			var roll := rng.randf()
			var op := ""
			var arg := "-"
			if not menu.visible:
				op = "open"
			elif roll < 0.12:
				op = "close"
			elif roll < 0.3:
				op = "slots"
			elif roll < 0.42:
				op = "main"
			elif roll < 0.85:
				op = "slot"
			else:
				op = "quit1"
			match op:
				"open":
					menu.open()
				"close":
					menu.close()
				"slots":
					menu._show_slots()
				"main":
					menu._show_main()
				"slot":
					var s := rng.randi_range(0, SLOTS - 1)
					# Mostly the slot just armed: the second press.
					if menu._pending_overwrite >= 0 and rng.randf() < 0.5:
						s = menu._pending_overwrite
					arg = str(s)
					menu._on_slot_pressed(s)
				"quit1":
					if menu._exit_pending:
						op = "noop"  # the second press leaves the scene: the engine tests cover it
					else:
						menu._on_main_menu_pressed()
			counts[op] = counts.get(op, 0) + 1
			row = PackedStringArray([str(r), op, arg])
			row.append_array(game)
			row.append_array(state(menu))
			out.store_line("\t".join(row))
	out.close()
	if menu.visible:
		menu.close()
	for s in range(SLOTS):
		SaveManager.delete_slot(s)
	print("PAUSE_FIXTURE rows=%d ops=%s" % [ROWS, counts])
	print("PAUSE_FIXTURE done")
	get_tree().quit(0)
