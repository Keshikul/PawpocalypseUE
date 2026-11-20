extends SceneTree

# Ground truth for PawCore's PcLocales: the REAL scripts/systems/locales.gd (scratch copy of the project, with the port's
# fixes applied: tools/godot_fixes.py, F52), recompiled from its own source with what it asks the machine swapped for
# probes: OS.get_locale() and OS.get_locale_language() (the system's language), TranslationServer.get_loaded_locales()
# (which .po files are loaded) and OS.has_feature("editor") (always false: the port lists no test locale). The
# TranslationServer itself stays real, so draft_marker() reads each language's own "(draft)" from Godot's .po files.
#
# Rows (tab-separated):
#   avail   <loaded, comma-joined>                  available(), comma-joined
#   system  <OS locale>  <loaded>                   from_system()
#   steam   <Steam language>  <loaded>              from_steam()
#   label   <code>  native_name()  picker_label()  draft_marker()

const SRC := "res://scripts/systems/locales.gd"
const SUBSETS := 60

var _failed := false

# The OS locales a player's machine can report (Godot spells them with '_'): every code the game has, regions it names and
# regions it does not, script forms, and nonsense.
const OS_LOCALES := [
	"en", "en_US", "en_GB", "en_AU", "zh_Hans", "zh_Hant", "zh", "zh_CN", "zh_SG", "zh_TW", "zh_HK", "zh_MO",
	"zh_Hans_CN", "zh_Hant_TW", "zh_Hant_HK", "ru", "ru_RU", "ru_UA", "ru_KZ", "es", "es_ES", "es_MX", "es_AR", "es_CO",
	"es_CL", "es_PE", "es_US", "es_419", "pt", "pt_PT", "pt_BR", "pt_AO", "pt_MZ", "de", "de_DE", "de_AT", "de_CH",
	"ja", "ja_JP", "fr", "fr_FR", "fr_CA", "fr_BE", "fr_CH", "pl", "pl_PL", "ko", "ko_KR", "tr", "tr_TR", "th", "th_TH",
	"es_MX", "uk", "uk_UA", "it", "it_IT", "it_CH", "cs", "cs_CZ", "hu", "hu_HU", "vi", "vi_VN", "sv", "sv_SE", "sv_FI",
	"nl", "nl_NL", "nl_BE", "da", "da_DK", "fi_FI", "nb_NO", "ar_SA", "el_GR", "sr_Latn_RS", "ca_ES", "gl_ES", "eu_ES",
	"C", "", "xx_YY", "esp", "ptbr",
]

const STEAM := [
	"english", "schinese", "russian", "spanish", "brazilian", "german", "japanese", "french", "polish", "koreana",
	"tchinese", "turkish", "thai", "latam", "ukrainian", "italian", "czech", "hungarian", "portuguese", "vietnamese",
	"swedish", "dutch", "danish", "finnish", "arabic", "", "English", "korean",
]


func _fail(why: String) -> void:
	push_error("LOCALES_FIXTURE " + why)
	_failed = true


## The real script's source, its machine questions swapped for the probes, compiled.
func _probe() -> GDScript:
	var src := FileAccess.get_file_as_string(SRC)
	if src.is_empty():
		_fail("cannot read " + SRC)
		return null
	var kept := PackedStringArray()
	for line in src.split("\n"):
		if line.begins_with("class_name "):
			continue
		kept.append(line)
		if line.begins_with("extends "):
			kept.append("static var _p_locale := \"\"")
			kept.append("static var _p_lang := \"\"")
			kept.append("static var _p_loaded := PackedStringArray()")
			kept.append("static var _p_editor := false")
	src = "\n".join(kept)
	for pair in [["OS.get_locale_language()", "_p_lang"], ["OS.get_locale()", "_p_locale"],
			["TranslationServer.get_loaded_locales()", "_p_loaded"], ["OS.has_feature(\"editor\")", "_p_editor"]]:
		if src.find(pair[0]) < 0:
			_fail("nothing to swap for %s (the script changed shape)" % pair[0])
			return null
		src = src.replace(pair[0], pair[1])
	var script := GDScript.new()
	script.source_code = src
	if script.reload() != OK:
		_fail("the probe of locales.gd does not compile")
		return null
	return script


func _init() -> void:
	var L := _probe()
	if L == null:
		quit(1)
		return
	var rng := RandomNumberGenerator.new()
	rng.seed = 7023
	var codes: Array = []
	for code in L.ORDER:
		if code != L.TEST_LOCALE:
			codes.append(code)
	# The loaded sets: none, all, each one alone, every one but one, and random subsets.
	var sets: Array = [PackedStringArray(), PackedStringArray(codes)]
	for code in codes:
		sets.append(PackedStringArray([code]))
		var but := PackedStringArray()
		for other in codes:
			if other != code:
				but.append(other)
		sets.append(but)
	for i in SUBSETS:
		var pick := PackedStringArray()
		for code in codes:
			if rng.randf() < 0.5:
				pick.append(code)
		sets.append(pick)

	var out := FileAccess.open("res://locales_fixture.tsv", FileAccess.WRITE)
	var rows := 0
	L._p_editor = false
	for loaded in sets:
		L._p_loaded = loaded
		var joined := ",".join(loaded)
		out.store_line("avail\t%s\t\t%s" % [joined, ",".join(L.available())])
		rows += 1
		for wanted in OS_LOCALES:
			L._p_locale = wanted
			# OS::get_locale_language(): the locale's first three characters without the '_'.
			L._p_lang = String(wanted).left(3).replace("_", "")
			out.store_line("system\t%s\t%s\t%s" % [wanted, joined, L.from_system()])
			rows += 1
		for steam in STEAM:
			out.store_line("steam\t%s\t%s\t%s" % [steam, joined, L.from_steam(steam)])
			rows += 1
	for code in codes:
		out.store_line("label\t%s\t%s\t%s\t%s" % [code, L.native_name(code), L.picker_label(code), L.draft_marker(code)])
		rows += 1
	out.close()
	if _failed:
		quit(1)
		return
	print("LOCALES_FIXTURE %d rows" % rows)
	quit(0)
