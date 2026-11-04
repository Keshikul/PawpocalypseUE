extends SceneTree

# Ground truth for the PawCore port of Amounts.short: Godot's own code (scripts/systems/amounts.gd in a scratch copy of the project)
# over every value in amounts_values.txt, in English and in a 10000-grouping, comma-decimal language (built the way
# check_hud.gd builds it). Each line: the exact double as 16 hex digits (little-endian bytes), the English result,
# the 10000-grouping result.

func _init() -> void:
	var t := Translation.new()
	t.locale = "zz"
	t.add_message("1000", "10000", "amount grouping")
	t.add_message(".", ",", "decimal separator")
	t.add_message("K", "W", "amount suffix")
	t.add_message("M", "Y", "amount suffix")
	t.add_message("B", "Z", "amount suffix")
	t.add_message("T", "J", "amount suffix")
	t.add_message("Qa", "G", "amount suffix")
	t.add_message("Qi", "X", "amount suffix")
	TranslationServer.add_translation(t)

	var Amounts := load("res://scripts/systems/amounts.gd")
	var out := FileAccess.open("res://amounts_fixture.tsv", FileAccess.WRITE)
	var count := 0
	for line in FileAccess.get_file_as_string("res://amounts_values.txt").split("\n", false):
		var v := line.to_float()
		var bytes := var_to_bytes(v)
		# Variant encoding: 4-byte header, then the 8-byte double (64-bit flag set for floats in Godot 4).
		# The whole encoded Variant: Godot writes a float as 32 bits when that loses nothing (flag in the header).
		var hex := bytes.hex_encode()
		TranslationServer.set_locale("en")
		var en: String = Amounts.short(v)
		TranslationServer.set_locale("zz")
		var zz: String = Amounts.short(v)
		out.store_line("%s\t%s\t%s" % [hex, en, zz])
		count += 1
	out.close()
	print("AMOUNTS_FIXTURE wrote %d lines" % count)
	quit(0)
