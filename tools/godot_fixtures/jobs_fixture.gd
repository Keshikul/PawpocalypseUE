extends Node

# Ground truth for PawCore's PcJobs: the REAL GameState job functions over random states (scratch copy of the project,
# sandboxed APPDATA; tools/make_godot_fixtures.py). Records the roster order, what is held and available, Union Rules and
# its share, the job(s) in effect, every job term and the additive/counted ones, the hat, JobData's two predicates and
# JobPicker.should_ask(); then one offer_job(), lock_run_job(), lock_run_job_second() and take_job() probe, in that order.
#
# The shipping data has a single Union Rules level at 0.5, which cannot tell a clamp, a floor or a round apart: 40 % of the
# rows overwrite that level's effect IN MEMORY (the copy is scratch) with another share, including ones the clamp must
# catch, and write the share used into the row.

const ROWS := 3000
const FIELDS := ["paw_damage_multiplier", "aura_damage_multiplier", "aura_radius_multiplier", "kitten_damage_multiplier",
	"pounce_damage_multiplier", "all_damage_multiplier", "catnip_multiplier", "crit_chance_multiplier",
	"attack_speed_multiplier", "crit_damage_multiplier", "enemy_health_multiplier", "spawn_rate_multiplier"]
const SHARES := [0.0, 0.25, 0.7, 1.0, 1.3, -0.2, 2.0]


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func held(d: Dictionary) -> String:
	var keys := []
	for k in d:
		if d[k]:
			keys.append(String(k))
	keys.sort()
	return ";".join(PackedStringArray(keys))


func flag(b: bool) -> String:
	return "1" if b else "0"


# A probe id: mostly held jobs (the paths that succeed), then any job, the bare neck, an unknown id.
func pick(rng: RandomNumberGenerator, held_ids: Array, all_ids: Array) -> String:
	var roll := rng.randf()
	if roll < 0.5 and not held_ids.is_empty():
		return held_ids[rng.randi_range(0, held_ids.size() - 1)]
	if roll < 0.75:
		return all_ids[rng.randi_range(0, all_ids.size() - 1)]
	if roll < 0.9:
		return ""
	return "no_such_job"


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261029
	var ids: Array = GameState.jobs.keys()
	ids.sort()
	var card_jobs: Array = []
	for j in ids:
		if (GameState.jobs[j] as JobData).extra_cards_in_hand > 0:
			card_jobs.append(j)
	var asked: Array = ids.duplicate()
	asked.append("no_such_job")
	var union: SkillBranchData = SkillTreeManager.get_branch("union_rules")
	var shipped: Array[float] = union.level_effects.duplicate()
	var out := FileAccess.open("res://jobs_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["jobs_held", "union_level", "union_effect", "run_job",
		"run_job_second", "current_job", "run_job_chosen",
		"all_jobs", "asked", "holds", "available", "has_union", "share", "equipped", "secondary", "terms", "crit_bonus",
		"extra_cards", "pounce_lock", "hat", "job_ids", "neutral", "has_cost", "should_ask",
		"offer", "offered", "held_after", "lock", "locked", "run_after", "current_after",
		"lock_second", "locked_second", "second_after", "take", "taken", "current_after_take"])))
	for i in range(ROWS):
		var jobs := {}
		for j in ids:
			if rng.randf() < 0.4:
				jobs[j] = true
		GameState.jobs_held = jobs
		var held_ids: Array = jobs.keys()
		held_ids.sort()
		var union_level := 1 if rng.randf() < 0.6 else 0
		SkillTreeManager.levels = {"union_rules": union_level} if union_level > 0 else {}
		var effects: Array[float] = shipped.duplicate()
		if rng.randf() < 0.4:
			effects[0] = SHARES[rng.randi_range(0, SHARES.size() - 1)] if rng.randf() < 0.6 else rng.randf_range(-0.5, 1.5)
		union.level_effects = effects
		# The run state as a save could leave it: anything, including a job no longer held or one that does not exist.
		GameState.run_job = pick(rng, held_ids, ids)
		# A card job as the second one, often: the floored share is otherwise met a handful of times per thousand.
		var second_roll := rng.randf()
		if second_roll < 0.15:
			GameState.run_job_second = GameState.run_job
		elif second_roll < 0.35:
			GameState.run_job_second = card_jobs[rng.randi_range(0, card_jobs.size() - 1)]
		else:
			GameState.run_job_second = pick(rng, held_ids, ids)
		GameState.current_job = pick(rng, held_ids, ids)
		GameState.run_job_chosen = rng.randf() < 0.3

		var cols := PackedStringArray([held(jobs), str(union_level), h(effects[0]), GameState.run_job,
			GameState.run_job_second, GameState.current_job, flag(GameState.run_job_chosen)])
		var order := PackedStringArray()
		for data in GameState.all_jobs():
			order.append((data as JobData).id)
		cols.append(";".join(order))
		cols.append(";".join(PackedStringArray(asked)))
		var holds := PackedStringArray()
		var available := PackedStringArray()
		for j in asked:
			holds.append(flag(GameState.holds_job(j)))
			available.append(flag(GameState.job_is_available(j)))
		cols.append(";".join(holds))
		cols.append(";".join(available))
		cols.append(flag(GameState.has_union_rules()))
		cols.append(h(GameState.union_rules_share()))
		var equipped := GameState.equipped_job_data()
		var second := GameState.secondary_job_data()
		cols.append(equipped.id if equipped != null else "")
		cols.append(second.id if second != null else "")
		var terms := PackedStringArray()
		for f in FIELDS:
			terms.append(h(GameState.job_term(f)))
		cols.append(";".join(terms))
		cols.append(h(GameState.get_job_crit_chance_bonus()))
		cols.append(str(GameState.get_job_extra_cards()))
		cols.append(h(GameState.get_job_pounce_lock_sec()))
		cols.append(GameState.get_job_hat_path())
		cols.append(";".join(PackedStringArray(ids)))
		var neutral := PackedStringArray()
		var cost := PackedStringArray()
		for j in ids:
			neutral.append(flag((GameState.jobs[j] as JobData).is_neutral()))
			cost.append(flag((GameState.jobs[j] as JobData).has_a_cost()))
		cols.append(";".join(neutral))
		cols.append(";".join(cost))
		cols.append(flag(JobPicker.should_ask()))

		# The four ways the state changes, in the order a run meets them.
		var offer := pick(rng, [], ids)
		cols.append(offer)
		cols.append(flag(GameState.offer_job(offer)))
		cols.append(held(GameState.jobs_held))
		held_ids = GameState.jobs_held.keys()
		held_ids.sort()
		var lock := pick(rng, held_ids, ids)
		cols.append(lock)
		cols.append(flag(GameState.lock_run_job(lock)))
		cols.append(GameState.run_job)
		cols.append(GameState.current_job)
		var lock_second := GameState.run_job if rng.randf() < 0.15 else pick(rng, held_ids, ids)
		cols.append(lock_second)
		cols.append(flag(GameState.lock_run_job_second(lock_second)))
		cols.append(GameState.run_job_second)
		var take := GameState.current_job if rng.randf() < 0.15 else pick(rng, held_ids, ids)
		cols.append(take)
		cols.append(flag(GameState.take_job(take)))
		cols.append(GameState.current_job)
		out.store_line("\t".join(cols))
	union.level_effects = shipped
	out.close()
	print("JOBS_FIXTURE wrote %d rows" % ROWS)
	get_tree().quit(0)
