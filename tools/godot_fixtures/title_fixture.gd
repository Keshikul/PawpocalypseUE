extends Node

# Ground truth for PawCore's PcTitle and UPawMainMenu: the REAL title screen (main_menu.tscn and main_menu.gd, with the real
# SaveManager and its slot files; scratch copy of the project, sandboxed APPDATA; tools/make_godot_fixtures.py).
#
# Each row sets a random game in memory and writes a random set of the five slot files (some missing, one now and then
# unreadable), then asks the live menu what it shows: _refresh() (Continue / Load enabled, the status line),
# _refresh_slot_cards() (each card's Delete and text), _refresh_leaderboard() (its text) and where _focus_top() puts the
# selection on the menu and on the slot screen. Every row also records the three formatters on their own random inputs:
# MainMenu.format_when(unix) (against the clock it read: "now"), PauseMenu.format_duration(seconds) and
# PauseMenu._kills_text(kills).
#
# The screen's resolved look (fonts, colours, sizes, styles) is printed once as "LOOK key=value" lines.

const ROWS := 3000
const SLOTS := 5


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func flag(b: bool) -> String:
	return "1" if b else "0"


func esc(s: String) -> String:
	return s.replace("\\", "\\\\").replace("\n", "\\n").replace("\t", "\\t")


## Seconds since a save, away from the edges of format_when's buckets: the formatter reads the clock again, so a value a
## hair from an edge could land either side between the stamp and the call.
func elapsed(rng: RandomNumberGenerator) -> float:
	var roll := rng.randf()
	if roll < 0.15:
		return rng.randf_range(0.0, 55.0)                      # just now
	if roll < 0.35:
		return 60.0 * (rng.randi_range(1, 59) + rng.randf_range(0.1, 0.9))       # minutes
	if roll < 0.55:
		return 3600.0 * (rng.randi_range(1, 23) + rng.randf_range(0.05, 0.95))   # hours
	if roll < 0.75:
		return 86400.0 * (rng.randi_range(1, 6) + rng.randf_range(0.05, 0.95))   # days
	if roll < 0.82:
		return 86400.0 * rng.randf_range(6.05, 9.95) if rng.randf() < 0.5 else 86400.0 * rng.randf_range(6.6, 7.4)  # the week's edge
	return 86400.0 * rng.randf_range(7.5, 4000.0)               # a date


func seconds(rng: RandomNumberGenerator) -> float:
	var roll := rng.randf()
	if roll < 0.05:
		return 0.0
	if roll < 0.1:
		return -rng.randf_range(0.0, 500.0)
	if roll < 0.25:
		var edge: float = [59.0, 60.0, 61.0, 3599.0, 3600.0, 3601.0, 3660.0, 35999.0, 36000.0, 86399.0, 86400.0][rng.randi_range(0, 10)]
		var nudge: float = [0.0, 0.999, -0.001, 0.5][rng.randi_range(0, 3)]
		return edge + nudge
	if roll < 0.45:
		return rng.randf_range(0.0, 60.0)
	if roll < 0.7:
		return rng.randf_range(60.0, 3600.0)
	return rng.randf_range(3600.0, 5.0e6)


func kills(rng: RandomNumberGenerator) -> int:
	var roll := rng.randf()
	if roll < 0.1:
		return 0
	if roll < 0.25:
		var edge: int = [999, 1000, 1001, 1049, 1050, 1051, 9999, 99949, 99950, 999949, 999950, 999999, 1000000, 1049999, 1050000][rng.randi_range(0, 14)]
		return edge
	if roll < 0.45:
		return rng.randi_range(1, 999)
	if roll < 0.75:
		return rng.randi_range(1000, 999999)
	return rng.randi_range(1000000, 2000000000)


func amount(rng: RandomNumberGenerator) -> float:
	var roll := rng.randf()
	if roll < 0.35:
		return 0.0
	return pow(10.0, rng.randf_range(-2.0, 15.0))


func write_slot(slot: int, rng: RandomNumberGenerator, now: float) -> void:
	var d := PawpocalypseSaveData.new()
	d.version = PawpocalypseSaveData.CURRENT_VERSION
	d.unicorn_split = true
	d.reawaken_count = rng.randi_range(0, 40) if rng.randf() < 0.8 else 0
	d.lifetime_kills = kills(rng)
	d.lifetime_catnip_ever = amount(rng)
	d.total_play_seconds = seconds(rng)
	d.final_boss_seconds = seconds(rng) if rng.randf() < 0.2 else 0.0
	var roll := rng.randf()
	if roll < 0.08:
		d.last_played_unix = 0.0                                  # never stamped: "unknown"
	else:
		d.last_played_unix = now - elapsed(rng)
	if ResourceSaver.save(d, SaveManager.slot_path(slot)) != OK:
		push_error("TITLE_FIXTURE: could not write slot %d" % slot)


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261104
	var menu := load("res://scenes/ui/main_menu.tscn").instantiate() as MainMenu
	add_child(menu)
	await get_tree().process_frame
	look_lines(menu)
	var header := ["catnip_ever", "reawakens", "kills", "final_seconds", "final_defeated", "has_slots", "unreadable",
		"continue_disabled", "load_disabled", "status", "menu_focus", "slot_focus_last", "slot_focus", "leaderboard"]
	for i in range(SLOTS):
		header.append_array(["s%d_now" % i, "s%d_unix" % i, "s%d_reawakens" % i, "s%d_kills" % i, "s%d_play" % i,
			"s%d_delete" % i, "s%d_info" % i])
	header.append_array(["when_now", "when_unix", "when", "dur_in", "dur", "kills_in", "kills_out"])
	var out := FileAccess.open("res://title_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(header)))
	var shown_found := 0
	var unreadable_rows := 0
	for r in range(ROWS):
		var cols := PackedStringArray()
		# The game in memory (what Continue and the status line read).
		GameState.lifetime_catnip_ever = amount(rng) if rng.randf() < 0.6 else 0.0
		GameState.reawaken_count = rng.randi_range(0, 30) if rng.randf() < 0.5 else 0
		GameState.lifetime_kills = kills(rng)
		GameState.final_boss_seconds = seconds(rng) if rng.randf() < 0.3 else 0.0
		GameState.final_boss_defeated = rng.randf() < 0.4
		# The slot files.
		var now := Time.get_unix_time_from_system()
		var present := ""
		var unreadable := -1
		for s in range(SLOTS):
			SaveManager.delete_slot(s)
			if rng.randf() < 0.5:
				write_slot(s, rng, now)
				present += str(s)
		if present != "" and rng.randf() < 0.1:
			unreadable = int(present.substr(rng.randi_range(0, present.length() - 1), 1))
			var f := FileAccess.open(SaveManager.slot_path(unreadable), FileAccess.WRITE)
			f.store_string("not a save")
			f.close()
			unreadable_rows += 1
		SettingsManager.last_slot = rng.randi_range(-1, 6)
		cols.append_array([h(GameState.lifetime_catnip_ever), str(GameState.reawaken_count), str(GameState.lifetime_kills),
			h(GameState.final_boss_seconds), flag(GameState.final_boss_defeated), present, str(unreadable)])
		# The menu.
		menu.slot_panel.visible = false
		menu.confirm_panel.visible = false
		menu.credits_panel.visible = false
		menu.leaderboard_panel.visible = false
		menu._menu_return = null
		menu._refresh()
		cols.append(flag(menu.continue_button.disabled))
		cols.append(flag(menu.load_button.disabled))
		cols.append(esc(menu.status_label.text))
		shown_found += 1 if SaveManager.has_save() else 0
		menu._focus_top()
		var owner := get_viewport().gui_get_focus_owner()
		cols.append(owner.name if owner != null else "-")
		# The slot screen: the selection opens on the last slot used.
		menu._refresh_slot_cards()
		menu.slot_panel.visible = true
		menu._focus_top()
		cols.append(str(SettingsManager.last_slot))
		var sel := get_viewport().gui_get_focus_owner()
		var sel_index := -1
		for k in range(menu._slot_cards.size()):
			if menu._slot_cards[k]["card"] == sel:
				sel_index = k
		cols.append(str(sel_index))
		menu.slot_panel.visible = false
		menu._refresh_leaderboard()
		cols.append(esc(menu.leaderboard_text.text))
		var card_now := Time.get_unix_time_from_system()
		for s in range(SLOTS):
			var summary := SaveManager.slot_summary(s)
			cols.append(h(card_now))
			cols.append(h(float(summary.get("last_played_unix", -1.0))))
			cols.append(str(int(summary.get("reawakens", -1))))
			cols.append(str(int(summary.get("kills", -1))))
			cols.append(h(float(summary.get("play_seconds", -1.0))))
			cols.append(flag(menu._slot_cards[s]["delete"].visible))
			cols.append(esc(menu._slot_cards[s]["info"].text))
		# The formatters on their own.
		var wnow := Time.get_unix_time_from_system()
		var wunix: float = 0.0 if rng.randf() < 0.05 else (-50.0 if rng.randf() < 0.03 else wnow - elapsed(rng))
		var when := MainMenu.format_when(wunix)
		cols.append_array([h(wnow), h(wunix), esc(when)])
		var dur_in := seconds(rng)
		cols.append_array([h(dur_in), esc(PauseMenu.format_duration(dur_in))])
		var k_in := kills(rng)
		cols.append_array([str(k_in), esc(PauseMenu._kills_text(k_in))])
		out.store_line("\t".join(cols))
	out.close()
	for s in range(SLOTS):
		SaveManager.delete_slot(s)
	print("TITLE_FIXTURE rows=%d with_save=%d unreadable=%d" % [ROWS, shown_found, unreadable_rows])
	print("TITLE_FIXTURE done")
	get_tree().quit(0)


## A stylebox as one line.
func style_desc(sb: StyleBox) -> String:
	if sb == null:
		return "null"
	var desc := "%s" % sb.get_class()
	if sb is StyleBoxFlat:
		var f := sb as StyleBoxFlat
		desc += " bg=%s border=%s w=%d radius=%d margins=%s,%s,%s,%s" % [f.bg_color, f.border_color, f.border_width_left,
			f.corner_radius_top_left, f.content_margin_left, f.content_margin_top, f.content_margin_right, f.content_margin_bottom]
	elif sb is StyleBoxTexture:
		var t := sb as StyleBoxTexture
		desc += " texture=%s margins=%s,%s,%s,%s" % [t.texture.resource_path if t.texture else "null",
			t.content_margin_left, t.content_margin_top, t.content_margin_right, t.content_margin_bottom]
	return desc


func look(name: String, c: Control) -> void:
	print("LOOK %s.rect=%s" % [name, c.get_global_rect()])
	if c is Label or c is Button:
		print("LOOK %s.font_size=%d" % [name, c.get_theme_font_size("font_size")])
		print("LOOK %s.font_color=%s" % [name, c.get_theme_color("font_color")])
		print("LOOK %s.outline_size=%d" % [name, c.get_theme_constant("outline_size")])
	if c is Button:
		print("LOOK %s.normal=%s" % [name, style_desc(c.get_theme_stylebox("normal"))])
		print("LOOK %s.font_disabled_color=%s" % [name, c.get_theme_color("font_disabled_color")])
	if c is PanelContainer or c is Panel:
		print("LOOK %s.panel=%s" % [name, style_desc(c.get_theme_stylebox("panel"))])
	if c is ColorRect:
		print("LOOK %s.color=%s" % [name, (c as ColorRect).color])


func look_lines(menu: MainMenu) -> void:
	var root := menu.get_node("Root")
	for path in ["Background", "Title", "VersionLabel", "Panel", "Panel/Body/Menu/ContinueButton",
			"Panel/Body/Menu/NewGameButton", "Panel/Body/StatusLabel", "SlotPanel/Scrim", "SlotPanel/Body/Title",
			"SlotPanel/Body/BackButton", "ConfirmPanel/Scrim", "ConfirmPanel/Window", "ConfirmPanel/Window/Body/Prompt",
			"ConfirmPanel/Window/Body/Row/YesButton", "CreditsPanel/Window", "CreditsPanel/Window/Body/Title",
			"CreditsPanel/Window/Body/Scroll/Text", "LeaderboardPanel/Window", "LeaderboardPanel/Window/Body/Title"]:
		var n := root.get_node_or_null(path) as Control
		if n != null:
			look(path, n)
	var card: Button = menu._slot_cards[0]["card"]
	look("slot0.card", card)
	look("slot0.info", menu._slot_cards[0]["info"])
	look("slot0.delete", menu._slot_cards[0]["delete"])
	look("slot0.number", menu._slot_cards[0]["number"])
	if menu.hint != null:
		print("LOOK hint.text=%s" % esc(menu.hint.text))
		look("hint", menu.hint)
	if menu.cluster != null:
		look("cluster", menu.cluster)
		for b in menu.cluster.get_children():
			look("cluster." + b.name, b)
	if menu.boba_badge != null:
		look("boba", menu.boba_badge)
	print("LOOK version.text=%s" % menu.version_label.text)
	print("LOOK credits.text=%s" % esc((root.get_node("CreditsPanel/Window/Body/Scroll/Text") as Label).text))
