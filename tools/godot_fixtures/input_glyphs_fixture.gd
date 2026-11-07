extends Node

# Ground truth for PawGlyphs (docs/05 §5): what the real InputGlyphs (scripts/systems/input_glyphs.gd) shows for every action
# of the project's input map in every prompt family, the label it draws on a keycap for every key, and what its {token}
# replacement makes of the prompt templates the game draws (and of a placeholder that is not an action).
#
# Three tables:
#   input_glyphs_fixture.tsv   action, family, kind (none | glyph | keycap | blank), stem, file exists, keycap label
#   input_keys_fixture.tsv     Godot keycode, key_label (headless: the position's US label, as InputGlyphs says)
#   input_prompts_fixture.tsv  template id, family, the template, InputGlyphs.bbcode() (the converter neutralises the BBCode)

const TEMPLATES := {
	"hud_upgrades": "{press} Buy   {buy_amount} Amount   {open_skill_tree} Skills   {pause} Pause",
	"hud_other": "{press} Select   {open_skill_tree} Skills   {pause} Pause",
	"tree_legend": "{buy_amount} Next affordable   {zoom_out} {zoom_in} Zoom   {back} Close",
	"tree_buy": "{press} Buy",
	"title_hint": "In game:  {tab_prev} {tab_next} menu tabs    {open_skill_tree} skill tree    {pause} pause",
	"blurb_pounce": "Teaches you to POUNCE — a burst on {pounce}, worth more if you time it into a vulnerable window. Its two shop lines appear once you own it.",
	"tutorial_pounce": "{pounce} to Pounce.",
	"tutorial_steer": "{steer_right}",
	"not_actions": "{n} of {max}: {press}{press} {recentre} {unknown_action} {",
	"empty": "",
}

# Every key the port can bind (tools/export_input_map.py KEYS) plus the printable ASCII range and Godot's named keys.
const NAMED_KEYS := [KEY_SPACE, KEY_ENTER, KEY_KP_ENTER, KEY_ESCAPE, KEY_TAB, KEY_SHIFT, KEY_CTRL, KEY_ALT, KEY_BACKSPACE,
	KEY_LEFT, KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_PAGEUP, KEY_PAGEDOWN, KEY_HOME, KEY_END, KEY_INSERT, KEY_DELETE,
	KEY_F1, KEY_F12, KEY_KP_0, KEY_KP_9, KEY_KP_ADD, KEY_KP_SUBTRACT, KEY_KP_MULTIPLY, KEY_KP_DIVIDE, KEY_CAPSLOCK]


func _ready() -> void:
	var g := FileAccess.open("res://input_glyphs_fixture.tsv", FileAccess.WRITE)
	g.store_line("# " + "\t".join(PackedStringArray(["action", "family", "kind", "stem", "exists", "label"])))
	var rows := 0
	var actions: Array = []
	for action in InputMap.get_actions():
		if String(action).begins_with("ui_") and not ProjectSettings.has_setting("input/" + action):
			continue
		actions.append(String(action))
	actions.sort()
	for action in actions:
		for family in 5:
			var ev := InputGlyphs.event_for(StringName(action), family)
			var kind := "none"
			var stem := ""
			var exists := "0"
			var label := ""
			if ev != null:
				stem = InputGlyphs.glyph_stem(ev, family)
				var path := InputGlyphs.path_for_stem(stem)
				if not path.is_empty() and ResourceLoader.exists(path):
					kind = "glyph"
					exists = "1"
				elif ev is InputEventKey:
					kind = "keycap"
					label = InputGlyphs.key_label(ev as InputEventKey)
				else:
					kind = "blank"
			g.store_line("\t".join(PackedStringArray([action, str(family), kind, stem, exists, label])))
			rows += 1
	g.close()

	var k := FileAccess.open("res://input_keys_fixture.tsv", FileAccess.WRITE)
	k.store_line("# keycode\tlabel")
	var codes: Array = []
	for c in range(33, 127):
		codes.append(c)
	codes.append_array(NAMED_KEYS)
	for code in codes:
		var e := InputEventKey.new()
		e.physical_keycode = code
		k.store_line("%d\t%s" % [code, InputGlyphs.key_label(e)])
	k.close()

	var p := FileAccess.open("res://input_prompts_fixture.tsv", FileAccess.WRITE)
	p.store_line("# id\tfamily\ttemplate\tbbcode")
	var ids := TEMPLATES.keys()
	ids.sort()
	for id in ids:
		for family in 5:
			p.store_line("%s\t%d\t%s\t%s" % [id, family, TEMPLATES[id], InputGlyphs.bbcode(TEMPLATES[id], family, 28)])
	p.close()
	print("INPUT_GLYPHS_FIXTURE done %d" % rows)
	get_tree().quit()
