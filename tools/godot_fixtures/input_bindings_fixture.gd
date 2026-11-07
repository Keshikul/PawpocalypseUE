extends Node

# Ground truth for PawCore's PcBindings (scripts/systems/input_bindings.gd: the logical rows, rebind() with its swaps and
# refusals, reset_row(), the saved diff, working_state(), apply() with Swap sticks / Swap confirm and back and the stick dead
# zone, binding_from()) and for the [bindings] section of settings.cfg (settings_manager.gd save_settings / load_settings).
# Scratch copy of the project, sandboxed user:// (tools/make_godot_fixtures.py).
#
#   input_bindings_fixture.tsv   3,000 rows x 8 random ops (rebind, reset a row, reset all, a reload through the saved diff);
#                                after each: the result, the diff as saved, the event each row's cell shows per device; then
#                                apply() with random switches: every project action's live events and the steer dead zone
#   input_binding_from.tsv       binding_from() on 3,000 random raw events
#   input_bindings_cfg.tsv       settings.cfg with random [bindings] (hand edits too): the text save_settings() wrote, what
#                                load_settings() read back, the working state made from it, the text a second save writes
#   input_bindings_defaults.tsv  the project's map as InputBindings captured it, in InputMap order (project actions only)
#
# An event is written k<physical keycode>, m<mouse button>, b<joy button>, a<joy axis><sign +/-/0>; none is "-".

const ROWS := 3000
const OPS := 8
const FROM_ROWS := 3000
const CFG_ROWS := 1500

# Keys a player might press: every key the project binds, free letters, digits, Esc, Tab, F1, Shift, keypad Enter.
const KEYS := [KEY_A, KEY_D, KEY_S, KEY_W, KEY_Q, KEY_T, KEY_U, KEY_SPACE, KEY_ENTER, KEY_KP_ENTER, KEY_LEFT, KEY_RIGHT,
	KEY_UP, KEY_DOWN, KEY_PAGEUP, KEY_PAGEDOWN, KEY_ESCAPE, KEY_1, KEY_2, KEY_6, KEY_7, KEY_0, KEY_EQUAL, KEY_MINUS,
	KEY_I, KEY_J, KEY_K, KEY_L, KEY_O, KEY_P, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_E, KEY_R, KEY_F, KEY_G, KEY_TAB, KEY_F1,
	KEY_SHIFT, KEY_BACKSPACE]


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


func events_text(list: Array) -> String:
	var out := PackedStringArray()
	for e in list:
		out.append(ev(e))
	return ",".join(out)


# The actions project.godot's [input] names (Data/InputMap.csv's), in InputMap order: Godot's own editor actions
# (ui_copy, ui_text_*) are not the game's.
func project_actions() -> Array:
	var cfg := ConfigFile.new()
	cfg.load("res://project.godot")
	var named := cfg.get_section_keys("input")
	var out: Array = []
	for action in InputMap.get_actions():
		if named.has(String(action)):
			out.append(String(action))
	return out


func state_text(state: Dictionary, actions: Array) -> String:
	var out := PackedStringArray()
	for action in actions:
		out.append("%s=%s" % [action, events_text(state.get(action, []))])
	return ";".join(out)


func live_text(actions: Array) -> String:
	var out := PackedStringArray()
	for action in actions:
		out.append("%s=%s" % [action, events_text(InputMap.action_get_events(action))])
	return ";".join(out)


func val_text(v) -> String:
	match typeof(v):
		TYPE_INT:
			return "i" + str(v)
		TYPE_FLOAT:
			return "f" + str(v)
		TYPE_STRING:
			return "s" + v
		TYPE_BOOL:
			return "b1" if v else "b0"
	return "?" + type_string(typeof(v))


func dict_text(d) -> String:
	if not (d is Dictionary):
		return "<" + type_string(typeof(d)) + ">"
	var kv := PackedStringArray()
	for k in d:
		kv.append("%s:%s" % [k, val_text(d[k])])
	return "{" + "|".join(kv) + "}"


func saved_text(saved: Dictionary) -> String:
	var parts := PackedStringArray()
	for action in saved:
		var list = saved[action]
		if not (list is Array):
			parts.append("%s=<%s>" % [action, type_string(typeof(list))])
			continue
		var evs := PackedStringArray()
		for d in list:
			evs.append(dict_text(d))
		parts.append("%s=[%s]" % [action, ",".join(evs)])
	return ";".join(parts)


func cells_text(state: Dictionary) -> String:
	var out := PackedStringArray()
	for logical in InputBindings.LOGICAL:
		for device in ["keyboard", "pad"]:
			out.append(ev(InputBindings.event_for(logical, device, state)))
	return ",".join(out)


func random_event(rng: RandomNumberGenerator) -> InputEvent:
	var roll := rng.randf()
	if roll < 0.5:
		var k := InputEventKey.new()
		k.physical_keycode = KEYS[rng.randi_range(0, KEYS.size() - 1)]
		return k
	if roll < 0.58:
		var m := InputEventMouseButton.new()
		m.button_index = [MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE, MOUSE_BUTTON_XBUTTON1,
			MOUSE_BUTTON_WHEEL_UP][rng.randi_range(0, 4)]
		return m
	if roll < 0.9:
		var b := InputEventJoypadButton.new()
		b.button_index = rng.randi_range(0, 15) as JoyButton
		return b
	var a := InputEventJoypadMotion.new()
	if rng.randf() < 0.7:
		a.axis = [JOY_AXIS_TRIGGER_LEFT, JOY_AXIS_TRIGGER_RIGHT][rng.randi_range(0, 1)]
		a.axis_value = 1.0
	else:
		a.axis = rng.randi_range(0, 3) as JoyAxis
		a.axis_value = -1.0 if rng.randf() < 0.5 else 1.0
	return a


func random_logical(rng: RandomNumberGenerator) -> String:
	var ids: Array = InputBindings.LOGICAL.keys()
	return ids[rng.randi_range(0, ids.size() - 1)]


# A few random rebinds from the defaults: a saved diff as a player would leave it.
func random_saved(rng: RandomNumberGenerator, n: int) -> Dictionary:
	var state := InputBindings.working_state({})
	for i in range(n):
		InputBindings.rebind(state, random_logical(rng), random_event(rng))
	return InputBindings.diff_from_defaults(state)


# What a hand-edited settings.cfg can hold that serialize() never writes.
func hand_edit(rng: RandomNumberGenerator, saved: Dictionary) -> void:
	var odd := [
		{"type": "key", "physical_keycode": 74.0},
		{"type": "mouse"},
		{"type": "joy_axis", "axis": 5},
		{"type": "joy_axis", "axis": 4, "sign": -1.0},
		{"type": "joy_axis", "axis": 2, "sign": 0},
		{"type": "bogus", "button": 3},
		{},
		{"type": "key", "physical_keycode": 0},
		{"type": "key", "physical_keycode": "75"},
		{"type": "joy_button", "button": true},
		{"type": "joy_button", "button": 2.5},
		{"physical_keycode": 76, "type": "key"},
		{"type": "key", "physical_keycode": 4194306, "note": "tab"},
	]
	var actions := ["pounce", "press", "ui_accept", "menu_up", "tab_next", "buy_amount", "recentre", "jump", "zoom_in"]
	for i in range(rng.randi_range(1, 3)):
		var action: String = actions[rng.randi_range(0, actions.size() - 1)]
		var roll := rng.randf()
		if roll < 0.15:
			saved[action] = []
		elif roll < 0.25:
			saved[action] = [5, "x", 2.0][rng.randi_range(0, 2)]   # not a list: load_settings() drops it
		else:
			var list: Array = []
			for j in range(rng.randi_range(1, 3)):
				if rng.randf() < 0.5:
					list.append(odd[rng.randi_range(0, odd.size() - 1)].duplicate())
				else:
					list.append(InputBindings.serialize(random_event(rng)))
			saved[action] = list


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261105
	var actions := project_actions()
	InputBindings.working_state({})  # the defaults, captured before anything changes the map

	# --- the defaults --------------------------------------------------------------------------------------------------
	var d := FileAccess.open("res://input_bindings_defaults.tsv", FileAccess.WRITE)
	d.store_line("# action\tevents\tdeadzone")
	for action in actions:
		d.store_line("%s\t%s\t%s" % [action, events_text(InputBindings.defaults_for(action)), h(InputMap.action_get_deadzone(action))])
	d.close()

	# --- the rules -----------------------------------------------------------------------------------------------------
	var out := FileAccess.open("res://input_bindings_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["row", "op", "arg", "result", "other", "saved", "cells", "swap_sticks",
		"swap_confirm", "deadzone", "steer_dz", "live"])))
	var counts := {}
	var carry: Dictionary = {}
	for r in range(ROWS):
		var start: Dictionary = carry if rng.randf() < 0.5 else {}
		var state := InputBindings.working_state(start)
		# A hand-edited settings.cfg can leave a row with nothing on one device (the defaults never do): one row's FIRST member
		# loses every event of one device, so a rebind there adds a key the mirrors do not get.
		if rng.randf() < 0.25:
			var logical := random_logical(rng)
			var primary: String = InputBindings.members(logical)[0]
			var device := "keyboard" if rng.randf() < 0.5 else "pad"
			var kept: Array = []
			for e in state[primary]:
				if InputBindings.device_of(e) != device:
					kept.append(e)
			if not kept.is_empty():
				state[primary] = kept
		out.store_line("\t".join(PackedStringArray([str(r), "start", "-", "-", "-",
			saved_text(InputBindings.diff_from_defaults(state)), cells_text(state), "-", "-", "-", "-", "-"])))
		for k in range(OPS):
			var roll := rng.randf()
			var op := ""
			var arg := "-"
			var result := "-"
			var other := "-"
			if roll < 0.72:
				op = "rebind"
				var logical := random_logical(rng)
				var e := random_event(rng)
				arg = "%s/%s" % [logical, ev(e)]
				var res: Array = InputBindings.rebind(state, logical, e)
				result = str(res[0])
				other = String(res[1]) if String(res[1]) != "" else "-"
			elif roll < 0.86:
				op = "reset_row"
				arg = random_logical(rng)
				InputBindings.reset_row(state, arg)
			elif roll < 0.92:
				op = "reset_all"
				state = InputBindings.working_state({})
			else:
				op = "reload"
				state = InputBindings.working_state(InputBindings.diff_from_defaults(state))
			counts[op + ("" if result == "-" else "_" + result)] = counts.get(op + ("" if result == "-" else "_" + result), 0) + 1
			out.store_line("\t".join(PackedStringArray([str(r), op, arg, result, other,
				saved_text(InputBindings.diff_from_defaults(state)), cells_text(state), "-", "-", "-", "-", "-"])))
		carry = InputBindings.diff_from_defaults(state)
		var ss := rng.randf() < 0.4
		var sc := rng.randf() < 0.4
		var dz: float = rng.randf_range(-0.1, 1.0) if rng.randf() < 0.7 else [0.05, 0.2, 0.5, 0.9][rng.randi_range(0, 3)]
		InputBindings.apply(carry, ss, sc, dz)
		out.store_line("\t".join(PackedStringArray([str(r), "apply", "-", "-", "-", saved_text(carry), "-",
			"1" if ss else "0", "1" if sc else "0", h(dz), h(InputMap.action_get_deadzone("steer_left")), live_text(actions)])))
	InputBindings.apply({}, false, false, 0.2)
	out.close()

	# --- binding_from ----------------------------------------------------------------------------------------------------
	var f := FileAccess.open("res://input_binding_from.tsv", FileAccess.WRITE)
	f.store_line("# kind\tcode\tkeycode\tpressed\techo\tvalue\tout")
	for r in range(FROM_ROWS):
		var roll := rng.randf()
		var kind := ""
		var code := 0
		var keycode := 0
		var pressed := rng.randf() < 0.8
		var echo := rng.randf() < 0.15
		var value := "-"
		var e: InputEvent
		if roll < 0.35:
			kind = "key"
			var k := InputEventKey.new()
			code = KEYS[rng.randi_range(0, KEYS.size() - 1)] if rng.randf() < 0.8 else 0
			keycode = KEYS[rng.randi_range(0, KEYS.size() - 1)] if rng.randf() < 0.5 else 0
			k.physical_keycode = code
			k.keycode = keycode
			k.pressed = pressed
			k.echo = echo
			e = k
		elif roll < 0.55:
			kind = "mouse"
			var m := InputEventMouseButton.new()
			code = rng.randi_range(1, 9)
			m.button_index = code
			m.pressed = pressed
			e = m
		elif roll < 0.75:
			kind = "joy_button"
			var b := InputEventJoypadButton.new()
			code = rng.randi_range(0, 15)
			b.button_index = code
			b.pressed = pressed
			e = b
		else:
			kind = "joy_axis"
			var a := InputEventJoypadMotion.new()
			code = rng.randi_range(0, 5)
			a.axis = code
			var v: float = [-1.0, -0.75, -0.5, -0.4999, -0.3, 0.0, 0.3, 0.4999, 0.5, 0.75, 1.0][rng.randi_range(0, 10)]
			a.axis_value = v
			value = str(v)
			e = a
		f.store_line("%s\t%d\t%d\t%s\t%s\t%s\t%s" % [kind, code, keycode, "1" if pressed else "0", "1" if echo else "0", value,
			ev(InputBindings.binding_from(e))])
	f.close()

	# --- settings.cfg [bindings] -----------------------------------------------------------------------------------------
	var c := FileAccess.open("res://input_bindings_cfg.tsv", FileAccess.WRITE)
	c.store_line("# row\twritten\tloaded\twork\trewritten")
	for r in range(CFG_ROWS):
		var saved := {} if rng.randf() < 0.1 else random_saved(rng, rng.randi_range(1, 6))
		if rng.randf() < 0.35:
			hand_edit(rng, saved)
		SettingsManager.bindings = saved
		SettingsManager.save_settings()
		var written := FileAccess.get_file_as_string(SettingsManager.SETTINGS_PATH)
		SettingsManager.load_settings()
		var loaded: Dictionary = SettingsManager.bindings
		var work := InputBindings.working_state(loaded)
		SettingsManager.save_settings()
		var rewritten := FileAccess.get_file_as_string(SettingsManager.SETTINGS_PATH)
		c.store_line("%d\t%s\t%s\t%s\t%s" % [r, esc(written), saved_text(loaded), state_text(work, actions), esc(rewritten)])
	c.close()
	SettingsManager.bindings = {}
	print("INPUT_BINDINGS_FIXTURE ops=%s" % [counts])
	print("INPUT_BINDINGS_FIXTURE done")
	get_tree().quit(0)
