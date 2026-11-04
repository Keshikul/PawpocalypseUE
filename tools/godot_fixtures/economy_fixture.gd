extends Node

# Ground truth for PawCore's PcEconomy: the REAL GameState economy functions (kill rewards, waves, biome gates, Reawaken,
# bait, Legacy) over random states, in a scratch copy of the project (tools/make_godot_fixtures.py; the original is never
# written) with a sandboxed APPDATA. State is set field by field; the only calls that mutate are add_currency() and
# buy_bait(), measured as the change in Catnip and undone by the next row's reset. Doubles as encoded Variant hex.

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


func realm_ids() -> Array:
	var out := []
	for realm in GameState.REALMS:
		out.append(String(realm["data"]).get_file().get_basename())
	return out


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261029
	var job_ids: Array = GameState.jobs.keys()
	job_ids.sort()
	var ids := realm_ids()
	# Branches the economy reads, bought more often than the rest so every path is exercised.
	var focus := ["fortune", "fence", "deep_pockets", "audit", "legacy"]
	var out := FileAccess.open("res://economy_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["upgrades", "branches", "cards", "run_job", "run_job_second",
		"reawaken_count", "deepest", "bosses", "final", "stardust", "current_biome", "tier_kill_count", "kills_by_biome",
		"bait_spent", "cleared", "wave_index", "kills_this_wave", "catnip_cycle", "catnip_ever", "amount", "base_reward",
		"probe_realm",
		"whole", "kill_catnip", "biome_mult", "fortune", "golden_bonus", "golden_for_probe", "job_catnip", "fence",
		"current_wave", "wave_hp", "wave_reward", "wave_done", "wave_costs", "unlocked", "until_probe",
		"requirement", "pending", "can_reawaken", "suggested", "requirement_catnip", "until_reawaken", "bar_earned",
		"bar_required", "projected", "bait_earned", "bait_held", "audit", "shortfall", "summon_cost",
		"carryover", "start_level", "start_biome"])))
	for i in range(ROWS):
		# --- the state ---
		var up := {}
		for id in GameState.upgrades:
			up[id] = rng.randi_range(0, 40) if rng.randf() < 0.7 else 0
		var br := {}
		for id in SkillTreeManager.branches:
			var data: SkillBranchData = SkillTreeManager.branches[id]
			var p := 0.6 if focus.has(id) else 0.3
			if rng.randf() < p:
				br[id] = rng.randi_range(1, data.max_level())
		var cards := {}
		for card in CardManager.deck:
			if rng.randf() < 0.25:
				cards[card.id] = rng.randi_range(1, card.level_values.size())
		GameState.upgrade_levels = up
		SkillTreeManager.levels = br
		CardManager.owned = cards
		GameState.run_job = "" if rng.randf() < 0.3 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		GameState.run_job_second = "" if rng.randf() < 0.6 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		GameState.reawaken_count = rng.randi_range(0, 15)
		GameState.deepest_biome_index = rng.randi_range(0, 11)
		var bosses := {}
		for k in range(rng.randi_range(0, 5)):
			bosses[GameState.BOSS_REALMS[k]] = true
		GameState.bosses_defeated = bosses
		GameState.final_boss_defeated = rng.randf() < 0.1
		var cleared := {}
		for realm_id in GameState.BOSS_REALMS:
			if bosses.has(realm_id) and rng.randf() < 0.7:
				cleared[realm_id] = true
		GameState.bosses_cleared_this_cycle = cleared
		GameState.stardust_granted_ever = floor(pow(10.0, rng.randf_range(0.0, 6.0))) if rng.randf() < 0.8 else 0.0
		GameState.current_biome_id = "" if rng.randf() < 0.05 else ids[rng.randi_range(0, ids.size() - 1)]
		GameState.tier_kill_count = rng.randi_range(0, 400000) if rng.randf() < 0.8 else rng.randi_range(0, 300)
		var by_biome := {}
		var spent := {}
		for realm_id in ids:
			if rng.randf() < 0.4:
				by_biome[realm_id] = rng.randi_range(0, 300000)
			if rng.randf() < 0.2:
				spent[realm_id] = rng.randi_range(0, 120)
		GameState.tier_kills_by_biome = by_biome
		GameState.bait_spent_by_biome = spent
		GameState.current_wave_index = rng.randi_range(0, 64)
		GameState.kills_this_wave = rng.randi_range(0, 250)
		GameState.lifetime_catnip_this_cycle = pow(10.0, rng.randf_range(0.0, 15.0))
		GameState.lifetime_catnip_ever = GameState.lifetime_catnip_this_cycle + (pow(10.0, rng.randf_range(0.0, 16.0)) if rng.randf() < 0.8 else 0.0)
		var amount := rng.randf_range(-1.0, 4.0) if rng.randf() < 0.3 else float(rng.randi_range(0, 8)) * 0.5
		var base_reward := pow(10.0, rng.randf_range(-1.0, 7.0))
		var probe: String = ids[rng.randi_range(0, ids.size() - 1)]
		var probe_realm := GameState.realm_data("res://data/%s.tres" % probe)

		# --- the answers ---
		var cols := PackedStringArray([pairs(up), pairs(br), pairs(cards), GameState.run_job, GameState.run_job_second,
			str(GameState.reawaken_count), str(GameState.deepest_biome_index), ";".join(PackedStringArray(bosses.keys())),
			"1" if GameState.final_boss_defeated else "0", h(GameState.stardust_granted_ever), GameState.current_biome_id,
			str(GameState.tier_kill_count), pairs(by_biome), pairs(spent), ";".join(PackedStringArray(cleared.keys())),
			str(GameState.current_wave_index), str(GameState.kills_this_wave), h(GameState.lifetime_catnip_this_cycle),
			h(GameState.lifetime_catnip_ever), h(amount), h(base_reward), probe])
		cols.append(h(GameState.whole_catnip(amount)))
		# add_currency() also counts the kill as lifetime Catnip: save both and put them back exactly (a + b - b need not be a).
		var cycle_before := GameState.lifetime_catnip_this_cycle
		var ever_before := GameState.lifetime_catnip_ever
		GameState.catnip = 0.0
		GameState.add_currency(base_reward)
		cols.append(h(GameState.catnip))
		GameState.lifetime_catnip_this_cycle = cycle_before
		GameState.lifetime_catnip_ever = ever_before
		cols.append(h(GameState.get_biome_reward_multiplier()))
		cols.append(h(GameState.get_fortune_yield_multiplier()))
		cols.append(h(GameState.get_golden_chance_bonus()))
		cols.append(h(GameState.golden_chance_for(probe_realm)))
		cols.append(h(GameState.get_job_catnip_multiplier()))
		cols.append(h(GameState.get_fence_multiplier()))
		cols.append(str(GameState.current_wave()))
		cols.append(h(GameState.wave_hp_multiplier()))
		cols.append(h(GameState.wave_reward_multiplier()))
		var wp: Array = GameState.wave_progress()
		cols.append(h(wp[0]))
		cols.append(h(wp[1]))
		var unlocked := ""
		for realm_id in ids:
			unlocked += "1" if GameState.is_biome_unlocked(GameState.realm_data("res://data/%s.tres" % realm_id)) else "0"
		cols.append(unlocked)
		cols.append(h(GameState.catnip_until_unlocked(probe_realm)))
		cols.append(h(GameState.get_reawaken_requirement()))
		cols.append(h(GameState.get_pending_stardust()))
		cols.append("1" if GameState.can_reawaken() else "0")
		cols.append("1" if GameState.is_reawaken_suggested() else "0")
		cols.append(h(GameState.get_reawaken_requirement_catnip()))
		cols.append(h(GameState.catnip_until_reawaken()))
		var bar: Array = GameState.get_reawaken_bar_progress()
		cols.append(h(bar[0]))
		cols.append(h(bar[1]))
		cols.append(h(GameState.get_projected_reawaken_multiplier()))
		var earned := PackedStringArray()
		for realm_id in ids:
			earned.append(str(GameState.bait_earned(realm_id)))
		cols.append(";".join(earned))
		cols.append(str(GameState.bait_held(GameState.current_biome_id)))
		cols.append(h(GameState.get_audit_catnip_multiple()))
		var realm := GameState.current_realm()
		var boss: BossData = realm.boss_data if realm != null else null
		if boss == null:
			cols.append("")
			cols.append("")
		else:
			cols.append(str(GameState.bait_shortfall(boss)))
			# buy_bait() charges the summon's Catnip; measured on a bank that keeps every cost exact.
			GameState.catnip = 1.0e9
			var bought := GameState.buy_bait(boss)
			cols.append(h(1.0e9 - GameState.catnip) if bought else "")
		cols.append(h(GameState.get_catnip_carryover()))
		cols.append(str(GameState.get_starting_upgrade_level()))
		cols.append(str(GameState.get_starting_biome_index()))
		out.store_line("\t".join(cols))
	out.close()
	print("ECONOMY_FIXTURE wrote %d rows" % ROWS)
	get_tree().quit(0)
