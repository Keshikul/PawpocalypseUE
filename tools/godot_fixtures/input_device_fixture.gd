extends Node

# Ground truth for PawCore's PcDevice and UPawInputDeviceSubsystem: the REAL InputDevice autoload (input_device.gd), driven
# through its public observe() with synthesized events, as its own gate drives it (a headless run has no pads and no OS input).
#
# Each row: a fresh device state (the autoload's fields put back), some pads registered with random vendor / product / names,
# a random prompt style and Swap-sticks switch, then 10 random steps: a key, a click, mouse motion, a pad button, a stick or
# trigger motion, a warp of the pointer, the stick claimed (R3), a pad connected or disconnected, the prompt style changed, a
# modal's press guard asked. Real time passes between some steps (OS.delay_msec), so the mouse-quiet, warp-echo and
# boot-quiet windows are crossed both ways; every step records the clock it ran at, and the port replays that clock.
# After every step: the family, what was detected, the cursor source, the active pad, the pointer-last flag, the hidden
# pointer, the mouse travel, when the press began, the toast. Second table: family_from_info on random ids and names.

const ROWS := 700
const STEPS := 10
const STYLES := ["auto", "keyboard", "xbox", "playstation", "steam_deck"]
const NAMES := ["Xbox Wireless Controller", "DualSense Wireless Controller", "PS4 Controller", "Wireless Controller",
	"XInput Controller", "Steam Deck Controller", "Generic USB Joystick", "8BitDo Pro 2", "PLAYSTATION(R)3 Controller", ""]
const VENDORS := [0x054C, 0x045E, 0x28DE, 0x2DC8, 0x0000]
const PRODUCTS := [0x1205, 0x11FF, 0x0CE6, 0x02EA, 0x0001]


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func flag(b: bool) -> String:
	return "1" if b else "0"


func esc(s: String) -> String:
	return s.replace("\\", "\\\\").replace("\n", "\\n").replace("\t", "\\t")


func info_for(rng: RandomNumberGenerator) -> Array:
	var info := {}
	if rng.randf() < 0.7:
		info["vendor_id"] = str(VENDORS[rng.randi_range(0, VENDORS.size() - 1)])
		info["product_id"] = str(PRODUCTS[rng.randi_range(0, PRODUCTS.size() - 1)])
	if rng.randf() < 0.5:
		info["raw_name"] = NAMES[rng.randi_range(0, NAMES.size() - 1)]
	return [info, NAMES[rng.randi_range(0, NAMES.size() - 1)]]


func state(d) -> PackedStringArray:
	return PackedStringArray([str(d.family), str(d.detected_family), str(d.cursor_source), str(d.active_device),
		flag(d.last_was_pointer), flag(d.pointer_hidden), h(d._mouse_travel), str(d._press_began_msec), flag(d._press_axis_down),
		str(d._last_motion_msec), str(d._warp_until_msec), flag(d.toast_visible()), esc(d._toast_label.text)])


func _ready() -> void:
	var d = get_node("/root/InputDevice")
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261104
	var out := FileAccess.open("res://input_device_fixture.tsv", FileAccess.WRITE)
	var head := ["row", "step", "op", "now", "after", "a", "b", "c", "e", "family", "detected", "cursor", "active", "pointer_last", "hidden",
		"travel", "press_began", "press_axis", "last_motion", "warp_until", "toast", "toast_text"]
	out.store_line("# " + "\t".join(PackedStringArray(head)))
	var counts := {}
	var moved := 0  # steps whose clock ticked on while they ran (their recorded time may be a millisecond off)
	for r in range(ROWS):
		# A fresh state, as at boot; the boot clock is the row's start (so "already plugged in" is quiet for 2 s).
		d.family = InputGlyphs.Family.KEYBOARD
		d.detected_family = InputGlyphs.Family.KEYBOARD
		d.cursor_source = 0
		d.active_device = -1
		d._device_families = {}
		d._mouse_travel = 0.0
		d._last_motion_msec = 0
		d._warp_until_msec = 0
		d._press_began_msec = -1
		d._press_axis_down = false
		d.pointer_hidden = false
		d.last_was_pointer = true
		d.pointer_over_field = false
		d._toast.visible = false
		d._toast_label.text = ""
		d._boot_msec = Time.get_ticks_msec() - (0 if rng.randf() < 0.5 else 2500)
		SettingsManager.prompt_style = STYLES[rng.randi_range(0, STYLES.size() - 1)]
		SettingsManager.swap_sticks = rng.randf() < 0.25
		d._refresh_family()
		var t_reset := Time.get_ticks_msec()
		var row := PackedStringArray([str(r), "-1", "reset", str(t_reset), str(t_reset), SettingsManager.prompt_style,
			flag(SettingsManager.swap_sticks), str(d._boot_msec), "-"])
		row.append_array(state(d))
		out.store_line("\t".join(row))
		# Pads 0..2 registered with what the OS would say about them.
		for dev in range(3):
			var pair := info_for(rng)
			d.register_device(dev, pair[0], pair[1])
			var t_reg := Time.get_ticks_msec()
			row = PackedStringArray([str(r), "-1", "register", str(t_reg), str(t_reg), str(dev),
				esc(JSON.stringify(pair[0])), esc(pair[1]), "-"])
			row.append_array(state(d))
			out.store_line("\t".join(row))
		for k in range(STEPS):
			if rng.randf() < 0.18:
				OS.delay_msec(rng.randi_range(0, 300))
			var roll := rng.randi_range(0, 13)
			var op := ""
			var a := "-"
			var b := "-"
			var c := "-"
			var e := "-"
			# The step runs at the START of a fresh millisecond, so the clock observe() reads inside is the one recorded.
			var tick := Time.get_ticks_msec()
			while Time.get_ticks_msec() == tick:
				pass
			var now := Time.get_ticks_msec()
			match roll:
				0, 1:
					op = "key"
					var ev := InputEventKey.new()
					ev.physical_keycode = [KEY_SPACE, KEY_A, KEY_ENTER, KEY_W][rng.randi_range(0, 3)]
					ev.pressed = rng.randf() < 0.8
					ev.echo = ev.pressed and rng.randf() < 0.2
					a = str(ev.physical_keycode)
					b = flag(ev.pressed)
					c = flag(ev.echo)
					e = flag(ev.is_action("press"))
					d.observe(ev)
				2:
					op = "click"
					var ev := InputEventMouseButton.new()
					ev.button_index = MOUSE_BUTTON_LEFT
					ev.pressed = rng.randf() < 0.6
					b = flag(ev.pressed)
					d.observe(ev)
				3, 4:
					op = "motion"
					var ev := InputEventMouseMotion.new()
					ev.relative = Vector2(rng.randf_range(-5.0, 5.0), rng.randf_range(-5.0, 5.0))
					a = h(ev.relative.length())
					d.observe(ev)
				5, 6:
					op = "pad_button"
					var ev := InputEventJoypadButton.new()
					ev.device = rng.randi_range(0, 3)
					ev.button_index = [JOY_BUTTON_A, JOY_BUTTON_B, JOY_BUTTON_START][rng.randi_range(0, 2)]
					ev.pressed = rng.randf() < 0.75
					a = str(ev.device)
					b = flag(ev.pressed)
					c = str(ev.button_index)
					e = flag(ev.is_action("press"))
					d.observe(ev)
				7, 8, 9:
					op = "pad_motion"
					var ev := InputEventJoypadMotion.new()
					ev.device = rng.randi_range(0, 3)
					ev.axis = rng.randi_range(0, 5)
					ev.axis_value = rng.randf_range(-1.0, 1.0) if ev.axis < 4 else rng.randf_range(0.0, 1.0)
					a = str(ev.device)
					b = str(ev.axis)
					c = h(ev.axis_value)
					e = flag(ev.is_action("press")) + flag(ev.is_action_pressed("press"))
					d.observe(ev)
				10:
					op = "warp"
					d.warp_pointer(get_viewport(), Vector2(100, 100))
				11:
					op = "claim_stick"
					d.claim_stick()
				12:
					op = "joy"
					var dev := rng.randi_range(0, 3)
					var connected := rng.randf() < 0.5
					a = str(dev)
					b = flag(connected)
					d._on_joy_connection_changed(dev, connected)
				13:
					if rng.randf() < 0.5:
						op = "style"
						SettingsManager.prompt_style = STYLES[rng.randi_range(0, STYLES.size() - 1)]
						a = SettingsManager.prompt_style
						d._refresh_family()
					else:
						op = "guard"
						var opened := now - rng.randi_range(-100, 900)
						a = str(opened)
						b = flag(d.press_counts_for(opened))
			counts[op] = counts.get(op, 0) + 1
			var after := Time.get_ticks_msec()
			if after != now:
				moved += 1
			row = PackedStringArray([str(r), str(k), op, str(now), str(after), a, b, c, e])
			row.append_array(state(d))
			out.store_line("\t".join(row))
	out.close()

	var fam := FileAccess.open("res://input_family_fixture.tsv", FileAccess.WRITE)
	fam.store_line("# " + "\t".join(PackedStringArray(["vendor", "product", "name", "raw_name", "on_deck", "family"])))
	for i in range(3000):
		var info := {}
		var vid: int = VENDORS[rng.randi_range(0, VENDORS.size() - 1)] if rng.randf() < 0.8 else rng.randi_range(0, 0xFFFF)
		var pid: int = PRODUCTS[rng.randi_range(0, PRODUCTS.size() - 1)] if rng.randf() < 0.8 else rng.randi_range(0, 0xFFFF)
		info["vendor_id"] = str(vid)
		info["product_id"] = str(pid)
		var raw: String = NAMES[rng.randi_range(0, NAMES.size() - 1)]
		if rng.randf() < 0.3:
			raw = raw.to_upper()
		info["raw_name"] = raw
		var name: String = NAMES[rng.randi_range(0, NAMES.size() - 1)]
		var deck := rng.randf() < 0.3
		fam.store_line("\t".join(PackedStringArray([str(vid), str(pid), esc(name), esc(raw), flag(deck),
			str(InputDevice.family_from_info(info, name, deck))])))
	fam.close()

	# Player.stick_velocity: the cat's speed for a stick deflection (get_vector's output) and a field width.
	var vel := FileAccess.open("res://input_velocity_fixture.tsv", FileAccess.WRITE)
	vel.store_line("# " + "\t".join(PackedStringArray(["ix", "iy", "width", "vx", "vy"])))
	for i in range(2000):
		var a := rng.randf_range(0.0, TAU)
		var m := rng.randf_range(0.0, 1.0) if i % 10 != 0 else (0.0 if i % 20 == 0 else 1.0)
		var input := Vector2(cos(a), sin(a)) * m
		var width := rng.randf_range(600.0, 3000.0)
		var v := Player.stick_velocity(input, width)
		vel.store_line("\t".join(PackedStringArray([h(input.x), h(input.y), h(width), h(v.x), h(v.y)])))
	vel.close()
	print("INPUT_DEVICE_FIXTURE rows=%d ops=%s clock_moved=%d" % [ROWS, counts, moved])
	print("INPUT_DEVICE_FIXTURE done")
	get_tree().quit(0)
