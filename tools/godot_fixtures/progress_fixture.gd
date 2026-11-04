extends Node

# Ground truth for PawCore's PcProgress: the REAL GameState mutations (scratch copy of the project, sandboxed APPDATA;
# tools/make_godot_fixtures.py). Each row sets a random state, records it (s0_*), runs a random sequence of operations -
# kills, Catnip and Stardust credited and spent, travel, boss kills, bait, shop purchases (single, bulk, preview), the
# Reawaken, the clocks, a new game - recording each one's result and a digest of the state after it, then records the
# whole state again (s1_*) with the queries that read it (visual stage, caps, visibility, kitten locks).
#
# The ops are written into the row, so the C++ side replays the same sequence. Signals Godot emits are counted here
# (boss_defeated, wave_changed) and the Reawaken's grant is read from `reawakened`, so the C++ results can be compared.

const ROWS := 3000
const OPS := 12


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func pairs(d: Dictionary) -> String:
	var keys := d.keys()
	keys.sort()
	var parts := PackedStringArray()
	for k in keys:
		parts.append("%s=%d" % [k, int(d[k])])
	return ";".join(parts)


# Non-zero entries only: "never set" and "set to 0" are the same level.
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


func flag(b: bool) -> String:
	return "1" if b else "0"


func state() -> PackedStringArray:
	var G := GameState
	return PackedStringArray([pairs_nz(G.upgrade_levels), pairs_nz(SkillTreeManager.levels), pairs(CardManager.owned),
		held(CardManager.banished), str(CardManager.catches_level), h(CardManager.catches_points),
		str(CardManager.extra_choices), held(G.jobs_held), G.current_job, G.run_job, G.run_job_second,
		flag(G.run_job_chosen), h(G.catnip), h(G.stardust), str(G.reawaken_count), str(G.deepest_biome_index),
		held(G.bosses_defeated), flag(G.final_boss_defeated), h(G.stardust_granted_ever), G.current_biome_id,
		str(G.tier_kill_count), pairs(G.tier_kills_by_biome), pairs(G.bait_spent_by_biome),
		held(G.bosses_cleared_this_cycle), str(G.current_wave_index), str(G.kills_this_wave),
		h(G.lifetime_catnip_this_cycle), h(G.lifetime_catnip_ever), str(G.lifetime_kills), str(G.kills_since_tithe),
		str(G.kills_this_cycle), str(G.waves_here), str(G.cards_taken_this_run), h(G.infestation_bonus),
		h(G.pending_second_helping), h(G.play_seconds_this_cycle), h(G.total_play_seconds), h(G.final_boss_seconds),
		h(G._realtime_interest_elapsed)])


const STATE_COLS := ["upgrades", "branches", "cards", "banished", "catches_level", "catches_points", "extra_choices",
	"jobs_held", "current_job", "run_job", "run_job_second", "run_job_chosen", "catnip", "stardust", "reawaken_count",
	"deepest", "bosses", "final", "granted", "biome", "tier_kills", "kills_by_biome", "bait_spent", "cleared",
	"wave_index", "kills_this_wave", "catnip_cycle", "catnip_ever", "lifetime_kills", "kills_since_tithe",
	"kills_this_cycle", "waves_here", "cards_taken", "infestation", "pending_second", "play_cycle", "play_total",
	"final_seconds", "interest_elapsed"]


func digest() -> String:
	var G := GameState
	return ",".join(PackedStringArray([h(G.catnip), h(G.stardust), G.current_biome_id, str(G.tier_kill_count),
		str(G.current_wave_index), str(G.kills_this_wave), str(G.reawaken_count)]))


var boss_signals := 0
var wave_signals := 0
var reawaken_grant := -1.0


func _ready() -> void:
	GameState.boss_defeated.connect(func(_t: String, _s: int) -> void: boss_signals += 1)
	GameState.wave_changed.connect(func(_w: int) -> void: wave_signals += 1)
	GameState.reawakened.connect(func(sd: float, _n: int) -> void: reawaken_grant = sd)
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261029
	var job_ids: Array = GameState.jobs.keys()
	job_ids.sort()
	var ids := []
	for realm in GameState.REALMS:
		ids.append(String(realm["data"]).get_file().get_basename())
	var boss_tiers := []
	for realm_id in ids:
		if GameState.realm_data("res://data/%s.tres" % realm_id).boss_data != null:
			boss_tiers.append(realm_id)
	var upgrade_ids: Array = GameState.upgrades.keys()
	upgrade_ids.sort()
	# The branches these rules read, owned more often than the rest so every path is exercised.
	var focus := ["infestation", "compound", "compound_interest", "second_helping", "legacy", "deep_pockets", "audit",
		"unlock_pounce", "unlock_overkill", "unlock_aura", "unlock_unicorn", "vitality", "fortune"]
	var header := PackedStringArray()
	for c in STATE_COLS:
		header.append("s0_" + c)
	header.append_array(PackedStringArray(["ops", "results", "digests"]))
	for c in STATE_COLS:
		header.append("s1_" + c)
	header.append_array(PackedStringArray(["visual_stage", "evolution_stage", "at_cap", "visible", "kitten_power_locked",
		"kitten_speed_locked", "costs", "bait_unlocked", "bait_affordable"]))
	var out := FileAccess.open("res://progress_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(header))
	for i in range(ROWS):
		# --- the state ---
		var up := {}
		for id in upgrade_ids:
			up[id] = rng.randi_range(0, 30) if rng.randf() < 0.6 else 0
		var br := {}
		for id in SkillTreeManager.branches:
			var data: SkillBranchData = SkillTreeManager.branches[id]
			if rng.randf() < (0.5 if focus.has(id) else 0.25):
				br[id] = rng.randi_range(1, data.max_level())
		var cards := {}
		var banished := {}
		for card in CardManager.deck:
			if rng.randf() < 0.2:
				cards[card.id] = rng.randi_range(1, card.level_values.size())
			elif rng.randf() < 0.05:
				banished[card.id] = true
		GameState.upgrade_levels = up
		SkillTreeManager.levels = br
		CardManager.owned = cards
		CardManager.banished = banished
		CardManager.catches_level = rng.randi_range(0, 30)
		CardManager.catches_points = rng.randf_range(0.0, 500.0)
		CardManager.extra_choices = rng.randi_range(0, 2)
		var jobs := {}
		for j in job_ids:
			if rng.randf() < 0.3:
				jobs[j] = true
		GameState.jobs_held = jobs
		GameState.current_job = "" if rng.randf() < 0.4 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		GameState.run_job = "" if rng.randf() < 0.4 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		GameState.run_job_second = "" if rng.randf() < 0.7 else job_ids[rng.randi_range(0, job_ids.size() - 1)]
		GameState.run_job_chosen = rng.randf() < 0.5
		GameState.catnip = pow(10.0, rng.randf_range(0.0, 12.0)) if rng.randf() < 0.9 else 0.0
		GameState.stardust = floor(pow(10.0, rng.randf_range(0.0, 5.0))) if rng.randf() < 0.8 else 0.0
		GameState.reawaken_count = rng.randi_range(0, 15)
		GameState.deepest_biome_index = rng.randi_range(0, ids.size() - 1)
		var bosses := {}
		for k in range(rng.randi_range(0, 5)):
			bosses[GameState.BOSS_REALMS[k]] = true
		GameState.bosses_defeated = bosses
		GameState.final_boss_defeated = bosses.size() == 5 and rng.randf() < 0.5
		var cleared := {}
		for realm_id in GameState.BOSS_REALMS:
			if bosses.has(realm_id) and rng.randf() < 0.6:
				cleared[realm_id] = true
		GameState.bosses_cleared_this_cycle = cleared
		GameState.stardust_granted_ever = GameState.stardust + (floor(pow(10.0, rng.randf_range(0.0, 6.0))) if rng.randf() < 0.8 else 0.0)
		GameState.current_biome_id = "" if rng.randf() < 0.05 else ids[rng.randi_range(0, ids.size() - 1)]
		GameState.tier_kill_count = rng.randi_range(0, 300000) if rng.randf() < 0.5 else rng.randi_range(0, 300)
		var by_biome := {}
		var spent := {}
		for realm_id in ids:
			if rng.randf() < 0.4:
				by_biome[realm_id] = rng.randi_range(0, 30000) if rng.randf() < 0.5 else rng.randi_range(0, 300)
			if rng.randf() < 0.2:
				spent[realm_id] = rng.randi_range(0, 60)
		GameState.tier_kills_by_biome = by_biome
		GameState.bait_spent_by_biome = spent
		# The cached wave: right half the time, stale otherwise, so the first kill turns it over.
		GameState.current_wave_index = GameState.current_wave() if rng.randf() < 0.5 else rng.randi_range(0, 64)
		GameState.kills_this_wave = rng.randi_range(0, 250)
		GameState.lifetime_catnip_this_cycle = pow(10.0, rng.randf_range(0.0, 15.0))
		GameState.lifetime_catnip_ever = GameState.lifetime_catnip_this_cycle + (pow(10.0, rng.randf_range(0.0, 16.0)) if rng.randf() < 0.8 else 0.0)
		GameState.lifetime_kills = rng.randi_range(0, 2000000)
		GameState.kills_since_tithe = rng.randi_range(0, 120)
		GameState.kills_this_cycle = 10000 * rng.randi_range(1, 3) - rng.randi_range(1, 10) if rng.randf() < 0.3 else rng.randi_range(0, 50000)
		GameState.waves_here = rng.randi_range(0, 40)
		GameState.cards_taken_this_run = rng.randi_range(0, 30)
		GameState.infestation_bonus = rng.randf_range(0.0, 2.0) if rng.randf() < 0.5 else 0.0
		GameState.pending_second_helping = rng.randf_range(0.0, 5000.0) if rng.randf() < 0.4 else 0.0
		GameState.play_seconds_this_cycle = rng.randf_range(0.0, 20000.0)
		GameState.total_play_seconds = GameState.play_seconds_this_cycle + rng.randf_range(0.0, 200000.0)
		GameState.final_boss_seconds = 0.0 if rng.randf() < 0.7 else rng.randf_range(1.0, 100000.0)
		GameState._realtime_interest_elapsed = rng.randf_range(0.0, 70.0)
		GameState.refresh_stats()

		var cols := state()

		# --- the operations ---
		var ops := PackedStringArray()
		var results := PackedStringArray()
		var digests := PackedStringArray()
		for k in range(OPS):
			var roll := rng.randf()
			var op := ""
			var result := ""
			if roll < 0.2:
				var n := 1 if rng.randf() < 0.4 else (rng.randi_range(2, 50) if rng.randf() < 0.6 else rng.randi_range(100, 400))
				wave_signals = 0
				for m in range(n):
					GameState.increment_kill_count()
				op = "kill:%d" % n
				result = "%d,%d" % [wave_signals, GameState.current_wave_index]
			elif roll < 0.3:
				var base := pow(10.0, rng.randf_range(-1.0, 7.0))
				GameState.add_currency(base)
				op = "add:" + h(base)
			elif roll < 0.35:
				var amount := rng.randf_range(-5.0, 1e6)
				GameState.credit_untouched_catnip(amount)
				op = "untouched:" + h(amount)
			elif roll < 0.4:
				var amount := rng.randf_range(-3.0, 50.0)
				GameState.grant_achievement_stardust(amount)
				op = "grant:" + h(amount)
			elif roll < 0.43:
				var amount := rng.randf_range(0.0, 100.0)
				GameState.spend_stardust(amount)
				op = "spend:" + h(amount)
			elif roll < 0.53:
				var to: String = GameState.current_biome_id if rng.randf() < 0.15 else ids[rng.randi_range(0, ids.size() - 1)]
				GameState.begin_biome(to)
				op = "travel:" + to
			elif roll < 0.6:
				var tier: String = GameState.BOSS_REALMS[rng.randi_range(0, 4)] if rng.randf() < 0.8 else ids[rng.randi_range(0, ids.size() - 1)]
				boss_signals = 0
				var is_new := GameState.mark_boss_defeated(tier)
				op = "boss:" + tier
				result = "%s,%d" % [flag(is_new), boss_signals]
			elif roll < 0.68:
				var tier: String = GameState.current_biome_id if (rng.randf() < 0.6 and boss_tiers.has(GameState.current_biome_id)) \
					else boss_tiers[rng.randi_range(0, boss_tiers.size() - 1)]
				var boss: BossData = GameState.realm_data("res://data/%s.tres" % tier).boss_data
				op = "bait:" + tier
				result = flag(GameState.buy_bait(boss))
			elif roll < 0.78:
				var id: String = upgrade_ids[rng.randi_range(0, upgrade_ids.size() - 1)]
				op = "buy:" + id
				result = flag(GameState.purchase_upgrade(id))
			elif roll < 0.84:
				var id: String = upgrade_ids[rng.randi_range(0, upgrade_ids.size() - 1)]
				var count: int = [1, 5, 10, -1, 0][rng.randi_range(0, 4)]
				op = "bulk:%s,%d" % [id, count]
				result = str(GameState.purchase_upgrade_bulk(id, count))
			elif roll < 0.88:
				var id: String = upgrade_ids[rng.randi_range(0, upgrade_ids.size() - 1)]
				var count: int = [1, 5, 10, -1, 0][rng.randi_range(0, 4)]
				var p: Dictionary = GameState.preview_bulk_purchase(id, count)
				op = "preview:%s,%d" % [id, count]
				result = "%d,%s" % [int(p["levels"]), h(p["cost"])]
			elif roll < 0.92:
				var force := rng.randf() < 0.3
				reawaken_grant = -1.0
				var done := GameState.reawaken(force)
				op = "reawaken:" + flag(force)
				result = "%s,%s" % [flag(done), h(reawaken_grant if done else 0.0)]
			elif roll < 0.94:
				var d := rng.randf_range(0.0, 2.0)
				GameState._process(d)
				op = "play:" + h(d)
			elif roll < 0.97:
				var d := rng.randf_range(0.0, 40.0)
				GameState.tick_realtime_interest(d)
				op = "interest:" + h(d)
			elif roll < 0.995:
				var d := rng.randf_range(0.0, 3.0)
				GameState.tick_infestation(d)
				op = "infest:" + h(d)
			else:
				GameState.debug_reset_progress()
				op = "newgame:"
			ops.append(op)
			results.append(result)
			digests.append(digest())
		cols.append("|".join(ops))
		cols.append("|".join(results))
		cols.append("|".join(digests))
		cols.append_array(state())

		# --- the reads of the state the ops left ---
		cols.append(str(GameState.get_visual_stage()))
		cols.append(str(GameState.get_evolution_stage()))
		var at_cap := PackedStringArray()
		var visible := PackedStringArray()
		var costs := PackedStringArray()
		for id in upgrade_ids:
			at_cap.append(flag(GameState.is_upgrade_at_cap(id)))
			visible.append(flag(GameState.is_upgrade_visible(id)))
			costs.append(h(GameState.get_upgrade_cost(id)))
		cols.append(";".join(at_cap))
		cols.append(";".join(visible))
		cols.append(flag(GameState.is_kitten_power_locked()))
		cols.append(flag(GameState.is_kitten_speed_locked()))
		cols.append(";".join(costs))
		var bait_unlocked := PackedStringArray()
		var bait_affordable := PackedStringArray()
		for tier in boss_tiers:
			var boss: BossData = GameState.realm_data("res://data/%s.tres" % tier).boss_data
			bait_unlocked.append(flag(GameState.is_bait_unlocked(boss)))
			bait_affordable.append(flag(GameState.can_afford_bait(boss)))
		cols.append(";".join(bait_unlocked))
		cols.append(";".join(bait_affordable))
		out.store_line("\t".join(cols))
	out.close()
	print("PROGRESS_FIXTURE wrote %d rows" % ROWS)
	get_tree().quit(0)
