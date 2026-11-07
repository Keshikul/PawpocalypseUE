extends Node

# Ground truth for PcBackdrop (phase 6 slice 2): what the REAL BiomeBackdrop (scripts/systems/biome_backdrop.gd) draws for
# each biome, viewport size and effects budget. A probe subclass records, in the order Godot's own _draw() makes them, every
# prop it draws (through _draw_swaying: the scatter, the landmarks, the border rows, the corner pieces) and every patch or
# puddle outline (through _blob); nothing of the layout is computed here. Also Godot's hash() of the three seeds, so the
# port's hashes are checked on their own.
#
# Row: id, w, h, budget, floor (the floor texture's stem), seed_config, seed_variation, seed_border, blobs, props.
#   blobs: "at_x,at_y,radius,x0:y0:x1:y1..." joined by ";"   (patches first, then puddles: _draw_variation's order)
#   props: "kind,stem,x,y,w,h,sway,phase,hz" joined by ";"  (kind s scatter, l landmark, b border, e edge; w < 0 = mirrored;
#          sway 0 = drawn still; phase, hz the sway's)

const IDS := ["tier_1", "tier_2", "tier_3", "tier_4", "tier_5", "tier_6", "tier_7", "tier_8", "tier_9", "tier_10",
	"tier_11", "tier_12", "menu"]
const SIZES := [Vector2i(1920, 1080), Vector2i(1728, 1080), Vector2i(2520, 1080), Vector2i(3840, 1080), Vector2i(1440, 1080),
	Vector2i(1280, 1080), Vector2i(2400, 1080)]
# effect_budget(): effects off -> 0; low 0.35; medium 0.70; high 1.0.
const BUDGETS := [[true, 2], [true, 1], [true, 0], [false, 2]]


class Probe extends BiomeBackdrop:
	var kind := ""
	var props: PackedStringArray = []
	var blobs: PackedStringArray = []

	func h(x) -> String:
		return var_to_bytes(float(x)).hex_encode()

	func _blob(rng: RandomNumberGenerator, at: Vector2, radius: float) -> PackedVector2Array:
		var pts := super(rng, at, radius)
		var v := PackedStringArray()
		for p in pts:
			v.append(h(p.x))
			v.append(h(p.y))
		blobs.append("%s,%s,%s,%s" % [h(at.x), h(at.y), h(radius), ":".join(v)])
		return pts

	func _draw_swaying(tex: Texture2D, rect: Rect2, tint: Color, shape: Dictionary) -> void:
		var stem := tex.resource_path.get_file().get_basename()
		var sway := not shape.is_empty()
		props.append(",".join(PackedStringArray([kind, stem, h(rect.position.x), h(rect.position.y), h(rect.size.x),
			h(rect.size.y), "1" if sway else "0", h(shape.get("phase", 0.0)), h(shape.get("hz", 0.0))])))

	func _draw_prop(shape: Dictionary) -> void:
		kind = "s"
		super(shape)

	func _draw_landmarks() -> void:
		kind = "l"
		super()

	func _draw_border_props() -> void:
		kind = "b"
		super()

	func _draw_edge_props() -> void:
		kind = "e"
		super()


func _ready() -> void:
	var out := FileAccess.open("res://backdrop_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["id", "w", "h", "budget", "floor", "seed_config", "seed_variation",
		"seed_border", "blobs", "props"])))
	var rows := 0
	for id in IDS:
		for size in SIZES:
			for b in BUDGETS:
				SettingsManager.effects_enabled = b[0]
				SettingsManager.graphics_quality = b[1]
				var vp := SubViewport.new()
				vp.size = size
				vp.disable_3d = true
				add_child(vp)
				var p := Probe.new()
				p.biome_override = id
				vp.add_child(p)
				# _resolve_biome is deferred; the first draw follows it.
				await get_tree().process_frame
				await get_tree().process_frame
				p.props.clear()
				p.blobs.clear()
				p.queue_redraw()
				await get_tree().process_frame
				await get_tree().process_frame
				var floor_stem := ""
				if p._backdrop_texture != null:
					floor_stem = p._backdrop_texture.resource_path.get_file().get_basename()
				out.store_line("\t".join(PackedStringArray([id, str(size.x), str(size.y), p.h(SettingsManager.effect_budget()),
					floor_stem, str(hash(p._config)), str(hash("variation:" + id)), str(hash("border:" + id)),
					";".join(p.blobs), ";".join(p.props)])))
				rows += 1
				vp.queue_free()
				await get_tree().process_frame
	out.close()
	print("BACKDROP_FIXTURE done %d rows" % rows)
	get_tree().quit()
