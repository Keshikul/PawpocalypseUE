"""Which running Unreal processes forbid an engine run of THIS project (CLAUDE.md: never two test runs at once; never touch
the designer's editor).

Every UnrealEditor* process blocks, except one whose command line names ANOTHER project's .uproject: that one shares
neither this project's binaries nor its Saved folder (another project's soak, seen 2026-10-31). A process naming this
project, or naming no project at all (an editor opened through the project browser may be the designer's), blocks.

    py tools/unreal_procs.py          # prints the blocking processes; exit 1 if any, 0 if none (one look)
    py tools/unreal_procs.py --wait   # the same after wait_blocking(): what every engine gate and tool asks (check_all too)
    py tools/unreal_procs.py --self-test      # which processes block, and the wait for them (guard_self_test)

Every guard asks wait_blocking(), never one look: a gate starts the moment the one before it returns, and a look can catch
a process on its way out (or one that blinks into the list): one look refused L3 UI in 1 s on 2026-11-13 10:53:50.
"""
import json
import os
import re
import subprocess
import sys
import time

THIS = os.path.normcase(os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Pawpocalypse.uproject")))
RE_UPROJECT = re.compile(r'"?([A-Za-z]:[^"]*?\.uproject)"?', re.IGNORECASE)


def unreal_processes():
    """[(pid, name, command line)] of every UnrealEditor* process."""
    ps = ("Get-CimInstance Win32_Process -Filter \"Name LIKE 'UnrealEditor%'\" | "
          "Select-Object ProcessId, Name, CommandLine | ConvertTo-Json -Compress")
    out = subprocess.run(["powershell", "-NoProfile", "-Command", ps], capture_output=True, text=True).stdout.strip()
    if not out:
        return []
    rows = json.loads(out)
    rows = rows if isinstance(rows, list) else [rows]
    return [(r.get("ProcessId"), r.get("Name") or "", r.get("CommandLine") or "") for r in rows]


def blocks(command_line):
    """True unless the command line names another project's .uproject."""
    m = RE_UPROJECT.search(command_line or "")
    if not m:
        return True
    return os.path.normcase(os.path.abspath(m.group(1))) == THIS


def blocking():
    return [p for p in unreal_processes() if blocks(p[2])]


def self_test():
    cases = [
        ('"C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Binaries\\Win64\\UnrealEditor.exe"', True),  # project browser
        ('UnrealEditor.exe "%s"' % THIS, True),
        ('UnrealEditor-Cmd.exe %s -run=pythonscript' % THIS, True),
        ('UnrealEditor.exe "C:\\Games\\Other\\Other.uproject" /Game/Maps/L_Main -game', False),
        ('UnrealEditor.exe C:/Games/Other/Other.uproject -game', False),
        ("", True),
    ]
    bad = [(c, want) for c, want in cases if blocks(c) != want]
    for c, want in bad:
        print("self-test FAIL: %r should %s" % (c, "block" if want else "not block"))
    print("unreal_procs self-test: %s" % ("FAIL" if bad else "PASS"))
    return 1 if bad else 0


def wait_until_clear(check, sleep, now, limit=30.0):
    """[] once `check()` has found nothing twice in a row; else, after `limit` seconds of finding something, what it found.

    The run before this one may still be exiting (check_all starts the next gate the moment the last returns: seen
    2026-11-05, a gate refused on its predecessor's dying process), so anything seen is waited for. The old loop checked
    once more after it and refused at once on what that check saw: a process that appeared between the two refused the gate
    within a second, without waiting and without saying what it was (2026-11-13 10:53:50, L3 UI; likely the cut nightly's
    Audio refusal at 22:31:40 too)."""
    deadline = now() + limit
    clean = 0
    while clean < 2:
        found = check()
        if not found:
            clean += 1
            if clean < 2:
                sleep(1.0)
            continue
        clean = 0
        if now() >= deadline:
            return found
        sleep(2.0)
    return []


def wait_blocking(limit=30.0):
    """The blocking processes still there after waiting up to `limit` s for them ([] = clear: go ahead)."""
    return wait_until_clear(blocking, time.sleep, time.time, limit)


def print_blocking(found):
    for pid, name, command in found:
        print("  blocking: %s %s %s" % (pid, name, (command or "(no command line)")[:300]))


def guard_self_test():
    """wait_until_clear on stubbed process lists (each check 5 s later on a stubbed clock). Run by --self-test (check_all
    -Full's Unreal guard self-test) and `py tools/run_automation.py --self-test`. Seen to FAIL on the old loop."""
    fake = [(4242, "UnrealEditor.exe", "UnrealEditor.exe")]
    cases = [
        ("nothing running", [False], False),
        ("a process appears after the first clean check, then goes", [False, True, False, False], False),
        ("a process appears after the first clean check and stays", [False, True], True),
        ("the previous run exits within the wait", [True, True, False, False], False),
        ("a process that never goes", [True], True),
    ]
    bad = 0
    for name, seq, refuses in cases:
        calls = [0]

        def check(seq=seq, calls=calls):
            calls[0] += 1
            return fake if seq[min(calls[0] - 1, len(seq) - 1)] else []

        got = wait_until_clear(check, lambda s: None, lambda calls=calls: 5.0 * calls[0])
        if bool(got) != refuses:
            bad += 1
            print("guard self-test FAIL: %s: %s" % (name, "refused" if got else "went ahead"))
    print("Unreal guard wait self-test: %s (%d cases)" % ("FAIL" if bad else "PASS", len(cases)))
    return 1 if bad else 0


if __name__ == "__main__":
    if "--self-test" in sys.argv:
        # Which processes block, then how every guard waits for them (one gate: the Unreal guard self-test).
        sys.exit(max(self_test(), guard_self_test()))
    found = wait_blocking() if "--wait" in sys.argv else blocking()
    print_blocking(found)
    sys.exit(1 if found else 0)
