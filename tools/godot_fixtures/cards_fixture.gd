extends Node

# Ground truth for PawCore's PcCards: the REAL CardManager over random states (scratch copy of the project, sandboxed
# APPDATA; tools/make_godot_fixtures.py). Records which cards may be offered, the hand size, every effect's summed and
# best-low value, the Catches curve, and one add_kill_catches() step (including Second Opinion's reroll grant).

const ROWS := 3000


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func pairs(d: Dictionary) -> String:
	var keys := d.keys()
	keys.sort()
	var parts := PackedStringArray()
	for k in keys:
		parts.append("%s=%d" % [k, int(d[k])])
	return ";".join(parts)


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261030
	var job_ids: Array = GameState.jobs.keys()
	job_ids.sort()
	var focus := ["unlock_aura", "unlock_pounce", "unlock_overkill", "unicorn", "vitality", "curiosity", "union_rules",
		"second_opinion"]
	var effects: Array = CardData.Effect.values()
	var out := FileAccess.open("res://cards_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["upgrades", "branches", "cards", "banished", "run_job",
		"run_job_second", "reawaken_count", "deepest", "extra_choices", "catches_level", "catches_points", "hand_pending",
		"amount", "probe_level",
		"eligible", "hand_size", "effect_values", "effect_best_low", "catches_for_probe", "total_for_probe",
		"step_level", "step_points", "step_levelled", "step_rerolls"])))
	for i in range(ROWS):
		var up := {}
		for id in GameState.upgrades:
			up[id] = rng.randi_range(0, 30) if rng.randf() < 0.6 else 0
		var br := {}
		for id in SkillTreeManager.branches:
			var data: SkillBranchData = SkillTreeManager.branches[id]
			if rng.randf() < (0.5 if focus.has(id) else 0.2):
				br[id] = rng.randi_range(1, data.max_level())
		var cards := {}
		var banished := {}
		for card in CardManager.deck:
			var r := rng.randf()
			if r < 0.3:
				cards[card.id] = rng.randi_range(1, card.level_values.size())
			elif r < 0.4:
				banished[card.id] = true
		GameState.upgrade_levels = up
		SkillTreeManager.levels = br
		CardManager.owned = cards
		CardManager.banished = banished
		GameState.run_job = "" if rng.randf() < 0.4 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		GameState.run_job_second = "" if rng.randf() < 0.5 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		GameState.reawaken_count = rng.randi_range(0, 15)
		GameState.deepest_biome_index = rng.randi_range(0, 11)
		CardManager.extra_choices = rng.randi_range(0, 2)
		CardManager.catches_level = rng.randi_range(0, 25)
		var total_here: float = CardManager.total_catches_for_level(CardManager.catches_level)
		var total_next: float = CardManager.total_catches_for_level(CardManager.catches_level + 1)
		# Points between this level and a little past the next, so both branches of the step are exercised.
		CardManager.catches_points = total_here + (total_next - total_here) * rng.randf_range(0.0, 1.2)
		var pending := rng.randf() < 0.2
		var amount := rng.randf_range(0.0, 50.0) if rng.randf() < 0.9 else 0.0
		var probe := rng.randi_range(-2, 60)

		var cols := PackedStringArray([pairs(up), pairs(br), pairs(cards), ";".join(PackedStringArray(banished.keys())),
			GameState.run_job, GameState.run_job_second, str(GameState.reawaken_count), str(GameState.deepest_biome_index),
			str(CardManager.extra_choices), str(CardManager.catches_level), h(CardManager.catches_points),
			"1" if pending else "0", h(amount), str(probe)])
		var eligible := PackedStringArray()
		for card in CardManager.eligible_cards():
			eligible.append(card.id)
		cols.append(";".join(eligible))
		cols.append(str(CardManager.hand_size()))
		var values := PackedStringArray()
		var lows := PackedStringArray()
		for e in effects:
			values.append(h(CardManager.effect_value(e)))
			lows.append(h(CardManager.effect_best_low(e)))
		cols.append(";".join(values))
		cols.append(";".join(lows))
		cols.append(h(CardManager.catches_for_level(probe)))
		cols.append(h(CardManager.total_catches_for_level(probe)))

		# One add_kill_catches() step. A level-up offers a hand (and may pause): undone right after.
		var hand: Array[CardData] = []
		if pending and not CardManager.deck.is_empty():
			hand.append(CardManager.deck[0])
		CardManager._hand = hand
		CardManager.rerolls = 0
		# Auto-pick off for this step: a card it takes (Spite grants rerolls) is the DEAL's, which the draw fixture covers;
		# this column is Second Opinion's grant alone. (Found 2026-11-03 when F10 shifted Godot's random sequence.)
		SettingsManager.set_auto_pick_card(false)
		var levelled := CardManager.add_kill_catches(amount)
		cols.append(str(CardManager.catches_level))
		cols.append(h(CardManager.catches_points))
		cols.append("1" if levelled else "0")
		cols.append(str(CardManager.rerolls))
		var cleared: Array[CardData] = []
		CardManager._hand = cleared
		get_tree().paused = false
		out.store_line("\t".join(cols))
	out.close()
	print("CARDS_FIXTURE wrote %d rows" % ROWS)
	get_tree().quit(0)
