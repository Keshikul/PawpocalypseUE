extends Node

# Ground truth for PawCore's PcStats: the REAL GameState._rebuild_stats() (this is a scratch copy of the Godot project; the
# original is never written), over random progress states. Fields are set directly (no signals, so no autosave), and the
# run's APPDATA is a sandbox. Inputs and every CombatStats field are written exactly (doubles as encoded Variant hex).

const STATS := ["paw_damage_multiplier", "attack_speed_multiplier", "shred_per_stack", "execute_threshold", "aura_damage_multiplier",
	"aura_radius_branch_multiplier", "evolution_damage_multiplier", "weapon_multiplier", "additive_damage_sum",
	"multiplicative_damage_product", "crit_chance", "crit_multiplier", "overkill_fraction", "kitten_power_multiplier",
	"kitten_additive_damage_sum", "kitten_multiplicative_damage_product", "poop_power_multiplier", "poop_additive_damage_sum",
	"poop_multiplicative_damage_product", "kitten_speed_multiplier", "shield_pierce_fraction", "pounce_power_multiplier",
	"pounce_cooldown_reduction", "reawaken_multiplier"]


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
	rng.seed = 20261028
	var job_ids: Array = GameState.jobs.keys()
	job_ids.sort()
	var out := FileAccess.open("res://stats_fixture.tsv", FileAccess.WRITE)
	out.store_line("# upgrades\tbranches\tcards\trun_job\trun_job_second\treawaken_count\tdeepest\tbosses\tfinal\tstardust\twaves\tcards_taken\tkills\taura_owned\t" + "\t".join(PackedStringArray(STATS)))
	for i in range(3000):
		var up := {}
		for id in GameState.upgrades:
			up[id] = rng.randi_range(0, 40) if rng.randf() < 0.8 else 0
		var br := {}
		for id in SkillTreeManager.branches:
			var data: SkillBranchData = SkillTreeManager.branches[id]
			if rng.randf() < 0.5:
				br[id] = rng.randi_range(1, data.max_level())
		var cards := {}
		for card in CardManager.deck:
			if rng.randf() < 0.3:
				cards[card.id] = rng.randi_range(1, card.level_values.size())
		GameState.upgrade_levels = up
		SkillTreeManager.levels = br
		CardManager.owned = cards
		GameState.run_job = "" if rng.randf() < 0.3 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		GameState.run_job_second = "" if rng.randf() < 0.5 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		GameState.reawaken_count = rng.randi_range(0, 15)
		GameState.deepest_biome_index = rng.randi_range(0, 11)
		var bosses := {}
		var beaten := rng.randi_range(0, 5)
		for k in range(beaten):
			bosses[GameState.BOSS_REALMS[k]] = true
		if rng.randf() < 0.2:
			bosses[GameState.BOSS_REALMS[rng.randi_range(0, 4)]] = true  # out of order: the ladder stops at the first gap
		GameState.bosses_defeated = bosses
		GameState.final_boss_defeated = rng.randf() < 0.15
		GameState.stardust_granted_ever = pow(10.0, rng.randf_range(0.0, 5.0)) if rng.randf() < 0.9 else 0.0
		GameState.waves_here = rng.randi_range(0, 50)
		GameState.cards_taken_this_run = rng.randi_range(0, 30)
		GameState.kills_this_cycle = rng.randi_range(0, 2000000)
		GameState._rebuild_stats()
		var s: CombatStats = GameState._cached_stats
		var bosses_keys := bosses.keys()
		bosses_keys.sort()
		var cols := PackedStringArray([pairs(up), pairs(br), pairs(cards), GameState.run_job, GameState.run_job_second,
			str(GameState.reawaken_count), str(GameState.deepest_biome_index), ";".join(PackedStringArray(bosses_keys)),
			"1" if GameState.final_boss_defeated else "0", h(GameState.stardust_granted_ever), str(GameState.waves_here),
			str(GameState.cards_taken_this_run), str(GameState.kills_this_cycle), "1" if s.aura_owned else "0"])
		for f in STATS:
			cols.append(h(s.get(f)))
		out.store_line("\t".join(cols))
	out.close()
	print("STATS_FIXTURE wrote 3000 rows")
	get_tree().quit(0)
