"""Imports the game's sounds and builds the audio bank (docs/07 §6; phase 6 slice 4).

    py tools/import_audio.py --run        (headless; editor closed)

SourceAudio/sfx/<file> -> /Game/Pawpocalypse/Audio/Sfx/SW_<name>, SourceAudio/music/<file> -> /Game/Pawpocalypse/Audio/Music/SW_<name>
(looping), then DA_AudioBank (UPawAudioBank, rewritten whole) from Data/AudioEvents.csv and Data/AudioMusic.csv. UE 5.8 imports
OGG Vorbis and WAV directly (SoundFactory). The files are exactly what tools/export_audio.py copied (only what something
plays); a sound asset whose file is gone is reported as a stray (Pawpocalypse.Audio.Bank fails on it).
"""

import csv
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EDITOR_CMD = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
DEST = "/Game/Pawpocalypse/Audio"


def asset_name(file_name: str) -> str:
    stem = os.path.splitext(file_name)[0]
    return "SW_" + re.sub(r"_+", "_", re.sub(r"[^A-Za-z0-9]+", "_", stem)).strip("_")


def inside_editor():
    import unreal
    eal = unreal.EditorAssetLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    log = unreal.log_warning
    failed = []
    project = unreal.Paths.project_dir()
    waves = {}
    for group, folder, loop in (("sfx", DEST + "/Sfx", False), ("music", DEST + "/Music", True)):
        src = os.path.join(project, "SourceAudio", group)
        files = sorted(os.listdir(src))
        tasks = []
        for f in files:
            t = unreal.AssetImportTask()
            t.filename = os.path.join(src, f)
            t.destination_path = folder
            t.destination_name = asset_name(f)
            t.automated = True
            t.replace_existing = True
            t.save = False
            tasks.append(t)
        tools.import_asset_tasks(tasks)
        wanted = set()
        for f in files:
            path = folder + "/" + asset_name(f)
            wanted.add(asset_name(f))
            w = eal.load_asset(path)
            if not w:
                failed.append("import " + path)
                continue
            w.set_editor_property("looping", loop)
            if not eal.save_asset(path, only_if_is_dirty=False):
                failed.append("save " + path)
            waves[(group, f)] = w
        for p in eal.list_assets(folder, recursive=False):
            if p.split(".")[-1] not in wanted:
                log("IMPORT stray (no file): " + p)
        log("IMPORT %s: %d sounds in %s" % (group, len(files), folder))
    bank_path = DEST + "/DA_AudioBank"
    bank = eal.load_asset(bank_path) if eal.does_asset_exist(bank_path) else None
    if bank is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.PawAudioBank)
        bank = tools.create_asset("DA_AudioBank", DEST, unreal.PawAudioBank, factory)
    events = []
    with open(os.path.join(project, "Data", "AudioEvents.csv"), encoding="utf-8") as fh:
        for r in csv.DictReader(fh):
            e = unreal.PawAudioEvent()
            e.set_editor_property("id", r["event"])
            e.set_editor_property("min_interval", float(r["min_interval"]))
            e.set_editor_property("pitch", float(r["pitch"]))
            e.set_editor_property("volume_db", float(r["volume_db"]))
            sounds = []
            for f in [s for s in r["sounds"].split(";") if s]:
                if ("sfx", f) not in waves:
                    failed.append("event %s plays %s, not imported" % (r["event"], f))
                    continue
                sounds.append(waves[("sfx", f)])
            e.set_editor_property("sounds", sounds)
            events.append(e)
    tracks = []
    with open(os.path.join(project, "Data", "AudioMusic.csv"), encoding="utf-8") as fh:
        for r in csv.DictReader(fh):
            t = unreal.PawAudioTrack()
            t.set_editor_property("id", r["track"])
            if ("music", r["sound"]) not in waves:
                failed.append("track %s plays %s, not imported" % (r["track"], r["sound"]))
                continue
            t.set_editor_property("sound", waves[("music", r["sound"])])
            tracks.append(t)
    bank.set_editor_property("events", events)
    bank.set_editor_property("music", tracks)
    if not eal.save_asset(bank_path, only_if_is_dirty=False):
        failed.append("save " + bank_path)
    log("IMPORT bank: %d events, %d tracks" % (len(events), len(tracks)))
    for f in failed:
        log("IMPORT FAILED " + f)
    log("IMPORT FAILED" if failed else "IMPORT DONE")


def from_shell():
    sys.path.insert(0, str(ROOT / "tools"))
    import unreal_procs
    if unreal_procs.blocking():
        print("import_audio: an Unreal process of this project is running; close it first")
        return 2
    script = (ROOT / "tools" / "import_audio.py").as_posix()
    cmd = [EDITOR_CMD, str(ROOT / "Pawpocalypse.uproject"), "-run=pythonscript", "-script=%s" % script,
           "-nullrhi", "-unattended", "-nop4", "-SCCProvider=None", "-stdout"]
    r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    lines = sorted({l[l.index("IMPORT "):] for l in r.stdout.splitlines() if "IMPORT " in l and "LogInit" not in l})
    for l in lines:
        print(l)
    if "IMPORT DONE" not in lines:
        log = ROOT / "build" / "import_audio.log"
        log.parent.mkdir(exist_ok=True)
        log.write_text(r.stdout + "\n--- stderr ---\n" + r.stderr, encoding="utf-8")
        print("import_audio: FAILED (log: %s)" % log)
        return 1
    return 0


if __name__ == "__main__":
    if "--run" in sys.argv[1:]:
        sys.exit(from_shell())
    inside_editor()
