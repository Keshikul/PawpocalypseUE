extends Node

# Ground truth for phase 6 slice 6a: what the REAL unicorn_ceremony.gd does, frame by frame.
#
# THE PROBE (as slice 5d's `rainbow`): the script's own source is read at run time and compiled again with ONLY its draw calls
# (`_sky.draw_arc`, `_sky.draw_colored_polygon`, `_sky.draw_polyline`), its global random draws (`randf`, `randf_range`) and
# its sounds (`AudioManager.play`) renamed to recorders, and its class name replaced by its base class (the copy is not a
# second global class). Every other line is Godot's. Its tweens are stepped by hand with `custom_step` (slice 5c), after the
# node's `_process`, in the order Godot's SceneTree steps them (nodes, then tweens, each in the order made); presses are fed
# to its `_input` at chosen frames. Refuses to run when a rename finds nothing or the copy does not compile.
#
# Table unicorn.tsv, one line per frame (doubles as Variant hex, turned into C hex floats by make_godot_fixtures.py):
#   run, frame, w, h, press (- none, key, echo, mouse, pad, release), dt, clock, dim_a, x, y, rot, scale, root_a, grow,
#   done (1 after _end), finished (1 once `finished` fired), sounds (this frame, ;), draws (the random draws taken, ;),
#   bits (every SAMPLE-th bit: x,y,vx,vy,rot,spin,sx,sy,r,g,b,star,rain), arcs (cx,cy,r,from,to,n,r,g,b,width,aa),
#   shapes (the sampled bits' polygons and star rims: kind:r:g:b:a:width:x0:y0:x1:y1...)

const SRC := "res://scripts/systems/unicorn_ceremony.gd"
const SIZES := [Vector2i(1920, 1080), Vector2i(1280, 720), Vector2i(3840, 1080), Vector2i(1440, 1080)]
const SAMPLE := 23   # the bits recorded: 0, 23, 46, 69 (the burst), 92-161 (the rain), 184 (a star)

const RECORDERS := """

var _rec: Array = []
var _draws: Array = []
var _sounds: Array = []

func _rec_arc(c: Vector2, r: float, a0: float, a1: float, n: int, col: Color, w: float = -1.0, aa: bool = false) -> void:
	_rec.append(["arc", c, r, a0, a1, n, col, w, aa])

func _rec_colored_polygon(pts: PackedVector2Array, col: Color, uvs: PackedVector2Array = PackedVector2Array(), t: Texture2D = null) -> void:
	_rec.append(["polygon", pts, col])

func _rec_polyline(pts: PackedVector2Array, col: Color, w: float = -1.0, aa: bool = false) -> void:
	_rec.append(["polyline", pts, col, w, aa])

func _rec_randf_range(a: float, b: float) -> float:
	var v := randf_range(a, b)
	_draws.append(v)
	return v

func _rec_randf() -> float:
	var v := randf()
	_draws.append(v)
	return v

func _rec_sound(id: String) -> void:
	_sounds.append(id)
"""

var _rng := RandomNumberGenerator.new()
var _failed := false


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func _fail(why: String) -> void:
	push_error("UNICORN_FIXTURE " + why)
	_failed = true


func _probe() -> GDScript:
	var src := FileAccess.get_file_as_string(SRC)
	if src.is_empty():
		_fail("cannot read " + SRC)
		return null
	var kept := PackedStringArray()
	for line in src.split("\n"):
		if not line.begins_with("class_name "):
			kept.append(line)
	src = "\n".join(kept)
	for pair in [["UnicornCeremony", "CanvasLayer"], ["_sky.draw_arc(", "_rec_arc("],
			["_sky.draw_colored_polygon(", "_rec_colored_polygon("], ["_sky.draw_polyline(", "_rec_polyline("],
			["randf_range(", "_rec_randf_range("], ["randf()", "_rec_randf()"], ["AudioManager.play(", "_rec_sound("]]:
		if src.find(pair[0]) < 0:
			_fail("nothing to rename for %s (the script changed shape)" % pair[0])
			return null
		src = src.replace(pair[0], pair[1])
	var script := GDScript.new()
	script.source_code = src + RECORDERS
	if script.reload() != OK:
		_fail("the probe does not compile")
		return null
	return script


func _event(kind: String) -> InputEvent:
	match kind:
		"key", "echo":
			var k := InputEventKey.new()
			k.keycode = KEY_SPACE
			k.pressed = true
			k.echo = kind == "echo"
			return k
		"mouse":
			var m := InputEventMouseButton.new()
			m.button_index = MOUSE_BUTTON_LEFT
			m.pressed = true
			return m
		"pad":
			var j := InputEventJoypadButton.new()
			j.button_index = JOY_BUTTON_A
			j.pressed = true
			return j
		"release":
			var r := InputEventKey.new()
			r.keycode = KEY_SPACE
			r.pressed = false
			return r
	return null


func _ready() -> void:
	_rng.seed = 6006
	seed(6006) # the global RNG the fiesta draws from: a re-run reproduces the file byte for byte
	var script := _probe()
	if script == null:
		get_tree().quit(1)
		return
	var out := FileAccess.open("res://unicorn.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["run", "frame", "w", "h", "press", "dt", "clock", "dim_a", "x", "y",
		"rot", "scale", "root_a", "grow", "done", "finished", "sounds", "draws", "bits", "arcs", "shapes"])))
	for run in range(16):
		var size: Vector2i = SIZES[run % SIZES.size()]
		var vp := SubViewport.new()
		vp.size = size
		vp.disable_3d = true
		add_child(vp)
		var before := get_tree().get_processed_tweens()
		var c := CanvasLayer.new()
		c.set_script(script)
		var state := {"finished": false}
		c.connect("finished", func() -> void: state["finished"] = true)
		vp.add_child(c)     # _ready builds it and starts it (the first whinny, the main tween)
		c.set_process(false) # stepped by hand from here
		# Presses: none in a quarter of the runs; otherwise one or two at random moments, some before the 0.6 s skip gate.
		var presses := {}
		if run % 4 != 0:
			for k in range(_rng.randi_range(1, 2)):
				presses[_rng.randi_range(1, 40)] = ["key", "echo", "mouse", "pad", "release"][_rng.randi_range(0, 4)]
		var frame := 0
		var last_finished := -1
		while frame < 400:
			var dt: float = [1.0 / 60.0, 1.0 / 30.0, _rng.randf_range(0.0, 0.1), 0.25][_rng.randi_range(0, 3)]
			var press: String = presses.get(frame, "-")
			if press != "-":
				c._input(_event(press))
			c._process(dt)
			for t in get_tree().get_processed_tweens():
				if not before.has(t) and t.is_valid():
					t.custom_step(dt)
			c._rec.clear()
			c._draw_sky()
			_row(out, run, frame, size, press, dt, c, state["finished"])
			# Cleared AFTER the row: the first frame's row carries what _ready did (the first whinny).
			c._draws.clear()
			c._sounds.clear()
			if state["finished"] and last_finished < 0:
				last_finished = frame
			if last_finished >= 0 and frame >= last_finished + 2:
				break
			frame += 1
		if not state["finished"]:
			_fail("run %d never finished" % run)
		vp.queue_free()
	out.close()
	if _failed:
		get_tree().quit(1)
		return
	print("UNICORN_FIXTURE done")
	get_tree().quit()


func _row(out: FileAccess, run: int, frame: int, size: Vector2i, press: String, dt: float, c: Node, finished: bool) -> void:
	var sprite: TextureRect = c._sprite
	var dim: ColorRect = c._dim
	var root: Control = c._root
	var bits := PackedStringArray()
	for i in range(0, c._bits.size(), SAMPLE):
		var b: Dictionary = c._bits[i]
		var p: Vector2 = b["p"]
		var v: Vector2 = b["v"]
		var s: Vector2 = b["size"]
		var col: Color = b["colour"]
		bits.append(",".join(PackedStringArray([h(p.x), h(p.y), h(v.x), h(v.y), h(b["rot"]), h(b["spin"]), h(s.x), h(s.y),
			h(col.r), h(col.g), h(col.b), "1" if b["star"] else "0", "1" if b.get("rain", false) else "0"])))
	# The draw list: the arcs first (when the rainbow grows), then per bit a polygon (a star: and its rim).
	var arcs := PackedStringArray()
	var shapes := PackedStringArray()
	var k := 0
	while k < c._rec.size() and c._rec[k][0] == "arc":
		var a: Array = c._rec[k]
		var col: Color = a[6]
		arcs.append(",".join(PackedStringArray([h(a[1].x), h(a[1].y), h(a[2]), h(a[3]), h(a[4]), str(a[5]), h(col.r),
			h(col.g), h(col.b), h(a[7]), "1" if a[8] else "0"])))
		k += 1
	for i in range(c._bits.size()):
		var star: bool = c._bits[i]["star"]
		var n := 2 if star else 1
		if i % SAMPLE == 0:
			for j in range(n):
				if k + j >= c._rec.size():
					_fail("the draw list is shorter than the bits")
					return
				var e: Array = c._rec[k + j]
				var col: Color = e[2]
				var v := PackedStringArray([e[0], h(col.r), h(col.g), h(col.b), h(col.a), h(e[3]) if e[0] == "polyline" else "-"])
				for p in e[1]:
					v.append(h(p.x))
					v.append(h(p.y))
				shapes.append(":".join(v))
		k += n
	if k != c._rec.size():
		_fail("the draw list has %d calls, the bits account for %d" % [c._rec.size(), k])
	var draws := PackedStringArray()
	for d in c._draws:
		draws.append(h(d))
	out.store_line("\t".join(PackedStringArray([str(run), str(frame), str(size.x), str(size.y), press, h(dt), h(c._clock),
		h(dim.color.a), h(sprite.position.x), h(sprite.position.y), h(sprite.rotation), h(sprite.scale.x),
		h(root.modulate.a), h(c._rainbow_grow), "1" if c._done else "0", "1" if finished else "0",
		";".join(PackedStringArray(c._sounds)), ";".join(draws), ";".join(bits), ";".join(arcs), ";".join(shapes)])))
