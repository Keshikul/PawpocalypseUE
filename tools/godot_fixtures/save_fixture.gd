extends Node

# Ground truth for PawCore's PcSave: the REAL SaveManager.save_game() and load_game() (scratch copy of the project, sandboxed
# APPDATA, so user:// is a throwaway folder; tools/make_godot_fixtures.py). Each row:
#   1. a fresh game (what the title's New Game does), then a random live state A (s0_*: every GameState / CardManager
#      field the progress fixture records, the achievements, the per-slot toggles, the tutorial);
#   2. save_game(0): the written save_slot_1.tres read back field by field (sv_*), and the live state after the save (s1_*:
#      the save banks the live biome's kills into the per-biome map);
#   3. a fresh game and a second random live state B (b_*), a random save S written to save_slot_2.tres (ld_*: sometimes an
#      old version, old skill-tree ids, an unknown upgrade id, a job not held, no run job, no unicorn_split), then
#      load_game(1): the live state after it (a_*).
# The loop never awaits, so no timer (the 30 s autosave) runs inside it.

const ROWS := 3000
const TUTORIAL_STEPS := ["card", "tree", "boss", "reawaken", "bait", "travel", "kitten", "buy", "claim", "pounce", "hover"]
const OLD_BRANCHES := ["aura_power", "pounce", "overkill", "unicorn", "reflex", "ferocity", "mystique", "fortune_yield",
	"fortune_golden", "legacy_hoard", "legacy_memory", "legacy_territory", "not_a_branch"]

var rng := RandomNumberGenerator.new()


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func pairs(d: Dictionary) -> String:
	var keys := d.keys()
	keys.sort()
	var parts := PackedStringArray()
	for k in keys:
		parts.append("%s=%d" % [k, int(d[k])])
	return ";".join(parts)


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


const STATE_COLS := ["upgrades", "branches", "cards", "banished", "catches_level", "catches_points", "extra_choices",
	"jobs_held", "current_job", "run_job", "run_job_second", "run_job_chosen", "catnip", "stardust", "reawaken_count",
	"deepest", "bosses", "final", "granted", "biome", "tier_kills", "kills_by_biome", "bait_spent", "cleared",
	"wave_index", "kills_this_wave", "catnip_cycle", "catnip_ever", "lifetime_kills", "kills_since_tithe",
	"kills_this_cycle", "waves_here", "cards_taken", "infestation", "pending_second", "play_cycle", "play_total",
	"final_seconds", "interest_elapsed", "earned", "claimed", "auto_travel", "auto_pick", "tutorial", "rerolls", "banishes",
	"fixation", "hand", "promised", "prototype"]


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
		h(G._realtime_interest_elapsed), held(AchievementManager.earned), held(AchievementManager.claimed),
		flag(SettingsManager.auto_travel), flag(SettingsManager.auto_pick_card), held(G.tutorial_done),
		str(CardManager.rerolls), str(CardManager.banishes), str(CardManager.fixation_hands),
		";".join(CardManager.run_save_ids()["hand"]), ";".join(CardManager.run_save_ids()["promised"]),
		CardManager.run_save_ids()["prototype"]])


const SAVE_COLS := ["version", "catnip", "stardust", "reawaken_count", "upgrades", "branches", "final", "catnip_ever",
	"catnip_cycle", "granted", "bosses", "lifetime_kills", "earned", "claimed", "stamped", "play_cycle", "play_total",
	"final_seconds", "auto_travel", "auto_pick", "kills_by_biome", "bait_spent", "cleared", "jobs_held", "current_job",
	"run_job", "run_job_chosen", "run_job_second", "unicorn_split", "tutorial", "run_cards_saved", "cards", "banished", "catches_level",
	"catches_points", "rerolls", "extra_choices", "banishes", "fixation", "hand", "promised", "prototype", "cards_taken"]


# The save's levels are printed WHOLE (pairs, zeros kept): which ids a save names decides what a load overwrites.
func save_cols(d: PawpocalypseSaveData) -> PackedStringArray:
	return PackedStringArray([str(d.version), h(d.catnip), h(d.stardust), str(d.reawaken_count), pairs(d.upgrade_levels),
		pairs(d.skill_branch_levels), flag(d.final_boss_defeated), h(d.lifetime_catnip_ever),
		h(d.lifetime_catnip_this_cycle), h(d.stardust_granted_ever), held(d.bosses_defeated), str(d.lifetime_kills),
		held(d.achievements_earned), held(d.achievements_claimed), flag(d.last_played_unix > 0.0),
		h(d.play_seconds_this_cycle), h(d.total_play_seconds), h(d.final_boss_seconds), flag(d.auto_travel),
		flag(d.auto_pick_card), pairs(d.tier_kill_counts), pairs(d.bait_spent), held(d.bosses_cleared_cycle),
		held(d.jobs_held), d.current_job, d.run_job, flag(d.run_job_chosen), d.run_job_second, flag(d.unicorn_split), held(d.tutorial_done),
		flag(d.run_cards_saved), pairs(d.cards_owned), held(d.cards_banished), str(d.catches_level), h(d.catches_points),
		str(d.rerolls), str(d.extra_choices), str(d.banishes), str(d.fixation_hands), ";".join(d.hand),
		";".join(d.promised), d.prototype_card, str(d.cards_taken_this_run)])


func amount() -> float:
	var roll := rng.randf()
	if roll < 0.15:
		return 0.0
	if roll < 0.2:
		return float(rng.randi_range(1, 50))
	if roll < 0.6:
		return rng.randf_range(0.0, 1.0e6)
	if roll < 0.95:
		return pow(10.0, rng.randf_range(0.0, 30.0)) * rng.randf()
	return 1.0e300 * rng.randf()


func seconds() -> float:
	var roll := rng.randf()
	if roll < 0.1:
		return 0.0
	if roll < 0.15:
		return -rng.randf_range(0.0, 100.0)  # a negative clock: apply_save_data floors the cycle's at 0
	return rng.randf_range(0.0, 400000.0)


func subset(ids: Array, p: float) -> Dictionary:
	var out := {}
	for id in ids:
		if rng.randf() < p:
			out[String(id)] = true
	return out


func level_map(ids: Array, p: float, top: int) -> Dictionary:
	var out := {}
	for id in ids:
		if rng.randf() < p:
			out[String(id)] = rng.randi_range(0, top)
	return out


func tier_ids() -> Array:
	var out := []
	for entry in GameState.REALMS:
		out.append(String(entry["data"]).get_file().get_basename())
	return out


func pick(ids: Array) -> String:
	return String(ids[rng.randi_range(0, ids.size() - 1)]) if not ids.is_empty() else ""


func a_job(held_jobs: Dictionary, all_jobs: Array) -> String:
	var roll := rng.randf()
	if roll < 0.3:
		return ""
	if roll < 0.8 and not held_jobs.is_empty():
		return pick(held_jobs.keys())
	return pick(all_jobs)  # possibly not held


func fresh() -> void:
	GameState.debug_reset_progress()
	SkillTreeManager.apply_save_data({})
	AchievementManager.apply_save_data({})
	SettingsManager.set_auto_travel(false)
	SettingsManager.set_auto_pick_card(false)
	GameState.tutorial_done = {}


func randomise_live() -> void:
	var G := GameState
	var tiers := tier_ids()
	var all_jobs: Array = G.jobs.keys()
	all_jobs.sort()
	for id in G.upgrade_levels.keys():
		G.upgrade_levels[id] = rng.randi_range(1, 40) if rng.randf() < 0.4 else 0
	for id in SkillTreeManager.levels.keys():
		var data := SkillTreeManager.get_branch(id)
		SkillTreeManager.levels[id] = rng.randi_range(1, data.max_level()) if rng.randf() < 0.3 else 0
	G.jobs_held = subset(all_jobs, 0.3)
	G.current_job = a_job(G.jobs_held, all_jobs)
	G.run_job = a_job(G.jobs_held, all_jobs)
	G.run_job_second = a_job(G.jobs_held, all_jobs)
	G.run_job_chosen = rng.randf() < 0.5
	G.catnip = amount()
	G.stardust = amount()
	G.reawaken_count = rng.randi_range(0, 20) if rng.randf() < 0.7 else 0
	G.deepest_biome_index = rng.randi_range(0, tiers.size() - 1)
	G.bosses_defeated = subset(G.BOSS_REALMS, 0.5)
	if rng.randf() < 0.05:
		G.bosses_defeated["tier_1"] = true  # not a boss realm: the ledger keeps whatever it is given
	G.final_boss_defeated = rng.randf() < 0.2
	G.stardust_granted_ever = amount()
	G.current_biome_id = pick(tiers) if rng.randf() < 0.8 else ""
	G.tier_kill_count = rng.randi_range(0, 50000)
	G.tier_kills_by_biome = level_map(tiers, 0.3, 90000)
	G.bait_spent_by_biome = level_map(G.BOSS_REALMS, 0.3, 200)
	G.bosses_cleared_this_cycle = subset(G.BOSS_REALMS, 0.4)
	G.current_wave_index = rng.randi_range(0, 64)
	G.kills_this_wave = rng.randi_range(0, 900)
	G.lifetime_catnip_this_cycle = amount()
	G.lifetime_catnip_ever = amount() if rng.randf() < 0.85 else 0.0
	G.lifetime_kills = rng.randi_range(0, 9000000)
	G.kills_since_tithe = rng.randi_range(0, 500)
	G.kills_this_cycle = rng.randi_range(0, 900000)
	G.waves_here = rng.randi_range(0, 64)
	G.cards_taken_this_run = rng.randi_range(0, 20)
	G.infestation_bonus = rng.randf_range(0.0, 3.0)
	G.pending_second_helping = amount()
	G.play_seconds_this_cycle = seconds()
	G.total_play_seconds = seconds()
	G.final_boss_seconds = seconds() if rng.randf() < 0.3 else 0.0
	G._realtime_interest_elapsed = rng.randf_range(0.0, 60.0)
	CardManager.owned = {}
	for card in CardManager.deck:
		if rng.randf() < 0.15:
			CardManager.owned[card.id] = rng.randi_range(1, 4)
	CardManager.banished = {}
	for card in CardManager.deck:
		if rng.randf() < 0.05:
			CardManager.banished[card.id] = true
	CardManager.rerolls = rng.randi_range(0, 5)
	CardManager.banishes = rng.randi_range(0, 3)
	CardManager.fixation_hands = rng.randi_range(0, 2)
	CardManager._hand = random_cards(3) if rng.randf() < 0.4 else ([] as Array[CardData])
	CardManager._promised = random_cards(rng.randi_range(0, 2)) if rng.randf() < 0.3 else ([] as Array[CardData])
	CardManager._prototype_card = CardManager._hand[0] if not CardManager._hand.is_empty() and rng.randf() < 0.5 else null
	CardManager.catches_level = rng.randi_range(0, 20)
	CardManager.catches_points = rng.randf_range(0.0, 50000.0)
	CardManager.extra_choices = rng.randi_range(0, 2)
	var ach_ids := []
	for a in AchievementManager.achievements:
		ach_ids.append(a.id)
	AchievementManager.earned = subset(ach_ids, 0.3)
	if rng.randf() < 0.1:
		AchievementManager.earned["retired_achievement"] = true
	AchievementManager.claimed = {}
	for id in AchievementManager.earned:
		if rng.randf() < 0.5:
			AchievementManager.claimed[id] = true
	SettingsManager.set_auto_travel(rng.randf() < 0.5)
	SettingsManager.set_auto_pick_card(rng.randf() < 0.5)
	G.tutorial_done = subset(TUTORIAL_STEPS, 0.4)


func random_cards(n: int) -> Array[CardData]:
	var out: Array[CardData] = []
	var tries := 0
	while out.size() < n and tries < 50:
		tries += 1
		var card: CardData = CardManager.deck[rng.randi_range(0, CardManager.deck.size() - 1)]
		if not out.has(card):
			out.append(card)
	return out


func ids(cards: Array[CardData]) -> PackedStringArray:
	var out := PackedStringArray()
	for c in cards:
		out.append(c.id)
	return out


func random_save() -> PawpocalypseSaveData:
	var G := GameState
	var tiers := tier_ids()
	var all_jobs: Array = G.jobs.keys()
	all_jobs.sort()
	var d := PawpocalypseSaveData.new()
	var roll := rng.randf()
	d.version = 7 if roll < 0.8 else (rng.randi_range(1, 6) if roll < 0.92 else 8)
	d.catnip = amount()
	d.stardust = amount()
	d.reawaken_count = rng.randi_range(0, 20)
	var ups := {}
	for id in G.upgrade_levels.keys():
		if rng.randf() < 0.7:  # a save that names only some ids: the rest keep the level in memory
			ups[id] = rng.randi_range(0, 40)
	if rng.randf() < 0.2:
		ups["multi_strike"] = rng.randi_range(1, 9)  # a retired line: ignored
	d.upgrade_levels = ups
	var br := {}
	for id in SkillTreeManager.levels.keys():
		if rng.randf() < 0.25:
			br[id] = rng.randi_range(0, SkillTreeManager.get_branch(id).max_level())
	if rng.randf() < 0.3:
		for old in OLD_BRANCHES:
			if rng.randf() < 0.4:
				br[old] = rng.randi_range(0, 14)  # past a ladder's top sometimes: the migration clamps
	if rng.randf() < 0.3:
		br["unlock_unicorn"] = 1
		if rng.randf() < 0.5:
			br["unlock_poop"] = 0
	d.skill_branch_levels = br
	d.final_boss_defeated = rng.randf() < 0.2
	d.lifetime_catnip_ever = amount()
	d.lifetime_catnip_this_cycle = amount()
	d.stardust_granted_ever = amount()
	d.bosses_defeated = subset(G.BOSS_REALMS, 0.5)
	d.lifetime_kills = rng.randi_range(0, 9000000)
	var ach_ids := []
	for a in AchievementManager.achievements:
		ach_ids.append(a.id)
	d.achievements_earned = subset(ach_ids, 0.3)
	d.achievements_claimed = subset(d.achievements_earned.keys(), 0.5)
	d.last_played_unix = rng.randf_range(1.6e9, 1.9e9)
	d.play_seconds_this_cycle = seconds()
	d.total_play_seconds = seconds()
	d.final_boss_seconds = seconds() if rng.randf() < 0.3 else 0.0
	d.auto_travel = rng.randf() < 0.5
	d.auto_pick_card = rng.randf() < 0.5
	d.tier_kill_counts = level_map(tiers, 0.3, 90000)
	d.bait_spent = level_map(G.BOSS_REALMS, 0.3, 200)
	d.bosses_cleared_cycle = subset(G.BOSS_REALMS, 0.4)
	d.jobs_held = subset(all_jobs, 0.3)
	d.current_job = a_job(d.jobs_held, all_jobs)
	d.run_job = a_job(d.jobs_held, all_jobs) if rng.randf() < 0.7 else ""
	d.run_job_chosen = rng.randf() < 0.5
	d.run_job_second = a_job(d.jobs_held, all_jobs) if rng.randf() < 0.5 else ""
	d.unicorn_split = rng.randf() < 0.6
	d.tutorial_done = subset(TUTORIAL_STEPS, 0.4)
	# The run's cards (F24): sometimes a save from before them, a card since removed, a mark off the hand.
	d.run_cards_saved = rng.randf() < 0.8
	var owned := {}
	for card in CardManager.deck:
		if rng.randf() < 0.15:
			owned[card.id] = rng.randi_range(1, 4)
	if rng.randf() < 0.1:
		owned["retired_card"] = 2
	d.cards_owned = owned
	var ban := {}
	for card in CardManager.deck:
		if rng.randf() < 0.05:
			ban[card.id] = true
	d.cards_banished = ban
	d.catches_level = rng.randi_range(0, 20)
	d.catches_points = rng.randf_range(0.0, 50000.0) / 3.0
	d.rerolls = rng.randi_range(0, 5)
	d.extra_choices = rng.randi_range(0, 2)
	d.banishes = rng.randi_range(0, 3)
	d.fixation_hands = rng.randi_range(0, 2)
	var hand := ids(random_cards(3)) if rng.randf() < 0.4 else PackedStringArray()
	if not hand.is_empty() and rng.randf() < 0.1:
		hand.append("retired_card")
	d.hand = hand
	d.promised = ids(random_cards(rng.randi_range(0, 2))) if rng.randf() < 0.3 else PackedStringArray()
	var roll2 := rng.randf()
	d.prototype_card = hand[0] if not hand.is_empty() and roll2 < 0.5 else (CardManager.deck[0].id if roll2 < 0.6 else "")
	d.cards_taken_this_run = rng.randi_range(0, 20)
	return d


func _ready() -> void:
	rng.seed = 20261103
	var header := []
	for c in STATE_COLS:
		header.append("s0_" + c)
	for c in SAVE_COLS:
		header.append("sv_" + c)
	for c in STATE_COLS:
		header.append("s1_" + c)
	for c in STATE_COLS:
		header.append("b_" + c)
	for c in SAVE_COLS:
		header.append("ld_" + c)
	for c in STATE_COLS:
		header.append("a_" + c)
	header.append("reoffer1")
	header.append("reoffer2")
	var out := FileAccess.open("res://save_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(header)))
	var offered := ["-"]
	CardManager.hand_offered.connect(func(hand: Array[CardData], _level: int) -> void: offered[0] = ";".join(ids(hand)))
	var old_versions := 0
	var migrated := 0
	for i in range(ROWS):
		var cols := PackedStringArray()
		# 1-2: save.
		fresh()
		randomise_live()
		cols.append_array(state())
		SaveManager.save_game(0)
		var written := ResourceLoader.load(SaveManager.slot_path(0), "PawpocalypseSaveData",
			ResourceLoader.CACHE_MODE_IGNORE) as PawpocalypseSaveData
		if written == null:
			push_error("SAVE_FIXTURE: save_game wrote nothing readable")
			get_tree().quit(1)
			return
		cols.append_array(save_cols(written))
		written = null
		cols.append_array(state())
		# 3: load.
		fresh()
		randomise_live()
		cols.append_array(state())
		var d := random_save()
		if d.version < 7:
			old_versions += 1
		for old in OLD_BRANCHES:
			if d.skill_branch_levels.has(old):
				migrated += 1
				break
		if ResourceSaver.save(d, SaveManager.slot_path(1)) != OK:
			push_error("SAVE_FIXTURE: could not write the save to load")
			get_tree().quit(1)
			return
		d = null
		# What the load will SEE: the file read back. Godot's text resources do not round-trip every double (16 significant
		# digits; a double that is exactly a float32 is written with float32's digits), so the save as built in memory is
		# not what load_game() reads.
		var seen := ResourceLoader.load(SaveManager.slot_path(1), "PawpocalypseSaveData",
			ResourceLoader.CACHE_MODE_IGNORE) as PawpocalypseSaveData
		cols.append_array(save_cols(seen))
		seen = null
		SaveManager.load_game(1)
		cols.append_array(state())
		# The realm's start offers a restored hand again, once (TierController._ready -> reoffer_after_load).
		for k in range(2):
			offered[0] = "-"
			CardManager.reoffer_after_load()
			cols.append(offered[0])
		out.store_line("\t".join(cols))
	out.close()
	print("SAVE_FIXTURE rows=%d old_versions=%d with_old_branches=%d" % [ROWS, old_versions, migrated])
	get_tree().quit(0)
