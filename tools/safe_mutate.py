"""Power-safe in-place edits of project files, for the negative tests that plant a fault in the real tree.

Why (2026-11-13): the nightly's PawCore sabotage held each original only in memory and wrote it back in `finally`. The
power went about 2 s after a mutant was written; `finally` never ran, and after the reboot PcHud.cpp had the mutant's size
and every byte 0x00 (NTFS had journaled the new size, not the unflushed data). Recovered from git only because the tree had
been clean when the run started.

Here, before a file is changed, its original goes to a journal (build/mutation_journal: the bytes, then an entry naming the
file, its owner and both versions' SHA-256), each flushed to disk. Every write (the mutant, and the restore) goes to a
flushed temporary file that then replaces the target in one rename, so the file on disk is always the whole original or the
whole mutant, never a torn or zeroed mix. A run that dies leaves its entry, and the next run (and check_all, first thing)
restores the original.

    with safe_mutate.mutated(path, new_bytes, "sabotage_pawcore rng_bias"):
        ...build, run the test...

    py tools/safe_mutate.py               # restore what a dead run left in place (check_all's first gate)
    py tools/safe_mutate.py --list        # show the journal, change nothing
    py tools/safe_mutate.py --self-test   # each leftover state (mutant, zeros, missing, changed, live) handled
Exit 0 = nothing left in place (or all of it restored); 2 = something needs a person: a file changed since the run that left
it, a damaged entry, or a run still holding a mutant (never two test runs at once).
"""

import argparse
import contextlib
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
JOURNAL = ROOT / "build" / "mutation_journal"
STALE_TMP_SECONDS = 600  # a durable write takes milliseconds; an older .tmp was left by a run that died mid-write


def _sha(data):
    return hashlib.sha256(data).hexdigest()


def _entry_id(path):
    return hashlib.sha1(str(path).lower().encode("utf-8")).hexdigest()[:16]


def _durable_write(path, data, tmp_dir):
    """Write `data` to `path` so that a power cut leaves either the old file or the new one, whole."""
    tmp_dir.mkdir(parents=True, exist_ok=True)
    tmp = tmp_dir / ("%s.%d.tmp" % (path.name, os.getpid()))
    with open(tmp, "wb") as f:
        f.write(data)
        f.flush()
        os.fsync(f.fileno())
    # A reader without FILE_SHARE_DELETE (a scanner, an indexer) can refuse the rename for a moment.
    for attempt in range(50):
        try:
            os.replace(tmp, path)
            return
        except PermissionError:
            if attempt == 49:
                tmp.unlink(missing_ok=True)
                raise
            time.sleep(0.1)


def _process_start(pid):
    """The creation time of a RUNNING process, or None. With the pid it names one process, even after a reboot reuses pids."""
    if os.name != "nt" or not pid:
        return None
    import ctypes
    from ctypes import wintypes
    k = ctypes.WinDLL("kernel32", use_last_error=True)
    k.OpenProcess.restype = wintypes.HANDLE
    k.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    k.GetExitCodeProcess.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
    k.GetProcessTimes.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
    k.CloseHandle.argtypes = [wintypes.HANDLE]
    h = k.OpenProcess(0x1000, False, pid)  # PROCESS_QUERY_LIMITED_INFORMATION
    if not h:
        return None
    try:
        code = wintypes.DWORD()
        if not k.GetExitCodeProcess(h, ctypes.byref(code)) or code.value != 259:  # STILL_ACTIVE
            return None
        times = [wintypes.FILETIME() for _ in range(4)]
        if not k.GetProcessTimes(h, *(ctypes.byref(t) for t in times)):
            return None
        return (times[0].dwHighDateTime << 32) | times[0].dwLowDateTime
    finally:
        k.CloseHandle(h)


def _forget(journal, eid):
    (journal / (eid + ".json")).unlink(missing_ok=True)
    (journal / (eid + ".orig")).unlink(missing_ok=True)


@contextlib.contextmanager
def mutated(path, data, owner, journal=JOURNAL):
    """Hold `path` with the bytes `data` for the body, journaled first; the original is back (and durable) on exit."""
    path = Path(path).resolve()
    original = path.read_bytes()
    if data == original:
        raise ValueError("the mutation changes nothing: %s" % path)
    eid = _entry_id(path)
    entry = journal / (eid + ".json")
    if entry.exists():
        raise RuntimeError("%s already holds a mutation (%s); run py tools/safe_mutate.py first" % (path, entry))
    _durable_write(journal / (eid + ".orig"), original, journal)
    meta = {"path": str(path), "owner": owner, "pid": os.getpid(), "pid_start": _process_start(os.getpid()),
            "original_sha256": _sha(original), "mutant_sha256": _sha(data),
            "written": time.strftime("%Y-%m-%d %H:%M:%S")}
    _durable_write(entry, json.dumps(meta, indent=1).encode("utf-8"), journal)
    try:
        _durable_write(path, data, journal)
        yield path
    finally:
        _durable_write(path, original, journal)
        if _sha(path.read_bytes()) != meta["original_sha256"]:
            raise RuntimeError("the restore of %s did not land; the journal keeps the original (%s)" % (path, entry))
        _forget(journal, eid)


def _state(meta, orig_file):
    """(state, original bytes) for one journal entry: LIVE, BROKEN, CLEAN, MUTANT, ZEROS, MISSING or CHANGED."""
    if meta.get("pid_start") is not None and _process_start(meta.get("pid")) == meta["pid_start"]:
        return "LIVE", None
    original = orig_file.read_bytes() if orig_file.exists() else None
    if original is None or _sha(original) != meta.get("original_sha256"):
        return "BROKEN", None
    target = Path(meta["path"])
    if not target.exists():
        return "MISSING", original
    current = target.read_bytes()
    if _sha(current) == meta["original_sha256"]:
        return "CLEAN", original
    if _sha(current) == meta["mutant_sha256"]:
        return "MUTANT", original
    if not current.strip(b"\0"):
        return "ZEROS", original
    return "CHANGED", original


SAYS = {
    "LIVE": "a running process (pid %(pid)s, %(owner)s) holds this mutation; one test run at a time",
    "BROKEN": "the journal's copy of the original is missing or damaged; restore the file from git by hand",
    "CLEAN": "already the original; the entry %(fix)s dropped",
    "MUTANT": "the mutant is still in place (%(owner)s, %(written)s); the original %(fix)s restored",
    "ZEROS": "the file is zeros or empty (a power cut mid-write, %(owner)s, %(written)s); the original %(fix)s restored",
    "MISSING": "the file is gone (%(owner)s, %(written)s); the original %(fix)s restored",
    "CHANGED": "the file is neither the original nor the mutant (%(owner)s, %(written)s): someone changed it since. Nothing "
               "touched; compare it with %(orig)s by hand, then delete that file and %(entry)s",
}


def recover(journal=JOURNAL, dry_run=False, say=print):
    """Restore every original a dead run left in place. Returns 0, or 2 when an entry needs a person."""
    if not journal.exists():
        return 0
    now = time.time()
    for tmp in journal.glob("*.tmp"):
        if not dry_run and now - tmp.stat().st_mtime > STALE_TMP_SECONDS:
            tmp.unlink(missing_ok=True)
    code = 0
    for entry in sorted(journal.glob("*.json")):
        orig_file = entry.with_suffix(".orig")
        try:
            meta = json.loads(entry.read_text(encoding="utf-8"))
            state, original = _state(meta, orig_file)
        except (ValueError, KeyError, OSError) as e:
            meta, state, original = {"path": entry.name}, "BROKEN", None
            say("BROKEN   %s: unreadable entry (%s)" % (entry, e))
            code = 2
            continue
        words = SAYS[state] % dict(meta, orig=orig_file, entry=entry, fix="would be" if dry_run else "is")
        if dry_run:
            say("%-8s %s: %s" % (state, meta["path"], words))
            code = 2 if state != "CLEAN" else code
            continue
        if state in ("MUTANT", "ZEROS", "MISSING"):
            target = Path(meta["path"])
            _durable_write(target, original, journal)
            if _sha(target.read_bytes()) != meta["original_sha256"]:
                say("FAILED   %s: the restore did not land; the entry is kept" % target)
                code = 2
                continue
        if state in ("CLEAN", "MUTANT", "ZEROS", "MISSING"):
            entry.unlink()
            orig_file.unlink(missing_ok=True)
        else:
            code = 2
        say("%-8s %s: %s" % ("RESTORED" if state in ("MUTANT", "ZEROS", "MISSING") else state, meta["path"], words))
    return code


def _die_holding(target, journal):
    """Self-test helper: take a mutation and die inside it, as a power cut would (no `finally`, no restore)."""
    path = Path(target)
    with mutated(path, path.read_bytes() + b"// mutant\r\n", "self-test (dies holding it)", Path(journal)):
        os._exit(1)


def self_test():
    work = ROOT / "build" / "safe_mutate_selftest"
    shutil.rmtree(work, ignore_errors=True)
    journal = work / "journal"
    target = work / "Target.cpp"
    original = b"int Planted() { return 1; }\r\n\xef\xbb\xbf// CRLF kept byte for byte\r\n"
    failures = []
    log = []

    def check(what, ok):
        print("self-test: %-58s %s" % (what, "ok" if ok else "FAILED"))
        if not ok:
            failures.append(what)

    def entries():
        return sorted(p.name for p in journal.glob("*")) if journal.exists() else []

    def die_holding():
        subprocess.run([sys.executable, __file__, "--die-holding", str(target), str(journal)], check=False)

    work.mkdir(parents=True)
    target.write_bytes(original)
    file_ids = [target.stat().st_ino]
    with mutated(target, b"mutant\r\n", "self-test", journal):
        file_ids.append(target.stat().st_ino)
        check("the body sees the mutant; the journal holds the original", target.read_bytes() == b"mutant\r\n"
              and len(entries()) == 2 and (journal / (_entry_id(target.resolve()) + ".orig")).read_bytes() == original)
        check("a run that still holds its mutation is LIVE, never undone",
              recover(journal, say=log.append) == 2 and target.read_bytes() == b"mutant\r\n")
        try:
            with mutated(target, b"second\r\n", "self-test", journal):
                pass
            check("a second mutation of a held file is refused", False)
        except RuntimeError:
            check("a second mutation of a held file is refused", target.read_bytes() == b"mutant\r\n")
    check("a clean exit restores the original and empties the journal", target.read_bytes() == original and not entries())
    # A power cut cannot be staged here; what makes one harmless can be seen: the mutant and the restore each arrive as a
    # new, already flushed file renamed over the target (a new file id), never written into the target in place.
    file_ids.append(target.stat().st_ino)
    check("the mutant and the restore each arrive by rename, not in place", len(set(file_ids)) == 3)

    try:
        with mutated(target, b"mutant\r\n", "self-test", journal):
            raise KeyError("the body fails")
    except KeyError:
        pass
    check("a body that raises still restores and empties the journal", target.read_bytes() == original and not entries())

    die_holding()
    check("a run that dies inside leaves the mutant and its entry", target.read_bytes() != original and len(entries()) == 2)
    check("recover restores the original after the mutant was left",
          recover(journal, say=log.append) == 0 and target.read_bytes() == original and not entries())

    die_holding()
    target.write_bytes(b"\0" * (len(original) + 11))
    check("recover restores the original over a zero-filled file",
          recover(journal, say=log.append) == 0 and target.read_bytes() == original and not entries())

    die_holding()
    target.unlink()
    check("recover restores a file that is gone",
          recover(journal, say=log.append) == 0 and target.read_bytes() == original and not entries())

    die_holding()
    target.write_bytes(b"someone's own edit\r\n")
    check("a file changed since is left alone and reported",
          recover(journal, say=log.append) == 2 and target.read_bytes() == b"someone's own edit\r\n" and len(entries()) == 2)
    check("--list reports it and changes nothing",
          recover(journal, dry_run=True, say=log.append) == 2 and len(entries()) == 2)
    for p in journal.glob("*"):
        p.unlink()

    target.write_bytes(original)
    die_holding()
    orig_file = journal / (_entry_id(target.resolve()) + ".orig")
    orig_file.write_bytes(b"damaged")
    mutant = target.read_bytes()
    check("a damaged journal copy is never written over the file",
          recover(journal, say=log.append) == 2 and target.read_bytes() == mutant)
    for p in journal.glob("*"):
        p.unlink()

    stale = journal / "Target.cpp.999999.tmp"
    stale.write_bytes(b"half")
    os.utime(stale, (time.time() - 2 * STALE_TMP_SECONDS,) * 2)
    check("a stale temporary file from a dead write is removed", recover(journal, say=log.append) == 0 and not stale.exists())

    if failures:
        print("\n".join(log))
        print("FAIL  %d of the self-test's checks failed" % len(failures))
        return 1
    shutil.rmtree(work, ignore_errors=True)
    print("PASS  every leftover state is handled")
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--list", action="store_true", help="show the journal; change nothing")
    ap.add_argument("--self-test", action="store_true")
    ap.add_argument("--die-holding", nargs=2, help=argparse.SUPPRESS)
    args = ap.parse_args()
    if args.die_holding:
        _die_holding(*args.die_holding)
    if args.self_test:
        return self_test()
    code = recover(dry_run=args.list)
    if code == 0:
        print("safe_mutate: no mutation left in place")
    return code


if __name__ == "__main__":
    sys.exit(main())
