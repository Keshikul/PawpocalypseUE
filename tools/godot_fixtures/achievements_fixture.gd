extends Node

# Ground truth for PawCore's PcAchievements: the REAL AchievementManager over random states (scratch copy of the project,
# sandboxed APPDATA; tools/make_godot_fixtures.py). Each row sets random counters (s0_*), then runs a random sequence of
# operations - a save loaded (apply_save_data), counters moving and the re-scan the signal would run, claims one by one and
# all at once, a new game - recording each one's result (what was earned, in the order it was emitted, or the Stardust
# paid) and a digest after it (earned and claimed as bits in roster order, the sets' sizes, both Stardust totals). Then the reads of the state the ops left: every
# achievement's progress value, reward, earned/claimable/claimed, and the roster's totals.
#
# The shipping roster pays whole Stardust and has whole thresholds, which cannot tell a >= from a >, a reward of 0 from a
# missing one or a sum from a count: 40 % of the rows overwrite a few achievements' threshold and reward IN MEMORY (the
# copy is scratch) and write the values used into the row. The roster order (the sorted file names) is recorded once.

const ROWS := 3000
const OPS := 10
const REWARDS := [0.0, -1.0, 0.5, 3.25, 1e-9, 12.0]
const THRESHOLDS := [0.0, -1.0, 0.5, 2.5, 1000.25, 1e12]

var emitted: PackedStringArray = []


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func flag(b: bool) -> String:
	return "1" if b else "0"


func pairs_nz(d: Dictionary) -> String:
	var keys := d.keys()
	keys.sort()
	var parts := PackedStringArray()
	for k in keys:
		if int(d[k]) != 0:
			parts.append("%s=%d" % [k, int(d[k])])
	return ";".join(parts)


func held(d: Dictionary) -> String:
	var keys := []
	for k in d:
		if d[k]:
			keys.append(String(k))
	keys.sort()
	return ";".join(PackedStringArray(keys))


func listed(a: Array) -> String:
	var keys := a.duplicate()
	keys.sort()
	return ";".join(PackedStringArray(keys))


func on_earned(data: AchievementData) -> void:
	emitted.append(data.id)


# A value near a threshold of `metric` (below, on, above), or anywhere.
func near(rng: RandomNumberGenerator, thresholds: Array, whole: bool, top: float) -> float:
	var roll := rng.randf()
	if roll < 0.15:
		return 0.0
	if roll < 0.7 and not thresholds.is_empty():
		var t: float = thresholds[rng.randi_range(0, thresholds.size() - 1)]
		var step: int = rng.randi_range(-1, 1)
		if whole:
			return maxf(t + step, 0.0)
		return t * (1.0 + step * 1e-12)
	var v := exp(rng.randf_range(0.0, log(top)))
	return floorf(v) if whole else v


func state() -> PackedStringArray:
	var G := GameState
	return PackedStringArray([str(G.lifetime_kills), h(G.lifetime_catnip_ever), str(G.reawaken_count),
		held(G.bosses_defeated), flag(G.final_boss_defeated), pairs_nz(G.upgrade_levels), pairs_nz(SkillTreeManager.levels),
		h(G.stardust), h(G.stardust_granted_ever)])


# Earned and claimed as one bit per roster entry (in roster order), with the sets' sizes, which also count ids the roster
# does not have.
func digest(roster: Array) -> String:
	var A := AchievementManager
	var e := ""
	var c := ""
	for id in roster:
		e += flag(bool(A.earned.get(id, false)))
		c += flag(bool(A.claimed.get(id, false)))
	return ",".join(PackedStringArray([e, c, str(A.earned.size()), str(A.claimed.size()),
		h(GameState.stardust), h(GameState.stardust_granted_ever)]))


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261029
	var A := AchievementManager
	A.achievement_earned.connect(on_earned)
	var roster: Array = []
	var shipped := {}
	var by_metric := {}
	for data in A.achievements:
		roster.append(data.id)
		shipped[data.id] = [data.threshold, data.stardust_reward]
		if not by_metric.has(data.metric):
			by_metric[data.metric] = []
		by_metric[data.metric].append(data)
	var upgrade_ids: Array = GameState.upgrades.keys()
	upgrade_ids.sort()
	var branch_ids: Array = SkillTreeManager.branches.keys()
	branch_ids.sort()
	var tier_ids: Array = []
	for n in range(1, 13):
		tier_ids.append("tier_%d" % n)
	var realms: Array = GameState.BOSS_REALMS
	var asked: Array = roster.duplicate()
	asked.append("no_such_achievement")

	var head := PackedStringArray(["roster", "overrides"])
	for col in ["kills", "catnip_ever", "reawaken_count", "bosses", "final", "upgrades", "branches", "stardust", "granted"]:
		head.append("s0_" + col)
	head.append_array(PackedStringArray(["ops", "results", "digests", "asked", "progress", "reward", "earned", "claimable",
		"claimed", "earned_count", "unclaimed_count", "unclaimed_stardust", "on_offer"]))
	var out := FileAccess.open("res://achievements_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(head))

	for i in range(ROWS):
		# The roster as shipped, then this row's overwrites.
		for data in A.achievements:
			data.threshold = shipped[data.id][0]
			data.stardust_reward = shipped[data.id][1]
		var overrides := PackedStringArray()
		if rng.randf() < 0.4:
			for n in range(rng.randi_range(1, 4)):
				var data: AchievementData = A.achievements[rng.randi_range(0, A.achievements.size() - 1)]
				if rng.randf() < 0.6:
					data.stardust_reward = REWARDS[rng.randi_range(0, REWARDS.size() - 1)]
				if rng.randf() < 0.4:
					data.threshold = THRESHOLDS[rng.randi_range(0, THRESHOLDS.size() - 1)]
			# Written after the loop: a later overwrite of the same achievement is the one in force.
			for data in A.achievements:
				if data.threshold != shipped[data.id][0] or data.stardust_reward != shipped[data.id][1]:
					overrides.append("%s,%s,%s" % [data.id, h(data.threshold), h(data.stardust_reward)])

		# The counters, near the roster's thresholds as often as not.
		var G := GameState
		var kill_t: Array = []
		var catnip_t: Array = []
		for data in by_metric.get(AchievementData.Metric.LIFETIME_KILLS, []):
			kill_t.append(data.threshold)
		for data in by_metric.get(AchievementData.Metric.LIFETIME_CATNIP, []):
			catnip_t.append(data.threshold)
		G.lifetime_kills = int(near(rng, kill_t, true, 3e6))
		G.lifetime_catnip_ever = near(rng, catnip_t, false, 1e13)
		G.reawaken_count = rng.randi_range(0, 14)
		var bosses := {}
		if rng.randf() < 0.5:
			for r in range(rng.randi_range(0, realms.size())):
				bosses[realms[r]] = true
		else:
			for t in tier_ids:
				if rng.randf() < 0.2:
					bosses[t] = true
		# A save may hold a boss as false: it must not count.
		if rng.randf() < 0.2:
			bosses[realms[rng.randi_range(0, realms.size() - 1)]] = false
		G.bosses_defeated = bosses
		G.final_boss_defeated = rng.randf() < 0.15
		var ups := {}
		for u in upgrade_ids:
			if rng.randf() < 0.3:
				ups[u] = rng.randi_range(0, 80)
		G.upgrade_levels = ups
		var levels := {}
		for b in branch_ids:
			if rng.randf() < 0.3:
				levels[b] = rng.randi_range(0, 8)
		SkillTreeManager.levels = levels
		G.stardust = floorf(rng.randf_range(0.0, 100.0))
		G.stardust_granted_ever = G.stardust + floorf(rng.randf_range(0.0, 500.0))

		var cols := PackedStringArray([";".join(PackedStringArray(roster)) if i == 0 else "", ";".join(overrides)])
		cols.append_array(state())

		var ops := PackedStringArray()
		var results := PackedStringArray()
		var digests := PackedStringArray()
		for k in range(OPS):
			var roll := rng.randf()
			var op := ""
			var result := ""
			emitted = PackedStringArray()
			if k == 0 or roll < 0.1:
				# A save: mostly real achievements, sometimes one the roster no longer has, sometimes a claim
				# without the earn (a hand-edited save).
				var earned := {}
				var claimed := {}
				for id in roster:
					if rng.randf() < 0.3:
						earned[id] = true
						if rng.randf() < 0.5:
							claimed[id] = true
					elif rng.randf() < 0.03:
						claimed[id] = true
				if rng.randf() < 0.1:
					earned["retired_achievement"] = true
				op = "load:%s,%s" % [listed(earned.keys()), listed(claimed.keys())]
				A.apply_save_data(earned, claimed)
				result = ";".join(emitted)
			elif roll < 0.45:
				# A counter moves, and the re-scan its signal would run.
				var what := rng.randi_range(0, 7)
				var arg := ""
				match what:
					0:
						G.lifetime_kills = int(near(rng, kill_t, true, 3e6))
						arg = "kills,%d" % G.lifetime_kills
					1:
						G.lifetime_catnip_ever = near(rng, catnip_t, false, 1e13)
						arg = "catnip,%s" % h(G.lifetime_catnip_ever)
					2:
						G.reawaken_count += 1
						arg = "reawaken,%d" % G.reawaken_count
					3:
						var r: String = realms[rng.randi_range(0, realms.size() - 1)]
						G.bosses_defeated[r] = true
						arg = "boss," + r
					4:
						G.final_boss_defeated = true
						arg = "final,1"
					5:
						var u: String = upgrade_ids[rng.randi_range(0, upgrade_ids.size() - 1)]
						G.upgrade_levels[u] = rng.randi_range(0, 80)
						arg = "up,%s,%d" % [u, G.upgrade_levels[u]]
					6:
						var b: String = branch_ids[rng.randi_range(0, branch_ids.size() - 1)]
						SkillTreeManager.levels[b] = rng.randi_range(0, 8)
						arg = "br,%s,%d" % [b, SkillTreeManager.levels[b]]
					_:
						arg = "none"
				op = "set:" + arg
				A.evaluate()
				result = ";".join(emitted)
			elif roll < 0.8:
				# Mostly something that might pay; sometimes an id that does not exist.
				var id: String = asked[rng.randi_range(0, asked.size() - 1)]
				var earned_ids: Array = A.earned.keys()
				if rng.randf() < 0.6 and not earned_ids.is_empty():
					id = earned_ids[rng.randi_range(0, earned_ids.size() - 1)]
				op = "claim:" + id
				result = h(A.claim(id))
			elif roll < 0.97:
				op = "claimall:"
				result = h(A.claim_all())
			else:
				# The title screen's New Game.
				op = "newgame:"
				G.debug_reset_progress()
				SkillTreeManager.apply_save_data({})
				A.apply_save_data({})
				result = ";".join(emitted)
			ops.append(op)
			results.append(result)
			digests.append(digest(roster))
		cols.append("|".join(ops))
		cols.append("|".join(results))
		cols.append("|".join(digests))

		var progress := PackedStringArray()
		var reward := PackedStringArray()
		var earned_q := PackedStringArray()
		var claimable := PackedStringArray()
		var claimed_q := PackedStringArray()
		for id in asked:
			var data: AchievementData = A._by_id(id)
			progress.append(h(A.progress_value(data)) if data != null else "")
			reward.append(h(A.reward_for(id)))
			earned_q.append(flag(A.is_earned(id)))
			claimable.append(flag(A.is_claimable(id)))
			claimed_q.append(flag(A.is_claimed(id)))
		cols.append(";".join(PackedStringArray(asked)))
		cols.append(";".join(progress))
		cols.append(";".join(reward))
		cols.append(";".join(earned_q))
		cols.append(";".join(claimable))
		cols.append(";".join(claimed_q))
		cols.append(str(A.earned_count()))
		cols.append(str(A.unclaimed_count()))
		cols.append(h(A.unclaimed_stardust()))
		cols.append(h(A.total_stardust_on_offer()))
		out.store_line("\t".join(cols))
	out.close()
	print("ACHIEVEMENTS_FIXTURE wrote %d rows" % ROWS)
	get_tree().quit(0)
