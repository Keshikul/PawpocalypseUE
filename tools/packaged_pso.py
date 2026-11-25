"""The PSO check (phase 8; doc 10 §7, "PSO precaching validated"): which pipeline states the game compiles the first time
something is drawn instead of ahead of time. On a player's PC each one is a hitch the first time (ms to 100s of ms, longer
with a cold driver cache).

Runs the packaged Development build with PSO precache validation on (`-ini:Engine:[ConsoleVariables]:r.PSOPrecache.Validation=2`:
the cvar is read-only, and validation exists only in a packaged non-Shipping build, KB U/12 §6) along a route:
    --route soak (default): the whole game at 20x through tools/packaged_soak.py (every biome, boss, screen and effect the
                            soak meets; ~12 min)
    --route bench:          one full field through tools/packaged_bench.py --runs 1 --no-budget (~30 s; its frame time
                            reported, not judged: the frame gate is the bench's own, best of three, run before this in
                            package.py; one run with the validation on FAILED this check on a 4.6 ms p99 with 0 misses,
                            HANDOFF-LOG (144))
then reads the build's log:
  - each `PSO PRECACHING MISS` block: its state (Missed / Too Late), type, material, vertex factory and pass;
  - each `Encountered a new graphics|compute PSO: N (PSOPrecacheState: Missed)` line: the engine's global shaders (Slate,
    post-processing), which the game's components cannot precache;
each placed in time: start-up (before the realm's first frame), play (with the route's last line before it), or exit.
Verdict: FAIL on any `Missed` with one of THIS game's materials (an `M_*` / `MI_*` asset under Content/Pawpocalypse), or a
route that did not PASS. Too Late, the engine's materials and the global shaders are reported, not failed (KB U/12 §6).

It never clears the GPU driver's shader cache (`-clearPSODriverCache` deletes, on NVIDIA, the cache every game on this PC
shares): the validation counts what was not precached whatever the driver has cached.

    py tools/packaged_pso.py [--route soak|bench] [--build build/package] [--log FILE (read an existing log, no run)] [--self-test]
Report: build/pso/report.txt. Exit 0 = PASS; 1 = FAIL; 2 = no build / a copy running (the route's own refusal).
"""
import argparse
import glob
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VALIDATION = "-ini:Engine:[ConsoleVariables]:r.PSOPrecache.Validation=2"
STAMP = re.compile(r"^\[(\d{4}\.\d\d\.\d\d-\d\d\.\d\d\.\d\d:\d{3})\]\[\s*\d+\]")
FIELD = re.compile(r"^\s*(Type|PSOPrecachingState|Material|VertexFactoryType|MeshPassName|GlobalTypeName|PassName|MDCStatsCategory):\s*(.*?)\s*$")
NEW_PSO = re.compile(r"Encountered a new (graphics|compute) PSO: (\d+) \(PSOPrecacheState: ([^)]*)\)")
PLAY_START = ("PAW_BENCH started", "PAW_SOAK ")
EXIT = ("RequestExit", "Engine exit requested")


def project_materials():
    names = set()
    for p in glob.glob(os.path.join(ROOT, "Content", "Pawpocalypse", "**", "*.uasset"), recursive=True):
        n = os.path.splitext(os.path.basename(p))[0]
        if n.startswith("M_") or n.startswith("MI_"):
            names.add(n)
    return names


def parse(text):
    """-> (misses: [dict], new_psos: [dict]); each with 'when' (startup / play / exit) and 'context' (the route's last line)."""
    misses, news = [], []
    phase, context, stamp = "startup", "", ""
    cur = None
    for line in text.splitlines():
        m = STAMP.match(line)
        if m:
            stamp = m.group(1)
            if cur is not None:
                misses.append(cur)
                cur = None
            if phase == "startup" and any(k in line for k in PLAY_START):
                phase = "play"
            if phase == "play" and any(k in line for k in EXIT):
                phase = "exit"
            if "PAW_SOAK " in line or "PAW_BENCH " in line:
                context = line.split("LogPaw: Display: ", 1)[-1].strip()
            n = NEW_PSO.search(line)
            if n:
                news.append(dict(kind=n.group(1), hash=n.group(2), state=n.group(3), when=phase, context=context, stamp=stamp))
            continue
        if "PSO PRECACHING MISS:" in line:
            if cur is not None:
                misses.append(cur)
            cur = dict(when=phase, context=context, stamp=stamp)
            continue
        if cur is not None:
            f = FIELD.match(line)
            if f and f.group(1) not in cur:
                cur[f.group(1)] = f.group(2)
    if cur is not None:
        misses.append(cur)
    return misses, news


def judge(misses, news, ours, route_ok=True):
    """-> (passed, report lines)."""
    out, bad = [], []
    groups = {}
    for m in misses:
        key = (m.get("PSOPrecachingState", "?"), m.get("Type", "?"), m.get("Material", "?"), m.get("VertexFactoryType", "?"),
               m.get("MeshPassName", m.get("GlobalTypeName", m.get("PassName", "?"))), m["when"])
        groups.setdefault(key, []).append(m)
    out.append("PSO precaching misses: %d" % len(misses))
    for key, ms in sorted(groups.items(), key=lambda kv: -len(kv[1])):
        state, typ, mat, vf, pass_, when = key
        mine = mat in ours
        flag = "FAIL" if (mine and state == "Missed") else "note"
        if flag == "FAIL":
            bad.append(mat)
        out.append("  %s  %dx %s, %s, material %s%s, %s, pass %s, %s (first %s: %s)" % (
            flag, len(ms), state, typ, mat, " (this game's)" if mine else "", vf, pass_, when, ms[0]["stamp"], ms[0]["context"][:90]))
    by = {}
    for n in news:
        by.setdefault((n["kind"], n["state"], n["when"]), []).append(n)
    out.append("new PSOs compiled at first use (the engine's global shaders and the misses above): %d" % len(news))
    for (kind, state, when), ns in sorted(by.items()):
        out.append("  note  %dx %s, %s, %s%s" % (len(ns), kind, state, when,
                                                  (" (first %s: %s)" % (ns[0]["stamp"], ns[0]["context"][:90])) if when == "play" else ""))
    if not route_ok:
        out.append("FAIL the route did not PASS (its verdict above)")
    passed = route_ok and not bad
    out.append("packaged_pso: %s%s" % ("PASS" if passed else "FAIL", (" (this game's materials missed: %s)" % ", ".join(sorted(set(bad)))) if bad else ""))
    return passed, out


def route_cmd(route, build):
    """-> (command, the log it leaves). The bench route judges everything but the game thread's budget (--no-budget)."""
    if route == "soak":
        return ([sys.executable, os.path.join(ROOT, "tools", "packaged_soak.py"), "--build", build, "--speed", "20", "--arg=" + VALIDATION],
                os.path.join(build, "Windows", "Pawpocalypse", "Saved", "Logs", "Pawpocalypse.log"))
    return ([sys.executable, os.path.join(ROOT, "tools", "packaged_bench.py"), "--build", build, "--runs", "1", "--no-budget",
             "--arg=" + VALIDATION], os.path.join(ROOT, "build", "bench", "run1", "Pawpocalypse.log"))


def self_test():
    def block(stamp, state, mat, typ="ShadersOnly", vf="FLocalVertexFactory", pass_="TranslucencyAfterDOF"):
        return ("[%s][  1]LogEngine: \n\nPSO PRECACHING MISS:\n\tType:\t\t\t\t\t%s\n\tPSOPrecachingState:\t\t%s\n\tMaterial:\t\t\t\t%s\n"
                "\tVertexFactoryType:\t\t%s\n\tMeshPassName:\t\t\t%s\n\tShader Hashes:\n\t\tVertexShader:\t\tAB\n\n"
                % (stamp, typ, state, mat, vf, pass_))
    head = ("[2026.11.24-09.20.27:932][  0]LogRHI: Display: Encountered a new graphics PSO: 1 (PSOPrecacheState: Missed)\n"
            "[2026.11.24-09.20.27:965][  1]LogPaw: Display: PAW_SOAK started\n")
    tail = ("[2026.11.24-09.30.00:000][  9]LogRHI: Display: Encountered a new graphics PSO: 3 (PSOPrecacheState: Missed)\n"
            "[2026.11.24-09.31.00:000][  9]LogWindows: FPlatformMisc::RequestExit(0, paw.Soak)\n"
            "[2026.11.24-09.31.00:001][  9]LogRHI: Display: Encountered a new graphics PSO: 4 (PSOPrecacheState: Missed)\n")
    ours = {"M_BackdropFill", "M_Sprite"}
    cases = [
        ("one of this game's materials Missed: FAIL", head + block("2026.11.24-09.21.00:000", "Missed", "M_BackdropFill") + tail, True, False),
        ("an engine material Missed: PASS (noted)", head + block("2026.11.24-09.21.00:000", "Missed", "DefaultTextMaterialOpaque") + tail, True, True),
        ("this game's material Too Late: PASS (noted)", head + block("2026.11.24-09.21.00:000", "Too Late", "M_Sprite", "FullPSO") + tail, True, True),
        ("global shaders only: PASS", head + tail, True, True),
        ("a clean log but the route failed: FAIL", head + tail, False, False),
        ("a miss before the first log stamp after it still counts", head + block("2026.11.24-09.21.00:000", "Too Late", "M_Sprite", "FullPSO")
         + block("2026.11.24-09.21.00:001", "Missed", "M_Sprite") + tail, True, False),
    ]
    ok = True
    for name, text, route_ok, want in cases:
        misses, news = parse(text)
        got, _ = judge(misses, news, ours, route_ok)
        good = got == want
        ok &= good
        print("%s  %s" % ("PASS" if good else "FAIL (judged %s)" % got, name))
    misses, news = parse(head + block("2026.11.24-09.21.00:000", "Missed", "M_BackdropFill") + tail)
    good = [n["when"] for n in news] == ["startup", "play", "exit"] and misses and misses[0]["when"] == "play" \
        and misses[0].get("MeshPassName") == "TranslucencyAfterDOF"
    ok &= bool(good)
    print("%s  each event placed in time (start-up, play, exit) and its fields read" % ("PASS" if good else "FAIL %s %s" % (news, misses)))
    # A block ends at the next stamped line: another multi-line message's field-like line is not read into it.
    other = "[2026.11.24-09.22.00:000][  2]LogRHI: Display: New Graphics PSO (5)\n\tMeshPassName:\t\tSomethingElse\n"
    misses, _ = parse(head + block("2026.11.24-09.21.00:000", "Missed", "M_BackdropFill").replace("\tMeshPassName:\t\t\tTranslucencyAfterDOF\n", "")
                      + other + tail)
    good = len(misses) == 1 and "MeshPassName" not in misses[0]
    ok &= good
    print("%s  a miss block ends at the next log line (no other message's fields read into it)" % ("PASS" if good else "FAIL %s" % misses))
    cmd, _ = route_cmd("bench", "B")
    good = "--no-budget" in cmd and VALIDATION in " ".join(cmd)
    ok &= good
    print("%s  the bench route runs with the validation on and its frame budget not judged (--no-budget)" % ("PASS" if good else "FAIL %s" % cmd))
    real = project_materials()
    good = {"M_BackdropFill", "M_Sprite"} <= real
    ok &= good
    print("%s  this game's materials found in Content/Pawpocalypse (%d)" % ("PASS" if good else "FAIL", len(real)))
    print("packaged_pso self-test: %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--route", choices=["soak", "bench"], default="soak")
    ap.add_argument("--build", default=os.path.join(ROOT, "build", "package"))
    ap.add_argument("--log", help="judge an existing log instead of running")
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    route_ok = True
    if args.log:
        log = args.log
    else:
        cmd, log = route_cmd(args.route, args.build)
        print("packaged_pso: the %s route with PSO validation on" % args.route, flush=True)
        r = subprocess.run(cmd)
        if r.returncode == 2:
            return 2
        route_ok = r.returncode == 0
    if not os.path.isfile(log):
        print("packaged_pso: no log at %s" % log)
        return 1
    with open(log, encoding="utf-8", errors="replace") as f:
        text = f.read()
    if "r.PSOPrecache.Validation:2" not in text:
        print("packaged_pso: FAIL, the log does not show the validation on (Set CVar [[r.PSOPrecache.Validation:2]])")
        return 1
    misses, news = parse(text)
    passed, out = judge(misses, news, project_materials(), route_ok)
    os.makedirs(os.path.join(ROOT, "build", "pso"), exist_ok=True)
    with open(os.path.join(ROOT, "build", "pso", "report.txt"), "w", encoding="utf-8") as f:
        f.write("log: %s\nroute: %s\n" % (log, "existing log" if args.log else args.route) + "\n".join(out) + "\n")
    print("\n".join(out))
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
