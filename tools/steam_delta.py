"""What a Steam update would send between two packaged builds (docs/10 §3; phase 8: "a one-number balance patch measured at a
small delta").

    py tools/steam_delta.py OLD_ARCHIVE NEW_ARCHIVE [--chunk 1048576] [--self-test]

SteamPipe cuts every file into chunks at fixed offsets (~1 MiB) and an update sends the chunks whose bytes changed, plus
whole new files (KB Godot/04 §1, docs/10 §3). This counts exactly that, uncompressed (SteamPipe compresses the chunks it
sends, so the real download is smaller): per file, the 1 MiB chunks at the same offset whose hash differs, every chunk of a
file new in NEW, and the files gone. Prints the files that changed with their changed bytes, and the total.
Exit 0; 1 on a bad argument.
"""
import argparse
import hashlib
import os
import sys
import tempfile


def chunk_hashes(path, size):
    out = []
    with open(path, "rb") as f:
        while True:
            b = f.read(size)
            if not b:
                break
            out.append((hashlib.sha1(b).digest(), len(b)))
    return out


def files_of(root):
    found = {}
    for base, _dirs, files in os.walk(root):
        for name in files:
            p = os.path.join(base, name)
            found[os.path.relpath(p, root).replace(os.sep, "/")] = p
    return found


def delta(old_root, new_root, size):
    """(changed: [(rel, bytes changed, file size, why)], total bytes, new total size)."""
    old, new = files_of(old_root), files_of(new_root)
    changed, total = [], 0
    for rel, path in sorted(new.items()):
        if rel not in old:
            n = os.path.getsize(path)
            changed.append((rel, n, n, "new"))
            total += n
            continue
        a, b = chunk_hashes(old[rel], size), chunk_hashes(path, size)
        diff = sum(length for i, (h, length) in enumerate(b) if i >= len(a) or a[i][0] != h)
        if diff:
            changed.append((rel, diff, os.path.getsize(path), "%d of %d chunks" % (
                sum(1 for i, (h, _) in enumerate(b) if i >= len(a) or a[i][0] != h), len(b))))
            total += diff
    for rel in sorted(set(old) - set(new)):
        changed.append((rel, 0, 0, "gone"))
    return changed, total, sum(os.path.getsize(p) for p in new.values())


def self_test():
    ok = True
    with tempfile.TemporaryDirectory() as tmp:
        a, b = os.path.join(tmp, "a"), os.path.join(tmp, "b")
        os.makedirs(a)
        os.makedirs(b)
        big = bytes(range(256)) * (4096 * 3)  # 3 MiB
        for root in (a, b):
            with open(os.path.join(root, "same.bin"), "wb") as f:
                f.write(big)
        with open(os.path.join(a, "edit.bin"), "wb") as f:
            f.write(big)
        edited = bytearray(big)
        edited[1048576 + 10] ^= 0xFF  # one byte in the second chunk
        with open(os.path.join(b, "edit.bin"), "wb") as f:
            f.write(bytes(edited))
        with open(os.path.join(b, "new.bin"), "wb") as f:
            f.write(b"x" * 1000)
        with open(os.path.join(a, "gone.bin"), "wb") as f:
            f.write(b"y")
        changed, total, _ = delta(a, b, 1048576)
        got = {rel: (n, why) for rel, n, _, why in changed}
        want = {"edit.bin": (1048576, "1 of 3 chunks"), "new.bin": (1000, "new"), "gone.bin": (0, "gone")}
        ok = got == want and total == 1048576 + 1000
        print("%s  one byte changed costs its chunk; a new file whole; an unchanged file nothing" % ("PASS" if ok else "FAIL %s" % got))
        # An insertion near the front shifts every chunk after it (why SteamPipe needs aligned layouts).
        with open(os.path.join(b, "edit.bin"), "wb") as f:
            f.write(b"!" + big)
        changed, total, _ = delta(a, b, 1048576)
        shifted = dict((rel, n) for rel, n, _, _ in changed).get("edit.bin", 0)
        good = shifted >= 3 * 1048576
        print("%s  one byte inserted at the front costs every chunk after it" % ("PASS" if good else "FAIL %d" % shifted))
        ok &= good
    print("steam_delta self-test: %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("old", nargs="?")
    ap.add_argument("new", nargs="?")
    ap.add_argument("--chunk", type=int, default=1048576)
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    if not args.old or not args.new or not os.path.isdir(args.old) or not os.path.isdir(args.new):
        print("steam_delta: give two archive folders")
        return 1
    changed, total, size = delta(args.old, args.new, args.chunk)
    for rel, n, fsize, why in sorted(changed, key=lambda c: -c[1]):
        print("%10.2f MiB  %-14s %s (%.1f MiB)" % (n / 1048576.0, why, rel, fsize / 1048576.0))
    print("steam_delta: %.2f MiB to send (uncompressed) of %.1f MiB, %d file(s) changed" % (total / 1048576.0, size / 1048576.0,
                                                                                         sum(1 for c in changed if c[1] > 0)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
