"""Run the game's engine automation tests headless (docs/09 L3; KB U/18 §2) and turn the log into an exit code.

    py tools/run_automation.py [FILTER]               # default filter: Pawpocalypse.
    py tools/run_automation.py --no-build [FILTER]    # the caller has JUST built (check_all's build gate, sabotage_realm.py)
    py tools/run_automation.py --self-test            # the guard's wait (tools/unreal_procs.py), on stubbed process lists
Exit 0 = every test ran and passed; 1 = a failure, the build failed, or no test matched (a filter that runs nothing proves
nothing); 2 = an Unreal editor is still running after 30 s (never two runs at once, never the designer's editor closed for
us); the refusal names each blocking process.

It builds the editor target with UBT first (seconds when nothing changed): the headless editor loads whatever module DLLs
are on disk, so without the build a test passes on the code as it was at the LAST build (found by review 2026-11-04: the
everyday check_all ran L3 on a DLL older than the source it claimed to test).
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EDITOR = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
UPROJECT = os.path.join(ROOT, "Pawpocalypse.uproject")
UBT = [r"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat", "PawpocalypseEditor", "Win64", "Development",
       "-Project=" + UPROJECT, "-WaitMutex"]
RE_DONE = re.compile(r"Test Completed\. Result=\{(\w+)\} Name=\{([^}]*)\} Path=\{([^}]*)\}")
# Every test that STARTS must complete: a crash mid-test ends the run with the test started and no result, and only the
# completed ones used to be counted (found 2026-11-05: an access violation in Pawpocalypse.Realm.View passed as "10 tests").
RE_START = re.compile(r"Test Started\. Name=\{([^}]*)\} Path=\{([^}]*)\}")


def build():
    r = subprocess.run(UBT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    out = r.stdout + r.stderr
    return r.returncode == 0 and "Result: Succeeded" in out, out


def main():
    # The guard (tools/unreal_procs.py): any Unreal process of this project, or one naming no project, waited for (two clean
    # looks in a row, up to 30 s for anything seen); another project's run is not ours.
    from unreal_procs import guard_self_test, print_blocking, wait_blocking
    # A failure's message can hold any language's letters (the translations, 2026-11-20): a console that cannot show one
    # gets a stand-in, never a crash that hides the verdict.
    sys.stdout.reconfigure(errors="replace")
    args = sys.argv[1:]
    if "--self-test" in args:
        return guard_self_test()
    skip_build = "--no-build" in args
    args = [a for a in args if a != "--no-build"]
    filt = args[0] if args else "Pawpocalypse."
    found = wait_blocking()
    if found:
        print("run_automation: an Unreal editor is running; close it first (this never closes it for you)")
        print_blocking(found)
        return 2
    if not skip_build:
        ok, out = build()
        if not ok:
            print(out[-3000:])
            print("run_automation: FAIL, the editor build failed: nothing was tested (the DLLs on disk are not this source)")
            return 1
    # -PawUsKeyboard: keys with a place map through the US layout whatever this machine types (the fixtures' labels are Godot's
    # headless US ones; a test that wants another layout sets it: Pawpocalypse.Input.Azerty).
    cmd = [EDITOR, UPROJECT, '-ExecCmds=Automation RunTests %s;Quit' % filt, "-TestExit=Automation Test Queue Empty",
           "-nullrhi", "-unattended", "-nop4", "-SCCProvider=None", "-stdout", "-log", "-PawUsKeyboard"]
    r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    text = r.stdout + r.stderr
    log = os.path.join(ROOT, "build", "automation_%s.log" % re.sub(r"[^\w.]", "_", filt))
    os.makedirs(os.path.dirname(log), exist_ok=True)
    with open(log, "w", encoding="utf-8") as f:
        f.write(text)
    results = RE_DONE.findall(text)
    for verdict, _, path in results:
        print("%-8s %s" % (verdict.upper(), path))
    for line in text.splitlines():
        if "PAW_" in line or ("LogAutomationController: Error" in line):
            print("  " + line.strip()[-300:])
    started = {path for _, path in RE_START.findall(text)}
    unfinished = sorted(started - {path for _, _, path in results})
    for path in unfinished:
        print("CRASHED  %s (started, never completed)" % path)
    crashed = "=== Critical error: ===" in text
    if crashed:
        print("  the editor hit a critical error (see the log's callstack)")
    ok = bool(results) and all(v == "Success" for v, _, _ in results) and not unfinished and not crashed
    print("run_automation: %s, %d test(s) (log: %s)" % ("PASS" if ok else "FAIL", len(results), log))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
