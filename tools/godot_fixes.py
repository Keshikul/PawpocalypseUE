"""The port's fixes to Godot's code (D16, designer 2026-11-10: "This is a standalone version. If godot has rules or mechanics that
don't make sense we should change them. Do not port faults from godot.").

The port fixes a Godot fault in its own code, never in the Godot repo. Its tests still prove every rule against the REAL Godot
code: tools/make_godot_fixtures.py runs Godot on a scratch copy of the project, and applies these fixes to that copy (never to
the repo) before it runs. So a fixture shows Godot's behaviour with the port's fixes, row for row, and every other row is
still Godot's own. Each fix names its finding (docs/01 §5), the file, and an exact text replaced: it must be found exactly once
and must change something, or nothing runs (a fix whose target Godot rewrote fails here, not silently).
A fix may change DATA too (a .tres is text; F3, 2026-11-12, the first): the tools that read the Godot repo itself apply the
same list in memory (fixed_text / fixed_read): tools/export_godot_data.py, so Data/*.csv carries the port's numbers and L0's
data-export gate still compares Data/ with a fresh export; tools/pawsim.py's .tres loader, so the pawsim parity sweep
compares like with like.

    py tools/godot_fixes.py --list         # the fixes, and whether each applies to the Godot repo as it is today
    py tools/godot_fixes.py --self-test    # a planted stale target and a no-op are refused
"""
import sys
from pathlib import Path

GODOT = Path("C:/Games/pawpocalypse")

# (finding, file under the Godot project, the text as Godot has it, the text with the port's fix)
FIXES = [
    ("F34", "scripts/systems/pause_menu.gd",
     "func _show_main() -> void:\n\tmain_page.visible = true\n",
     "func _show_main() -> void:\n\t_pending_overwrite = -1  # PORT FIX F34: leaving the Save page disarms its overwrite\n"
     "\tmain_page.visible = true\n"),
    # F37: with Button prompts pinned (keyboard), a pad of another family changes the pad detected but not the family, so no
    # signal the Controls page hears: its controller column kept the old pad's buttons. A signal for the pad detected, and
    # the page redraws on it.
    ("F37", "scripts/autoload/input_device.gd",
     "signal bindings_changed\n",
     "signal bindings_changed\n## PORT FIX F37: the pad detected changed (the Controls page's controller column follows it).\n"
     "signal detected_family_changed(family: int)\n"),
    ("F37", "scripts/autoload/input_device.gd",
     "func _set_detected(value: int) -> void:\n\tdetected_family = value\n\t_refresh_family()\n",
     "func _set_detected(value: int) -> void:\n\tvar was := detected_family\n\tdetected_family = value\n\t_refresh_family()\n"
     "\tif value != was:\n\t\tdetected_family_changed.emit(value)  # PORT FIX F37\n"),
    ("F37", "scripts/systems/controls_panel.gd",
     "\tInputDevice.scheme_changed.connect(func(_f: int) -> void: sync())\n",
     "\tInputDevice.scheme_changed.connect(func(_f: int) -> void: sync())\n"
     "\tInputDevice.detected_family_changed.connect(func(_f: int) -> void: sync())  # PORT FIX F37: a new pad redraws\n"),
    # F49: saved card levels were taken as they were, where shop and tree levels are range-checked on load (F32): a level
    # under 1 is no card, one past the card's levels its top level.
    ("F49", "scripts/autoload/card_manager.gd",
     "\tfor id in data.cards_owned:\n\t\tif card_by_id(String(id)) != null:\n\t\t\towned[String(id)] = int(data.cards_owned[id])\n",
     "\tfor id in data.cards_owned:\n\t\tvar card := card_by_id(String(id))\n\t\tvar lvl := int(data.cards_owned[id])\n"
     "\t\tif card != null and lvl >= 1:  # PORT FIX F49: range-checked like the shop and the tree (F32)\n"
     "\t\t\towned[String(id)] = mini(lvl, card.max_level())\n"),
    # F47 (review CR2): a slot holding a file this build cannot read showed "Empty", and pressing it played a fresh game over
    # it without a word. Its card says it cannot be read (pressing it still starts a new game there; the port keeps the old
    # file aside as slot_N_corrupt). The title fixture's unreadable slot has no backup, so "cannot be read" is the port's
    # rule exactly (a broken slot whose backup reads shows the backup's game in the port; the fixture never writes one).
    ("F47", "scripts/systems/main_menu.gd",
     "\t\tif summary.is_empty():\n\t\t\tinfo.text = tr(\"Empty\\n\\nStart a new game here.\")\n",
     "\t\tif summary.is_empty():\n"
     "\t\t\t# PORT FIX F47: a slot whose file cannot be read says so\n"
     "\t\t\tinfo.text = tr(\"Cannot be read\\n\\nStart a new game here.\") if SaveManager.has_slot(i) "
     "else tr(\"Empty\\n\\nStart a new game here.\")\n"),
    # F3 (designer, 2026-11-12): Patience paid crit damage for the cat's FINAL attack speed, so with the Sphinx every Attack
    # Speed level bought shrank it, and from Patience 5 with the aura every level lowered DPS (the shop sold a trap). It
    # reads the job's slowness only (the Sphinx's x0.5), and pays +6% a level (+60% at 10; the aura build's best stays
    # where it was). The first D16 change to a NUMBER: a .tres is text, so the same exact replacement carries it, and
    # every reader of Godot's data applies it (fixed_text): the exporter (Data/*.csv), pawsim's .tres loader, the fixtures.
    ("F3", "scripts/autoload/game_state.gd",
     "\tstats.crit_multiplier *= 1.0 + get_patience_crit_bonus(stats.attack_speed_multiplier)\n",
     "\tstats.crit_multiplier *= 1.0 + get_patience_crit_bonus(get_job_attack_speed_multiplier())"
     "  # PORT FIX F3: the job's slowness only\n"),
    ("F3", "data/skill_tree/patience.tres",
     "level_effects = Array[float]([0.2, 0.4, 0.6, 0.8, 1, 1.2, 1.4, 1.6, 1.8, 2])\n",
     "level_effects = Array[float]([0.06, 0.12, 0.18, 0.24, 0.3, 0.36, 0.42, 0.48, 0.54, 0.6])\n"),
    # D17, the economy (designer, 2026-11-12; docs/ECONOMY-REVIEW-2026-11-12.md): "the player should never be left for long
    # periods with nothing to buy. Every awakening should make the player feel stronger and unlocks should feel meaningful."
    # E1: a beaten boss stays beaten. Passage re-locked on every Reawaken and each rematch cost 40 Bait paid in kills, which no
    # Reawaken speeds up: 52 % of the game was the re-climb. A save from before keeps its beaten bosses open on load.
    # E2: Bait earned toward a boss survives a Reawaken: its own bank, counted on every kill (the wave bank still resets).
    ("E1", "scripts/autoload/game_state.gd",
     "\ttier_kills_by_biome.clear()\n\tbait_spent_by_biome.clear()\n\tbosses_cleared_this_cycle.clear()\n"
     "\treawaken_count += 1\n",
     "\ttier_kills_by_biome.clear()\n"
     "\t# PORT FIX E1/E2 (D17): a beaten boss stays beaten, and Bait earned and spent is kept\n"
     "\treawaken_count += 1\n"),
    ("E2", "scripts/autoload/game_state.gd",
     "var bait_spent_by_biome: Dictionary = {}\n",
     "var bait_spent_by_biome: Dictionary = {}\n"
     "## PORT FIX E2 (D17): kills toward each biome's Bait, kept through a Reawaken (tier_kills_by_biome carries the waves).\n"
     "var bait_kills_by_biome: Dictionary = {}\n"),
    ("E2", "scripts/autoload/game_state.gd",
     "func increment_kill_count() -> void:\n\ttier_kill_count += 1\n",
     "func increment_kill_count() -> void:\n\ttier_kill_count += 1\n"
     "\tif not current_biome_id.is_empty():  # PORT FIX E2\n"
     "\t\tbait_kills_by_biome[current_biome_id] = int(bait_kills_by_biome.get(current_biome_id, 0)) + 1\n"),
    ("E2", "scripts/autoload/game_state.gd",
     "\tvar kills: int = tier_kill_count if biome_id == current_biome_id \\\n"
     "\t\telse int(tier_kills_by_biome.get(biome_id, 0))\n\treturn kills / per\n",
     "\tvar kills: int = int(bait_kills_by_biome.get(biome_id, 0))  # PORT FIX E2: the Bait bank\n\treturn kills / per\n"),
    ("E2", "scripts/autoload/game_state.gd",
     "\ttier_kills_by_biome.clear()\n\tbait_spent_by_biome.clear()\n\tbosses_cleared_this_cycle.clear()\n"
     "\tcurrent_biome_id = \"\"\n",
     "\ttier_kills_by_biome.clear()\n\tbait_spent_by_biome.clear()\n\tbosses_cleared_this_cycle.clear()\n"
     "\tbait_kills_by_biome.clear()  # PORT FIX E2\n\tcurrent_biome_id = \"\"\n"),
    ("E2", "scripts/data/pawpocalypse_save_data.gd",
     "@export var bait_spent: Dictionary = {}\n",
     "@export var bait_spent: Dictionary = {}\n"
     "## PORT FIX E2 (D17): kills toward each biome's Bait, kept through a Reawaken.\n"
     "@export var bait_kills: Dictionary = {}\n"),
    ("E2", "scripts/autoload/save_manager.gd",
     "\tdata.bait_spent = GameState.bait_spent_by_biome.duplicate()\n",
     "\tdata.bait_spent = GameState.bait_spent_by_biome.duplicate()\n"
     "\tdata.bait_kills = GameState.bait_kills_by_biome.duplicate()  # PORT FIX E2\n"),
    ("E1", "scripts/autoload/save_manager.gd",
     "\tGameState.bosses_cleared_this_cycle = data.bosses_cleared_cycle.duplicate()\n",
     "\tGameState.bosses_cleared_this_cycle = data.bosses_cleared_cycle.duplicate()\n"
     "\t# PORT FIX E1/E2 (D17): a boss beaten in an older save stays beaten; an older save's Bait is its kills this run\n"
     "\tfor realm_id in GameState.bosses_defeated:\n"
     "\t\tif GameState.bosses_defeated[realm_id]:\n"
     "\t\t\tGameState.bosses_cleared_this_cycle[realm_id] = true\n"
     "\tGameState.bait_kills_by_biome = data.bait_kills.duplicate() if not data.bait_kills.is_empty() \\\n"
     "\t\telse data.tier_kill_counts.duplicate()\n"),
    # E3: the Reawaken multiplies Catnip from kills too, by the damage multiplier to reawaken_catnip_exponent (0.5: x1.37 a
    # Reawaken). It multiplied damage only, and the run is paced by Catnip: runs 3-6 started exactly as fast as the one before.
    ("E3", "scripts/data/reawaken_constants_data.gd",
     "@export var reawaken_mult_exponent: float = 0.5\n",
     "@export var reawaken_mult_exponent: float = 0.5\n"
     "## PORT FIX E3 (D17): Catnip from kills is multiplied by the Reawaken multiplier to this power (0 = off).\n"
     "@export var reawaken_catnip_exponent: float = 0.0\n"),
    ("E3", "data/reawaken_constants.tres",
     "reawaken_stardust_coefficient = 0.125\n",
     "reawaken_stardust_coefficient = 0.125\nreawaken_catnip_exponent = 0.5\n"),
    ("E3", "scripts/autoload/game_state.gd",
     "\t_credit_currency(whole_catnip(catnip_amount\n\t\t* get_currency_find_multiplier()\n",
     "\t_credit_currency(whole_catnip(catnip_amount\n\t\t* get_reawaken_catnip_multiplier()  # PORT FIX E3\n"
     "\t\t* get_currency_find_multiplier()\n"),
    ("E3", "scripts/autoload/game_state.gd",
     "func get_reawaken_multiplier() -> float:\n\treturn _multiplier_for(stardust_granted_ever)\n",
     "func get_reawaken_multiplier() -> float:\n\treturn _multiplier_for(stardust_granted_ever)\n\n\n"
     "## PORT FIX E3 (D17): what the Reawaken multiplies Catnip from kills by.\n"
     "func get_reawaken_catnip_multiplier() -> float:\n"
     "\treturn pow(get_reawaken_multiplier(), reawaken_constants.reawaken_catnip_exponent)\n"),
    # E4: bigger kitten buys (the alternative measured 2026-10-01): 595 of 1,222 buys were kittens at 0.2 % / 1.8 %.
    ("E4", "data/upgrade_kitten_power.tres",
     "cost_growth = 2.04\neffect_per_level = 1.3\n",
     "cost_growth = 3.6372\neffect_per_level = 1.6\n"),
    ("E4", "data/upgrade_kitten_count.tres",
     "cost_growth = 1.2\neffect_per_level = 1.0\n",
     "cost_growth = 1.35\neffect_per_level = 2.0\n"),
    # E5: the Overkill unlock's splash 2 % -> 10 % (it paid +3 % income; a number too small to notice).
    ("E5", "data/skill_tree/unlock_overkill.tres",
     "level_effects = Array[float]([0.02])\n",
     "level_effects = Array[float]([0.1])\n"),
    # EU: the Unicorn is a damage step (designer, 2026-11-12: x1.5): owning it multiplies paw and aura damage (the
    # multiplicative bracket); it gave +13 % with Rainbow Poop for 2,500 Stardust and a ceremony.
    ("EU", "data/skill_tree/unlock_unicorn.tres",
     "effect_type = 4\nlevel_costs = Array[float]([2500])\nlevel_effects = Array[float]([1])\n",
     "effect_type = 3\nlevel_costs = Array[float]([2500])\nlevel_effects = Array[float]([1.5])\n"),
    ("EU", "scripts/autoload/game_state.gd",
     "\t\tstats.multiplicative_damage_product *= SkillTreeManager.get_effect(\"claws\", \"mult\")\n",
     "\t\tstats.multiplicative_damage_product *= SkillTreeManager.get_effect(\"claws\", \"mult\")\n"
     "\tif is_unicorn_owned():  # PORT FIX EU (D17): the Unicorn is a damage step\n"
     "\t\tstats.multiplicative_damage_product *= SkillTreeManager.get_effect(\"unlock_unicorn\")\n"),
    # E6: boss HP re-priced for E1-E3's damage curve, so the FIRST fight is a fight at the moment its Bait is ready (a
    # player engages then): 89 / 148 / 171 / 171 / 169 s in tools/econ_review.py (time limits 300-3,700 s). Without it every
    # boss died in 2-74 s. Fitted by fixed point on the first-Bait-ready fight length (doc 02 §4: HP no longer sets when a
    # gate opens; the Bait does).
    ("E6", "data/boss_park_tyrant.tres", "total_hp = 2.93798e+06\n", "total_hp = 6.96301e+06\n"),  # x2.37: 89 s
    ("E6", "data/boss_rival_champion.tres", "total_hp = 3.64195e+09\n", "total_hp = 7.42958e+09\n"),  # x2.04: 148 s
    ("E6", "data/boss_void_god.tres", "total_hp = 3.49125e+10\n", "total_hp = 5.51618e+11\n"),  # x15.8: 171 s
    ("E6", "data/boss_factory_foreman.tres", "total_hp = 2.07098e+12\n", "total_hp = 6.42004e+13\n"),  # x31: 171 s
    ("E6", "data/boss_litter_king.tres", "total_hp = 8.0768e+12\n", "total_hp = 1.12268e+15\n"),  # x139: 169 s
]


class FixError(Exception):
    pass


def _in_file_endings(text, s):
    return s.replace("\n", "\r\n") if "\r\n" in text else s


def apply_to(text, old, new, where):
    """The text with one fix applied; FixError when its target is not there exactly once, or the fix changes nothing."""
    old = _in_file_endings(text, old)
    new = _in_file_endings(text, new)
    n = text.count(old)
    if n != 1:
        raise FixError("%s: the target is found %d times (Godot rewrote it? update the fix)" % (where, n))
    if old == new:
        raise FixError("%s: the fix changes nothing" % where)
    return text.replace(old, new)


def apply_all(project, repo=GODOT):
    """Applies every fix to a scratch copy of the Godot project (never the repo itself). A copy reused from an earlier run
    already has a fix when its new text is there once: it is left as it is. Returns the findings applied now."""
    project = Path(project).resolve()
    if project == Path(repo).resolve():
        raise FixError("refusing to write the Godot repo itself: the fixes go into a scratch copy")
    done = []
    for finding, rel, old, new in FIXES:
        path = project / rel
        data = path.read_bytes().decode("utf-8")
        if data.count(_in_file_endings(data, new)) == 1:
            continue  # a reused copy: applied before
        path.write_bytes(apply_to(data, old, new, "%s %s" % (finding, rel)).encode("utf-8"))
        done.append(finding)
    return done


def fixed_text(rel, text):
    """A Godot file's text as the port fixes it, for tools that READ the Godot repo instead of a scratch copy (the data
    exporter, pawsim's .tres loader): every fix for `rel` (the path under the Godot project, forward slashes) applied in
    memory. Text that already holds a fix (read from a fixed scratch copy) keeps it; a stale target raises FixError."""
    rel = rel.replace("\\", "/")
    for finding, frel, old, new in FIXES:
        if frel != rel or text.count(_in_file_endings(text, new)) == 1:
            continue
        text = apply_to(text, old, new, "%s %s" % (finding, rel))
    return text


def fixed_read(godot_root, path):
    """The file at `path` (under `godot_root`) read as the port fixes it (fixed_text). Other files read as they are."""
    text = Path(path).read_bytes().decode("utf-8")
    try:
        rel = Path(path).resolve().relative_to(Path(godot_root).resolve()).as_posix()
    except ValueError:
        return text
    return fixed_text(rel, text)


def check_repo(godot=GODOT):
    """Every fix still applies to the Godot repo as it is (read only): a stale target is a failure."""
    bad = []
    for finding, rel, old, new in FIXES:
        try:
            apply_to((Path(godot) / rel).read_bytes().decode("utf-8"), old, new, "%s %s" % (finding, rel))
        except (FixError, OSError) as e:
            bad.append(str(e))
    return bad


def self_test():
    ok = True
    for label, text, old, new in [("a stale target", "a\nb\n", "c\n", "d\n"), ("a target found twice", "a\na\n", "a\n", "b\n"),
                                  ("a no-op", "a\nb\n", "a\n", "a\n")]:
        try:
            apply_to(text, old, new, label)
            print("FAIL  %s was not refused" % label)
            ok = False
        except FixError:
            print("PASS  %s is refused" % label)
    crlf = apply_to("x\r\ny\r\n", "x\n", "z\n", "CRLF")
    if crlf != "z\r\ny\r\n":
        print("FAIL  a CRLF file keeps its line endings")
        ok = False
    else:
        print("PASS  a CRLF file keeps its line endings")
    # fixed_text (the readers' path): a file's fixes applied once; already applied, left alone; another file untouched.
    # A file with exactly one fix (fixed_text applies EVERY fix of a file, so a file with several would need them all).
    single = [f for f in FIXES if sum(1 for g in FIXES if g[1] == f[1]) == 1]
    rel, old, new = single[-1][1], single[-1][2], single[-1][3]
    once = fixed_text(rel, "x\n" + old)
    twice = fixed_text(rel, once)
    other = fixed_text("not/a/fixed/file.tres", "x\n" + old)
    if once != "x\n" + new or twice != once or other != "x\n" + old:
        print("FAIL  fixed_text applies a file's fixes once, and only that file's")
        ok = False
    else:
        print("PASS  fixed_text applies a file's fixes once, and only that file's")
    try:
        fixed_text(rel, "x\n")
        print("FAIL  fixed_text did not refuse a stale target")
        ok = False
    except FixError:
        print("PASS  fixed_text refuses a stale target")
    import tempfile
    with tempfile.TemporaryDirectory() as tmp:  # a stand-in repo: the real one is never handed to apply_all here
        try:
            apply_all(tmp, repo=tmp)
            print("FAIL  the repo itself was not refused")
            ok = False
        except FixError:
            print("PASS  the repo itself is refused (a scratch copy only)")
    return ok


if __name__ == "__main__":
    if "--self-test" in sys.argv:
        sys.exit(0 if self_test() else 1)
    bad = check_repo()
    for finding, rel, _, _ in FIXES:
        print("%-4s %s" % (finding, rel))
    for b in bad:
        print("STALE " + b)
    sys.exit(1 if bad else 0)
