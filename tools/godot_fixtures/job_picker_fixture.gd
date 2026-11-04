extends Node

# Ground truth for PawCore's PcHud JobPicker / PickJob: the REAL JobPicker (job_picker.gd) over random states (scratch copy
# of the project, sandboxed APPDATA; tools/make_godot_fixtures.py). Each row opens the picker (the first ask, or the second
# under Union Rules), reads what it offers (its buttons, in order, with the "(last run)" mark, the row the selection opens
# on, the second ask's "At n% strength."), presses one offered row through _pick(), and records the run state after; when
# the press asks again for a second job, the reopened list is read and pressed too.
#
# A row's job id is read back from its button text (the English display names are unique; the fixture refuses otherwise).
# jobs_held sometimes names a job that does not exist: Godot offers only jobs with data.

const ROWS := 3000
const SHARES := [0.0, 0.25, 0.5, 0.7, 1.0, 1.3, -0.2, 0.125, 0.335, 0.005]
const LAST_RUN := "   (last run)"


func flag(b: bool) -> String:
	return "1" if b else "0"


func held(d: Dictionary) -> String:
	var keys := []
	for k in d:
		if d[k]:
			keys.append(String(k))
	keys.sort()
	return ";".join(PackedStringArray(keys))


func pick(rng: RandomNumberGenerator, held_ids: Array, all_ids: Array) -> String:
	var roll := rng.randf()
	if roll < 0.45 and not held_ids.is_empty():
		return held_ids[rng.randi_range(0, held_ids.size() - 1)]
	if roll < 0.7:
		return all_ids[rng.randi_range(0, all_ids.size() - 1)]
	if roll < 0.9:
		return ""
	return "no_such_job"


var _by_name := {}


# What the open picker offers: [ids with ":1" for the last-run mark, focus index (-1 = none: the first row), the first
# blurb's leading sentence when asking for the second job ("" otherwise)].
func read(picker: JobPicker) -> Array:
	var ids := PackedStringArray()
	var focus := -1
	var i := 0
	for btn in picker._rows:
		var text: String = btn.text
		var last := text.ends_with(LAST_RUN)
		if last:
			text = text.substr(0, text.length() - LAST_RUN.length())
		if not _by_name.has(text):
			push_error("JOB_PICKER_FIXTURE: unknown row label " + text)
			get_tree().quit(1)
		ids.append(String(_by_name[text]) + (":1" if last else ":0"))
		if btn == picker._default_row:
			focus = i
		i += 1
	var lead := ""
	if picker._asking_for_second:
		for child in picker._list.get_children():
			if child is Label and not child.is_queued_for_deletion():
				lead = (child as Label).text.split(".")[0]
				break
	return [";".join(ids), str(focus), lead]


func row_ids(offered: String) -> Array:
	var out := []
	for cell in offered.split(";"):
		out.append(cell.split(":")[0])
	return out


func _ready() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261102
	var ids: Array = GameState.jobs.keys()
	ids.sort()
	for j in ids:
		var name := tr((GameState.jobs[j] as JobData).display_name, "job")
		if _by_name.has(name):
			push_error("JOB_PICKER_FIXTURE: two jobs named " + name)
			get_tree().quit(1)
		_by_name[name] = j
	_by_name[tr("No job")] = ""
	_by_name[tr("No second job")] = ""
	var union: SkillBranchData = SkillTreeManager.get_branch("union_rules")
	var shipped: Array[float] = union.level_effects.duplicate()
	var picker := JobPicker.new()
	add_child(picker)

	var out := FileAccess.open("res://job_picker_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["jobs_held", "union_level", "union_effect", "run_job",
		"run_job_second", "current_job", "second", "should_ask",
		"offered", "focus", "lead", "press", "asks_again", "run_after", "second_after", "current_after", "chosen_after",
		"offered2", "focus2", "lead2", "press2", "run_final", "second_final", "current_final", "chosen_final"])))
	for i in range(ROWS):
		var jobs := {}
		for j in ids:
			if rng.randf() < 0.4:
				jobs[j] = true
		if rng.randf() < 0.1:
			jobs["no_such_job"] = true
		GameState.jobs_held = jobs
		var held_ids: Array = jobs.keys()
		held_ids.sort()
		var union_level := 1 if rng.randf() < 0.6 else 0
		SkillTreeManager.levels = {"union_rules": union_level} if union_level > 0 else {}
		var effects: Array[float] = shipped.duplicate()
		if rng.randf() < 0.5:
			effects[0] = SHARES[rng.randi_range(0, SHARES.size() - 1)] if rng.randf() < 0.7 else rng.randf_range(-0.2, 1.2)
		union.level_effects = effects
		GameState.run_job = pick(rng, held_ids, ids)
		GameState.run_job_second = pick(rng, held_ids, ids)
		GameState.current_job = pick(rng, held_ids, ids)
		GameState.run_job_chosen = false
		var second := rng.randf() < 0.3
		var cols := PackedStringArray([held(jobs), str(union_level), var_to_bytes(float(effects[0])).hex_encode(),
			GameState.run_job, GameState.run_job_second, GameState.current_job, flag(second), flag(JobPicker.should_ask())])

		picker._asking_for_second = second
		picker.open()
		var seen := read(picker)
		cols.append_array(PackedStringArray(seen))
		var offered := row_ids(seen[0])
		var press: String = offered[rng.randi_range(0, offered.size() - 1)]
		cols.append(press)
		picker._pick(press)
		var again: bool = picker._asking_for_second and picker.visible
		cols.append(flag(again))
		cols.append_array(PackedStringArray([GameState.run_job, GameState.run_job_second, GameState.current_job,
			flag(GameState.run_job_chosen)]))
		if again:
			var seen2 := read(picker)
			cols.append_array(PackedStringArray(seen2))
			var offered2 := row_ids(seen2[0])
			var press2: String = offered2[rng.randi_range(0, offered2.size() - 1)]
			cols.append(press2)
			picker._pick(press2)
		else:
			cols.append_array(PackedStringArray(["-", "-", "-", "-"]))
		cols.append_array(PackedStringArray([GameState.run_job, GameState.run_job_second, GameState.current_job,
			flag(GameState.run_job_chosen)]))
		# The picker is reused across Reawakens: leave it closed and asking for the first job, as _close() and _pick() do.
		picker._asking_for_second = false
		picker.visible = false
		out.store_line("\t".join(cols))
	union.level_effects = shipped
	get_tree().paused = false
	out.close()
	print("JOB_PICKER_FIXTURE wrote %d rows" % ROWS)
	get_tree().quit(0)
