"""pawsim loader parity (docs/06 §7): the CSV-loaded simulator must print exactly what the .tres-loaded one prints.

Runs tools/pawsim.py with --data tres (the Godot project, read-only) and --data csv (Data/*.csv) over a grid of simulator
settings and compares the full reports byte for byte. Any difference means the export or the CSV loader changed a number,
a type, or an order. Long: the full grid is 108 settings x 2 loaders (about 1.5-2 h at 5 workers).

    py tools/check_pawsim_parity.py                 # full grid
    py tools/check_pawsim_parity.py --quick         # 4 settings, ~10 min
    py tools/check_pawsim_parity.py --workers 8
    py tools/check_pawsim_parity.py --self-test     # one CSV cell changed in a copy: the sweep must see it (~10 min)
Exit code 0 = every setting identical. Report: build/pawsim/parity_report.txt (+ a diff per mismatch).
"""
import argparse
import difflib
import itertools
import os
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PAWSIM = os.path.join(ROOT, "tools", "pawsim.py")
OUT = os.path.join(ROOT, "build", "pawsim")


def grid(quick: bool) -> list:
    if quick:
        return [["--policy", "exploit"], ["--policy", "patient"], ["--policy", "v4"], ["--policy", "exploit", "--perfect-targeting", "--fixed"]]
    settings = []
    for policy, targeting, fixed, leak, hp in itertools.product(
            ["exploit", "patient", "v4"], [False, True], [False, True], ["0.0", "0.25", "0.5"], ["0.5", "1.0", "2.0"]):
        if policy == "v4" and not fixed:
            continue  # v4 forces the fixes on; the pair would repeat the fixed run
        args = ["--policy", policy, "--add-leak", leak, "--boss-hp-scale", hp]
        if targeting:
            args.append("--perfect-targeting")
        if fixed:
            args.append("--fixed")
        settings.append(args)
    return settings


def run(args: list, data: str, csv_dir: str = "") -> tuple:
    t = time.time()
    if csv_dir:
        # pawsim reads Data/ through its module-level CSV_DIR; point it at a copy WITHOUT editing pawsim.py (a sweep may be
        # running, and every one of its runs loads pawsim.py afresh).
        code = ("import sys; from pathlib import Path; sys.path.insert(0, %r); import pawsim; pawsim.CSV_DIR = Path(%r); "
                "sys.argv = ['pawsim.py'] + %r; sys.exit(pawsim.main())" % (os.path.dirname(PAWSIM), csv_dir, [*args, "--data", data]))
        cmd = [sys.executable, "-c", code]
    else:
        cmd = [sys.executable, PAWSIM, *args, "--data", data]
    p = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", cwd=ROOT)
    return p.returncode, p.stdout + p.stderr, time.time() - t


def self_test() -> int:
    """The sweep must notice one changed CSV cell (an enemy's HP +1 %), and the untouched copy must still match."""
    import csv
    import shutil
    work = os.path.join(OUT, "selftest_data")
    if os.path.exists(work):
        shutil.rmtree(work)
    shutil.copytree(os.path.join(ROOT, "Data"), work)
    args = ["--policy", "exploit"]
    with ThreadPoolExecutor(max_workers=3) as pool:
        tres = pool.submit(run, args, "tres")
        clean = pool.submit(run, args, "csv", work)
        rc_t, out_t, _ = tres.result()
        rc_c, out_c, _ = clean.result()
    clean_same = rc_t == rc_c == 0 and out_t == out_c
    path = os.path.join(work, "EnemyData.csv")
    with open(path, encoding="utf-8", newline="") as f:
        rows = list(csv.reader(f))
    hp = rows[0].index("hp")
    rows[1][hp] = repr(float(rows[1][hp]) * 1.01)
    with open(path, "w", encoding="utf-8", newline="") as f:
        csv.writer(f, lineterminator="\n").writerows(rows)
    rc_s, out_s, _ = run(args, "csv", work)
    caught = rc_s != 0 or out_s != out_t
    shutil.rmtree(work, ignore_errors=True)
    print("%s  the untouched copy matches the .tres run" % ("PASS    " if clean_same else "FAIL    "))
    print("%s  enemy %s hp +1%%" % ("CAUGHT  " if caught else "MISSED  ", rows[1][0]))
    return 0 if clean_same and caught else 1


def check(args: list) -> tuple:
    rc_t, out_t, sec_t = run(args, "tres")
    rc_c, out_c, sec_c = run(args, "csv")
    same = rc_t == rc_c == 0 and out_t == out_c
    diff = "" if same else "".join(difflib.unified_diff(out_t.splitlines(True), out_c.splitlines(True), "tres", "csv", n=1))
    if rc_t or rc_c:
        diff = f"exit codes tres={rc_t} csv={rc_c}\n{out_t[-2000:]}\n{out_c[-2000:]}\n" + diff
    return args, same, diff, sec_t + sec_c


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--workers", type=int, default=5)
    ap.add_argument("--self-test", action="store_true")
    a = ap.parse_args()
    if a.self_test:
        return self_test()
    os.makedirs(OUT, exist_ok=True)
    settings = grid(a.quick)
    print(f"pawsim parity: {len(settings)} settings x 2 loaders, {a.workers} workers", flush=True)
    lines, failures, t0 = [], 0, time.time()
    with ThreadPoolExecutor(max_workers=a.workers) as pool:
        futures = [pool.submit(check, s) for s in settings]
        for i, f in enumerate(as_completed(futures), 1):
            args, same, diff, sec = f.result()
            tag = " ".join(args)
            line = f"[{i}/{len(settings)}] {'SAME' if same else 'DIFF'} {sec:6.0f}s  {tag}"
            print(line, flush=True)
            lines.append(line)
            if not same:
                failures += 1
                name = "parity_diff_" + "_".join(x.strip("-").replace(".", "p") for x in args) + ".txt"
                open(os.path.join(OUT, name), "w", encoding="utf-8").write(diff)
    summary = f"RESULT {'PASS' if failures == 0 else 'FAIL'}: {len(settings) - failures} identical, {failures} different, {time.time() - t0:.0f}s"
    print(summary)
    open(os.path.join(OUT, "parity_report.txt"), "w", encoding="utf-8").write("\n".join([summary] + sorted(lines)) + "\n")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
