"""Uploads the Steam package (build/package_steam, `py tools/package.py --config Shipping --steam`) to Steam with SteamCMD
(SteamPipe; doc 10 §5).

    py tools/steam_upload.py --user <the Steamworks account> [--branch NAME] [--preview] [--desc TEXT]

--branch NAME   sets the build live on that beta branch at once (SteamCMD can never set the default branch: that is
                Steamworks > SteamPipe > Builds > Preview Change > Set Build Live Now, with the account's confirmation code).
                Without it the build lands on the Builds page, live nowhere.
--preview       SteamPipe's preview: the manifest and what would change, nothing uploaded.
The App ID is Platform/PawSteam.h's; the depot is App ID + 1 (Steam's default depot). The VDF is written to
build/steam/upload/app_build_<app>.vdf. SteamCMD: %USERPROFILE%/tools/steamcmd/steamcmd.exe (or --steamcmd); its first
login asks for the password and Steam Guard in the developer's own terminal, later ones use the cached login.
Refuses: no Steam package, a Saved folder in it (a test run's files), the default branch.
"""
import argparse
import datetime
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CONTENT = ROOT / "build" / "package_steam" / "Windows"
OUT = ROOT / "build" / "steam" / "upload"
STEAM_H = ROOT / "Source" / "Pawpocalypse" / "Platform" / "PawSteam.h"
GAME_INI = ROOT / "Config" / "DefaultGame.ini"
STEAM_EXE = CONTENT / "Pawpocalypse" / "Binaries" / "Win64" / "PawpocalypseSteam-Win64-Shipping.exe"
# The .pdb never ships (build/symbols keeps it); README and Send logs.bat are for testers given a zip, not Steam players.
EXCLUDE = ["*.pdb", "*.bat", "README.txt", "Manifest_*.txt"]


def app_id():
    m = re.search(r"constexpr uint32 AppId = (\d+);", STEAM_H.read_text(encoding="utf-8"))
    if not m:
        raise SystemExit("steam_upload: no AppId in %s" % STEAM_H)
    return int(m.group(1))


def version():
    m = re.search(r"^ProjectVersion=(.+)$", GAME_INI.read_text(encoding="utf-8"), re.M)
    return m.group(1).strip() if m else "?"


def commit():
    try:
        head = subprocess.run(["git", "-C", str(ROOT), "rev-parse", "--short", "HEAD"], capture_output=True, text=True).stdout.strip()
        dirty = subprocess.run(["git", "-C", str(ROOT), "status", "--porcelain", "--", "Source", "Config", "Content", "Data"],
                               capture_output=True, text=True).stdout.strip()
        return head + ("+changes" if dirty else "")
    except OSError:
        return "?"


def vdf(app, depot, desc, branch, preview):
    exclusions = "\n".join('            "FileExclusion" "%s"' % e for e in EXCLUDE)
    return """"AppBuild"
{
    "AppID" "%d"
    "Desc" "%s"
    "Preview" "%d"
    "SetLive" "%s"
    "ContentRoot" "%s"
    "BuildOutput" "%s"
    "Depots"
    {
        "%d"
        {
            "FileMapping"
            {
                "LocalPath" "*"
                "DepotPath" "."
                "recursive" "1"
            }
%s
        }
    }
}
""" % (app, desc.replace('"', "'"), 1 if preview else 0, branch, CONTENT.as_posix(), (OUT / "output").as_posix(), depot, exclusions)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--user", required=True, help="the Steamworks account SteamCMD logs in as (its cached login)")
    ap.add_argument("--branch", default="", help="a beta branch to set the build live on (never the default branch)")
    ap.add_argument("--preview", action="store_true")
    ap.add_argument("--desc", default="")
    ap.add_argument("--steamcmd", default=os.path.join(os.path.expanduser("~"), "tools", "steamcmd", "steamcmd.exe"))
    a = ap.parse_args()
    if a.branch.lower() in ("default", "public"):
        print("steam_upload: the default branch is set live by hand on Steamworks (SteamCMD cannot)")
        return 2
    if not STEAM_EXE.is_file():
        print("steam_upload: no Steam package at %s (py tools/package.py --config Shipping --steam)" % CONTENT)
        return 2
    saved = [p for p in CONTENT.rglob("Saved") if p.is_dir()]
    if saved:
        print("steam_upload: a Saved folder in the package (%s): package again" % saved[0])
        return 2
    if not os.path.isfile(a.steamcmd):
        print("steam_upload: no SteamCMD at %s" % a.steamcmd)
        return 2
    app = app_id()
    desc = a.desc or "%s Shipping, %s, %s" % (version(), commit(), datetime.date.today().isoformat())
    OUT.mkdir(parents=True, exist_ok=True)
    script = OUT / ("app_build_%d.vdf" % app)
    script.write_text(vdf(app, app + 1, desc, a.branch, a.preview), encoding="ascii")
    print("steam_upload: app %d depot %d, %s (%s%s)" % (app, app + 1, desc, "live on " + a.branch if a.branch else "live nowhere",
                                                         ", preview only" if a.preview else ""))
    r = subprocess.run([a.steamcmd, "+login", a.user, "+run_app_build", str(script), "+quit"], capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    lines = (r.stdout or "").splitlines()
    for line in lines:
        if re.search(r"Successfully finished|BuildID|ERROR|FAILED|Error|Logged in|Summary|missing chunks", line):
            print("  " + line.strip())
    done = re.search(r"Successfully finished AppID \d+ build \(BuildID (\d+)\)", r.stdout or "")
    if not done:
        print("steam_upload: FAILED (exit %d): see %s and SteamCMD's logs" % (r.returncode, OUT / "output"))
        return 1
    print("steam_upload: BuildID %s%s" % (done.group(1), "" if a.branch else ": set it live on Steamworks > SteamPipe > Builds"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
