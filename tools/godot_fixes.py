"""The port's fixes to Godot's code (D16, designer 2026-11-10: "This is a standalone version. If godot has rules or mechanics that
don't make sense we should change them. Do not port faults from godot.").

The port fixes a Godot fault in its own code, never in the Godot repo. Its tests still prove every rule against the REAL Godot
code: tools/make_godot_fixtures.py runs Godot on a scratch copy of the project, and applies these fixes to that copy (never to
the repo) before it runs. So a fixture shows Godot's behaviour with the port's fixes, row for row, and every other row is
still Godot's own. Each fix names its finding (docs/01 §5), the file, and an exact text replaced: it must be found exactly once
and must change something, or nothing runs (a fix whose target Godot rewrote fails here, not silently).

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
