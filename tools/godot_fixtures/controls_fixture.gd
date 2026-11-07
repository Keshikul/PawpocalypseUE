extends Node

# Ground truth for PawCore's PcControls and UPawControlsPanel: the REAL ControlsPanel (controls_panel.gd) inside the REAL
# settings dialog (settings_dialog.tscn), on the real SettingsManager, InputDevice and InputBindings (scratch copy of the
# project, sandboxed user://; tools/make_godot_fixtures.py).
#
# controls_fixture.tsv: ROWS rows. Each row STARTS from a state the fixture sets and records (no bindings or a few, the six
# switches, the two sliders, the prompt style, the pad last detected, nothing listening, an empty status line), then makes
# OPS random moves the way a player does: press a cell (it listens), send an event while it listens (a key, an echo, a
# release, a mouse button or motion, a pad button, a stick or trigger), let time pass (the 5 s timeout), press a row's
# Reset or Reset all, tick a switch, move a slider, pick a prompt style, or bind directly (panel.bind, the gate's path).
# After every move it records what the page shows: the status line, which cell listens, every cell (its text, disabled,
# the family its prompt draws, whether the prompt shows anything), every Reset's disabled, the switches, the sliders and
# their lines, the controller column's family, the device's families, and the bindings as saved.
#
# An event is k<physical keycode>[e|u] (e: echo, u: released), m<button>[u], mm (motion), b<button>[u], a<axis>:<value>.

const ROWS := 1500
const OPS := 12

const KEYS := [KEY_A, KEY_D, KEY_S, KEY_W, KEY_Q, KEY_T, KEY_U, KEY_SPACE, KEY_ENTER, KEY_LEFT, KEY_UP, KEY_ESCAPE, KEY_1,
	KEY_EQUAL, KEY_I, KEY_J, KEY_K, KEY_L, KEY_O, KEY_P, KEY_Z, KEY_X, KEY_E, KEY_R, KEY_F1, KEY_TAB, KEY_PAGEUP]
const CHECKS := ["show_prompts", "tutorial_tips", "swap_confirm_back", "swap_sticks", "invert_x", "invert_y"]
const STYLES := ["auto", "keyboard", "xbox", "playstation", "steam_deck"]


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func esc(s: String) -> String:
	return s.replace("\\", "\\\\").replace("\n", "\\n").replace("\t", "\\t")


func ev(e) -> String:
	if e == null:
		return "-"
	if e is InputEventKey:
		return "k%d" % InputBindings._key_of(e)
	if e is InputEventMouseButton:
		return "m%d" % e.button_index
	if e is InputEventJoypadButton:
		return "b%d" % e.button_index
	if e is InputEventJoypadMotion:
		var s := signf(e.axis_value)
		return "a%d%s" % [e.axis, "+" if s > 0.0 else ("-" if s < 0.0 else "0")]
	return "?"


func saved_text(saved: Dictionary) -> String:
	var parts := PackedStringArray()
	for action in saved:
		var evs := PackedStringArray()
		for d in saved[action]:
			evs.append(ev(InputBindings.deserialize(d)))
		parts.append("%s=%s" % [action, ",".join(evs)])
	return ";".join(parts)


func random_event(rng: RandomNumberGenerator) -> Array:
	# [text, event]
	var roll := rng.randf()
	if roll < 0.45:
		var k := InputEventKey.new()
		k.physical_keycode = KEYS[rng.randi_range(0, KEYS.size() - 1)]
		var r := rng.randf()
		k.pressed = r >= 0.1
		k.echo = r >= 0.1 and r < 0.2
		return ["k%d%s" % [k.physical_keycode, "u" if not k.pressed else ("e" if k.echo else "")], k]
	if roll < 0.55:
		var m := InputEventMouseButton.new()
		m.button_index = [MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE, MOUSE_BUTTON_WHEEL_UP][rng.randi_range(0, 3)]
		m.pressed = rng.randf() < 0.85
		return ["m%d%s" % [m.button_index, "" if m.pressed else "u"], m]
	if roll < 0.6:
		var mm := InputEventMouseMotion.new()
		mm.relative = Vector2(3, 1)
		return ["mm", mm]
	if roll < 0.85:
		var b := InputEventJoypadButton.new()
		b.button_index = rng.randi_range(0, 14) as JoyButton
		b.pressed = rng.randf() < 0.85
		return ["b%d%s" % [b.button_index, "" if b.pressed else "u"], b]
	var a := InputEventJoypadMotion.new()
	a.axis = rng.randi_range(0, 5) as JoyAxis
	var v: float = [-1.0, -0.6, -0.3, 0.0, 0.3, 0.49, 0.5, 0.8, 1.0][rng.randi_range(0, 8)]
	a.axis_value = v
	return ["a%d:%s" % [a.axis, str(v)], a]


func random_binding(rng: RandomNumberGenerator) -> Array:
	var r = random_event(rng)
	var b := InputBindings.binding_from(r[1])
	return [ev(b), b]


func state(dialog: SettingsDialog, panel: ControlsPanel) -> PackedStringArray:
	var cells := PackedStringArray()
	for logical in InputBindings.LOGICAL:
		for device in ["keyboard", "pad"]:
			var cell: Button = panel._cells["%s/%s" % [logical, device]]
			var icon := cell.get_node("Binding") as PromptIcon
			cells.append("%s|%s|%d|%s" % [esc(cell.text), "1" if cell.disabled else "0", icon.family_override,
				"1" if icon.shows_anything() else "0"])
	var resets := PackedStringArray()
	for logical in InputBindings.LOGICAL:
		resets.append("1" if (panel._resets[logical] as Button).disabled else "0")
	var checks := PackedStringArray()
	for key in CHECKS:
		checks.append("1" if (panel._checks[key] as CheckBox).button_pressed else "0")
	var s := SettingsManager
	return PackedStringArray([esc(panel._status.text), panel.listening_key() if panel.listening_key() != "" else "-",
		";".join(cells), ",".join(resets), ",".join(checks),
		h((panel._sliders["cursor_speed"] as HSlider).value), h((panel._sliders["stick_deadzone"] as HSlider).value),
		esc((panel._slider_labels["cursor_speed"] as Label).text), esc((panel._slider_labels["stick_deadzone"] as Label).text),
		h(s.cursor_speed), h(s.stick_deadzone), str(ControlsPanel.pad_column_family()), str(InputDevice.family),
		str(InputDevice.detected_family), saved_text(s.bindings)])


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261106
	var dialog := load("res://scenes/ui/settings_dialog.tscn").instantiate() as SettingsDialog
	add_child(dialog)
	await get_tree().process_frame
	dialog.set_process(false)  # no input polling: the moves are the fixture's
	var panel: ControlsPanel = dialog.controls_panel
	panel.set_process(false)   # the timeout runs on the fixture's clock (panel._process(dt))
	dialog.open()
	dialog.show_section("Controls")
	await get_tree().process_frame

	var header := ["row", "op", "arg", "result", "status", "listening", "cells", "resets", "checks", "cursor_slider",
		"deadzone_slider", "cursor_line", "deadzone_line", "cursor_speed", "deadzone", "pad_column", "family", "detected",
		"saved"]
	var out := FileAccess.open("res://controls_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(header)))
	var labels := FileAccess.open("res://controls_words.tsv", FileAccess.WRITE)
	labels.store_line("# what\ttext")
	for logical in InputBindings.LOGICAL:
		var row := (panel._resets[logical] as Button).get_parent()
		labels.store_line("row_%s\t%s" % [logical, esc((row.get_node("Label") as Label).text)])
	labels.store_line("reset\t%s" % esc((panel._resets["pounce"] as Button).text))
	labels.store_line("reset_all\t%s" % esc(panel._reset_all.text))
	for key in CHECKS:
		labels.store_line("check_%s\t%s" % [key, esc((panel._checks[key] as CheckBox).text)])
	labels.close()

	var counts := {}
	for r in range(ROWS):
		# --- the start: everything set, and recorded ---
		var saved := {}
		if rng.randf() < 0.6:
			var st := InputBindings.working_state({})
			for i in range(rng.randi_range(1, 4)):
				var logical: String = InputBindings.LOGICAL.keys()[rng.randi_range(0, InputBindings.LOGICAL.size() - 1)]
				var b = random_binding(rng)[1]
				if b != null:
					InputBindings.rebind(st, logical, b)
			saved = InputBindings.diff_from_defaults(st)
		SettingsManager.set_bindings(saved)
		var flags := PackedStringArray()
		for key in CHECKS:
			var on := rng.randf() < (0.8 if key == "show_prompts" or key == "tutorial_tips" else 0.25)
			SettingsManager.set_control(key, on)
			flags.append("1" if on else "0")
		var speed: float = [0.5, 1.0, 1.25, 2.0, rng.randf_range(0.5, 2.0)][rng.randi_range(0, 4)]
		var dz: float = [0.05, 0.2, 0.35, 0.5, rng.randf_range(0.05, 0.5)][rng.randi_range(0, 4)]
		SettingsManager.set_control("cursor_speed", speed)
		SettingsManager.set_control("stick_deadzone", dz)
		var style: String = STYLES[rng.randi_range(0, STYLES.size() - 1)]
		SettingsManager.set_prompt_style(style)
		var detected: int = rng.randi_range(0, 4)
		InputDevice._set_detected(detected)
		panel._listening = ""
		panel._captured_frame = -10
		panel._status.text = ""
		panel.sync()
		var arg := "%s/%s/%s/%s/%s/%d/%s" % [saved_text(saved), ",".join(flags), h(speed), h(dz), style, detected, "-"]
		var row := PackedStringArray([str(r), "start", arg, "-"])
		row.append_array(state(dialog, panel))
		out.store_line("\t".join(row))
		# --- the moves ---
		for k in range(OPS):
			var roll := rng.randf()
			var op := ""
			arg = "-"
			var result := "-"
			var logical: String = InputBindings.LOGICAL.keys()[rng.randi_range(0, InputBindings.LOGICAL.size() - 1)]
			if roll < 0.2:
				op = "listen"
				var device := "keyboard" if rng.randf() < 0.5 else "pad"
				arg = "%s/%s" % [logical, device]
				var cell: Button = panel._cells[arg]
				if cell.disabled:
					op = "listen_disabled"  # a player cannot press a disabled cell: nothing happens
				else:
					cell.pressed.emit()
			elif roll < 0.55:
				op = "event"
				var e = random_event(rng)
				arg = e[0]
				panel._input(e[1])
			elif roll < 0.62:
				op = "tick"
				var dt: float = [0.5, 1.0, 2.0, 4.9, 5.0, 5.1][rng.randi_range(0, 5)]  # 5.0: the timeout's exact edge (<= 0)
				arg = h(dt)
				panel._process(dt)
			elif roll < 0.68:
				op = "reset_row"
				arg = logical
				var reset: Button = panel._resets[logical]
				if reset.disabled:
					op = "reset_disabled"
				else:
					reset.pressed.emit()
			elif roll < 0.7:
				op = "reset_all"
				panel._reset_all.pressed.emit()
			elif roll < 0.78:
				op = "check"
				var key: String = CHECKS[rng.randi_range(0, CHECKS.size() - 1)]
				arg = key
				var box: CheckBox = panel._checks[key]
				box.button_pressed = not box.button_pressed
			elif roll < 0.84:
				op = "slider"
				var key: String = ["cursor_speed", "stick_deadzone"][rng.randi_range(0, 1)]
				var v := rng.randf_range(0.0, 2.5)
				arg = "%s=%s" % [key, h(v)]
				(panel._sliders[key] as HSlider).value = v
			elif roll < 0.88:
				op = "style"
				var st: String = STYLES[rng.randi_range(0, STYLES.size() - 1)]
				arg = st
				SettingsManager.set_prompt_style(st)
			else:
				op = "bind"
				var b = random_binding(rng)
				if b[1] == null:
					op = "bind_none"
				else:
					arg = "%s/%s" % [logical, b[0]]
					result = str(panel.bind(logical, b[1]))
			counts[op] = counts.get(op, 0) + 1
			row = PackedStringArray([str(r), op, arg, result])
			row.append_array(state(dialog, panel))
			out.store_line("\t".join(row))
		# Every redraw queue_free()s the old prompts: let a frame free them (thousands piled up and crashed Godot).
		await get_tree().process_frame
	out.close()
	print("CONTROLS_FIXTURE ops=%s" % [counts])
	print("CONTROLS_FIXTURE done")
	get_tree().quit(0)
