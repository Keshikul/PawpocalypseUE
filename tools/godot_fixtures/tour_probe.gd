extends Node

# The port's screenshot tour, Godot side (tools/godot_tour.py runs it in the scratch copy, sandboxed APPDATA, rendered, not
# headless: the dummy rasteriser writes blank PNGs). The same steps and state as the Unreal tour (tools/godot_tour.py --ue),
# so the two sets of shots compare one to one: a new game in tier_1 with 1000 Catnip; the six tabs; a card hand; the tree
# browsed; a Reawaken on a run made worth it (three jobs held, the third current): the ceremony at its hold, the spend step,
# the job picker; the ending at 4200 s.

const OUT := "user://port_tour"


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	DirAccess.make_dir_recursive_absolute(OUT)
	await _run()


func _run() -> void:
	print("tour -> %s" % ProjectSettings.globalize_path(OUT))
	if GameState.has_method("debug_reset_progress"):
		GameState.debug_reset_progress()
	GameState.current_biome_id = "tier_1"
	var tier: Node = (load("res://scenes/environments/tier_1.tscn") as PackedScene).instantiate()
	add_child(tier)
	await _settle(2.0)
	GameState.debug_add_currency(1000.0, 0.0, 0.0)
	var hud := tier.get_node_or_null("HUD") as HUD
	if hud == null:
		print("  !! no HUD")
		get_tree().quit(1)
		return
	await _settle(0.5)

	for tab in ["upgrades", "reawaken", "realms", "catches", "awards", "jobs"]:
		hud.show_tab(tab)
		await _settle(1.0)
		await _capture("tab_%s" % tab)
	hud.show_tab("upgrades")

	var tree := get_tree().get_first_node_in_group(SkillTreeUI.GROUP) as SkillTreeUI
	tree.open()
	await _settle(2.0)
	await _capture("tree_browse")
	tree._on_close_pressed()
	await _settle(0.5)

	CardManager._offer_hand()
	await _settle(1.0)
	await _capture("card_pick")
	var ui := get_tree().get_first_node_in_group(CatchesPickUI.GROUP) as CatchesPickUI
	var guard := 12
	while guard > 0 and (CardManager.has_pending_hand() or (ui != null and ui.visible)):
		guard -= 1
		var hand := CardManager.current_hand()
		if ui != null and not hand.is_empty():
			ui._on_take_pressed(hand[0])
		elif not hand.is_empty():
			CardManager.choose(hand[0])
		else:
			break
		await _settle(0.2)

	# A Reawaken: the run made worth it, three jobs held (the first three by id, the third current), the confirm off.
	GameState.lifetime_catnip_ever = maxf(GameState.lifetime_catnip_ever, 1.0e12)
	GameState.lifetime_catnip_this_cycle = maxf(GameState.lifetime_catnip_this_cycle, 1.0e12)
	var ids: Array = GameState.jobs.keys()
	ids.sort()
	for i in 3:
		GameState.jobs_held[ids[i]] = true
		GameState.current_job = ids[i]
	hud._run_reawaken_ceremony()
	await _settle(1.0)
	await _capture("ceremony")
	await _settle(2.0)
	await _capture("tree_spend")
	tree._on_close_pressed()
	await _settle(0.3)
	tree._on_close_pressed()
	await _settle(1.0)
	await _capture("job_picker")
	if hud._job_picker != null:
		hud._job_picker.visible = false
	get_tree().paused = false
	await _settle(0.5)

	hud.victory_screen.show_victory(4200.0)
	await _settle(1.0)
	await _capture("victory")
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
