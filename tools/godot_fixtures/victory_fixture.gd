extends Node

# Ground truth for PawCore's PcHud victory rules and UPawVictoryScreen: the REAL HUD (hud.tscn, which builds the real
# VictoryScreen and connects it to GameState.boss_defeated) over random states (scratch copy of the project, sandboxed
# APPDATA; tools/make_godot_fixtures.py). Each row sets a boss ledger and the clocks, calls GameState.mark_boss_defeated()
# for one tier, and records whether boss_defeated was emitted, whether the ending came up, the line it shows, and the
# pause flag before, while up, and after OK (dismiss). Every row also records VictoryScreen.format_duration(seconds).
#
# The screen's resolved look (fonts, colours, sizes, the OK button's style) is printed once as "LOOK key=value" lines:
# VictoryScreen is a CanvasLayer of its own, so what theme reaches it is read from Godot, not assumed.

const ROWS := 3000


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func flag(b: bool) -> String:
	return "1" if b else "0"


func seconds(rng: RandomNumberGenerator) -> float:
	var roll := rng.randf()
	if roll < 0.05:
		return 0.0
	if roll < 0.1:
		return -rng.randf_range(0.0, 500.0)
	if roll < 0.2:
		# the edges: a whole minute or hour, a hair either side
		var edge: float = [59.0, 60.0, 3599.0, 3600.0, 3660.0, 35999.0, 36000.0, 86399.0, 86400.0][rng.randi_range(0, 8)]
		var nudge: float = [0.0, 0.999, -0.001, 0.5][rng.randi_range(0, 3)]
		return edge + nudge
	if roll < 0.4:
		return rng.randf_range(0.0, 60.0)
	if roll < 0.7:
		return rng.randf_range(60.0, 3600.0)
	if roll < 0.95:
		return rng.randf_range(3600.0, 400000.0)
	return rng.randf_range(400000.0, 5.0e7)


## A stylebox as one line: a flat one's colours and geometry, a textured one's texture and margins.
func style_desc(sb: StyleBox) -> String:
	var desc := "%s" % sb.get_class()
	if sb is StyleBoxFlat:
		var f := sb as StyleBoxFlat
		desc += " bg=%s border=%s w=%d radius=%d margins=%s,%s,%s,%s" % [f.bg_color, f.border_color, f.border_width_left,
			f.corner_radius_top_left, f.content_margin_left, f.content_margin_top, f.content_margin_right, f.content_margin_bottom]
	elif sb is StyleBoxTexture:
		var t := sb as StyleBoxTexture
		desc += " texture=%s texture_margins=%s,%s,%s,%s margins=%s,%s,%s,%s" % [
			t.texture.resource_path if t.texture else "null", t.texture_margin_left, t.texture_margin_top,
			t.texture_margin_right, t.texture_margin_bottom, t.content_margin_left, t.content_margin_top,
			t.content_margin_right, t.content_margin_bottom]
	return desc


func look(name: String, c: Control, sp: String = "ok") -> void:
	print("LOOK %s.font_size=%d" % [name, c.get_theme_font_size("font_size")])
	print("LOOK %s.font_color=%s" % [name, c.get_theme_color("font_color")])
	var font := c.get_theme_font("font")
	print("LOOK %s.font=%s %s" % [name, font.resource_path if font else "null", font.get_font_name() if font else ""])
	print("LOOK %s.theme_owner_has_theme=%s" % [name, c.has_theme_color("font_color", "Label")])
	print("LOOK %s.size=%s min=%s pos=%s" % [name, c.size, c.get_combined_minimum_size(), c.global_position])
	if c is Label:
		print("LOOK %s.outline_size=%d outline_color=%s" % [name, c.get_theme_constant("outline_size"), c.get_theme_color("font_outline_color")])
		print("LOOK %s.line_spacing=%d" % [name, c.get_theme_constant("line_spacing")])
	if c is Button:
		for state in ["normal", "hover", "pressed", "focus", "disabled"]:
			var sb := c.get_theme_stylebox(state)
			var desc := "%s" % sb.get_class()
			if sb is StyleBoxFlat:
				var f := sb as StyleBoxFlat
				desc += " bg=%s border=%s w=%d,%d,%d,%d radius=%d,%d,%d,%d draw_center=%s margins=%s,%s,%s,%s" % [f.bg_color,
					f.border_color, f.border_width_left, f.border_width_top, f.border_width_right, f.border_width_bottom,
					f.corner_radius_top_left, f.corner_radius_top_right, f.corner_radius_bottom_right,
					f.corner_radius_bottom_left, f.draw_center, f.content_margin_left, f.content_margin_top,
					f.content_margin_right, f.content_margin_bottom]
			else:
				desc = style_desc(sb)
			print("LOOK %s.style.%s=%s" % [sp, state, desc])
		for col in ["font_hover_color", "font_pressed_color", "font_focus_color", "font_disabled_color"]:
			print("LOOK %s.%s=%s" % [sp, col, c.get_theme_color(col)])


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261103
	var hud := load("res://scenes/ui/hud.tscn").instantiate() as HUD
	add_child(hud)
	await get_tree().process_frame
	var vs: VictoryScreen = hud.victory_screen
	if vs == null:
		push_error("VICTORY_FIXTURE: the HUD built no victory screen")
		get_tree().quit(1)
		return
	var realms: Array = GameState.BOSS_REALMS
	var last: String = realms[-1]
	var others: Array = ["tier_1", "tier_2", "tier_4", "tier_5", "tier_7", "tier_9", "tier_10", "not_a_tier"]

	# The look, once, with the screen up at the window's size.
	get_tree().paused = false
	vs.show_victory(4200.0)
	await get_tree().process_frame
	await get_tree().process_frame
	print("LOOK viewport=%s" % get_viewport().get_visible_rect().size)
	print("LOOK layer=%d" % vs.layer)
	var box := vs._ok.get_parent() as VBoxContainer
	print("LOOK box.size=%s pos=%s separation=%d" % [box.size, box.global_position, box.get_theme_constant("separation")])
	for child in box.get_children():
		look("Flavour" if String(child.name).begins_with("@") else String(child.name), child as Control)
	# Since F21 (designer 2026-11-03) the screen hangs off a themed root Control: root > dim, WindowPanel > box.
	var vroot := vs.get_child(0) as Control
	print("LOOK root.has_theme=%s" % (vroot.theme != null))
	var dim := vroot.get_child(0) as ColorRect
	print("LOOK dim.color=%s mouse_filter=%d" % [dim.color, dim.mouse_filter])
	var vpanel := box.get_parent() as PanelContainer
	print("LOOK panel.variation=%s min=%s" % [vpanel.theme_type_variation, vpanel.custom_minimum_size])
	print("LOOK panel.style=%s" % style_desc(vpanel.get_theme_stylebox("panel")))
	print("LOOK panel.size=%s pos=%s" % [vpanel.size, vpanel.global_position])
	print("LOOK headline.text=%s" % (box.get_child(0) as Label).text)
	print("LOOK flavour.text=%s" % (box.get_child(1) as Label).text.c_escape())
	print("LOOK flavour.autowrap=%d" % (box.get_child(1) as Label).autowrap_mode)
	print("LOOK time.text=%s" % vs._time_label.text)
	print("LOOK ok.text=%s ok.min=%s" % [vs._ok.text, vs._ok.custom_minimum_size])
	print("LOOK focus_owner=%s" % get_viewport().gui_get_focus_owner().name)
	vs.dismiss()
	get_tree().paused = false

	# The job picker: the same kind of screen (JobPicker.new() under the HUD, its own CanvasLayer), so the same question.
	var job_ids: Array = GameState.jobs.keys()
	job_ids.sort()
	GameState.jobs_held = {job_ids[0]: true, job_ids[1]: true, job_ids[2]: true}
	GameState.current_job = job_ids[2]
	GameState.run_job_chosen = false
	hud._ask_for_job_then(func() -> void: pass)
	await get_tree().process_frame
	await get_tree().process_frame
	var jp: JobPicker = hud._job_picker
	print("LOOK jp.layer=%d" % jp.layer)
	var panel := jp._root.get_child(1) as PanelContainer
	print("LOOK jp.panel.style=%s" % style_desc(panel.get_theme_stylebox("panel")))
	print("LOOK jp.panel.size=%s pos=%s" % [panel.size, panel.global_position])
	var body := panel.get_child(0) as VBoxContainer
	look("jp.title", body.get_child(0) as Control)
	look("jp.note", body.get_child(1) as Control)
	look("jp.button", jp._rows[0], "jp.button")
	look("jp.blurb", jp._list.get_child(1) as Control)
	print("LOOK jp.focus_owner=%s" % get_viewport().gui_get_focus_owner().text)
	jp.visible = false
	get_tree().paused = false
	GameState.jobs_held = {}
	GameState.current_job = ""

	var out := FileAccess.open("res://victory_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["seconds", "duration", "tier", "defeated_before", "cleared_before",
		"final_before", "play", "paused_before", "signalled", "new", "final_after", "shown", "time_text", "paused_up",
		"paused_after"])))
	var signalled := [false]
	GameState.boss_defeated.connect(func(_t: String, _s: int) -> void: signalled[0] = true)
	for i in range(ROWS):
		var secs := seconds(rng)
		var duration := VictoryScreen.format_duration(secs)
		var roll := rng.randf()
		var tier: String
		if roll < 0.55:
			tier = last
		elif roll < 0.9 or others.is_empty():
			tier = realms[rng.randi_range(0, realms.size() - 1)]
		else:
			tier = others[rng.randi_range(0, others.size() - 1)]
		var defeated := rng.randf() < 0.4
		var cleared := defeated and rng.randf() < 0.5
		GameState.bosses_defeated = {tier: true} if defeated else {}
		GameState.bosses_cleared_this_cycle = {tier: true} if cleared else {}
		var final_before := 0.0
		if rng.randf() < 0.4:
			final_before = seconds(rng)
		GameState.final_boss_seconds = final_before
		GameState.total_play_seconds = secs
		var paused_before := rng.randf() < 0.3
		get_tree().paused = paused_before
		signalled[0] = false
		var is_new := GameState.mark_boss_defeated(tier)
		var shown := vs.visible
		var text := vs._time_label.text if shown else "-"
		var paused_up := get_tree().paused
		vs.dismiss()
		var paused_after := get_tree().paused
		out.store_line("\t".join(PackedStringArray([h(secs), duration, tier, flag(defeated), flag(cleared), h(final_before),
			h(secs), flag(paused_before), flag(signalled[0]), flag(is_new), h(GameState.final_boss_seconds), flag(shown),
			text, flag(paused_up), flag(paused_after)])))
		get_tree().paused = false
	out.close()
	print("VICTORY_FIXTURE done")
	get_tree().quit(0)
