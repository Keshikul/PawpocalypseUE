"""The Steamworks upload files (doc 10 §5), made from the game's own data, and the checks that keep them and the game agreeing:

  achievements_definitions.csv   Stats & Achievements > Achievements > Bulk Import/Export (Steamworks' own export format)
  achievements_localization.csv  the same page: the name in every language of the game (the string tables' ACH_<id>_name),
                                 the description (docs/STEAM-ACHIEVEMENTS-DRAFT-*.csv: English, a draft for the designer)
  icons/<id>_ACHIEVED.png, icons/<id>_UNACHIEVED.png
                                 256 px, the game's own icon (SourceArt/svg/ui/achievement_<id>.svg) on the panel; Steamworks
                                 links an icon to its achievement by this file name
  rich_presence.vdf              Community > Rich Presence: "#Biome_<tier id>" = the biome's name, every language (one file)

    py tools/steam_files.py [--out DIR]   # default build/steam/steamworks
    py tools/steam_files.py --check       # everyday check_all: the files can be made, and the game agrees with them
    py tools/steam_files.py --self-test   # each check is seen to fail on a planted fault

The API names are the ids of Data/AchievementData.csv: the game sets those (Platform/PawSteam). Hidden on Steam = the CSV's
`secret`. The presence tokens are "#Biome_" + the tier ids of Data/TierData.csv: the game sends that (PawPlatformSubsystem).
--check fails when: an achievement has no name in a language the game offers, or no icon; a tier has no name in a language;
the descriptions file misses an achievement or names one that does not exist; the game's presence token or App ID is not the
one these files and tools/steam_upload.py assume.
"""
import argparse
import csv
import io
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "Data"
STRINGS = ROOT / "Content" / "Pawpocalypse" / "Localization" / "Strings"
TRANSLATIONS = ROOT / "Content" / "Pawpocalypse" / "Localization" / "Translations"
ICONS = ROOT / "SourceArt" / "svg" / "ui"
LOCALES = ROOT / "Source" / "PawCore" / "Private" / "PcLocales.cpp"
STEAM_H = ROOT / "Source" / "Pawpocalypse" / "Platform" / "PawSteam.h"
PLATFORM_CPP = ROOT / "Source" / "Pawpocalypse" / "Platform" / "PawPlatformSubsystem.cpp"
OUT = ROOT / "build" / "steam" / "steamworks"
ICON_PX = 256
PANEL = (0xFF, 0xF8, 0xEB)   # PawTokens UI_PANEL: the achievement plate
FRAME = (0x2A, 0x21, 0x31)   # UI_PANEL_FRAME / UI_TEXT: the ink
PRESENCE_PREFIX = "#Biome_"


def app_id():
    m = re.search(r"constexpr uint32 AppId = (\d+);", STEAM_H.read_text(encoding="utf-8"))
    return int(m.group(1)) if m else None


def languages():
    """[(culture, steam api name)] in the game's order: PawCore's table (Pc::Languages)."""
    text = LOCALES.read_text(encoding="utf-8")
    return re.findall(r'\{ "[^"]+", "([^"]+)", "[^"]*", "([a-z]+)" \}', text)


def read_csv(path):
    with io.open(path, encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def table(culture, name="ST_Data"):
    """Key -> text of a string table in one culture (English: the source strings)."""
    if culture == "en":
        return {r["Key"]: r["SourceString"] for r in read_csv(STRINGS / ("%s.csv" % name))}
    path = TRANSLATIONS / culture / ("%s.csv" % name)
    return {r["Key"]: r["Text"] for r in read_csv(path)} if path.exists() else {}


def descriptions():
    drafts = sorted((ROOT / "docs").glob("STEAM-ACHIEVEMENTS-DRAFT-*.csv"))
    if not drafts:
        return {}, None
    return {r["id"]: r["description"] for r in read_csv(drafts[-1])}, drafts[-1]


def gather():
    achievements = read_csv(DATA / "AchievementData.csv")
    tiers = [r["id"] for r in read_csv(DATA / "TierData.csv")]
    langs = languages()
    tables = {c: table(c) for c, _ in langs}
    desc, desc_file = descriptions()
    return achievements, tiers, langs, tables, desc, desc_file


def problems(achievements, tiers, langs, tables, desc, desc_file, icons_dir=ICONS, platform_cpp=PLATFORM_CPP, steam_h=STEAM_H):
    out = []
    if not langs or langs[0] != ("en", "english"):
        out.append("languages: PcLocales.cpp's table not read (English first)")
    ids = [a["id"] for a in achievements]
    for a in achievements:
        for culture, steam in langs:
            if not tables[culture].get("ACH_%s_name" % a["id"], "").strip():
                out.append("achievement %s: no name in %s (%s)" % (a["id"], culture, steam))
        if not (icons_dir / ("achievement_%s.svg" % a["id"])).exists():
            out.append("achievement %s: no icon %s" % (a["id"], icons_dir / ("achievement_%s.svg" % a["id"])))
    if desc_file is None:
        out.append("no docs/STEAM-ACHIEVEMENTS-DRAFT-*.csv (the descriptions)")
    else:
        for i in ids:
            if not desc.get(i, "").strip():
                out.append("achievement %s: no description in %s" % (i, desc_file.name))
        for i in sorted(set(desc) - set(ids)):
            out.append("%s describes %s, which Data/AchievementData.csv does not have" % (desc_file.name, i))
    for t in tiers:
        for culture, steam in langs:
            if not tables[culture].get("TIER_%s_name" % t, "").strip():
                out.append("presence: tier %s has no name in %s (%s)" % (t, culture, steam))
    cpp = Path(platform_cpp).read_text(encoding="utf-8")
    if 'TEXT("%s%%s")' % PRESENCE_PREFIX not in cpp:
        out.append("presence: PawPlatformSubsystem.cpp no longer sends %s<tier id>" % PRESENCE_PREFIX)
    text_h = Path(steam_h).read_text(encoding="utf-8")
    if not re.search(r"constexpr uint32 AppId = \d+;", text_h):
        out.append("PawSteam.h: no AppId")
    return out


def vdf_quote(s):
    return '"%s"' % s.replace("\\", "\\\\").replace('"', '\\"')


def icon(svg_path, size, locked):
    import resvg_py
    from PIL import Image, ImageDraw, ImageOps
    img = Image.new("RGB", (size, size), PANEL)
    d = ImageDraw.Draw(img)
    w = max(4, size // 40)
    d.rounded_rectangle([w // 2, w // 2, size - 1 - w // 2, size - 1 - w // 2], radius=size // 8, outline=FRAME, width=w)
    inner = int(size * 0.74)
    art = Image.open(io.BytesIO(bytes(resvg_py.svg_to_bytes(svg_path=str(svg_path), width=inner, height=inner)))).convert("RGBA")
    art.thumbnail((inner, inner))
    img.paste(art, ((size - art.width) // 2, (size - art.height) // 2), art)
    if locked:
        # Not earned: grey and faded (Steamworks' "generate unachieved icons" is left off: these are ours).
        grey = ImageOps.grayscale(img).convert("RGB")
        img = Image.blend(grey, Image.new("RGB", img.size, (0xC8, 0xC4, 0xC0)), 0.35)
    return img


def write(out, achievements, tiers, langs, tables, desc):
    (out / "icons").mkdir(parents=True, exist_ok=True)
    with io.open(out / "achievements_definitions.csv", "w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(["api_name (required, unique; the name your game passes to the API)", "setid (optional; the id of a set that already exists)",
                    "permission (one of Client, GameServer, OfficialGameServer)", "spoiler (true or false)", "archived (true or false)",
                    "progress_stat_name (optional; the name of a stat that already exists, blank for no progress bar)",
                    "progress_stat_min (number, no lower than the stat's own min)", "progress_stat_max (number, above min and no higher than the stat's own max)"])
        for a in achievements:
            w.writerow([a["id"], "", "Client", "true" if a["secret"].strip().lower() == "true" else "false", "false", "", 0, 0])
    with io.open(out / "achievements_localization.csv", "w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(["api_name (required; must match an api_name from the definitions CSV)", "field (name or description)"] + [s for _, s in langs])
        for a in achievements:
            w.writerow([a["id"], "name"] + [tables[c]["ACH_%s_name" % a["id"]] for c, _ in langs])
            # The descriptions are English only until the designer approves them (then they are translated): Steam shows
            # English where a language has none.
            w.writerow([a["id"], "description"] + [desc[a["id"]] if c == "en" else "" for c, _ in langs])
    for a in achievements:
        svg = ICONS / ("achievement_%s.svg" % a["id"])
        icon(svg, ICON_PX, False).save(out / "icons" / ("%s_ACHIEVED.png" % a["id"]))
        icon(svg, ICON_PX, True).save(out / "icons" / ("%s_UNACHIEVED.png" % a["id"]))
    lines = ['"lang"', "{"]
    for c, steam in langs:
        lines += ["\t%s" % vdf_quote(steam), "\t{", '\t\t"tokens"', "\t\t{"]
        for t in tiers:
            lines.append("\t\t\t%s %s" % (vdf_quote(PRESENCE_PREFIX + t), vdf_quote(tables[c]["TIER_%s_name" % t])))
        lines += ["\t\t}", "\t}"]
    lines.append("}")
    (out / "rich_presence.vdf").write_text("\n".join(lines) + "\n", encoding="utf-8")


def self_test():
    """Each check fails on a planted fault (CLAUDE.md: a test that has never failed has never been tested)."""
    import copy
    import tempfile
    base = gather()
    ok = not problems(*base)
    cases = []

    def plant(name, mutate, **kw):
        args = list(copy.deepcopy(base))
        mutate(args)
        cases.append((name, bool(problems(*args, **kw))))

    plant("a name missing in German", lambda a: a[3]["de"].pop("ACH_%s_name" % a[0][0]["id"]))
    plant("a tier without a Thai name", lambda a: a[3]["th"].pop("TIER_%s_name" % a[1][0]))
    plant("an achievement with no description", lambda a: a[4].pop(a[0][0]["id"]))
    plant("a description of an achievement that does not exist", lambda a: a[4].__setitem__("no_such", "x"))
    plant("an achievement with no icon", lambda a: a[0].append(dict(a[0][0], id="no_icon_here")) or a[4].__setitem__("no_icon_here", "x")
          or [a[3][c].__setitem__("ACH_no_icon_here_name", "x") for c, _ in a[2]])
    plant("the languages not read", lambda a: a[2].clear())
    with tempfile.TemporaryDirectory() as tmp:
        cpp = Path(tmp) / "PawPlatformSubsystem.cpp"
        cpp.write_text(PLATFORM_CPP.read_text(encoding="utf-8").replace(PRESENCE_PREFIX, "#Realm_"), encoding="utf-8")
        plant("the game sends another presence token", lambda a: None, platform_cpp=cpp)
        h = Path(tmp) / "PawSteam.h"
        h.write_text(STEAM_H.read_text(encoding="utf-8").replace("constexpr uint32 AppId", "constexpr uint32 AppIdOld"), encoding="utf-8")
        plant("no App ID", lambda a: None, steam_h=h)
    print("steam_files self-test: the real data %s" % ("passes" if ok else "FAILS"))
    for name, caught in cases:
        print("  %-55s %s" % (name, "caught" if caught else "MISSED"))
    return 0 if ok and all(c for _, c in cases) else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", default=str(OUT))
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--self-test", action="store_true")
    a = ap.parse_args()
    if a.self_test:
        return self_test()
    data = gather()
    found = problems(*data)
    for p in found:
        print("steam_files: %s" % p)
    if a.check:
        print("steam_files: %s (%d achievements, %d biomes, %d languages, app %s)"
              % ("PASS" if not found else "FAIL", len(data[0]), len(data[1]), len(data[2]), app_id()))
        return 1 if found else 0
    if found:
        return 1
    out = Path(a.out)
    write(out, data[0], data[1], data[2], data[3], data[4])
    print("steam_files: %d achievements (+%d icons), %d presence tokens in %d languages -> %s (descriptions: %s)"
          % (len(data[0]), 2 * len(data[0]), len(data[1]), len(data[2]), out, data[5].name))
    return 0


if __name__ == "__main__":
    sys.exit(main())
