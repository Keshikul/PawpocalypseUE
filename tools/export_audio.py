"""Exports the Godot game's sound table and the sounds it plays (docs/07 §6; phase 6 slice 4).

    py tools/export_audio.py [--godot C:/Games/pawpocalypse] [--check]

Reads scripts/autoload/audio_manager.gd: EVENTS (each event's min_interval, pitch spread, volume_db), the sounds each event
plays (a folder assets/audio/sfx/<event>/ when it has files, else FALLBACK_SFX's Kenney stems, exactly as
_load_all_variants() decides) and FALLBACK_MUSIC (track id -> file; an id with a file assets/audio/music/<id>.ogg of its
own would win, as _find_music decides). Writes:
  Data/AudioEvents.csv  event, min_interval, pitch, volume_db, sounds (';'-joined, in Godot's load order)
  Data/AudioMusic.csv   track, sound
and copies ONLY the files something plays into SourceAudio/ (LFS): sfx/<file>, music/<file>. A file nothing plays is not
copied (the prune: Godot shipped 130 unplayed files); tools/import_audio.py imports exactly these.

--check: exports into build/ and fails if any CSV or copied file differs (or one is missing / extra).
"""

import argparse
import ast
import csv
import io
import re
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SUFFIXES = (".ogg", ".wav", ".mp3")


def const_block(src: str, name: str) -> str:
    """The text of `const NAME := { ... }` (balanced braces)."""
    m = re.search(r"^const %s := \{" % name, src, re.M)
    if not m:
        raise SystemExit("export_audio: no const %s in audio_manager.gd" % name)
    i = m.end() - 1
    depth = 0
    for j in range(i, len(src)):
        if src[j] == "{":
            depth += 1
        elif src[j] == "}":
            depth -= 1
            if depth == 0:
                return src[i:j + 1]
    raise SystemExit("export_audio: unbalanced %s" % name)


def gd_dict(text: str):
    """A GDScript dictionary literal of strings, numbers and lists as a Python value (comments stripped)."""
    text = re.sub(r"#[^\n]*", "", text)
    return ast.literal_eval(text)


def collect(godot: Path):
    src = (godot / "scripts" / "autoload" / "audio_manager.gd").read_text(encoding="utf-8")
    events = gd_dict(const_block(src, "EVENTS"))
    fallback = gd_dict(const_block(src, "FALLBACK_SFX"))
    music = gd_dict(const_block(src, "FALLBACK_MUSIC"))
    sfx_root = godot / "assets" / "audio" / "sfx"
    music_root = godot / "assets" / "audio" / "music"
    rows, files = [], {}
    for event, rules in events.items():
        folder = sfx_root / event
        own = sorted(p for p in folder.rglob("*") if p.suffix in SUFFIXES) if folder.is_dir() else []
        if own:
            sounds = [p for p in own]
        else:
            sounds = [sfx_root / "kenney" / (stem + ".ogg") for stem in fallback.get(event, [])]
        names = []
        for p in sounds:
            if not p.exists():
                raise SystemExit("export_audio: %s plays a missing file %s" % (event, p))
            files["sfx/" + p.name] = p
            names.append(p.name)
        rows.append([event, rules["min_interval"], rules["pitch"], rules["volume_db"], ";".join(names)])
    tracks = []
    for track, name in music.items():
        own = [music_root / (track + s) for s in SUFFIXES if (music_root / (track + s)).exists()]
        p = own[0] if own else music_root / (name + ".ogg")
        if not p.exists():
            raise SystemExit("export_audio: track %s maps to a missing file %s" % (track, p))
        files["music/" + p.name] = p
        tracks.append([track, p.name])
    return rows, tracks, files


def write(out_data: Path, out_audio: Path, rows, tracks, files):
    out_data.mkdir(parents=True, exist_ok=True)
    def csv_text(header, body):
        buf = io.StringIO()
        w = csv.writer(buf, lineterminator="\n")
        w.writerow(header)
        w.writerows(body)
        return buf.getvalue()
    (out_data / "AudioEvents.csv").write_text(csv_text(["event", "min_interval", "pitch", "volume_db", "sounds"],
        [[r[0], "%g" % r[1], "%g" % r[2], "%g" % r[3], r[4]] for r in rows]), encoding="utf-8", newline="\n")
    (out_data / "AudioMusic.csv").write_text(csv_text(["track", "sound"], tracks), encoding="utf-8", newline="\n")
    if out_audio.exists():
        shutil.rmtree(out_audio)
    for rel, src in files.items():
        dest = out_audio / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(src, dest)


def export_tables(godot, out) -> int:
    """The two CSVs only (export_godot_data.py chains this, so the data-export gate covers them)."""
    rows, tracks, files = collect(Path(godot))
    out = Path(out)
    work = ROOT / "build" / "audio_tables_scratch"
    write(out, work, rows, tracks, {})
    print("AudioEvents.csv / AudioMusic.csv: %d events, %d tracks -> %s" % (len(rows), len(tracks), out))
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--godot", default="C:/Games/pawpocalypse")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    rows, tracks, files = collect(Path(args.godot))
    if not args.check:
        write(ROOT / "Data", ROOT / "SourceAudio", rows, tracks, files)
        size = sum(p.stat().st_size for p in files.values())
        print("export_audio: %d events, %d tracks, %d files (%.1f MB) -> Data/Audio*.csv, SourceAudio/" % (
            len(rows), len(tracks), len(files), size / 1e6))
        return 0
    work = ROOT / "build" / "audio_check"
    write(work / "Data", work / "SourceAudio", rows, tracks, files)
    bad = []
    for name in ("AudioEvents.csv", "AudioMusic.csv"):
        a, b = ROOT / "Data" / name, work / "Data" / name
        if not a.exists() or a.read_bytes() != b.read_bytes():
            bad.append("Data/" + name)
    have = {p.relative_to(ROOT / "SourceAudio").as_posix() for p in (ROOT / "SourceAudio").rglob("*") if p.is_file()} \
        if (ROOT / "SourceAudio").exists() else set()
    want = set(files)
    bad += ["missing SourceAudio/" + f for f in sorted(want - have)]
    bad += ["unplayed SourceAudio/" + f for f in sorted(have - want)]
    for f in sorted(want & have):
        if (ROOT / "SourceAudio" / f).read_bytes() != files[f].read_bytes():
            bad.append("differs SourceAudio/" + f)
    for b in bad:
        print("  " + b)
    print("export_audio --check: %s" % ("current" if not bad else "%d DIFFERENCES" % len(bad)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
