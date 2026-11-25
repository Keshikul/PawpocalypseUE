"""The Steam page's moving pictures, recorded in the real game: text-free GIFs, the description clips and the trailer.

    py tools/store_clips.py                      # record every take, then cut everything
    py tools/store_clips.py --only hunt boss     # record these takes again (then cut everything)
    py tools/store_clips.py --cut                # cut only, from the takes on disk
    py tools/store_clips.py --list               # the takes and what each shows

RECORD (2026-11-25). One run of the game per take: the editor build with -game, as tools/store_shots.py, a FRESH sandboxed
save folder each (build/steam/clips/saves: never the designer's saves), started with -UseFixedTimeStep -FPS=60 so every
frame is 1/60 s of game time however long its file takes to write (KB U/18 §4): the frames play back at the game's real
speed. Each take is set up by the paw.* steps of the tour queue (Source/Pawpocalypse/Dev/PawShot.cpp); paw.Clip writes every
frame as a BMP into Saved/PawClips/<take>/, which is packed here into build/steam/clips/takes/<take>.mkv (lossless RGB) and
deleted.

NO TEXT IN THE GIFS (designer, 2026-11-25: "Make sure there is no text in gifs"). A take without `ui` is captured without a
single widget (FScreenshotRequest's bShowUI false: no HUD, no dock, no numbers, no prompts) and the field draws no text of
its own (no damage numbers: checked 2026-11-25). Those takes run with paw.Dock off (the field the whole window wide, so no
empty strip where the dock was) and paw.AutoPick (a level up takes a card without holding the field under a hand nobody
sees). Every GIF and every Steam clip comes from those takes; the `ui` takes (the dock, the cards, the tree) are the
trailer's alone.

CUT, into build/steam/clips/:
  gif/<name>.gif                  780 px (Steam's description column), 15 fps, each under 8 MB (Steam takes animated
                                  uploads up to 100 MB and makes video of them since 2025-08, Steam/05: the old 5 MB a GIF
                                  is gone; at 5 MB the busiest takes fell to 10 fps and 640 px)
  steam/<name>.webm + .mp4        Steam's description clips: 1170x658, 30 fps, at most 12 s, each under 3 MB (Steam/05)
  trailer/pawpocalypse_trailer.mp4   1920x1080 at 60 fps, H.264 + AAC: the takes cut on the score's bar lines, the logo last
  contact/<name>.jpg              a contact sheet of every GIF and clip (what to look at before anything is uploaded)

THE SCORE: Loyalty Freak Music, "Can't Stop My Feet !" (ROLLER DISCO DANCE DANCE), CC0, the first try's trailer score
(its bpm 130 and first downbeat 0.924 s were measured then). Copied into build/steam/clips/music on first use; nothing here
ships. The game's own sound is not in the takes (recording it with the fixed step is not solved: KB U/18 §4).
"""
import argparse
import json
import shutil
import subprocess
import sys
import time
from pathlib import Path

import imageio_ffmpeg

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import unreal_procs  # noqa: E402

EDITOR = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
FF = imageio_ffmpeg.get_ffmpeg_exe()
GIFSICLE = Path("C:/Users/Keshi/tools/gifsicle/gifsicle.exe")
OUT = ROOT / "build" / "steam" / "clips"
TAKES = OUT / "takes"
SAVES = OUT / "saves"
FRAMES = ROOT / "Saved" / "PawClips"
MUSIC_SRC = Path("C:/Games/pawpocalypse/build/trailer/music/cant_stop_my_feet.ogg")   # read only (the first try's build)
MUSIC = OUT / "music" / "cant_stop_my_feet.ogg"
BPM, DOWNBEAT = 130.0, 0.924
BAR = 4 * 60.0 / BPM
FPS = 60

# Slate.UseFixedDeltaTime: widgets tick with Slate's REAL-time delta even under -UseFixedTimeStep (measured 2026-11-25: the
# Unicorn ceremony, 3.75 s, lasted about 1 s of a take recorded slower than real time); on, every widget tick is 1/60 s.
START = ["paw.DebugHud 0", "paw.BackgroundFps 0", "DisableAllScreenMessages", "Slate.UseFixedDeltaTime 1"]
FIELD = ["paw.Dock off", "paw.AutoPick"]          # a take without the widgets (see the docstring)
CLEAR = ["paw.Take", "paw.Wait 10"] * 10           # store_shots.py: whatever card hands are open, taken
# paw.Row n presses Pc::ShopRowOrder()[n] (PcHud.cpp), which counts Aura Radius (5) even while it is hidden: 0 Paw Damage,
# 1 Attack Speed (at its cap after 2 on a fresh game), 2 Crit Chance, 3 Shred, 4 Execute, 5 Aura Radius, 6 Currency Find,
# 7 Spawn Rate, 8 Kitten Count (2026-11-25: 6 and 7 first bought Currency Find and Spawn Rate, and no kitten came).
STARTER = (["paw.Do paw.Cheat.Catnip 4000", "paw.Tab 1"] + ["paw.Row 0", "paw.Wait 2"] * 6 + ["paw.Row 1", "paw.Wait 2"] * 2
           + ["paw.Row 7", "paw.Wait 2"] * 3 + ["paw.Row 8", "paw.Wait 2"] * 2)


def wait(seconds):
    return ["paw.Wait %d" % round(seconds * FPS)]


# HOW STRONG THE CAT IS DECIDES WHAT A TAKE SHOWS (measured 2026-11-25, PAW_FIELD lines, one game second apart):
# - every upgrade maxed: every enemy dies on arrival and the maxed Kitten Count fills the frame with kittens (the first takes
#   showed no enemy at all); the Park Tyrant from its arrival to its death in ~9.5 s (with a Reawaken multiplier on top it
#   died before the take began);
# - the Backyard with a few upgrades (STARTER) and the aura: 70 -> 32 enemies on screen over 12 s, ~10 kills a second;
# - realm 8 with no upgrades: ~90 enemies, no kill; a Reawaken multiplier alone (1e9..1e15 Stardust ever): 1-2 kills a second
#   (the paw's rate, not its damage, is the limit), so the late takes add the starter upgrades and the aura to it.
#   No kittens there: with the multiplier the two kittens emptied realms 4-11 (2026-11-25), so LATE leaves Kitten Count out.
LATE = (["paw.Do paw.Cheat.UnlockStages"] + [s for s in STARTER if s != "paw.Row 8"]
        + ["paw.Own unlock_aura", "paw.Own aura_damage 3", "paw.Own aura_reach 2", "paw.Ever 1000000000000"])


def montage(name, realms, seconds_each):
    """One take of several realms: each travelled to and filled at 5x unrecorded, then recorded (paw.Clip append)."""
    frames = round(seconds_each * FPS)
    steps = []
    for n in realms:
        steps += (["paw.Travel %d" % n, "paw.Do paw.Cheat.Speed 5"] + wait(4) + ["paw.Do paw.Cheat.Speed 1"] + wait(0.5)
                  + ["paw.Clip %s %d append" % (name, frames), "paw.Wait %d" % (frames + 2)])
    return steps, frames * len(realms)


REALMS_STEPS, REALMS_FRAMES = montage("realms", (2, 4, 5, 6, 9, 11), 1.75)

# name: what it shows, ui (the widgets in the frames), seconds recorded, the steps before the take, the steps during it
# (or `steps` and `frames`: a take that places its own paw.Clip lines, a montage).
TAKES_LIST = {
    "hunt": dict(
        what="the Backyard: a young cat with a few upgrades and two kittens hunting a crowd",
        ui=False, seconds=12,
        pre=FIELD + STARTER
        + ["paw.Do paw.Cheat.Speed 5"] + wait(8) + ["paw.Do paw.Cheat.Speed 1", "paw.Steer hunt 900", "paw.Camera 1.5"] + wait(1),
        during=[]),
    "aura": dict(
        what="the Backyard: the aura round the cat thinning a crowd, two kittens",
        ui=False, seconds=12,
        pre=FIELD + STARTER + ["paw.Own unlock_aura", "paw.Own aura_damage 3", "paw.Own aura_reach 2", "paw.Do paw.Cheat.Speed 5"]
        + wait(8) + ["paw.Do paw.Cheat.Speed 1", "paw.Steer hunt 700", "paw.Camera 1.4"] + wait(0.5),
        during=[]),
    "pounce": dict(
        what="City Rooftops: Pounce slams into crowds",
        ui=False, seconds=10,
        pre=FIELD + LATE + ["paw.Own unlock_pounce", "paw.Own pounce 3", "paw.Travel 4", "paw.Do paw.Cheat.Speed 5"] + wait(6)
        + ["paw.Do paw.Cheat.Speed 1", "paw.Steer hunt 800", "paw.Camera 1.5"] + wait(0.5),
        during=(wait(0.6) + ["paw.Pounce"]) * 15),
    "boss": dict(
        what="the Park Tyrant called with Bait and beaten (~9.5 s in); the cat evolves",
        ui=False, seconds=12,
        pre=FIELD + ["paw.Do paw.Cheat.MaxUpgrades"] + CLEAR + ["paw.Steer boss 900", "paw.Camera 1.25", "paw.Boss"] + wait(0.1),
        during=[]),
    "realms": dict(
        what="six realms, 1.75 s each (travel never loads; each filled at 5x before it is recorded)",
        ui=False, seconds=REALMS_FRAMES / FPS, frames=REALMS_FRAMES,
        steps=FIELD + LATE + ["paw.Steer hunt 900", "paw.Camera 1.4"] + REALMS_STEPS),
    "rider": dict(
        what="the Mythic Realm on the unicorn: the aura and the paw in a crowd",
        ui=False, seconds=12,
        # Not the Unicorn nodes: Rainbow Poop cleared the field (2026-11-25: no enemy left in the take); the last stage rides
        # the unicorn without them. Nor LATE's aura damage and multiplier: at 1.6x the aura covers most of the view, and with
        # them it killed each enemy as it spawned (21 kills a second, none alive).
        pre=FIELD + ["paw.Do paw.Cheat.UnlockStages"] + [s for s in STARTER if s != "paw.Row 8"]
        + ["paw.Own unlock_aura", "paw.Ever 1000000000", "paw.Travel 8", "paw.Do paw.Cheat.Speed 5"] + wait(6)
        + ["paw.Do paw.Cheat.Speed 1", "paw.Steer hunt 800", "paw.Camera 1.6"] + wait(0.5),
        during=[]),
    # The trailer's takes with the widgets: what makes it read as an idle game at a glance.
    "upgrades_ui": dict(
        what="the dock's Upgrades page: levels bought while the cat hunts",
        ui=True, seconds=7,
        pre=["paw.AutoPick", "paw.Do paw.Cheat.Catnip 6000", "paw.Tab 1", "paw.Do paw.Cheat.Speed 5"] + wait(6)
        + ["paw.Do paw.Cheat.Speed 1", "paw.Steer hunt 800"] + wait(1),
        during=[s for r in (0, 0, 0, 1, 1, 7, 8, 0, 2, 3) for s in wait(0.45) + ["paw.Row %d" % r]]),
    "cards_ui": dict(
        what="a level up: three cards, one taken",
        ui=True, seconds=6,
        pre=STARTER + ["paw.Steer hunt 800"] + wait(4) + CLEAR,
        during=wait(0.8) + ["paw.Hand"] + wait(2.6) + ["paw.Take"]),
    "boss_ui": dict(
        what="the Park Tyrant's fight with the HUD, beaten ~9.5 s in",
        ui=True, seconds=12,
        pre=["paw.AutoPick", "paw.Do paw.Cheat.MaxUpgrades"] + CLEAR + ["paw.Steer boss 900", "paw.Boss"] + wait(0.1),
        during=[]),
    "reawaken_ui": dict(
        what="a Reawaken: the ceremony, then the skill tree's spend step, nodes bought",
        ui=True, seconds=11,
        pre=["paw.AutoPick", "paw.Do paw.Cheat.UnlockStages", "paw.Travel 3", "paw.Do paw.Cheat.MaxUpgrades",
             "paw.Steer hunt 800", "paw.Tab 2"] + wait(4),
        during=wait(0.6) + ["paw.Reawaken jobs"] + wait(4.4) + ["paw.Zoom 0.4"] + wait(0.4)
        + [s for b in ("claws", "claws", "instinct", "unlock_aura", "aura_damage", "fortune", "vitality", "unlock_pounce")
           for s in ["paw.Buy %s" % b] + wait(0.45)]),
    "unicorn_ui": dict(
        what="Unlock Unicorn bought: the ceremony",
        ui=True, seconds=6,
        pre=["paw.AutoPick", "paw.Tab 2", "paw.Reawaken jobs"] + wait(6) + ["paw.Do paw.Cheat.Stardust 100000"] + wait(1),
        during=wait(0.4) + ["paw.Buy unlock_unicorn"]),
}

# GIFs and Steam clips: (take, start s, length s). Text-free takes only.
# The boss takes start as the Park Tyrant arrives; it dies ~6 s in (PAW_FIELD, 2026-11-25) and the cat evolves after.
GIFS = {
    "hunt": ("hunt", 1.0, 6.0),
    "aura": ("aura", 1.0, 6.0),
    "pounce": ("pounce", 1.0, 6.0),
    "boss": ("boss", 0.5, 7.5),
    "realms": ("realms", 0.0, 7.0),      # four realms of the montage's six
    "rider": ("rider", 1.0, 6.0),
}
CLIPS = {
    "hunt": ("hunt", 0.5, 10.0),
    "aura": ("aura", 0.5, 10.0),
    "pounce": ("pounce", 0.5, 9.0),
    "boss": ("boss", 0.0, 10.0),
    "realms": ("realms", 0.0, 10.5),
    "rider": ("rider", 0.5, 10.0),
}
# The trailer: (take, start s, bars). Cuts land on bar lines. The music opens at START_BAR of the track, so the song's own drop
# (its bar 26, measured for the first try's trailer) falls on the first cut after the HUD's loop: the aura.
TRAILER = [
    ("hunt", 0.5, 3),
    ("upgrades_ui", 0.3, 2),
    ("cards_ui", 0.6, 2),
    ("aura", 0.5, 3),          # the drop
    ("pounce", 0.5, 2),
    ("boss_ui", 1.0, 4),       # the fight, the kill (~6 s in), the cat evolving
    ("realms", 0.0, 3),
    ("reawaken_ui", 0.4, 4),   # the press, the ceremony, the tree zoomed in, nodes bought
    ("unicorn_ui", 0.2, 2),
    ("rider", 0.5, 3),
]
DROP_BAR = 26
START_BAR = DROP_BAR - sum(b for _t, _s, b in TRAILER[:3])
END_BARS = 3
LOGO = Path("G:/My Drive/Games/Pawpocalypse/Promotional/Logo/library_logo.png")


def run(cmd, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, **kw)
    if r.returncode != 0:
        sys.exit("store_clips: failed: %s\n%s" % (" ".join(str(c) for c in cmd[:6]), r.stderr[-2000:]))
    return r


def record(name, take):
    if "steps" in take:
        frames = take["frames"]
        steps = START + take["steps"] + ["paw.Quit"]
    else:
        frames = round(take["seconds"] * FPS)
        steps = (START + take["pre"] + ["paw.Clip %s %d%s" % (name, frames, " ui" if take["ui"] else "")]
                 + take["during"] + ["paw.Quit"])
    if SAVES.exists():
        shutil.rmtree(SAVES)
    SAVES.mkdir(parents=True)
    out = FRAMES / name
    if out.exists():
        shutil.rmtree(out)
    log = "PawStoreClips_%s.log" % name
    cmd = [EDITOR, str(ROOT / "Pawpocalypse.uproject"), "/Game/Pawpocalypse/Maps/L_Realm", "-game", "-windowed",
           "-ResX=1920", "-ResY=1080", "-unattended", "-NoSave", "-nosplash", "-nosound", "-SCCProvider=None",
           "-UseFixedTimeStep", "-FPS=%d" % FPS, "-PawSaveDir=%s" % SAVES, "-ExecCmds=" + ",".join(steps), "-log=" + log]
    t0 = time.time()
    # The editor build's -game run crashes AFTER its log closes on every quit (0xC0000005, 2026-11-25, also with a plain
    # `quit`; KB U/18 §4) and its crash reporter then holds the process about a minute: once the log has closed, the run is
    # over. A take whose steps stall (an unknown command: a stale build) would wait forever: an hour at most, every step is
    # in the log (PAW_STEP).
    log_path = ROOT / "Saved" / "Logs" / log
    log_path.unlink(missing_ok=True)
    proc = subprocess.Popen(cmd)
    while proc.poll() is None and time.time() - t0 < 3600:
        time.sleep(1)
        if log_path.exists() and "Log file closed" in log_path.read_text(encoding="utf-8", errors="replace")[-400:]:
            time.sleep(2)
            break
    if proc.poll() is None:
        proc.kill()
        proc.wait()
    got = sorted(out.glob("f_*.bmp")) if out.exists() else []
    print("store_clips: %s: %d of %d frames in %.0f s" % (name, len(got), frames, time.time() - t0))
    warnings = [ln for ln in (ROOT / "Saved" / "Logs" / log).read_text(encoding="utf-8", errors="replace").splitlines()
                if "LogPaw: Warning" in ln and "paw.Take: no card taken" not in ln]
    for ln in warnings[:12]:
        print("   ", ln.strip())
    if len(got) != frames:
        return False
    TAKES.mkdir(parents=True, exist_ok=True)
    run([FF, "-y", "-v", "error", "-framerate", str(FPS), "-i", str(out / "f_%05d.bmp"),
         "-c:v", "libx264rgb", "-crf", "0", "-preset", "ultrafast", str(TAKES / (name + ".mkv"))])
    shutil.rmtree(out)
    (TAKES / (name + ".json")).write_text(json.dumps(dict(take, steps=steps), indent=1), encoding="utf-8")
    return True


def size_mb(p):
    return p.stat().st_size / (1024 * 1024)


def cut_gif(name, take, start, dur):
    out = OUT / "gif" / (name + ".gif")
    out.parent.mkdir(parents=True, exist_ok=True)
    src = TAKES / (take + ".mkv")
    for fps, lossy, width in ((15, 40, 780), (15, 80, 780), (12, 80, 780), (12, 120, 700), (10, 140, 640)):
        raw = out.with_suffix(".raw.gif")
        graph = ("fps=%d,scale=%d:-2:flags=lanczos,split[a][b];[a]palettegen=max_colors=256:stats_mode=diff[p];"
                 "[b][p]paletteuse=dither=bayer:bayer_scale=4:diff_mode=rectangle" % (fps, width))
        run([FF, "-y", "-v", "error", "-ss", "%.3f" % start, "-t", "%.3f" % dur, "-i", str(src),
             "-filter_complex", graph, "-loop", "0", str(raw)])
        if GIFSICLE.exists():
            run([str(GIFSICLE), "-O3", "--lossy=%d" % lossy, "-o", str(out), str(raw)])
            raw.unlink()
        else:
            raw.replace(out)
        if out.stat().st_size < 8_000_000:   # under 8 MB however it is counted (decimal or binary)
            break
    print("  gif   %-8s %4.1f s  %d fps %d px  %.2f MB" % (name, dur, fps, width, size_mb(out)))
    return out


def cut_clip(name, take, start, dur):
    dur = min(dur, 12.0)   # Steam's limit for an animated description asset
    folder = OUT / "steam"
    folder.mkdir(parents=True, exist_ok=True)
    src = TAKES / (take + ".mkv")
    vf = "fps=30,scale=1170:658:flags=lanczos,format=yuv420p"
    for crf_vp9, crf_264 in ((34, 23), (38, 26), (42, 29), (46, 32)):
        webm, mp4 = folder / (name + ".webm"), folder / (name + ".mp4")
        run([FF, "-y", "-v", "error", "-ss", "%.3f" % start, "-t", "%.3f" % dur, "-i", str(src), "-vf", vf, "-an",
             "-c:v", "libvpx-vp9", "-b:v", "0", "-crf", str(crf_vp9), "-row-mt", "1", "-deadline", "good", "-cpu-used", "2",
             str(webm)])
        run([FF, "-y", "-v", "error", "-ss", "%.3f" % start, "-t", "%.3f" % dur, "-i", str(src), "-vf", vf, "-an",
             "-c:v", "libx264", "-crf", str(crf_264), "-preset", "slow", "-movflags", "+faststart", str(mp4)])
        if size_mb(webm) < 3.0 and size_mb(mp4) < 3.0:
            break
    print("  clip  %-8s %4.1f s  webm %.2f MB  mp4 %.2f MB" % (name, dur, size_mb(webm), size_mb(mp4)))
    return mp4


def contact(path, name, dur):
    """Eight frames of a cut in one picture: what is looked at before anything is uploaded."""
    folder = OUT / "contact"
    folder.mkdir(parents=True, exist_ok=True)
    step = max(dur / 8.0, 0.1)
    run([FF, "-y", "-v", "error", "-i", str(path), "-vf", "fps=1/%.3f,scale=480:-2,tile=4x2" % step, "-frames:v", "1",
         str(folder / (name + ".jpg"))])


def music():
    if not MUSIC.exists():
        if not MUSIC_SRC.exists():
            sys.exit("store_clips: no score at %s" % MUSIC_SRC)
        MUSIC.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(MUSIC_SRC, MUSIC)
    return MUSIC


def cut_trailer():
    folder = OUT / "trailer"
    folder.mkdir(parents=True, exist_ok=True)
    parts = []
    for i, (take, start, bars) in enumerate(TRAILER):
        part = folder / ("part_%02d.mkv" % i)
        # The frame count of each cut is rounded on its own END bar, so the cuts never drift off the beat.
        b0 = sum(b for _t, _s, b in TRAILER[:i])
        frames = round((b0 + bars) * BAR * FPS) - round(b0 * BAR * FPS)
        run([FF, "-y", "-v", "error", "-ss", "%.3f" % start, "-i", str(TAKES / (take + ".mkv")), "-frames:v", str(frames),
             "-vf", "scale=1920:1080,format=yuv444p", "-c:v", "libx264", "-crf", "10", "-preset", "fast", "-r", str(FPS),
             str(part)])
        parts.append(part)
    # The end card: the library logo over the last take's final frame, dimmed, fading in.
    body_bars = sum(b for _t, _s, b in TRAILER)
    end_frames = round((body_bars + END_BARS) * BAR * FPS) - round(body_bars * BAR * FPS)
    last = TRAILER[-1]
    still = folder / "end_still.png"
    run([FF, "-y", "-v", "error", "-ss", "%.3f" % (last[1] + last[2] * BAR), "-i", str(TAKES / (last[0] + ".mkv")),
         "-frames:v", "1", str(still)])
    end = folder / "part_end.mkv"
    graph = ("[0:v]scale=1920:1080,eq=brightness=-0.18:saturation=0.75,boxblur=6[bg];"
             "[1:v]format=rgba,fade=t=in:st=0.15:d=0.5:alpha=1[logo];"
             "[bg][logo]overlay=(W-w)/2:(H-h)/2,format=yuv444p")
    run([FF, "-y", "-v", "error", "-loop", "1", "-framerate", str(FPS), "-i", str(still), "-loop", "1", "-framerate", str(FPS),
         "-i", str(LOGO), "-filter_complex", graph, "-frames:v", str(end_frames), "-c:v", "libx264", "-crf", "10",
         "-preset", "fast", "-r", str(FPS), str(end)])
    parts.append(end)
    listing = folder / "parts.txt"
    listing.write_text("".join("file '%s'\n" % p.as_posix() for p in parts), encoding="utf-8")
    total = (body_bars + END_BARS) * BAR
    score_from = DOWNBEAT + START_BAR * BAR
    out = folder / "pawpocalypse_trailer.mp4"
    run([FF, "-y", "-v", "error", "-f", "concat", "-safe", "0", "-i", str(listing), "-ss", "%.4f" % score_from,
         "-i", str(music()), "-map", "0:v", "-map", "1:a", "-t", "%.4f" % total,
         "-af", "afade=t=in:st=0:d=0.08,afade=t=out:st=%.4f:d=%.4f" % (total - 2 * BAR, 2 * BAR),
         "-c:v", "libx264", "-crf", "18", "-preset", "slow", "-pix_fmt", "yuv420p", "-profile:v", "high", "-r", str(FPS),
         "-c:a", "aac", "-b:a", "192k", "-movflags", "+faststart", str(out)])
    for p in parts + [listing, still]:
        p.unlink()
    print("  trailer  %.1f s  %.1f MB  (score from bar %d; the drop on bar %d)" % (total, size_mb(out), START_BAR, DROP_BAR))
    contact(out, "trailer", total)
    return out


def cut_all():
    missing = sorted({t for t, _s, _d in list(GIFS.values()) + list(CLIPS.values()) + TRAILER}
                     - {p.stem for p in TAKES.glob("*.mkv")})
    if missing:
        sys.exit("store_clips: no take for %s (record it first)" % ", ".join(missing))
    for name, (take, start, dur) in GIFS.items():
        if TAKES_LIST[take]["ui"]:
            sys.exit("store_clips: the GIF %s is cut from a take with the widgets (no text in the GIFs)" % name)
        contact(cut_gif(name, take, start, dur), "gif_" + name, dur)
    for name, (take, start, dur) in CLIPS.items():
        if TAKES_LIST[take]["ui"]:
            sys.exit("store_clips: the clip %s is cut from a take with the widgets" % name)
        contact(cut_clip(name, take, start, dur), "clip_" + name, min(dur, 12.0))
    cut_trailer()


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--only", nargs="*", help="record only these takes")
    ap.add_argument("--cut", action="store_true", help="cut only, from the takes on disk")
    ap.add_argument("--list", action="store_true")
    args = ap.parse_args(argv)
    if args.list:
        for name, t in TAKES_LIST.items():
            print("%-12s %-5s %4.1f s  %s" % (name, "ui" if t["ui"] else "field", t["seconds"], t["what"]))
        return 0
    if not args.cut:
        found = unreal_procs.blocking()
        if found:
            print("store_clips: an Unreal process of this project is running; close it first")
            unreal_procs.print_blocking(found)
            return 2
        names = args.only or list(TAKES_LIST)
        unknown = [n for n in names if n not in TAKES_LIST]
        if unknown:
            sys.exit("store_clips: no take named %s (--list)" % ", ".join(unknown))
        failed = [n for n in names if not record(n, TAKES_LIST[n])]
        if failed:
            print("store_clips: FAILED takes: %s" % ", ".join(failed))
            return 1
        if args.only:
            return 0
    cut_all()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
