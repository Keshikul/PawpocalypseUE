extends Node

# Ground truth for PawCore's PcSettings and UPawSettingsDialog: the REAL SettingsManager (settings_manager.gd) reading a
# settings.cfg that Godot's own ConfigFile wrote, and the REAL settings dialog (settings_dialog.tscn / settings_dialog.gd)
# showing it (scratch copy of the project, sandboxed APPDATA; tools/make_godot_fixtures.py).
#
# Each row resets the manager to its defaults, writes a random settings.cfg (keys left out, values out of range, unknown
# names), loads it, opens the dialog, then makes 6 random changes on the live dialog the way a player does (a slider moved,
# a box ticked, an item picked, the section paged). After the load and after every change it records every setting, the
# effects budget, and what the dialog shows: the three volume lines, the sliders, the boxes, the selected items, the
# section. The load line also carries the file's text, so the port reads the very file Godot wrote.
# Video changes that move the window (fullscreen, V-Sync, resolution) are recorded as loaded but never pressed here: the
# run is headless. Their rules are pure functions, recorded in settings_fit_fixture.tsv (fit_to_screen, centred_position).

const ROWS := 3000
const OPS := 6
const FIT_ROWS := 3000


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func flag(b: bool) -> String:
	return "1" if b else "0"


func esc(s: String) -> String:
	return s.replace("\\", "\\\\").replace("\n", "\\n").replace("\t", "\\t")


const DEFAULTS := {
	"master_volume": 1.0, "music_volume": 0.45, "sfx_volume": 1.0, "muted": false, "output_device": "Default",
	"fullscreen": false, "vsync": true, "graphics_quality": 2, "max_fps": 0, "effects_enabled": true,
	"language": "system", "last_slot": 0, "resolution_index": 2, "confirm_reawaken": true, "tutorial_tips": true,
	"prompt_style": "auto", "dock_side": "right", "show_prompts": true, "swap_confirm_back": false, "swap_sticks": false,
	"invert_x": false, "invert_y": false, "cursor_speed": 1.0, "stick_deadzone": 0.2,
}


func random_volume(rng: RandomNumberGenerator):
	var roll := rng.randf()
	if roll < 0.1:
		return rng.randi_range(0, 1)            # an int where a float is expected
	if roll < 0.25:
		return rng.randf_range(-0.5, 1.6)       # out of range: the load does not clamp volumes
	if roll < 0.4:
		return snappedf(rng.randf(), 0.05)
	return rng.randf()


func write_config(rng: RandomNumberGenerator) -> void:
	var c := ConfigFile.new()
	var p := 0.6
	if rng.randf() < p: c.set_value("audio", "master_volume", random_volume(rng))
	if rng.randf() < p: c.set_value("audio", "music_volume", random_volume(rng))
	if rng.randf() < p: c.set_value("audio", "sfx_volume", random_volume(rng))
	if rng.randf() < p: c.set_value("audio", "muted", rng.randf() < 0.5)
	if rng.randf() < p: c.set_value("audio", "output_device", ["Default", "Speakers (Realtek)", "DualShock \"4\""][rng.randi_range(0, 2)])
	if rng.randf() < p: c.set_value("video", "fullscreen", rng.randf() < 0.5)
	if rng.randf() < p: c.set_value("video", "vsync", rng.randf() < 0.5)
	if rng.randf() < p: c.set_value("video", "graphics_quality", rng.randi_range(-2, 5))
	if rng.randf() < p: c.set_value("video", "max_fps", [0, 30, 60, 120, 144, 240, 75, -5, 1000][rng.randi_range(0, 8)])
	if rng.randf() < p: c.set_value("video", "effects_enabled", rng.randf() < 0.5)
	if rng.randf() < p: c.set_value("game", "language", ["system", "en", "de", "xx"][rng.randi_range(0, 3)])
	if rng.randf() < p: c.set_value("game", "last_slot", rng.randi_range(-3, 8))
	if rng.randf() < p: c.set_value("video", "resolution_index", rng.randi_range(-3, 8))
	if rng.randf() < p: c.set_value("gameplay", "confirm_reawaken", rng.randf() < 0.5)
	if rng.randf() < p: c.set_value("gameplay", "tutorial_tips", rng.randf() < 0.5)
	if rng.randf() < p: c.set_value("controls", "prompt_style", ["auto", "keyboard", "xbox", "playstation", "steam_deck", "nintendo"][rng.randi_range(0, 5)])
	if rng.randf() < p: c.set_value("controls", "dock_side", ["left", "right", "middle"][rng.randi_range(0, 2)])
	if rng.randf() < p: c.set_value("controls", "show_prompts", rng.randf() < 0.5)
	if rng.randf() < p: c.set_value("controls", "swap_confirm_back", rng.randf() < 0.5)
	if rng.randf() < p: c.set_value("controls", "swap_sticks", rng.randf() < 0.5)
	if rng.randf() < p: c.set_value("controls", "invert_x", rng.randf() < 0.5)
	if rng.randf() < p: c.set_value("controls", "invert_y", rng.randf() < 0.5)
	if rng.randf() < p: c.set_value("controls", "cursor_speed", rng.randf_range(0.0, 3.0))
	if rng.randf() < p: c.set_value("controls", "stick_deadzone", rng.randf_range(0.0, 0.8))
	if rng.randf() < 0.1: c.set_value("video", "texture_quality", 2)   # a dead key an older build wrote
	c.save(SettingsManager.SETTINGS_PATH)


func state(d: SettingsDialog) -> PackedStringArray:
	var s := SettingsManager
	var items := PackedStringArray()
	for i in d.output_option.item_count:
		items.append(d.output_option.get_item_text(i))
	return PackedStringArray([
		h(s.master_volume), h(s.music_volume), h(s.sfx_volume), flag(s.muted), esc(s.output_device),
		flag(s.fullscreen), flag(s.vsync), str(s.graphics_quality), str(s.max_fps), flag(s.effects_enabled),
		esc(s.language), str(s.last_slot), str(s.resolution_index), flag(s.confirm_reawaken), flag(s.tutorial_tips),
		esc(s.prompt_style), esc(s.dock_side), flag(s.show_prompts), flag(s.swap_confirm_back), flag(s.swap_sticks),
		flag(s.invert_x), flag(s.invert_y), h(s.cursor_speed), h(s.stick_deadzone), h(s.effect_budget()),
		esc(d.volume_label.text), esc(d.music_label.text), esc(d.sfx_label.text),
		h(d.volume_slider.value), h(d.music_slider.value), h(d.sfx_slider.value),
		flag(d.mute_check.button_pressed), flag(d.fullscreen_check.button_pressed), flag(d.vsync_check.button_pressed),
		str(d.resolution_option.selected), flag(d.resolution_option.disabled), esc(d.resolution_option.get_item_text(d.resolution_option.selected)),
		str(d.quality_option.selected), str(d.max_fps_option.selected), esc(d.max_fps_option.get_item_text(d.max_fps_option.selected)),
		flag(d.effects_check.button_pressed), str(d.prompts_option.selected), str(d.dock_side_option.selected),
		esc("|".join(items)), str(d.output_option.selected), d._current_section])


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261104
	# DEFAULTS is a copy of settings_manager.gd's: it must be the script's own, or every load row starts from a stale value
	# (the dock side's default moved to right, 2026-11-09, and this copy still said left).
	var fresh: Node = load("res://scripts/autoload/settings_manager.gd").new()
	for key in DEFAULTS:
		if fresh.get(key) != DEFAULTS[key]:
			push_error("settings_fixture: DEFAULTS[%s] = %s, settings_manager.gd has %s" % [key, DEFAULTS[key], fresh.get(key)])
			get_tree().quit(1)
			return
	fresh.free()
	var dialog := load("res://scenes/ui/settings_dialog.tscn").instantiate() as SettingsDialog
	add_child(dialog)
	await get_tree().process_frame
	dialog.set_process(false)  # no input polling: the presses are the fixture's
	var header := ["row", "op", "arg", "cfg"]
	for c in ["master", "music", "sfx", "muted", "output", "fullscreen", "vsync", "quality", "max_fps", "effects",
			"language", "last_slot", "res_index", "confirm", "tips", "prompt", "dock", "show_prompts", "swap_cb",
			"swap_sticks", "invert_x", "invert_y", "cursor_speed", "deadzone", "budget",
			"vol_label", "music_label", "sfx_label", "vol_slider", "music_slider", "sfx_slider",
			"mute_check", "full_check", "vsync_check", "res_sel", "res_disabled", "res_text", "quality_sel", "fps_sel",
			"fps_text", "effects_check", "prompts_sel", "dock_sel", "output_items", "output_sel", "section"]:
		header.append(c)
	var out := FileAccess.open("res://settings_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(header)))
	var counts := {}
	for r in range(ROWS):
		for key in DEFAULTS:
			SettingsManager.set(key, DEFAULTS[key])
		write_config(rng)
		var text := FileAccess.get_file_as_string(SettingsManager.SETTINGS_PATH)
		SettingsManager.load_settings()
		dialog.open()
		var row := PackedStringArray([str(r), "load", "-", esc(text)])
		row.append_array(state(dialog))
		out.store_line("\t".join(row))
		for k in range(OPS):
			var roll := rng.randi_range(0, 10)
			var op := ""
			var arg := "-"
			match roll:
				0, 1, 2:
					op = ["vol", "music", "sfx"][roll]
					var v := rng.randf_range(-0.2, 1.2) if rng.randf() < 0.8 else snappedf(rng.randf(), 0.05)
					arg = h(v)
					var slider: HSlider = [dialog.volume_slider, dialog.music_slider, dialog.sfx_slider][roll]
					slider.value = v
				3:
					op = "mute"
					dialog.mute_check.button_pressed = not dialog.mute_check.button_pressed
				4:
					op = "effects"
					dialog.effects_check.button_pressed = not dialog.effects_check.button_pressed
				5:
					op = "quality"
					var i := rng.randi_range(0, 2)
					arg = str(i)
					dialog.quality_option.select(i)
					dialog.quality_option.item_selected.emit(i)
				6:
					op = "fps"
					var i := rng.randi_range(0, SettingsManager.FPS_OPTIONS.size() - 1)
					arg = str(i)
					dialog.max_fps_option.select(i)
					dialog.max_fps_option.item_selected.emit(i)
				7:
					op = "prompts"
					var i := rng.randi_range(0, SettingsManager.PROMPT_STYLES.size() - 1)
					arg = str(i)
					dialog.prompts_option.select(i)
					dialog.prompts_option.item_selected.emit(i)
				8:
					op = "dock"
					var i := rng.randi_range(0, 1)
					arg = str(i)
					dialog.dock_side_option.select(i)
					dialog.dock_side_option.item_selected.emit(i)
				9:
					op = "section"
					var dir := -1 if rng.randf() < 0.5 else 1
					arg = str(dir)
					dialog._step_section(dir)
				10:
					op = "reopen"
					dialog.open()
			counts[op] = counts.get(op, 0) + 1
			row = PackedStringArray([str(r), op, arg, "-"])
			row.append_array(state(dialog))
			out.store_line("\t".join(row))
	out.close()
	dialog.close()

	# The window's pure rules: the largest client area that fits, and a centred position never off the top or left.
	var fit := FileAccess.open("res://settings_fit_fixture.tsv", FileAccess.WRITE)
	fit.store_line("# " + "\t".join(PackedStringArray(["tx", "ty", "ux", "uy", "dx", "dy", "fx", "fy", "px", "py", "sx", "sy", "cx", "cy"])))
	for r in range(FIT_ROWS):
		var target: Vector2i = SettingsManager.RESOLUTIONS[rng.randi_range(0, SettingsManager.RESOLUTIONS.size() - 1)]
		if rng.randf() < 0.2:
			target = Vector2i(rng.randi_range(200, 5000), rng.randi_range(200, 3000))
		var usable := Vector2i(rng.randi_range(800, 4000), rng.randi_range(500, 2400))
		if rng.randf() < 0.05:
			usable = Vector2i(0, rng.randi_range(0, 1000))
		var deco := Vector2i(rng.randi_range(-4, 40), rng.randi_range(-4, 60))
		var fitted := SettingsManager.fit_to_screen(target, usable, deco)
		var pos := Vector2i(rng.randi_range(-2000, 4000), rng.randi_range(-500, 1500))
		var centred := SettingsManager.centred_position(pos, usable, fitted)
		fit.store_line("\t".join(PackedStringArray([str(target.x), str(target.y), str(usable.x), str(usable.y), str(deco.x),
			str(deco.y), str(fitted.x), str(fitted.y), str(pos.x), str(pos.y), str(usable.x), str(usable.y), str(centred.x),
			str(centred.y)])))
	fit.close()
	print("SETTINGS_FIXTURE rows=%d ops=%s" % [ROWS, counts])
	print("SETTINGS_FIXTURE done")
	get_tree().quit(0)
