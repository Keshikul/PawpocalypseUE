extends Node

# Ground truth for PawCore's PcTree: the REAL SkillTreeManager over random states (scratch copy of the project, sandboxed
# APPDATA; tools/make_godot_fixtures.py). Records, for every branch in BRANCH_PATHS order, whether the tree sells it, its
# next price, whether it is maxed, the rival that closed it, its Reawaken gate and whether the wallet covers it; the
# reverse job lookup; then one purchase_next_level() and one grant_level() probe.

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


func held(d: Dictionary) -> String:
	var keys := []
	for k in d:
		if d[k]:
			keys.append(String(k))
	keys.sort()
	return ";".join(PackedStringArray(keys))


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261029
	var ids: Array = SkillTreeManager.branches.keys()
	var job_ids: Array = GameState.jobs.keys()
	job_ids.sort()
	var boss_ids: Array = []
	for id in ids:
		var data: SkillBranchData = SkillTreeManager.branches[id]
		if not data.gated_by_boss.is_empty() and not boss_ids.has(data.gated_by_boss):
			boss_ids.append(data.gated_by_boss)
	boss_ids.sort()
	# The branches whose purchase does more than add a level: the automation switches and the job nodes.
	var focus := ["auto_travel", "auto_card"]
	for id in ids:
		if not SkillTreeManager.branches[id].job_id.is_empty():
			focus.append(id)
	var out := FileAccess.open("res://tree_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["branches", "reawaken_count", "bosses", "stardust", "jobs_held",
		"ids", "available", "next_cost", "maxed", "rival", "gate", "affordable", "job_branches", "trunks", "vitality_max",
		"probe", "bought", "stardust_after", "level_after", "jobs_after", "auto_travel", "auto_card",
		"grant_id", "grant_level", "grant_after"])))
	for i in range(ROWS):
		# Sparse levels, so parents, rivals and gates are each both met and unmet.
		var br := {}
		for id in ids:
			var data: SkillBranchData = SkillTreeManager.branches[id]
			if rng.randf() < 0.3:
				br[id] = rng.randi_range(1, data.max_level())
		SkillTreeManager.levels = br
		GameState.reawaken_count = rng.randi_range(0, 12)
		var bosses := {}
		for b in boss_ids:
			if rng.randf() < 0.5:
				bosses[b] = true
		GameState.bosses_defeated = bosses
		var jobs := {}
		for j in job_ids:
			if rng.randf() < 0.3:
				jobs[j] = true
		GameState.jobs_held = jobs
		var roll := rng.randf()
		var probe: String = ids[rng.randi_range(0, ids.size() - 1)]
		if roll < 0.3:
			probe = focus[rng.randi_range(0, focus.size() - 1)]
		elif roll > 0.95:
			probe = "no_such_branch"
		# Half the time, open every door in front of the probe, so the purchase path itself is exercised.
		var probe_data: SkillBranchData = SkillTreeManager.get_branch(probe)
		if probe_data != null and rng.randf() < 0.5:
			if probe_data.parent_is_required and probe_data.parent != "":
				br[probe_data.parent] = maxi(int(br.get(probe_data.parent, 0)), 1)
			for rival in probe_data.exclusive_with:
				br.erase(String(rival))
			if not probe_data.gated_by_boss.is_empty():
				bosses[probe_data.gated_by_boss] = true
			GameState.reawaken_count = maxi(GameState.reawaken_count, SkillTreeManager.branch_reawaken_gate(probe))
			if br.get(probe, 0) >= probe_data.max_level() and rng.randf() < 0.8:
				br.erase(probe)
		var probe_cost := SkillTreeManager.get_next_cost(probe)
		if probe_cost > 0.0 and probe_cost != INF and rng.randf() < 0.7:
			GameState.stardust = probe_cost * rng.randf_range(0.5, 1.5)
		else:
			GameState.stardust = rng.randf_range(0.0, 20000.0)

		var cols := PackedStringArray([pairs(br), str(GameState.reawaken_count), held(bosses), h(GameState.stardust),
			held(jobs)])
		var available := PackedStringArray()
		var costs := PackedStringArray()
		var maxed := PackedStringArray()
		var rivals := PackedStringArray()
		var gates := PackedStringArray()
		var affordable := PackedStringArray()
		for id in ids:
			available.append("1" if SkillTreeManager.is_branch_available(id) else "0")
			costs.append(h(SkillTreeManager.get_next_cost(id)))
			maxed.append("1" if SkillTreeManager.is_max_level(id) else "0")
			rivals.append(SkillTreeManager.blocking_rival(id))
			gates.append(str(SkillTreeManager.branch_reawaken_gate(id)))
			affordable.append("1" if SkillTreeManager.can_afford_next(id) else "0")
		cols.append(";".join(PackedStringArray(ids)))
		cols.append(";".join(available))
		cols.append(";".join(costs))
		cols.append(";".join(maxed))
		cols.append(";".join(rivals))
		cols.append(";".join(gates))
		cols.append(";".join(affordable))
		var job_branches := PackedStringArray()
		for j in job_ids:
			job_branches.append(SkillTreeManager.branch_for_job(j))
		cols.append(";".join(job_branches))
		cols.append(";".join(PackedStringArray(SkillTreeManager.trunk_branches())))
		cols.append(str(SkillTreeManager.get_max_possible_vitality_bonus()))

		# One purchase. The automation switches start OFF so the node turning its own on is seen.
		SettingsManager.set_auto_travel(false)
		SettingsManager.set_auto_pick_card(false)
		var bought := SkillTreeManager.purchase_next_level(probe)
		cols.append(probe)
		cols.append("1" if bought else "0")
		cols.append(h(GameState.stardust))
		cols.append(str(SkillTreeManager.get_level(probe)))
		cols.append(held(GameState.jobs_held))
		cols.append("1" if SettingsManager.auto_travel else "0")
		cols.append("1" if SettingsManager.auto_pick_card else "0")

		# One grant (Muscle Memory II's path): never lowers, clamps to the ladder.
		var grant_id: String = ids[rng.randi_range(0, ids.size() - 1)]
		var grant_level := rng.randi_range(-2, 8)
		SkillTreeManager.grant_level(grant_id, grant_level)
		cols.append(grant_id)
		cols.append(str(grant_level))
		cols.append(str(SkillTreeManager.get_level(grant_id)))
		out.store_line("\t".join(cols))
	SettingsManager.set_auto_travel(false)
	SettingsManager.set_auto_pick_card(false)
	out.close()
	print("TREE_FIXTURE wrote %d rows" % ROWS)
	get_tree().quit(0)
