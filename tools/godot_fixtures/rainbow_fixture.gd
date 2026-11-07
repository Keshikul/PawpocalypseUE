extends Node

# Ground truth for phase 6 slice 5d: what the REAL rainbow_bar.gd, rainbow_frame.gd and skill_tree_sky.gd draw.
#
# THE PROBE. Their drawing is _draw() code calling CanvasItem's draw_* directly, which a subclass cannot intercept (the
# script's calls bind to the native methods). So each script's own source is read from the project at run time and compiled
# again with ONLY its draw calls renamed to recorders (draw_rect -> _rec_rect, ...; the bar's global randf_range / randf too,
# so the port can replay the draws its sparks took): every other line is Godot's, whatever it says the day this runs. The
# fixture refuses to run when a rename finds nothing (the script changed shape) or the copy fails to compile.
#
# Tables (one line per frame; doubles as Variant hex, turned into C hex floats by make_godot_fixtures.py):
#   rainbow_bar.tsv     random bar sizes, ranges and values, random frames: the value the Range kept, the clock, the spark
#                       accumulator, the draws taken, every spark, every band (x,w,r,g,b) and spark circle (x,y,radius,r,g,b,a)
#   rainbow_frame.tsv   random sizes, corners, widths, speeds, frames: the phase and the polyline (x:y:r:g:b per point)
#   rainbow_follow.tsv  a frame following a plate (position, size, lean, shown or not): where the frame put itself
#   sky_layout.tsv      the sky at 4 viewport sizes x 4 effects budgets: every cloud and star _layout() rolled
#   sky_frames.tsv      then 10 frames of each: the clock, every cloud's centre, every star circle and cloud polygon drawn

const BAR_SRC := "res://scripts/systems/rainbow_bar.gd"
const FRAME_SRC := "res://scripts/systems/rainbow_frame.gd"
const SKY_SRC := "res://scripts/systems/skill_tree_sky.gd"

const RECORDERS := """

var _rec: Array = []
var _draws: Array = []

func _rec_rect(r: Rect2, c: Color, filled: bool = true, w: float = -1.0, aa: bool = false) -> void:
	_rec.append(["rect", r, c, filled])

func _rec_circle(p: Vector2, radius: float, c: Color, filled: bool = true, w: float = -1.0, aa: bool = false) -> void:
	_rec.append(["circle", p, radius, c, filled])

func _rec_polyline_colors(pts: PackedVector2Array, cols: PackedColorArray, w: float = -1.0, aa: bool = false) -> void:
	_rec.append(["polyline", pts, cols, w, aa])

func _rec_colored_polygon(pts: PackedVector2Array, c: Color, uvs: PackedVector2Array = PackedVector2Array(), t: Texture2D = null) -> void:
	_rec.append(["polygon", pts, c])

func _rec_randf_range(a: float, b: float) -> float:
	var v := randf_range(a, b)
	_draws.append(v)
	return v

func _rec_randf() -> float:
	var v := randf()
	_draws.append(v)
	return v
"""

const SKY_SIZES := [Vector2i(1920, 1080), Vector2i(1280, 720), Vector2i(3840, 1080), Vector2i(1440, 1080)]
# effect_budget(): effects off -> 0; low 0.35; medium 0.70; high 1.0.
const BUDGETS := [[true, 2], [true, 1], [true, 0], [false, 2]]

var _rng := RandomNumberGenerator.new()
var _failed := false


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func _fail(why: String) -> void:
	push_error("RAINBOW_FIXTURE " + why)
	_failed = true


## The real script's source, its draw calls (and `swaps`) renamed, the recorders appended, compiled.
func _probe(path: String, swaps: Array) -> GDScript:
	var src := FileAccess.get_file_as_string(path)
	if src.is_empty():
		_fail("cannot read " + path)
		return null
	# A second global class of the same name would not compile; the copy needs none.
	var kept := PackedStringArray()
	for line in src.split("\n"):
		if not line.begins_with("class_name "):
			kept.append(line)
	src = "\n".join(kept)
	for pair in swaps:
		if src.find(pair[0]) < 0:
			_fail("%s: nothing to rename for %s (the script changed shape)" % [path, pair[0]])
			return null
		src = src.replace(pair[0], pair[1])
	var script := GDScript.new()
	script.source_code = src + RECORDERS
	if script.reload() != OK:
		_fail("the probe of %s does not compile" % path)
		return null
	return script


func _ready() -> void:
	_rng.seed = 5150
	seed(5150) # the global RNG the bar's sparks draw from: a re-run reproduces every file byte for byte
	var bar := _probe(BAR_SRC, [["draw_rect(", "_rec_rect("], ["draw_circle(", "_rec_circle("],
		["randf_range(", "_rec_randf_range("], ["randf()", "_rec_randf()"]])
	var frame := _probe(FRAME_SRC, [["draw_polyline_colors(", "_rec_polyline_colors("]])
	var sky := _probe(SKY_SRC, [["draw_rect(", "_rec_rect("], ["draw_circle(", "_rec_circle("],
		["draw_colored_polygon(", "_rec_colored_polygon("]])
	if bar == null or frame == null or sky == null:
		get_tree().quit(1)
		return
	_bars(bar)
	_frames(frame)
	_follow(frame)
	await _skies(sky)
	if _failed:
		get_tree().quit(1)
		return
	print("RAINBOW_FIXTURE done")
	get_tree().quit()


# --- the bar ---------------------------------------------------------------------------------------------------------

func _bar_value() -> float:
	match _rng.randi_range(0, 5):
		0: return 0.0
		1: return 1.0
		2: return _rng.randf_range(-0.2, 0.0)
		3: return _rng.randf_range(1.0, 1.3)
		4: return _rng.randf_range(0.0, 0.02)
	return _rng.randf()


func _bars(script: GDScript) -> void:
	var out := FileAccess.open("res://rainbow_bar.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["seq", "frame", "w", "h", "min", "max", "asked", "value", "dt", "time",
		"acc", "draws", "sparks", "bands", "circles"])))
	for seq in range(30):
		var holder := Control.new()
		add_child(holder)
		var b := ProgressBar.new()
		b.set_script(script)
		# hud.tscn's ReawakenBar: max 1, no percentage; the Range's own step (0.01) left as the scene leaves it.
		b.show_percentage = false
		b.max_value = 1.0
		if seq % 5 == 4:
			# A range other than 0..1 (min > 0, a coarse step), so the fraction and the step's rounding are both seen.
			b.min_value = _rng.randf_range(0.0, 5.0)
			b.max_value = b.min_value + _rng.randf_range(0.5, 20.0)
			b.step = 0.25
		holder.add_child(b)
		b.set_process(false) # stepped by hand, frame by frame
		b.size = Vector2(_rng.randf_range(60.0, 640.0), _rng.randf_range(8.0, 40.0))
		var asked := 0.0
		for f in range(20):
			if f == 0 or _rng.randf() < 0.3:
				asked = _bar_value() * (b.max_value - b.min_value) + b.min_value
				b.value = asked
			if _rng.randf() < 0.08:
				b.size = Vector2(_rng.randf_range(60.0, 640.0), _rng.randf_range(8.0, 40.0))
			# Now and then a long frame (a hitch): 26 sparks at once, so a full bar crosses the 24 it stops at.
			var dt: float = [1.0 / 60.0, 1.0 / 30.0, _rng.randf_range(0.0, 0.12), 0.25, 0.0, 1.9][_rng.randi_range(0, 5)]
			b._draws.clear()
			b._process(dt)
			b._rec.clear()
			b._draw()
			var sparks := PackedStringArray()
			for s in b._sparks:
				var p: Vector2 = s["pos"]
				var v: Vector2 = s["vel"]
				sparks.append(",".join(PackedStringArray([h(p.x), h(p.y), h(v.x), h(v.y), h(s["age"]), h(s["hue"])])))
			var bands := PackedStringArray()
			var circles := PackedStringArray()
			for r in b._rec:
				match r[0]:
					"rect":
						var rc: Rect2 = r[1]
						var c: Color = r[2]
						if rc.position.y != 0.0 or rc.size.y != b.size.y or c.a != 1.0 or not r[3]:
							_fail("a band not full height, opaque and filled")
						bands.append(",".join(PackedStringArray([h(rc.position.x), h(rc.size.x), h(c.r), h(c.g), h(c.b)])))
					"circle":
						var p: Vector2 = r[1]
						var c: Color = r[3]
						circles.append(",".join(PackedStringArray([h(p.x), h(p.y), h(r[2]), h(c.r), h(c.g), h(c.b), h(c.a)])))
					_:
						_fail("the bar drew a " + r[0])
			var draws := PackedStringArray()
			for d in b._draws:
				draws.append(h(d))
			out.store_line("\t".join(PackedStringArray([str(seq), str(f), h(b.size.x), h(b.size.y), h(b.min_value),
				h(b.max_value), h(asked), h(b.value), h(dt), h(b._time), h(b._spark_accumulator), ";".join(draws),
				";".join(sparks), ";".join(bands), ";".join(circles)])))
		holder.queue_free()
	out.close()


# --- the frame -------------------------------------------------------------------------------------------------------

func _polyline_text(r: Array) -> String:
	var pts: PackedVector2Array = r[1]
	var cols: PackedColorArray = r[2]
	if pts.size() != cols.size():
		_fail("a polyline with %d points and %d colours" % [pts.size(), cols.size()])
	var v := PackedStringArray()
	for i in range(pts.size()):
		if cols[i].a != 1.0:
			_fail("a frame colour not opaque")
		v.append(":".join(PackedStringArray([h(pts[i].x), h(pts[i].y), h(cols[i].r), h(cols[i].g), h(cols[i].b)])))
	return ";".join(v)


func _frames(script: GDScript) -> void:
	var out := FileAccess.open("res://rainbow_frame.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["seq", "frame", "w", "h", "corner", "width", "turns", "active", "dt",
		"phase", "drawn", "line_width", "antialiased", "points"])))
	for seq in range(40):
		var holder := Control.new()
		add_child(holder)
		var fr := Control.new()
		fr.set_script(script)
		holder.add_child(fr)
		fr.set_process(false)
		# hud.gd's two: the zone plate (corner 8, width 4) and the Reawaken banner (corner 10, width 5); then anything.
		match seq % 4:
			0:
				fr.corner = 8.0
				fr.width = 4.0
			1:
				fr.corner = 10.0
				fr.width = 5.0
			_:
				fr.corner = _rng.randf_range(0.0, 40.0)
				fr.width = _rng.randf_range(1.0, 8.0)
				fr.turns_per_sec = _rng.randf_range(0.0, 1.5)
		fr.size = Vector2(_rng.randf_range(4.0, 420.0), _rng.randf_range(4.0, 160.0))
		fr.active = true
		for f in range(10):
			if _rng.randf() < 0.15:
				fr.active = not fr.active
			if _rng.randf() < 0.1:
				fr.size = Vector2(_rng.randf_range(4.0, 420.0), _rng.randf_range(4.0, 160.0))
			var dt: float = [1.0 / 60.0, _rng.randf_range(0.0, 0.5), 3.0][_rng.randi_range(0, 2)]
			fr._process(dt)
			fr._rec.clear()
			fr._draw()
			var drawn: int = fr._rec.size()
			var pts := ""
			var lw := 0.0
			var aa := false
			if drawn > 0:
				if drawn != 1 or fr._rec[0][0] != "polyline":
					_fail("the frame drew something other than one polyline")
				pts = _polyline_text(fr._rec[0])
				lw = fr._rec[0][3]
				aa = fr._rec[0][4]
			out.store_line("\t".join(PackedStringArray([str(seq), str(f), h(fr.size.x), h(fr.size.y), h(fr.corner),
				h(fr.width), h(fr.turns_per_sec), "1" if fr.active else "0", h(dt), h(fr._phase), str(drawn), h(lw),
				"1" if aa else "0", pts])))
		holder.queue_free()
	out.close()


func _follow(script: GDScript) -> void:
	var out := FileAccess.open("res://rainbow_follow.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["seq", "plate_ox", "plate_oy", "plate_w", "plate_h", "plate_rot",
		"plate_shown", "margin", "active", "dt", "x", "y", "w", "h", "rot", "visible", "phase"])))
	for seq in range(200):
		var holder := Control.new()
		add_child(holder)
		var plate := PanelContainer.new()
		holder.add_child(plate)
		plate.position = Vector2(_rng.randf_range(0.0, 1500.0), _rng.randf_range(0.0, 900.0))
		plate.size = Vector2(_rng.randf_range(10.0, 400.0), _rng.randf_range(10.0, 120.0))
		plate.pivot_offset = plate.size / 2.0
		plate.rotation = [0.0, _rng.randf_range(-0.1, 0.1)][_rng.randi_range(0, 1)]
		plate.visible = _rng.randf() < 0.8
		var fr := Control.new()
		fr.set_script(script)
		holder.add_child(fr)
		fr.set_process(false)
		fr.follow = plate
		fr.margin = [2.0, 3.0, _rng.randf_range(0.0, 8.0)][_rng.randi_range(0, 2)]
		fr.active = _rng.randf() < 0.8
		var dt := _rng.randf_range(0.0, 0.4)
		fr._process(dt)
		# The plate's global origin (its top-left corner, turned about its pivot): what a follower sees.
		var o := plate.get_global_transform().origin
		out.store_line("\t".join(PackedStringArray([str(seq), h(o.x), h(o.y), h(plate.size.x),
			h(plate.size.y), h(plate.rotation), "1" if plate.visible else "0", h(fr.margin), "1" if fr.active else "0", h(dt),
			h(fr.global_position.x), h(fr.global_position.y), h(fr.size.x), h(fr.size.y), h(fr.rotation),
			"1" if fr.visible else "0", h(fr._phase)])))
		holder.queue_free()
	out.close()


# --- the sky ---------------------------------------------------------------------------------------------------------

func _skies(script: GDScript) -> void:
	var lay := FileAccess.open("res://sky_layout.tsv", FileAccess.WRITE)
	lay.store_line("# " + "\t".join(PackedStringArray(["w", "h", "budget", "seed", "clouds", "stars"])))
	var frames := FileAccess.open("res://sky_frames.tsv", FileAccess.WRITE)
	frames.store_line("# " + "\t".join(PackedStringArray(["w", "h", "budget", "frame", "dt", "time", "at", "base", "stars",
		"clouds"])))
	for size in SKY_SIZES:
		for b in BUDGETS:
			SettingsManager.effects_enabled = b[0]
			SettingsManager.graphics_quality = b[1]
			var vp := SubViewport.new()
			vp.size = size
			vp.disable_3d = true
			add_child(vp)
			var sky := Control.new()
			sky.set_script(script)
			sky.set_anchors_preset(Control.PRESET_FULL_RECT)
			# _ready lays the sky out at once (the SubViewport has its size) and turns processing on: off again at once,
			# before a real frame can move a cloud by the wall clock's delta (a re-run would not reproduce).
			vp.add_child(sky)
			sky.set_process(false)
			var budget := SettingsManager.effect_budget()
			var clouds := PackedStringArray()
			for c in sky._clouds:
				var at: Vector2 = c["at"]
				var drift: Vector2 = c["drift"]
				clouds.append(",".join(PackedStringArray([h(at.x), h(at.y), h(c["radius"]), h(drift.x), h(drift.y),
					h(c["hue_period"]), h(c["phase"]), h(c["slow_amp"]), h(c["fast_amp"])])))
			var stars := PackedStringArray()
			for s in sky._stars:
				var at: Vector2 = s["at"]
				stars.append(",".join(PackedStringArray([h(at.x), h(at.y), h(s["radius"]), h(s["alpha"]), h(s["period"]),
					h(s["phase"])])))
			lay.store_line("\t".join(PackedStringArray([str(size.x), str(size.y), h(budget), str(hash("skill_tree_sky")),
				";".join(clouds), ";".join(stars)])))
			for f in range(10):
				# Small steps, then long ones: a cloud must leave and come back round the other side (the wrap).
				var dt: float = [1.0 / 60.0, _rng.randf_range(0.0, 0.5), _rng.randf_range(20.0, 160.0)][f % 3]
				sky._process(dt)
				sky._rec.clear()
				sky._draw()
				var ats := PackedStringArray()
				for c in sky._clouds:
					ats.append("%s:%s" % [h(c["at"].x), h(c["at"].y)])
				var base := ""
				var star_v := PackedStringArray()
				var cloud_v := PackedStringArray()
				for r in sky._rec:
					match r[0]:
						"rect":
							var rc: Rect2 = r[1]
							var c: Color = r[2]
							base = ",".join(PackedStringArray([h(rc.position.x), h(rc.position.y), h(rc.size.x), h(rc.size.y),
								h(c.r), h(c.g), h(c.b), h(c.a)]))
						"circle":
							var p: Vector2 = r[1]
							var c: Color = r[3]
							star_v.append(",".join(PackedStringArray([h(p.x), h(p.y), h(r[2]), h(c.r), h(c.g), h(c.b), h(c.a)])))
						"polygon":
							var pts: PackedVector2Array = r[1]
							var c: Color = r[2]
							var v := PackedStringArray([h(c.r), h(c.g), h(c.b), h(c.a)])
							for p in pts:
								v.append(h(p.x))
								v.append(h(p.y))
							cloud_v.append(":".join(v))
						_:
							_fail("the sky drew a " + r[0])
				frames.store_line("\t".join(PackedStringArray([str(size.x), str(size.y), h(budget), str(f), h(dt),
					h(sky._time), ";".join(ats), base, ";".join(star_v), ";".join(cloud_v)])))
			vp.queue_free()
			await get_tree().process_frame
	lay.close()
	frames.close()
