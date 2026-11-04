extends Node

# Ground truth for PawCore's card DRAW: the REAL CardManager dealing hands (scratch copy of the project, sandboxed APPDATA;
# tools/make_godot_fixtures.py). Each row sets a random run state (s0_*), then runs a random sequence of operations - XP
# gained (which levels, pays interest and deals), a bare draw, a forced deal, reroll, choose, banish - recording each one's
# result and a digest of the whole card state after it.
#
# Godot draws with the engine's GLOBAL randf()/randi(). Every op that can draw first calls seed(s) with a seed written into
# the op, so the C++ side replays the same stream with its own copy of Godot's PCG32 and randf(). The `calib` column (row 0)
# pins that copy: randi() and randf() sequences from known seeds, and a mixed one.

const ROWS := 3000
const OPS := 8


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func flag(b: bool) -> String:
	return "1" if b else "0"


func pairs(d: Dictionary) -> String:
	var keys := d.keys()
	keys.sort()
	var parts := PackedStringArray()
	for k in keys:
		parts.append("%s=%d" % [k, int(d[k])])
	return ";".join(parts)


func held(d: Dictionary) -> String:
	var keys := []
	for k in d:
		if d[k]:
			keys.append(String(k))
	keys.sort()
	return ";".join(PackedStringArray(keys))


func ids(cards: Array) -> String:
	var out := PackedStringArray()
	for c in cards:
		out.append((c as CardData).id if c != null else "")
	return ";".join(out)


func card(id: String) -> CardData:
	for c in CardManager.deck:
		if c.id == id:
			return c
	return null


func digest() -> String:
	var C := CardManager
	return ",".join(PackedStringArray([ids(C._hand), ids(C._promised),
		C._prototype_card.id if C._prototype_card != null else "", str(C.rerolls), str(C.banishes), str(C.fixation_hands),
		str(C.extra_choices), str(C.catches_level), h(C.catches_points), h(GameState.catnip), pairs(C.owned),
		held(C.banished), str(GameState.cards_taken_this_run)]))


func calibration() -> String:
	var parts := PackedStringArray()
	for s in [0, 1, 12345, 4294967295, 9007199254740993]:
		seed(s)
		var a := PackedStringArray()
		for k in range(6):
			a.append(str(randi()))
		seed(s)
		var b := PackedStringArray()
		for k in range(6):
			b.append(h(randf()))
		seed(s)
		var m := PackedStringArray([h(randf()), str(randi()), h(randf()), str(randi() % 7)])
		parts.append("%d=%s=%s=%s" % [s, ";".join(a), ";".join(b), ";".join(m)])
	return "|".join(parts)


# A probe card: mostly one in the hand (the paths that succeed), then any card, then an unknown id.
func pick_id(rng: RandomNumberGenerator) -> String:
	var C := CardManager
	var roll := rng.randf()
	if roll < 0.7 and not C._hand.is_empty():
		return C._hand[rng.randi_range(0, C._hand.size() - 1)].id
	if roll < 0.95:
		return C.deck[rng.randi_range(0, C.deck.size() - 1)].id
	return "no_such_card"


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261031
	var C := CardManager
	# The pick screen rebuilds its cards on every deal and frees them on a frame that never comes here (thousands of
	# deals crash it). It decides nothing: hand_offered is presentation.
	for ui in get_tree().get_nodes_in_group(CatchesPickUI.GROUP):
		ui.free()
	# AudioManager.play() randomises pitch with the same GLOBAL randf_range(), gated by a wall-clock rate limit: a sound
	# played mid-deal (an achievement earned when interest is credited) would shift the cards by a roll, and whether it
	# plays depends on real time. With no variants loaded, play() returns before rolling. Sound decides no rule.
	AudioManager._variants = {}
	var job_ids: Array = GameState.jobs.keys()
	job_ids.sort()
	var focus := ["unlock_aura", "unlock_pounce", "unlock_overkill", "unicorn", "vitality", "curiosity", "union_rules",
		"second_opinion", "blueprint", "prototype", "auto_card", "overtime"]
	var head := PackedStringArray(["calib", "s0_upgrades", "s0_branches", "s0_cards", "s0_banished", "s0_jobs_held",
		"s0_run_job", "s0_run_job_second", "s0_reawaken_count", "s0_deepest", "s0_auto_pick", "s0_digest",
		"ops", "results", "digests"])
	var out := FileAccess.open("res://draw_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(head))
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
		# Rows with a deck nearly run dry (most cards maxed or banished): the empty deal, the short draw and the
		# banish that empties the hand.
		var drained := rng.randf() < 0.15
		for c in C.deck:
			var r := rng.randf()
			if drained and r < 0.97:
				if r < 0.485:
					cards[c.id] = c.level_values.size()
				else:
					banished[c.id] = true
			elif r < 0.3:
				cards[c.id] = rng.randi_range(1, c.level_values.size())
			elif r < 0.37:
				banished[c.id] = true
		GameState.upgrade_levels = up
		SkillTreeManager.levels = br
		C.owned = cards
		C.banished = banished
		var held_jobs := {}
		for j in job_ids:
			if rng.randf() < 0.3:
				held_jobs[j] = true
		GameState.jobs_held = held_jobs
		GameState.run_job = "" if rng.randf() < 0.4 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		GameState.run_job_second = "" if rng.randf() < 0.6 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		GameState.reawaken_count = rng.randi_range(0, 15)
		GameState.deepest_biome_index = rng.randi_range(0, 11)
		GameState.catnip = floorf(exp(rng.randf_range(0.0, 20.0)))
		GameState.cards_taken_this_run = rng.randi_range(0, 20)
		SettingsManager.auto_pick_card = rng.randf() < 0.25
		C.extra_choices = rng.randi_range(0, 2)
		C.rerolls = rng.randi_range(0, 3)
		C.banishes = rng.randi_range(1, 3) if drained or rng.randf() < 0.6 else 0
		C.fixation_hands = rng.randi_range(1, 3) if rng.randf() < 0.3 else 0
		C.catches_level = rng.randi_range(0, 25)
		C.catches_points = C.total_catches_for_level(C.catches_level) + rng.randf() * C.catches_for_level(C.catches_level)
		var promised: Array[CardData] = []
		if rng.randf() < 0.3:
			for n in range(rng.randi_range(1, 2)):
				promised.append(C.deck[rng.randi_range(0, C.deck.size() - 1)])
		C._promised = promised
		C._prototype_card = C.deck[rng.randi_range(0, C.deck.size() - 1)] if rng.randf() < 0.2 else null
		# A hand already up (a pause, not a save): dealt by the real code; the digest records it.
		var empty: Array[CardData] = []
		C._hand = empty
		var s0_seed := rng.randi()
		if drained or rng.randf() < 0.35:
			seed(s0_seed)
			C._hand = C.draw_hand(C.hand_size())

		var cols := PackedStringArray([calibration() if i == 0 else "", pairs(up), pairs(br), pairs(cards),
			held(banished), held(held_jobs), GameState.run_job, GameState.run_job_second, str(GameState.reawaken_count),
			str(GameState.deepest_biome_index), flag(SettingsManager.auto_pick_card), digest()])

		var ops := PackedStringArray()
		var results := PackedStringArray()
		var digests := PackedStringArray()
		for k in range(OPS):
			var s := rng.randi()
			var roll := rng.randf()
			var op := ""
			var result := ""
			if roll < 0.3:
				# XP towards (and often past) the next level.
				var need: float = C.total_catches_for_level(C.catches_level + 1) - C.catches_points
				var amount := maxf(need, 0.0) * rng.randf_range(0.3, 1.6) if rng.randf() < 0.9 else 0.0
				op = "gain:%d,%s" % [s, h(amount)]
				seed(s)
				result = flag(C.add_kill_catches(amount))
			elif roll < 0.4:
				var size := rng.randi_range(0, 9) if rng.randf() < 0.8 else rng.randi_range(20, 30)
				op = "draw:%d,%d" % [s, size]
				seed(s)
				result = ids(C.draw_hand(size))
			elif roll < 0.55:
				op = "offer:%d" % s
				seed(s)
				C._offer_hand()
			elif roll < 0.7:
				op = "reroll:%d" % s
				seed(s)
				result = flag(C.reroll_hand())
			elif roll < 0.88:
				var id := pick_id(rng)
				op = "choose:" + id
				result = flag(C.choose(card(id)))
			else:
				var id := pick_id(rng)
				op = "banish:%d,%s" % [s, id]
				seed(s)
				result = flag(C.banish_from_hand(card(id)))
			get_tree().paused = false
			ops.append(op)
			results.append(result)
			digests.append(digest())
		cols.append("|".join(ops))
		cols.append("|".join(results))
		cols.append("|".join(digests))
		out.store_line("\t".join(cols))
	out.close()
	print("DRAW_FIXTURE wrote %d rows" % ROWS)
	get_tree().quit(0)
