"""Which running Unreal processes forbid an engine run of THIS project (CLAUDE.md: never two test runs at once; never touch
the designer's editor).

Every UnrealEditor* process blocks, except one whose command line names ANOTHER project's .uproject: that one shares
neither this project's binaries nor its Saved folder (another project's soak, seen 2026-10-31). A process naming this
project, or naming no project at all (an editor opened through the project browser may be the designer's), blocks.

    py tools/unreal_procs.py          # prints the blocking processes; exit 1 if any, 0 if none
    py tools/unreal_procs.py --self-test
"""
import json
import os
import re
import subprocess
import sys

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


if __name__ == "__main__":
    if "--self-test" in sys.argv:
        sys.exit(self_test())
    found = blocking()
    for pid, name, cmd in found:
        print("blocking: %s %s %s" % (pid, name, cmd[:200]))
    sys.exit(1 if found else 0)
