"""Package the game (docs/10 §2; phase 8): UAT BuildCookRun into a CLEAN archive, then the archive audited, the testers'
files written into it, and the packaged gates run on it.

    py tools/package.py [--config Development|Shipping] [--no-gates] [--bump]
    py tools/package.py --audit [ARCHIVE]   # the audit alone, on a package already made (reads it; writes build/ only)
    py tools/package.py --self-test         # the audit's rules against planted listings (nothing packaged)

Packaging is the designer's word (CLAUDE.md): this never runs from check_all; check_all runs --self-test.

The run, in order:
  1. Refuses (exit 2) while an editor of this project, a packaged copy (it locks the exe: UAT then keeps the OLD exe and
     reports success, KB U/19 §2) or another UAT is running.
  2. Deletes the archive (Development: build/package, Shipping: build/package_shipping): a package made over an old one kept
     every file the new one no longer has (found 2026-11-23: phase 4's README, "English only, no audio yet", 19 days on).
  3. --bump: the last number of ProjectVersion (Config/DefaultGame.ini) + 1, before the build, so the exe carries it (Godot's
     rule: every build that leaves the PC has its own version). The scheme is 0.MINOR.PATCH from 0.1.0 (the designer,
     2026-11-23: "Use minor type version numbers"); MINOR is raised by hand. Refused while ProjectVersion is unset.
     A package for the gates is not bumped.
  4. UAT BuildCookRun (log: build/package_<config>_<stamp>.log); the archived inner exe must be this source's build: newer
     than the run's start, or (UAT found it up to date: check_all's Shipping build minutes before, 2026-11-23) the very
     binary in Binaries/Win64 (same size and time) with no file under Source/ and no .uproject newer than it.
  5. The .pdb kept beside its exe in build/symbols/<stamp>-<config>/ (the archive has none: -nodebuginfo), for crash dumps.
     Then the archive trimmed of what the engine stages that this game never loads (NOT_NEEDED: the D3D12 and Vulkan debug
     layers, UAT's manifests, a plugin's Boost; 2026-11-23: 46.7 MB of a Development package, 5.4 of a Shipping one).
  6. The audit (below): FAIL stops here.
  7. README.txt and "Send logs.bat" written into the archive (KB U/19 §4).
  8. Development: the packaged gates, `packaged_languages.py`, `packaged_bench.py` and `packaged_pso.py --route bench` (the
     PSO validation over one full field; the whole-game route is run by hand) (their reports in build/). Shipping
     has no console: no gate runs in it yet.

The audit (also --audit): the archive's files, the IoStore container's packages (UnrealPak -List) and the pak's loose files.
  FAIL  a Saved folder or a .pdb in the archive; a /Game package outside /Game/Pawpocalypse (Developers, Collections...);
        a map other than L_Title / L_Realm (in Shipping; Development may carry test maps); either map missing; a string table
        or a culture's translation file missing (every culture folder of Content/Pawpocalypse/Localization/Translations);
        a paw cursor missing; ICU data without Thai's dictionary or zh-Hant (the English preset: HANDOFF-LOG (136)); no exe.
  FAIL  ... and a file the trim (step 5) removes left in the archive (the D3D12 / Vulkan debug layers, UAT's manifests,
        a plugin's Boost: the game never loads them).
Exit 0 = PASS; 1 = FAIL; 2 = refused or no package.
"""
import argparse
import datetime
import fnmatch
import os
import re
import shutil
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import unreal_procs  # noqa: E402

ENGINE = r"C:\Program Files\Epic Games\UE_5.8\Engine"
UAT = os.path.join(ENGINE, "Build", "BatchFiles", "RunUAT.bat")
UNREALPAK = os.path.join(ENGINE, "Binaries", "Win64", "UnrealPak.exe")
UPROJECT = os.path.join(ROOT, "Pawpocalypse.uproject")
GAME_INI = os.path.join(ROOT, "Config", "DefaultGame.ini")
CONTENT = os.path.join(ROOT, "Content", "Pawpocalypse")
ARCHIVES = {"Development": os.path.join(ROOT, "build", "package"),
            "Shipping": os.path.join(ROOT, "build", "package_shipping")}
EXES = {"Development": "Pawpocalypse/Binaries/Win64/Pawpocalypse.exe",
        "Shipping": "Pawpocalypse/Binaries/Win64/Pawpocalypse-Win64-Shipping.exe"}
PDBS = {"Development": "Pawpocalypse.pdb", "Shipping": "Pawpocalypse-Win64-Shipping.pdb"}
MAPS = {"Pawpocalypse/Content/Pawpocalypse/Maps/L_Title.umap", "Pawpocalypse/Content/Pawpocalypse/Maps/L_Realm.umap"}
TABLES = ("ST_UI.csv", "ST_Data.csv", "ST_Tree.csv")
LOC = "Pawpocalypse/Content/Pawpocalypse/Localization"
# ICU files only the All preset stages (HANDOFF-LOG (136): with English, zh-Hant refused and Thai never wrapped).
ICU_NEEDED = ("Engine/Content/Internationalization/icudt64l/brkitr/thaidict.dict",
              "Engine/Content/Internationalization/icudt64l/zh_Hant.res")
# Staged by the engine, not needed by this game: reported with their size (the size work), never failed.
# Boost came only with USD (FabLauncher pulled it in until the trim ran again, 2026-11-23); TBB is linked by the engine
# itself (still in the receipt after the trim): needed, not listed.
NOT_NEEDED = ("*/d3d12SDKLayers.dll", "*/VkLayer_*", "*/boost_*.dll", "Manifest_*.txt")


# --- the audit ---------------------------------------------------------------------------------------------------------

def expected_cultures():
    tr = os.path.join(CONTENT, "Localization", "Translations")
    return sorted(d for d in os.listdir(tr) if os.path.isdir(os.path.join(tr, d)))


def expected_cursors():
    folder = os.path.join(CONTENT, "UI", "Cursors")
    return sorted(f for f in os.listdir(folder) if f.lower().endswith(".png"))


def audit(files, packages, loose, config, cultures, cursors):
    """files: {archive-relative path ('/'): size}; packages: the IoStore's paths; loose: the pak's paths (both relative to
    the mount point, 'Pawpocalypse/Content/...'). Returns (failures, warnings)."""
    fail, warn = [], []
    norm = {p.replace("\\", "/"): s for p, s in files.items()}
    if EXES[config] not in norm:
        fail.append("no exe: %s" % EXES[config])
    saved = {}
    for p in sorted(norm):
        parts = p.split("/")
        if "Saved" in parts[:-1]:
            root = "/".join(parts[:parts.index("Saved") + 1])
            saved.setdefault(root, p)
    for root, first in saved.items():
        fail.append("a Saved folder in the archive: %s (%s)" % (root, first))
    for p in sorted(norm):
        if p.lower().endswith(".pdb"):
            fail.append("a .pdb in the archive: %s" % p)
    for p in sorted(packages) + sorted(loose):
        # The cook's shader library sits at the content root: ShaderArchive-*.ushaderbytecode, ShaderTypeInfo-*.stinfo.
        if p.startswith("Pawpocalypse/Content/") and not p.startswith("Pawpocalypse/Content/Pawpocalypse/") \
                and not re.match(r"Pawpocalypse/Content/Shader(Archive|TypeInfo)-[^/]+$", p):
            fail.append("outside /Game/Pawpocalypse: %s" % p)
        if re.search(r"/(Developers|Collections|Screenshots|Saved)/", p):
            fail.append("never ships: %s" % p)
    maps = {p for p in packages if p.endswith(".umap") and p.startswith("Pawpocalypse/")}
    for m in sorted(MAPS - maps):
        fail.append("map not cooked: %s" % m)
    for m in sorted(maps - MAPS):
        (fail if config == "Shipping" else warn).append("a map not shipped: %s" % m)
    have = set(loose)
    for t in TABLES:
        if "%s/Strings/%s" % (LOC, t) not in have:
            fail.append("string table not staged: %s" % t)
    for c in cultures:
        for t in TABLES:
            if "%s/Translations/%s/%s" % (LOC, c, t) not in have:
                fail.append("translation not staged: %s/%s" % (c, t))
    for c in cursors:
        if "Pawpocalypse/Content/Pawpocalypse/UI/Cursors/%s" % c not in have:
            fail.append("cursor not staged: %s" % c)
    for f in ICU_NEEDED:
        if f not in have:
            fail.append("ICU data without %s (InternationalizationPreset is not All?)" % f.rsplit("/", 1)[1])
    extra = [(p, s) for p, s in sorted(norm.items()) if any(fnmatch.fnmatch(p, pat) for pat in NOT_NEEDED)]
    if extra:
        fail.append("staged, not needed by this game (the trim removes them): %d files, %.1f MB (%s)" % (
            len(extra), sum(s for _, s in extra) / 1e6, ", ".join(sorted({p.rsplit("/", 1)[-1].split("-")[0].split("_")[0]
                                                                         for p, _ in extra}))))
    return fail, warn


def archive_files(archive):
    out = {}
    base = os.path.join(archive, "Windows")
    for d, _, names in os.walk(base):
        for n in names:
            full = os.path.join(d, n)
            out[os.path.relpath(full, base).replace("\\", "/")] = os.path.getsize(full)
    return out


def listing(container):
    """UnrealPak -List: the paths in a .utoc or .pak, relative to its mount point ('../../../' gone)."""
    r = subprocess.run([UNREALPAK, container, "-List"], capture_output=True, text=True, encoding="utf-8", errors="replace")
    if r.returncode != 0:
        raise RuntimeError("UnrealPak -List %s: exit %d" % (container, r.returncode))
    paths = []
    for m in re.finditer(r'Display: "([^"<][^"]*)" offset:', r.stdout):
        p = m.group(1)
        while p.startswith("../"):
            p = p[3:]
        paths.append(p)
    return paths


def run_audit(archive, config):
    paks = os.path.join(archive, "Windows", "Pawpocalypse", "Content", "Paks")
    utoc, pak = os.path.join(paks, "Pawpocalypse-Windows.utoc"), os.path.join(paks, "Pawpocalypse-Windows.pak")
    if not (os.path.isfile(utoc) and os.path.isfile(pak)):
        print("package: no package to audit at %s" % archive)
        return 2
    files = archive_files(archive)
    packages, loose = listing(utoc), listing(pak)
    fail, warn = audit(files, packages, loose, config, expected_cultures(), expected_cursors())
    report = os.path.join(ROOT, "build", "package_audit_%s.txt" % config)
    os.makedirs(os.path.dirname(report), exist_ok=True)
    total = sum(files.values())
    exe = files.get(EXES[config], 0)
    paksize = sum(s for p, s in files.items() if p.startswith("Pawpocalypse/Content/Paks/"))
    with open(report, "w", encoding="utf-8") as f:
        f.write("archive %s (%s)\n" % (archive, config))
        f.write("size %.1f MB: exe %.1f, paks %.1f, other %.1f\n" % (total / 1e6, exe / 1e6, paksize / 1e6,
                                                                    (total - exe - paksize) / 1e6))
        f.write("%d archive files, %d packages, %d loose files\n" % (len(files), len(packages), len(loose)))
        for line in fail:
            f.write("FAIL " + line + "\n")
        for line in warn:
            f.write("WARN " + line + "\n")
        f.write("\n# archive files\n")
        for p, s in sorted(files.items()):
            f.write("%10d  %s\n" % (s, p))
        f.write("\n# IoStore packages\n" + "\n".join(sorted(packages)) + "\n\n# pak loose files\n" + "\n".join(sorted(loose)) + "\n")
    print("audit: %.1f MB (exe %.1f, paks %.1f); %d packages, %d loose files -> %s" % (
        total / 1e6, exe / 1e6, paksize / 1e6, len(packages), len(loose), report))
    for line in warn:
        print("  WARN " + line)
    for line in fail[:40]:
        print("  FAIL " + line)
    if len(fail) > 40:
        print("  ... %d more in the report" % (len(fail) - 40))
    print("audit: %s" % ("FAIL (%d)" % len(fail) if fail else "PASS"))
    return 1 if fail else 0


def self_test():
    """The audit's rules on a clean listing, then one planted fault at a time: each must FAIL, the clean one PASS."""
    cultures, cursors = ["de", "th", "zh-Hant"], ["ui_cursor_paw_open.png", "ui_cursor_paw_closed.png"]

    def clean(config):
        files = {EXES[config]: 100, "Pawpocalypse/Content/Paks/Pawpocalypse-Windows.utoc": 10, "Pawpocalypse.exe": 5,
                 "Pawpocalypse/Binaries/Win64/D3D12/x64/D3D12Core.dll": 7}
        packages = sorted(MAPS) + ["Pawpocalypse/Content/Pawpocalypse/Data/DT_Cards.uasset",
                                   "Pawpocalypse/Content/ShaderArchive-Pawpocalypse-PCD3D_SM6-PCD3D_SM6.ushaderbytecode",
                                   "Engine/Content/EngineMaterials/DefaultMaterial.uasset"]
        loose = ["%s/Strings/%s" % (LOC, t) for t in TABLES] + ["%s/Translations/%s/%s" % (LOC, c, t) for c in cultures
                                                                for t in TABLES]
        loose += ["Pawpocalypse/Content/Pawpocalypse/UI/Cursors/%s" % c for c in cursors] + list(ICU_NEEDED)
        return files, packages, loose

    def plant(config, how):
        files, packages, loose = clean(config)
        how(files, packages, loose)
        return audit(files, packages, loose, config, cultures, cursors)

    plants = {
        "a Saved folder": ("a Saved folder", "Development", lambda f, p, l: f.__setitem__("Pawpocalypse/Saved/Logs/Pawpocalypse.log", 1)),
        "a .pdb": ("a .pdb", "Development", lambda f, p, l: f.__setitem__("Pawpocalypse/Binaries/Win64/Pawpocalypse.pdb", 1)),
        "no exe": ("no exe", "Shipping", lambda f, p, l: f.pop(EXES["Shipping"])),
        "a Developers package": ("never ships", "Development", lambda f, p, l: p.append(
            "Pawpocalypse/Content/Pawpocalypse/Developers/Keshi/Test.uasset")),
        "a package outside /Game/Pawpocalypse": ("outside /Game/Pawpocalypse", "Development", lambda f, p, l: p.append(
            "Pawpocalypse/Content/StarterContent/Props/SM_Chair.uasset")),
        "a test map in Shipping": ("a map not shipped", "Shipping", lambda f, p, l: p.append("Pawpocalypse/Content/Pawpocalypse/Maps/L_Test.umap")),
        "L_Realm not cooked": ("map not cooked", "Development", lambda f, p, l: p.remove("Pawpocalypse/Content/Pawpocalypse/Maps/L_Realm.umap")),
        "a string table not staged": ("string table not staged", "Development", lambda f, p, l: l.remove("%s/Strings/ST_Tree.csv" % LOC)),
        "a translation not staged": ("translation not staged: th/ST_Data", "Development", lambda f, p, l: l.remove("%s/Translations/th/ST_Data.csv" % LOC)),
        "a cursor not staged": ("cursor not staged", "Development", lambda f, p, l: l.remove("Pawpocalypse/Content/Pawpocalypse/UI/Cursors/ui_cursor_paw_closed.png")),
        "the English ICU preset (no Thai dictionary)": ("without thaidict", "Development", lambda f, p, l: l.remove(ICU_NEEDED[0])),
        "the English ICU preset (no zh-Hant)": ("without zh_Hant", "Development", lambda f, p, l: l.remove(ICU_NEEDED[1])),
        "a debug layer left in the archive": ("not needed by this game", "Shipping",
            lambda f, p, l: f.__setitem__("Pawpocalypse/Binaries/Win64/D3D12/x64/d3d12SDKLayers.dll", 7)),
    }
    ok = True
    for config in ("Development", "Shipping"):
        fail, warn = audit(*clean(config), config, cultures, cursors)
        good = not fail
        print("%s  clean %s listing passes" % ("PASS" if good else "FAIL", config))
        ok &= good
    fail, _ = plant("Development", lambda f, p, l: p.append("Pawpocalypse/Content/Pawpocalypse/Maps/L_Test.umap"))
    print("%s  a test map in Development is only warned" % ("PASS" if not fail else "FAIL"))
    ok &= not fail
    for name, (expect, config, how) in plants.items():
        fail, _ = plant(config, how)
        caught = any(expect in f for f in fail)
        print("%s  %s%s" % ("CAUGHT" if caught else "MISSED", name, "" if caught else " (%s)" % fail))
        ok &= caught
    # The listing parser on UnrealPak's own line shapes (IoStore and pak, measured 2026-11-23).
    sample = ('LogIoStore: Display: "../../../Pawpocalypse/Content/Pawpocalypse/Maps/L_Title.umap" offset: 0, size: 9 bytes\n'
              'LogIoStore: Display: "<ShaderCode>" offset: 1, size: 2 bytes\n'
              'LogPakFile: Display: "Engine/Config/Base.ini" offset: 0, size: 77 bytes, sha1: 99, compression: None.\n')
    parsed = [m.group(1).replace("../", "") for m in re.finditer(r'Display: "([^"<][^"]*)" offset:', sample)]
    good = parsed == ["Pawpocalypse/Content/Pawpocalypse/Maps/L_Title.umap", "Engine/Config/Base.ini"]
    print("%s  the listing's paths read (shader code skipped)" % ("PASS" if good else "FAIL"))
    ok &= good
    # The trim (step 5) on a scratch archive: the debug layers and manifests go (and the folder they empty), the rest stays.
    import tempfile
    tmp = tempfile.mkdtemp(prefix="pkg_trim_")
    try:
        made = {"Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_khronos_validation.dll": True,
                "Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_khronos_validation.json": True,
                "Pawpocalypse/Binaries/Win64/D3D12/x64/d3d12SDKLayers.dll": True,
                "Manifest_NonUFSFiles_Win64.txt": True,
                "Pawpocalypse/Binaries/Win64/D3D12/x64/D3D12Core.dll": False,
                "Pawpocalypse/Binaries/Win64/Pawpocalypse.exe": False}
        for rel in made:
            path = os.path.join(tmp, "Windows", *rel.split("/"))
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "wb") as h:
                h.write(b"x")
        n, _ = trim_archive(tmp)
        left = {rel for rel in made if os.path.exists(os.path.join(tmp, "Windows", *rel.split("/")))}
        good = n == 4 and left == {r for r, gone in made.items() if not gone} \
            and not os.path.exists(os.path.join(tmp, "Windows", "Engine", "Binaries", "ThirdParty", "Vulkan"))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("%s  the trim: the debug layers and the manifest gone, the game's files kept" % ("PASS" if good else "FAIL"))
    ok &= good
    # The exe guard (step 4): an exe UAT linked now; the up-to-date binary it copied; a stale one; another file.
    tmp = tempfile.mkdtemp(prefix="pkg_exe_")
    try:
        arch, built, src = (os.path.join(tmp, n) for n in ("archived.exe", "built.exe", "a.cpp"))
        for f, body in ((arch, b"exe"), (built, b"exe"), (src, b"code")):
            with open(f, "wb") as h:
                h.write(body)
        now = time.time()
        for f, t in ((src, now - 600), (built, now - 300), (arch, now - 300)):
            os.utime(f, (t, t))
        cases = [("linked by this run", exe_current(arch, built, now - 400, [src]), True),
                 ("the up-to-date binary, built before the run", exe_current(arch, built, now, [src]), True)]
        os.utime(src, (now - 100, now - 100))
        cases.append(("a source newer than the binary: stale", exe_current(arch, built, now, [src]), False))
        os.utime(src, (now - 600, now - 600))
        with open(built, "wb") as h:
            h.write(b"another exe")
        os.utime(built, (now - 300, now - 300))
        cases.append(("not the binary in Binaries: stale", exe_current(arch, built, now, [src]), False))
        cases.append(("no exe", exe_current(os.path.join(tmp, "none.exe"), built, now, [src]), False))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    for name, got, want in cases:
        print("%s  the exe guard: %s" % ("PASS" if got == want else "FAIL", name))
        ok &= got == want
    print("package self-test: %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


# --- the run -----------------------------------------------------------------------------------------------------------

def trim_archive(archive):
    """Removes from the archive's Windows folder every file NOT_NEEDED matches, and the folders it empties. Returns
    (files, bytes) removed."""
    root = os.path.join(archive, "Windows")
    n, size = 0, 0
    for base, _dirs, files in os.walk(root):
        for f in files:
            path = os.path.join(base, f)
            rel = os.path.relpath(path, root).replace(os.sep, "/")
            if any(fnmatch.fnmatch(rel, pat) for pat in NOT_NEEDED):
                size += os.path.getsize(path)
                os.remove(path)
                n += 1
    for base, dirs, files in sorted(os.walk(root), key=lambda w: -len(w[0])):
        if base != root and not os.listdir(base):
            os.rmdir(base)
    return n, size


def exe_current(archived, built, start, sources):
    """Whether the archived exe is this source's build (see the docstring, step 4). sources: the files a newer one of which
    means the binary is stale."""
    if not os.path.isfile(archived):
        return False
    if os.path.getmtime(archived) >= start - 5:
        return True
    if not os.path.isfile(built):
        return False
    a, b = os.stat(archived), os.stat(built)
    if a.st_size != b.st_size or abs(a.st_mtime - b.st_mtime) > 2:
        return False
    return not any(os.path.getmtime(f) > b.st_mtime + 2 for f in sources)


def source_files():
    out = [UPROJECT]
    for base, _dirs, files in os.walk(os.path.join(ROOT, "Source")):
        out += [os.path.join(base, f) for f in files if f.endswith((".cpp", ".h", ".cs"))]
    return out



def running(image):
    out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq %s" % image], capture_output=True, text=True).stdout
    return image.lower() in out.lower()


def uat_running():
    """UAT's processes (AutomationTool.exe, or dotnet running AutomationTool.dll). Matched by the executable or the dll,
    never by any command line that mentions the word: this query's own PowerShell does (it refused every run, 2026-11-23)."""
    ps = ("Get-CimInstance Win32_Process | Where-Object { $_.Name -like 'AutomationTool*' -or "
          "($_.Name -like 'dotnet*' -and $_.CommandLine -match 'AutomationTool\\.dll') } | "
          "Select-Object -ExpandProperty ProcessId")
    out = subprocess.run(["powershell", "-NoProfile", "-Command", ps], capture_output=True, text=True).stdout.split()
    return [p for p in out if p.isdigit() and int(p) != os.getpid()]


def project_version():
    text = open(GAME_INI, encoding="utf-8").read()
    m = re.search(r"^ProjectVersion=(\S+)\s*$", text, re.M)
    return m.group(1) if m else None


def bump_version():
    text = open(GAME_INI, encoding="utf-8").read()
    m = re.search(r"^ProjectVersion=((?:\d+\.)*)(\d+)\s*$", text, re.M)
    if not m:
        return None
    new = "%s%d" % (m.group(1), int(m.group(2)) + 1)
    with open(GAME_INI, "w", encoding="utf-8", newline="") as f:
        f.write(text[:m.start()] + "ProjectVersion=" + new + text[m.end(2):])
    return new


def git_head():
    r = subprocess.run(["git", "-C", ROOT, "rev-parse", "--short", "HEAD"], capture_output=True, text=True)
    dirty = subprocess.run(["git", "-C", ROOT, "status", "--porcelain", "--untracked-files=no"], capture_output=True, text=True)
    return r.stdout.strip() + ("+changes" if dirty.stdout.strip() else "")


README = """PAWPOCALYPSE - test build
{config} build, version {version}, made {date} from commit {commit}.

Run Pawpocalypse.exe. Keyboard and mouse, or a gamepad.
{saves}
If something goes wrong, run "Send logs.bat". It puts Pawpocalypse-logs.zip on your desktop (this game's logs, crash
reports, settings and saves, and your PC's graphics information). Send that file with a few words on what happened.
"""
SAVES = {"Development": "Saves, settings and logs are in Pawpocalypse\\Saved inside this folder.",
         "Shipping": "Saves, settings and logs are in %LOCALAPPDATA%\\Pawpocalypse\\Saved."}

SEND_LOGS = r"""@echo off
rem Collects this game's logs, crash reports, settings and saves, and the PC's graphics information, into one zip on the
rem desktop. Written by tools/package.py. PAW_LOGS_OUT (a zip path) and PAW_LOGS_NOPAUSE are for its own test.
setlocal
set "WORK=%TEMP%\Pawpocalypse-logs"
if exist "%WORK%" rmdir /s /q "%WORK%"
mkdir "%WORK%"
if exist "%~dp0Pawpocalypse\Saved" robocopy "%~dp0Pawpocalypse\Saved" "%WORK%\Saved (this folder)" /e /njh /njs /nfl /ndl >nul
if exist "%LOCALAPPDATA%\Pawpocalypse\Saved" robocopy "%LOCALAPPDATA%\Pawpocalypse\Saved" "%WORK%\Saved (LocalAppData)" /e /njh /njs /nfl /ndl >nul
echo Collecting graphics information (about 20 seconds)...
start "" /wait dxdiag /t "%WORK%\dxdiag.txt"
set "ZIP=%PAW_LOGS_OUT%"
if not defined ZIP for /f "usebackq delims=" %%d in (`powershell -NoProfile -Command "[Environment]::GetFolderPath('Desktop')"`) do set "ZIP=%%d\Pawpocalypse-logs.zip"
if exist "%ZIP%" del "%ZIP%"
rem Windows' own tar by its path: a bare "tar" can be Git's GNU tar, which reads "C:" as a remote host (measured).
if exist "%SystemRoot%\System32\tar.exe" (
  "%SystemRoot%\System32\tar.exe" -a -c -f "%ZIP%" -C "%WORK%" .
) else (
  powershell -NoProfile -Command "Compress-Archive -Path '%WORK%\*' -DestinationPath '%ZIP%'"
)
rmdir /s /q "%WORK%"
if exist "%ZIP%" (echo Saved: %ZIP%) else (echo Could not write %ZIP%)
if not defined PAW_LOGS_NOPAUSE pause
"""


def write_tester_files(archive, config, version):
    base = os.path.join(archive, "Windows")
    with open(os.path.join(base, "README.txt"), "w", encoding="utf-8", newline="\r\n") as f:
        f.write(README.format(config=config, version=version or "(unset)", date=datetime.date.today().isoformat(),
                              commit=git_head(), saves=SAVES[config]))
    with open(os.path.join(base, "Send logs.bat"), "w", encoding="ascii", newline="\r\n") as f:
        f.write(SEND_LOGS)


def package(config, gates, bump):
    found = unreal_procs.wait_blocking()
    if found:
        print("package: an Unreal editor of this project is running; close it first (this never closes it)")
        unreal_procs.print_blocking(found)
        return 2
    for image in ("Pawpocalypse.exe", "Pawpocalypse-Win64-Shipping.exe"):
        if running(image):
            print("package: %s is running; close it first (it locks the exe: UAT would keep the old one)" % image)
            return 2
    if uat_running():
        print("package: another UAT is running")
        return 2
    archive = ARCHIVES[config]
    shutil.rmtree(archive, ignore_errors=True)
    if os.path.exists(archive):
        print("package: could not delete the old archive %s" % archive)
        return 1
    version = project_version()
    if bump:
        version = bump_version()
        if not version:
            print("package: --bump with no ProjectVersion in Config/DefaultGame.ini (the scheme is the designer's)")
            return 2
        print("package: version %s" % version)
    stamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    log = os.path.join(ROOT, "build", "package_%s_%s.log" % (config, stamp))
    cmd = ('"%s" BuildCookRun -project="%s" -noP4 -platform=Win64 -clientconfig=%s -build -cook -stage -pak -archive '
           '-archivedirectory="%s" -nodebuginfo -utf8output -unattended' % (UAT, UPROJECT, config, archive))
    start = time.time()
    print("package: %s -> %s (log %s)" % (config, archive, log))
    with open(log, "w", encoding="utf-8", errors="replace") as f:
        r = subprocess.run(cmd, shell=True, stdout=f, stderr=subprocess.STDOUT)
    print("package: UAT exit %d in %.0f s" % (r.returncode, time.time() - start))
    if r.returncode != 0:
        return 1
    exe = os.path.join(archive, "Windows", *EXES[config].split("/"))
    built = os.path.join(ROOT, "Binaries", "Win64", os.path.basename(exe))
    if not exe_current(exe, built, start, source_files()):
        print("package: the archived exe is missing, or not this source's build: %s" % exe)
        return 1
    pdb = os.path.join(ROOT, "Binaries", "Win64", PDBS[config])
    if os.path.isfile(pdb):
        symbols = os.path.join(ROOT, "build", "symbols", "%s-%s" % (stamp, config))
        os.makedirs(symbols, exist_ok=True)
        shutil.copy2(pdb, symbols)
        shutil.copy2(exe, symbols)
        print("package: symbols -> %s" % symbols)
    else:
        print("package: no %s beside the build (crash dumps of this build will have no symbols)" % PDBS[config])
    n, size = trim_archive(archive)
    print("package: trimmed %d files, %.1f MB the game never loads (debug layers, manifests)" % (n, size / 1e6))
    if run_audit(archive, config) != 0:
        return 1
    write_tester_files(archive, config, version)
    if not gates or config != "Development":
        print("package: no packaged gates run (%s)" % ("--no-gates" if not gates else "Shipping has no console"))
        return 0
    results = {}
    for name, tool, extra in (("languages", "packaged_languages.py", []), ("bench", "packaged_bench.py", []),
                              ("pso", "packaged_pso.py", ["--route", "bench"])):
        r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", tool), "--build", archive] + extra)
        results[name] = r.returncode
    for name, rc in results.items():
        print("package: gate %s %s" % (name, "PASS" if rc == 0 else "FAIL (exit %d)" % rc))
    return 0 if all(rc == 0 for rc in results.values()) else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--config", choices=["Development", "Shipping"], default="Development")
    ap.add_argument("--no-gates", action="store_true")
    ap.add_argument("--bump", action="store_true", help="ProjectVersion's last number + 1 (a build that leaves the PC)")
    ap.add_argument("--audit", nargs="?", const="", default=None, metavar="ARCHIVE")
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    if args.audit is not None:
        return run_audit(args.audit or ARCHIVES[args.config], args.config)
    return package(args.config, not args.no_gates, args.bump)


if __name__ == "__main__":
    sys.exit(main())
