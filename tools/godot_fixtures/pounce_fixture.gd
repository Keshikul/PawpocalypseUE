extends Node

# Ground truth for PawCore's POUNCE rules (phase 2 slice 4), run on the REAL Godot code (scratch copy of the project,
# sandboxed APPDATA; tools/make_godot_fixtures.py --only pounce).
#
# Per row: a random progress state (upgrades, branches biased to the Pounce, Reflex and Daredevil nodes, cards biased to
# the Pounce cards, jobs, Reawakens and depth) and probes:
#   - GameState: is_pounce_owned(), is_auto_pounce_unlocked(), get_auto_pounce_multiplier(), has_pounce_insurance(),
#     get_pounce_crater_power(), get_pounce_lock_refund(), get_job_pounce_lock_sec()
#   - CardManager.effect_value() of POUNCE_CHAIN, POUNCE_CRATER, POUNCE_KILL_REFUND, KITTEN_MIMIC_POUNCE
#   - CombatResolver: pounce_cooldown(), pounce_damage() vulnerable and not, at x1 and at the auto-pounce multiplier.
# The Pounce itself (cooldown, lock, insurance, chain, craters) is frame- and input-driven: pinned by PawCore's rule tests.

const ROWS := 3000


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


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261102
	var job_ids: Array = GameState.jobs.keys()
	job_ids.sort()
	var biomes := []
	for i in range(1, 13):
		biomes.append("tier_%d" % i)
	var focus := ["unlock_pounce", "unlock_reflex", "job_daredevil", "nine_lives_insurance", "crash_landing", "momentum",
		"unlock_aura", "claws"]
	var pounce_effects := [CardData.Effect.POUNCE_CHAIN, CardData.Effect.POUNCE_CRATER, CardData.Effect.POUNCE_KILL_REFUND,
		CardData.Effect.KITTEN_MIMIC_POUNCE]

	var out := FileAccess.open("res://pounce_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["upgrades", "branches", "cards", "run_job", "run_job_second",
		"reawaken_count", "deepest", "current_biome", "stardust",
		"owned", "auto", "auto_mult", "insurance", "crater_power", "lock_refund", "lock_sec",
		"chain", "crater_card", "kill_refund", "mimic",
		"cooldown", "damage", "damage_vulnerable", "damage_auto", "damage_auto_vulnerable"])))
	for i in range(ROWS):
		var up := {}
		for id in GameState.upgrades:
			up[id] = rng.randi_range(0, 40) if rng.randf() < 0.6 else 0
		var br := {}
		for id in SkillTreeManager.branches:
			var data: SkillBranchData = SkillTreeManager.branches[id]
			var p := 0.6 if focus.has(id) else 0.2
			if rng.randf() < p:
				br[id] = rng.randi_range(1, data.max_level())
		var cards := {}
		for card in CardManager.deck:
			var p := 0.5 if pounce_effects.has(card.effect) else 0.2
			if rng.randf() < p:
				cards[card.id] = rng.randi_range(1, card.level_values.size())
		GameState.upgrade_levels = up
		SkillTreeManager.levels = br
		CardManager.owned = cards
		# The Daredevil (the one rooting job) worn more often than chance would.
		GameState.run_job = "daredevil" if rng.randf() < 0.3 else ("" if rng.randf() < 0.4 else job_ids[rng.randi_range(0, job_ids.size() - 1)])
		GameState.run_job_second = "" if rng.randf() < 0.7 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		# Every input set, never inherited: the sandbox's GameState keeps what an earlier fixture left.
		GameState.reawaken_count = rng.randi_range(0, 15)
		GameState.deepest_biome_index = rng.randi_range(0, 11)
		GameState.current_biome_id = biomes[rng.randi_range(0, biomes.size() - 1)]
		GameState.bosses_defeated = {}
		GameState.final_boss_defeated = false
		GameState.stardust_granted_ever = pow(10.0, rng.randf_range(0.0, 5.0)) if rng.randf() < 0.7 else 0.0
		GameState.waves_here = 0
		GameState.cards_taken_this_run = 0
		GameState.kills_this_cycle = 0
		GameState.player_still_seconds = 0.0
		GameState.lunch_seconds = 0.0
		GameState._rebuild_stats()
		var stats := GameState.get_stats()
		var auto_mult := GameState.get_auto_pounce_multiplier()

		var cols := PackedStringArray([pairs(up), pairs(br), pairs(cards), GameState.run_job, GameState.run_job_second,
			str(GameState.reawaken_count), str(GameState.deepest_biome_index), GameState.current_biome_id,
			h(GameState.stardust_granted_ever),
			flag(GameState.is_pounce_owned()), flag(GameState.is_auto_pounce_unlocked()), h(auto_mult),
			flag(GameState.has_pounce_insurance()), h(GameState.get_pounce_crater_power()),
			h(GameState.get_pounce_lock_refund()), h(GameState.get_job_pounce_lock_sec()),
			h(CardManager.effect_value(CardData.Effect.POUNCE_CHAIN)), h(CardManager.effect_value(CardData.Effect.POUNCE_CRATER)),
			h(CardManager.effect_value(CardData.Effect.POUNCE_KILL_REFUND)),
			h(CardManager.effect_value(CardData.Effect.KITTEN_MIMIC_POUNCE)),
			h(CombatResolver.pounce_cooldown(stats)), h(CombatResolver.pounce_damage(stats, false)),
			h(CombatResolver.pounce_damage(stats, true)), h(CombatResolver.pounce_damage(stats, false, auto_mult)),
			h(CombatResolver.pounce_damage(stats, true, auto_mult))])
		out.store_line("\t".join(cols))
	out.close()
	print("POUNCE_FIXTURE wrote %d rows" % ROWS)
	get_tree().quit(0)
