extends Node

# Where a Reawaken pressed on the REAL HUD lands, and whether the spend step (the skill tree) opened on the way
# (tools/check_godot_reawaken_travel.py runs it in the scratch copy, sandboxed). Designer 2026-11-01: Territory wins.
# One scenario per run (fresh autoloads), as a user arg:
#   terr_deep     Legacy 5 (Territory: biome 4), Reawaken in tier_5     none          no Territory, Reawaken in tier_5
#   terr_shallow  Legacy 5, Reawaken in tier_2                           terr_same     Legacy 5, Reawaken in tier_4
# A card pick that comes up mid-ceremony (Legacy restarts the lines high enough to kill and level) takes its first card;
# the tree is closed 0.5 s after it opens for spending. Last line: "PROBE end spend_step_seen=<bool> scene=... biome=<id>".

var scenario := ""
var t := 0.0
var last := ""
var spend_seen := false
var closed_at := -1.0
var phase := 0
var pick_at := -1.0
var start_scene := ""


func _ready() -> void:
	var args := OS.get_cmdline_user_args()
	scenario = args[0] if args.size() > 0 else "terr_deep"
	process_mode = Node.PROCESS_MODE_ALWAYS
	var watcher := Node.new()
	watcher.set_script(get_script())
	watcher.set_meta("watcher", true)
	if has_meta("watcher"):
		return
	set_process(false)
	# Survive the scene changes: the watcher hangs off the root, not the current scene.
	watcher.process_mode = Node.PROCESS_MODE_ALWAYS
	get_tree().root.add_child.call_deferred(watcher)


func _enter_tree() -> void:
	pass


func setup() -> void:
	# Bosses tier_3 and tier_6 down: stage 3, so biomes up to the one stage 3 opens are on the ladder.
	GameState.bosses_defeated["tier_3"] = true
	GameState.bosses_defeated["tier_6"] = true
	GameState.lifetime_catnip_this_cycle = 1e15
	GameState.lifetime_catnip_ever = 1e12
	GameState.deepest_biome_index = 11
	GameState.reawaken_count = 1
	if scenario != "none":
		SkillTreeManager.grant_level("legacy", 5)  # biome effect 4
	GameState._rebuild_stats()
	start_scene = "res://scenes/environments/tier_2.tscn" if scenario == "terr_shallow" else ("res://scenes/environments/tier_4.tscn" if scenario == "terr_same" else "res://scenes/environments/tier_5.tscn")
	print("PROBE setup scenario=%s legacy=%d starting_biome_index=%d pending_stardust=%f can=%s" % [scenario,
		SkillTreeManager.get_level("legacy"), GameState.get_starting_biome_index(), GameState.get_pending_stardust(),
		str(GameState.can_reawaken())])
	get_tree().change_scene_to_file(start_scene)


func state() -> String:
	var cs := get_tree().current_scene
	var path: String = cs.scene_file_path if cs != null else "<none>"
	var tree_ui := get_tree().get_first_node_in_group(SkillTreeUI.GROUP) as SkillTreeUI
	var tree_s := "none"
	if tree_ui != null:
		tree_s = "spend" if (tree_ui.visible and tree_ui._spend_mode) else ("open" if tree_ui.visible else "closed")
	return "scene=%s tree=%s biome=%s paused=%s" % [path.get_file(), tree_s, GameState.current_biome_id, str(get_tree().paused)]


func _process(delta: float) -> void:
	if not has_meta("watcher"):
		return
	t += delta
	if phase == 0:
		phase = 1
		setup()
		return
	var s := state()
	if s != last:
		print("PROBE t=%.2f %s" % [t, s])
		last = s
		if get_tree().paused:
			for n in get_tree().root.find_children("*", "Control", true, false):
				var c := n as Control
				if c.is_visible_in_tree() and c.process_mode == Node.PROCESS_MODE_ALWAYS:
					print("PROBE   visible-always: %s (%s)" % [str(c.get_path()), c.get_script().resource_path if c.get_script() else c.get_class()])
			for n in get_tree().root.find_children("*", "CanvasLayer", true, false):
				if n.visible and n.process_mode == Node.PROCESS_MODE_ALWAYS:
					print("PROBE   layer-always: %s (%s)" % [str(n.get_path()), n.get_script().resource_path if n.get_script() else ""])
	var tree_ui := get_tree().get_first_node_in_group(SkillTreeUI.GROUP) as SkillTreeUI
	# A card pick mid-ceremony (Legacy restarts the lines high enough to kill and level): take the first card.
	for n in get_tree().root.get_node("CardManager").get_children():
		if n is CanvasLayer and n.visible and n.has_method("first_take"):
			if pick_at < 0.0:
				pick_at = t + 0.6
			elif t >= pick_at and n.first_take() != null:
				print("PROBE t=%.2f card pick: taking the first card" % t)
				n.first_take().pressed.emit()
				pick_at = -1.0
	if phase == 1 and t > 1.0 and get_tree().current_scene != null and get_tree().current_scene.scene_file_path == start_scene:
		var hud := get_tree().get_first_node_in_group("hud")
		if hud == null:
			for n in get_tree().current_scene.find_children("*", "", true, false):
				if n.has_method("_run_reawaken_ceremony"):
					hud = n
					break
		if hud == null:
			print("PROBE no hud")
			quit_now()
			return
		print("PROBE t=%.2f reawaken pressed" % t)
		phase = 2
		hud._run_reawaken_ceremony()
		return
	if phase == 2 and tree_ui != null and tree_ui.visible and tree_ui._spend_mode and not spend_seen:
		spend_seen = true
		closed_at = t + 0.5
	if phase == 2 and spend_seen and closed_at > 0.0 and t >= closed_at:
		closed_at = -1.0
		print("PROBE t=%.2f closing the tree" % t)
		if tree_ui != null and tree_ui.visible:
			tree_ui._on_close_pressed()
		if tree_ui != null and tree_ui.visible:
			tree_ui._on_close_pressed()
	if t > 9.0:
		print("PROBE end spend_step_seen=%s %s" % [str(spend_seen), state()])
		quit_now()


func quit_now() -> void:
	print("REAWAKEN_PROBE_DONE")
	get_tree().quit()
