extends Node

# Ground truth for PawCore's PcJuice (phase 6 slice 5c, the UI's motion; tools/make_godot_fixtures.py, scratch copy).
#
# juice_curves.tsv  Godot's own Tween easing: Tween.interpolate_value(initial, delta, elapsed, duration, trans, ease) is the
#                   function every Tween step evaluates, so sampling it IS sampling the tweens. Each transition UiJuice uses
#                   (and LINEAR as a control) with each ease, at 401 points of a 1 s tween from 0 by 1, and at 41 points of
#                   UiJuice's hover (from 1 by 0.06 over 0.16 s).
# juice_seq.tsv     the REAL UiJuice (scripts/systems/ui_juice.gd) on a real Button: SEQS random sequences of its events
#                   (hover enter / exit, button down / up, punch, pop_in, jitter on / off, disabled, hidden, the effects
#                   setting) and frames of random length, each frame the live tweens stepped by hand (Tween.custom_step:
#                   the fixture never yields, so the SceneTree never steps them itself); after each line the Button's scale
#                   and rotation.
# juice_leans.tsv   each jitter step's lean, in order: UiJuice draws it with the global randf_range, which is
#                   RandomPCG::random(double, double) = randd(): THREE 32-bit draws (RandomNumberGenerator.randf_range is
#                   the float one, two draws, and drifts off after the first lean). A RandomNumberGenerator seeded the same
#                   (Godot seeds both the same way) rebuilds randd from its randi() beside it; every step that lands is
#                   checked against its lean, and the fixture refuses to finish on a mismatch.

const STEPS := 400
const SEQS := 160
const EVENTS := 60
const TRANS := {"linear": Tween.TRANS_LINEAR, "sine": Tween.TRANS_SINE, "quad": Tween.TRANS_QUAD,
	"back": Tween.TRANS_BACK, "elastic": Tween.TRANS_ELASTIC}
const EASES := {"in": Tween.EASE_IN, "out": Tween.EASE_OUT, "in_out": Tween.EASE_IN_OUT, "out_in": Tween.EASE_OUT_IN}
const OPS := ["enter", "exit", "down", "up", "punch", "pop_in", "jitter_on", "jitter_off", "disable", "enable", "hide", "show",
	"effects_off", "effects_on"]


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func _ready() -> void:
	_curves()
	if not _sequences():
		return
	print("JUICE_FIXTURE done")
	get_tree().quit(0)


func _curves() -> void:
	var out := FileAccess.open("res://juice_curves.tsv", FileAccess.WRITE)
	out.store_line("# trans\tease\tinitial\tdelta\tduration\telapsed\tvalue")
	for tn in TRANS:
		for en in EASES:
			for i in range(STEPS + 1):
				var t := float(i) / STEPS
				out.store_line("\t".join(PackedStringArray([tn, en, h(0.0), h(1.0), h(1.0), h(t),
					h(Tween.interpolate_value(0.0, 1.0, t, 1.0, TRANS[tn], EASES[en]))])))
			for i in range(41):
				var e := 0.16 * float(i) / 40.0
				out.store_line("\t".join(PackedStringArray([tn, en, h(1.0), h(0.06), h(0.16), h(e),
					h(Tween.interpolate_value(1.0, 0.06, e, 0.16, TRANS[tn], EASES[en]))])))
	out.close()


# RandomPCG::randd (core/math/random_pcg.h) from three randi() draws: the exponent from the first's leading zeros, the
# significand from the next two with the top and bottom bits set. (double)uint64 and hi * 2^32 + lo round alike (once, to
# nearest even); the power of two is exact.
func _randd(r: RandomNumberGenerator) -> float:
	var p := r.randi()
	if p == 0:
		return 0.0
	var hi := r.randi() | 0x80000000
	var lo := r.randi() | 1
	var clz := 0
	while clz < 32 and (p & (0x80000000 >> clz)) == 0:
		clz += 1
	return (float(hi) * 4294967296.0 + float(lo)) * pow(2.0, -64 - clz)


func _tween(b: Control, key: String) -> Tween:
	if not b.has_meta(key):
		return null
	var t = b.get_meta(key)
	return t if t is Tween and (t as Tween).is_valid() else null


func _sequences() -> bool:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261106
	var out := FileAccess.open("res://juice_seq.tsv", FileAccess.WRITE)
	out.store_line("# seq\tline\top\tdt\tscale_x\tscale_y\trotation\tjitter")
	var leans := FileAccess.open("res://juice_leans.tsv", FileAccess.WRITE)
	leans.store_line("# seq\tindex\tlean_deg")
	var mismatches := 0
	var counts := {}
	for s in SEQS:
		SettingsManager.effects_enabled = true
		var b := Button.new()
		b.size = Vector2(120, 44)
		add_child(b)
		UiJuice.hover(b)
		seed(9000 + s)
		var shadow := RandomNumberGenerator.new()
		shadow.seed = 9000 + s
		var lean_index := 0
		var pending_lean := INF
		var last_rot: Tween = null
		var line := 0
		for e in EVENTS:
			var op := "step"
			var dt := 0.0
			if rng.randf() < 0.3:
				op = OPS[rng.randi_range(0, OPS.size() - 1)]
			else:
				dt = rng.randf_range(0.004, 0.06) if rng.randf() < 0.9 else rng.randf_range(0.2, 0.7)
			counts[op] = counts.get(op, 0) + 1
			match op:
				"enter": b.mouse_entered.emit()
				"exit": b.mouse_exited.emit()
				"down": b.button_down.emit()
				"up": b.button_up.emit()
				"punch": UiJuice.punch(b)
				"pop_in": UiJuice.pop_in(b)
				"jitter_on": UiJuice.jitter(b, true)
				"jitter_off": UiJuice.jitter(b, false)
				"disable": b.disabled = true
				"enable": b.disabled = false
				"hide": b.visible = false
				"show": b.visible = true
				"effects_off": SettingsManager.effects_enabled = false
				"effects_on": SettingsManager.effects_enabled = true
				"step":
					var ts := _tween(b, UiJuice.META_TWEEN_SCALE)
					var tr := _tween(b, UiJuice.META_TWEEN_ROT)
					if ts != null:
						ts.custom_step(dt)
					if tr != null:
						var was_jitter := b.get_meta(UiJuice.META_JITTER, false) as bool
						tr.custom_step(dt)
						# A jitter step that ended landed on its lean (and the next one, if any, started in finished).
						# (Hidden, the chain stops at 0 instead: _jitter_step's sleep.)
						if was_jitter and pending_lean != INF and b.is_visible_in_tree() and _tween(b, UiJuice.META_TWEEN_ROT) != tr:
							if absf(b.rotation - deg_to_rad(pending_lean)) > 1e-6:
								mismatches += 1
			# A new jitter step started (by jitter_on, or by the last one's end): the lean it drew, from the shadow.
			var cur := _tween(b, UiJuice.META_TWEEN_ROT)
			if cur != null and cur != last_rot and b.get_meta(UiJuice.META_JITTER, false):
				pending_lean = _randd(shadow) * (UiJuice.JITTER_DEG - -UiJuice.JITTER_DEG) + -UiJuice.JITTER_DEG
				leans.store_line("%d\t%d\t%s" % [s, lean_index, h(pending_lean)])
				lean_index += 1
			last_rot = cur
			out.store_line("\t".join(PackedStringArray([str(s), str(line), op, h(dt), h(b.scale.x), h(b.scale.y), h(b.rotation),
				"1" if b.get_meta(UiJuice.META_JITTER, false) else "0"])))
			line += 1
		b.queue_free()
		remove_child(b)
	out.close()
	leans.close()
	SettingsManager.effects_enabled = true
	print("JUICE_FIXTURE counts=%s lean_mismatches=%d" % [counts, mismatches])
	if mismatches > 0:
		push_error("JUICE_FIXTURE the shadow leans do not match where the jitter landed")
		get_tree().quit(1)
		return false
	return true
