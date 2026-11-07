extends Node

# The port's biome tour, Godot side (tools/godot_tour.py --set biomes): each of the 12 biomes, a new game travelled there, the
# field after a short settle, the cat parked at the same place as the Unreal tour's (1250, 540: the OS pointer warped there).
# The backdrop (floor, patches, puddles, props) is laid out from fixed seeds, so it matches shot for shot; the mice do not.

const OUT := "user://port_tour"
const CAT := Vector2(1250, 540)


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	DirAccess.make_dir_recursive_absolute(OUT)
	await _run()


func _run() -> void:
	print("tour -> %s" % ProjectSettings.globalize_path(OUT))
	if GameState.has_method("debug_reset_progress"):
		GameState.debug_reset_progress()
	var tier: Node = null
	for n in range(1, 13):
		if tier != null:
			tier.queue_free()
			await _settle(0.2)
		GameState.current_biome_id = "tier_%d" % n
		tier = (load("res://scenes/environments/tier_%d.tscn" % n) as PackedScene).instantiate()
		add_child(tier)
		get_viewport().warp_mouse(CAT)
		await _settle(1.5)
		get_viewport().warp_mouse(CAT)
		await _settle(0.2)
		await _capture("biome_%d" % n)
	# The title and its garden (the menu biome, dancing).
	if tier != null:
		tier.queue_free()
	var title: Node = (load("res://scenes/ui/main_menu.tscn") as PackedScene).instantiate()
	add_child(title)
	await _settle(1.5)
	await _capture("title")
	print("TOUR_PROBE done")
	get_tree().quit(0)


func _settle(seconds: float) -> void:
	await get_tree().create_timer(seconds, true, false, true).timeout


func _capture(shot_name: String) -> void:
	await RenderingServer.frame_post_draw
	var img := get_viewport().get_texture().get_image()
	if img == null or img.is_empty():
		print("  !! %s captured nothing" % shot_name)
		return
	img.save_png("%s/%s.png" % [OUT, shot_name])
	print("  %s  %dx%d" % [shot_name, img.get_width(), img.get_height()])
