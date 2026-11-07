extends Node

# Ground truth for UPawHalfAxis (docs/05 §3): what Godot's own Input makes of a stick or trigger value for every action the
# project binds to an axis (project.godot [input]: the action's dead zone, the event's direction). Each row sends one
# InputEventJoypadMotion through Input.parse_input_event and records Input.is_action_pressed / get_action_strength /
# get_action_raw_strength for that action; the axis is put back to 0 between rows. Values: random, plus the edges
# (0, +-1, each action's exact dead zone and a float step either side of it).

const ROWS_PER_ACTION := 400


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func flag(b: bool) -> String:
	return "1" if b else "0"


func send(axis: int, value: float) -> void:
	var e := InputEventJoypadMotion.new()
	e.device = 0
	e.axis = axis
	e.axis_value = value
	Input.parse_input_event(e)
	Input.flush_buffered_events()


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261104
	var out := FileAccess.open("res://input_axis_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["action", "axis", "bound_sign", "deadzone", "value", "pressed", "strength", "raw"])))
	var rows := 0
	for action in InputMap.get_actions():
		if String(action).begins_with("ui_") and not ProjectSettings.has_setting("input/" + action):
			continue
		for ev in InputMap.action_get_events(action):
			var jm := ev as InputEventJoypadMotion
			if jm == null:
				continue
			var dz := InputMap.action_get_deadzone(action)
			var values: Array[float] = [0.0, 1.0, -1.0, dz, -dz]
			values.append(dz + 0.000001)
			values.append(dz - 0.000001)
			values.append(-dz + 0.000001)
			values.append(-dz - 0.000001)
			for i in range(ROWS_PER_ACTION):
				values.append(rng.randf_range(-1.0, 1.0))
			for v in values:
				send(jm.axis, v)
				# An event's axis_value is a float: record the value Godot actually holds.
				var held := float(v)
				var probe := InputEventJoypadMotion.new()
				probe.axis_value = v
				held = probe.axis_value
				out.store_line("\t".join(PackedStringArray([String(action), str(jm.axis), "1" if jm.axis_value > 0 else "-1",
					h(dz), h(held), flag(Input.is_action_pressed(action)), h(Input.get_action_strength(action)),
					h(Input.get_action_raw_strength(action))])))
				rows += 1
				send(jm.axis, 0.0)
	out.close()

	# Input.get_vector on the steer stick (the right stick: steer_left/right on axis 2, steer_up/down on axis 3), as the cat
	# and the skill tree read it: random deflections, the dead zone's ring, the rim and beyond the unit circle.
	var vec := FileAccess.open("res://input_vector_fixture.tsv", FileAccess.WRITE)
	vec.store_line("# " + "\t".join(PackedStringArray(["x", "y", "vx", "vy"])))
	var pairs: Array = [[0.0, 0.0], [1.0, 1.0], [-1.0, 0.0], [0.0, 1.0], [0.2, 0.0], [0.0, -0.2], [0.141, 0.141], [0.15, 0.15]]
	for i in range(3000):
		var a := rng.randf_range(0.0, TAU)
		var r := rng.randf_range(0.0, 1.45) if i % 3 != 0 else rng.randf_range(0.0, 0.3)
		pairs.append([clampf(cos(a) * r, -1.0, 1.0), clampf(sin(a) * r, -1.0, 1.0)])
	for p in pairs:
		send(2, p[0])
		send(3, p[1])
		var px := InputEventJoypadMotion.new()
		px.axis_value = p[0]
		var py := InputEventJoypadMotion.new()
		py.axis_value = p[1]
		var v := Input.get_vector("steer_left", "steer_right", "steer_up", "steer_down")
		vec.store_line("\t".join(PackedStringArray([h(px.axis_value), h(py.axis_value), h(v.x), h(v.y)])))
		send(2, 0.0)
		send(3, 0.0)
	vec.close()
	print("INPUT_AXIS_FIXTURE rows=%d" % rows)
	print("INPUT_AXIS_FIXTURE done")
	get_tree().quit(0)
