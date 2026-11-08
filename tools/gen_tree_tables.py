"""Generates the skill tree's tables from the Godot code that holds them (skill_tree_ui.gd), never by hand.

The tree's player-facing words and colours are not in /data: they are constants and a `match` in skill_tree_ui.gd. This reads
them and writes three files, which L0 (tools/check_pawcore_static.py) requires to be current:

  Content/Pawpocalypse/Localization/Strings/ST_Tree.csv   the string table:
      TREE_<id>_label   BRANCH_LABELS (the name drawn under a node: `overkill` is "Splash" here, "Overkill" in the data)
      TREE_<id>_blurb   BRANCH_BLURBS (the tooltip's paragraph; `{pounce}`-style action tokens kept for the prompts)
      FX_<EFFECT>[__<branch>]   one phrase per case of _describe() ("+{v}% damage"); a branch's own case (`if branch_id ==`)
                        gets its own key; a tr_n() case gets FX_<EFFECT>_one and FX_<EFFECT>_other
      CH_<channel>      _describe_channel(): a merged ladder's extra channels ("x{v} damage")
  Source/PawCore/Public/PcTreeWording.h   which key and which number each effect type / channel uses, in _describe()'s
      order (PawCore's PcTreeText formats the number exactly as GDScript does)
  Source/Pawpocalypse/UI/PawTreeTables.h  BRANCH_COLORS (8-bit sRGB, as Godot's Color draws), RAINBOW_BRANCHES, PULSE_BRANCHES

    py tools/gen_tree_tables.py [--godot C:/Games/pawpocalypse] [--check]
Anything in _describe() this does not recognise is an error, so a new phrasing or number format in Godot fails here, loudly.
"""
import argparse
import csv
import io
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT_ST = ROOT / "Content" / "Pawpocalypse" / "Localization" / "Strings" / "ST_Tree.csv"
OUT_CORE = ROOT / "Source" / "PawCore" / "Public" / "PcTreeWording.h"
OUT_UI = ROOT / "Source" / "Pawpocalypse" / "UI" / "PawTreeTables.h"
SCRIPT = "scripts/systems/skill_tree_ui.gd"
LINKS_SCRIPT = "scripts/systems/skill_tree_connector_layer.gd"

GD_STRING = r'"((?:[^"\\]|\\.)*)"'
# GDScript number expressions in _describe() -> PcTreeNumber.
NUMBERS = {
    'roundi(value * 100.0)': "R100",
    'int(value)': "Int",
    '"%.0f" % (value)': "F0",
    '"%.1f" % (value)': "F1",
    '"%.2f" % (value)': "F2",
    '"%.3f" % (value)': "F3",
    '"%.1f" % (value * 100.0)': "F1x100",
    '"%.0f" % (60.0 / maxf(value, 0.001))': "F0PerMinute",
    'roundi(1.0 / maxf(value, 0.001))': "RInverse",
}


def unescape(s):
    out, i = [], 0
    while i < len(s):
        c = s[i]
        if c == "\\" and i + 1 < len(s):
            n = s[i + 1]
            out.append({"n": "\n", "t": "\t", '"': '"', "\\": "\\", "'": "'"}.get(n, "\\" + n))
            i += 2
            continue
        out.append(c)
        i += 1
    return "".join(out)


def block(src, name, opener):
    m = re.search(r"^const %s\s*:?=\s*\%s" % (name, opener), src, re.M)
    if not m:
        raise SystemExit("gen_tree_tables: %s not found in %s" % (name, SCRIPT))
    closer = "}" if opener == "{" else "]"
    depth, i = 0, m.end() - 1
    while i < len(src):
        if src[i] == opener:
            depth += 1
        elif src[i] == closer:
            depth -= 1
            if depth == 0:
                return src[m.end():i]
        elif src[i] == '"':
            j = i + 1
            while src[j] != '"':
                j += 2 if src[j] == "\\" else 1
            i = j
        i += 1
    raise SystemExit("gen_tree_tables: %s is not closed" % name)


def string_dict(src, name):
    out = []
    for line in block(src, name, "{").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        m = re.match(r'^"([a-z0-9_]+)":\s*%s,?\s*(#.*)?$' % GD_STRING, line)
        if not m:
            raise SystemExit("gen_tree_tables: %s: cannot read %r" % (name, line))
        out.append((m.group(1), unescape(m.group(2))))
    return out


def colour_dict(src, name):
    out = []
    for line in block(src, name, "{").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        m = re.match(r'^"([a-z0-9_]+)":\s*Color\(\s*([\d.]+)\s*,\s*([\d.]+)\s*,\s*([\d.]+)\s*(?:,\s*([\d.]+)\s*)?\),?\s*(#.*)?$', line)
        if not m:
            raise SystemExit("gen_tree_tables: %s: cannot read %r" % (name, line))
        # Color.to_rgba32 rounds each channel: what Godot draws on an 8-bit target.
        alpha = float(m.group(5)) if m.group(5) else 1.0
        out.append((m.group(1), tuple(int(round(float(m.group(k)) * 255.0)) for k in (2, 3, 4)) + (int(round(alpha * 255.0)),)))
    return out


def string_list(src, name):
    return re.findall(GD_STRING, block(src, name, "["))


def func_body(src, name):
    m = re.search(r"^func %s\(.*$" % re.escape(name), src, re.M)
    if not m:
        raise SystemExit("gen_tree_tables: func %s not found" % name)
    nxt = re.search(r"^(func |## ---)", src[m.end():], re.M)
    return src[m.end(): m.end() + nxt.start() if nxt else len(src)]


def describe_cases(src):
    """_describe(): [(effect, branch or "", key, english, arg, number, plural_form or "")] in source order."""
    rows, effect, branch = [], None, ""
    tr = re.compile(r'^return tr\(%s\)(?:\.format\(\{"(\w+)":\s*(.+)\}\))?$' % GD_STRING)
    trn = re.compile(r'^return tr_n\(%s,\s*%s,\s*int\(value\)\)\.format\(\{"n":\s*int\(value\)\}\)$' % (GD_STRING, GD_STRING))
    for raw in func_body(src, "_describe").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or line.startswith("match effect_type"):
            continue
        m = re.match(r"^SkillBranchData\.EffectType\.([A-Z_]+):$", line)
        if m:
            effect, branch = m.group(1), ""
            continue
        m = re.match(r'^if branch_id == "([a-z0-9_]+)":$', line)
        if m:
            branch = m.group(1)
            continue
        if line == 'return "" if level > 0 else ""':
            continue
        if effect is None:
            raise SystemExit("gen_tree_tables: _describe: %r before any case" % line)
        m = trn.match(line)
        if m:
            base = "FX_%s" % effect
            rows.append((effect, branch, base + "_one", unescape(m.group(1)), "n", "Int", "one"))
            rows.append((effect, branch, base + "_other", unescape(m.group(2)), "n", "Int", "other"))
            branch = ""
            continue
        m = tr.match(line)
        if not m:
            raise SystemExit("gen_tree_tables: _describe: cannot read %r (case %s)" % (line, effect))
        text, arg, expr = unescape(m.group(1)), m.group(2) or "", (m.group(3) or "").strip()
        number = "None"
        if expr:
            if expr not in NUMBERS:
                raise SystemExit("gen_tree_tables: _describe: unknown number format %r (case %s)" % (expr, effect))
            number = NUMBERS[expr]
        key = "FX_%s" % effect + ("__%s" % branch if branch else "")
        rows.append((effect, branch, key, text, arg, number, ""))
        branch = ""
    return rows


def channel_cases(src):
    rows, channel = [], None
    for raw in func_body(src, "_describe_channel").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or line.startswith("match channel"):
            continue
        m = re.match(r'^"([a-z0-9_]+)":$', line)
        if m:
            channel = m.group(1)
            continue
        if line == 'return tr("")':
            continue
        m = re.match(r'^return tr\(%s\)\.format\(\{"(\w+)":\s*(.+)\}\)$' % GD_STRING, line)
        if not m or channel is None:
            raise SystemExit("gen_tree_tables: _describe_channel: cannot read %r" % line)
        expr = m.group(3).strip()
        if expr not in NUMBERS:
            raise SystemExit("gen_tree_tables: _describe_channel: unknown number format %r" % expr)
        rows.append((channel, "CH_%s" % channel, unescape(m.group(1)), m.group(2), NUMBERS[expr]))
        channel = None
    return rows


# The tooltip's bbcode colours in _on_node_hovered(), in order of first appearance, and what each colours.
TIP_COLOURS = ["TipBlurb", "TipFully", "TipNext", "TipReason", "TipShort"]


def tip_colours(src):
    seen = []
    for hexcode in re.findall(r"\[color=#([0-9A-Fa-f]{6})\]", func_body(src, "_on_node_hovered")):
        if hexcode.upper() not in seen:
            seen.append(hexcode.upper())
    if len(seen) != len(TIP_COLOURS):
        raise SystemExit("gen_tree_tables: _on_node_hovered has %d distinct colours (%s), expected %d (%s): update TIP_COLOURS"
                         % (len(seen), ", ".join(seen), len(TIP_COLOURS), ", ".join(TIP_COLOURS)))
    return list(zip(TIP_COLOURS, seen))


def const_colour(src, name, where):
    m = re.search(r"^const %s\s*:?=\s*Color\(\s*([\d.]+)\s*,\s*([\d.]+)\s*,\s*([\d.]+)\s*(?:,\s*([\d.]+)\s*)?\)" % name, src, re.M)
    if not m:
        raise SystemExit("gen_tree_tables: const %s = Color(...) not found in %s" % (name, where))
    return tuple(float(m.group(k)) if m.group(k) else 1.0 for k in (1, 2, 3, 4))


def restyled():
    """Godot's palette colour -> ours, for the colours the restyle changed (D14; gen_paw_tokens.colour_pairs against the
    palette frozen at the fork). The tree's words still come from Godot; a colour Godot wrote as a palette value is drawn in
    today's value of that palette colour. Keys and values are "RRGGBB"."""
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import gen_paw_tokens
    return {old[1:].upper(): new[1:].upper() for old, new, _ in gen_paw_tokens.colour_pairs(gen_paw_tokens.SNAPSHOT, ui=False)}


# The tooltip's text sits on the plum panel since R3: its palette colours go through the UI's TEXT role first (Godot's ink
# text -> Bone), and the two literals Godot wrote (the gold "Next" line, the grey reason) become their plum roles (Butter:
# numbers; Lilac: secondary text), or they would not read on plum.
TIP_LITERALS = {"B8860B": "butter", "8F836F": "lilac"}


def tip_colour(hexcode, table):
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import gen_paw_tokens
    key = hexcode.upper()
    if key in TIP_LITERALS:
        return gen_paw_tokens.palette.PLUM[TIP_LITERALS[key]][1:].upper()
    text, surface, _edges = gen_paw_tokens.ui_role_pairs(gen_paw_tokens.SNAPSHOT)
    for role in (text, surface):
        if "#" + key in role:
            return role["#" + key][1:].upper()
    return table.get(key, key)


def ours_bytes(rgba, table):
    """(r, g, b, a) bytes through restyled(); alpha kept."""
    key = "%02X%02X%02X" % tuple(rgba[:3])
    if key not in table:
        return tuple(rgba)
    new = table[key]
    return (int(new[0:2], 16), int(new[2:4], 16), int(new[4:6], 16), rgba[3])


def render(godot):
    src = (Path(godot) / SCRIPT).read_text(encoding="utf-8")
    table = restyled()
    labels = string_dict(src, "BRANCH_LABELS")
    blurbs = string_dict(src, "BRANCH_BLURBS")
    colours = colour_dict(src, "BRANCH_COLORS")
    rainbow = string_list(src, "RAINBOW_BRANCHES")
    pulse = string_list(src, "PULSE_BRANCHES")
    cases = describe_cases(src)
    channels = channel_cases(src)
    tips = tip_colours(src)
    links_src = (Path(godot) / LINKS_SCRIPT).read_text(encoding="utf-8")
    shine = const_colour(src, "SHINE_COLOR", SCRIPT)
    dim = const_colour(links_src, "DIM", LINKS_SCRIPT)
    shut = const_colour(links_src, "SHUT", LINKS_SCRIPT)

    st = io.StringIO()
    w = csv.writer(st, lineterminator="\n", quoting=csv.QUOTE_ALL)
    w.writerow(["Key", "SourceString", "Comment"])
    for bid, text in labels:
        w.writerow(["TREE_%s_label" % bid, text, "%s BRANCH_LABELS %s (the caption under the node)" % (SCRIPT, bid)])
    for bid, text in blurbs:
        w.writerow(["TREE_%s_blurb" % bid, text, "%s BRANCH_BLURBS %s (the tooltip)" % (SCRIPT, bid)])
    seen = set()
    for effect, branch, key, text, arg, number, plural in cases:
        if key in seen:
            raise SystemExit("gen_tree_tables: _describe: %s twice" % key)
        seen.add(key)
        note = "%s _describe %s%s%s" % (SCRIPT, effect, (" for " + branch) if branch else "", (" (tr_n " + plural + ")") if plural else "")
        w.writerow([key, text, note])
    for channel, key, text, arg, number in channels:
        w.writerow([key, text, "%s _describe_channel %s" % (SCRIPT, channel)])

    core = io.StringIO()
    core.write("#pragma once\n\n// GENERATED by tools/gen_tree_tables.py from %s (_describe, _describe_channel): do not edit.\n" % SCRIPT)
    core.write("// Which ST_Tree key and which number each effect type (and each merged ladder's channel) uses, in _describe()'s order:\n")
    core.write("// a branch's own case first. PcTreeText formats the number as GDScript does (PcTreeNumber).\n\n")
    core.write('#include "PcEnums.h"\n\nnamespace Pc\n{\n')
    core.write("\tenum class PcTreeNumber\n\t{\n")
    for n in ["None"] + sorted(set(NUMBERS.values()), key=list(NUMBERS.values()).index):
        core.write("\t\t%s,\n" % n)
    core.write("\t};\n\n")
    core.write("\tstruct PcTreeWording\n\t{\n\t\tBranchEffectType Effect;\n\t\tconst char* Branch; // \"\" = every branch of this effect\n")
    core.write("\t\tconst char* Key;    // ST_Tree\n\t\tconst char* Arg;    // the format argument (\"v\", \"n\") or \"\"\n")
    core.write("\t\tPcTreeNumber Number;\n\t\tconst char* Plural; // \"one\" / \"other\" for a tr_n() pair, else \"\"\n\t};\n\n")
    core.write("\tinline const PcTreeWording TreeWordings[] = {\n")
    for effect, branch, key, text, arg, number, plural in cases:
        core.write('\t\t{ BranchEffectType::%s, "%s", "%s", "%s", PcTreeNumber::%s, "%s" },\n' % (effect, branch, key, arg, number, plural))
    core.write("\t};\n\n")
    core.write("\tstruct PcTreeChannelWording\n\t{\n\t\tconst char* Channel;\n\t\tconst char* Key;\n\t\tconst char* Arg;\n\t\tPcTreeNumber Number;\n\t};\n\n")
    core.write("\tinline const PcTreeChannelWording TreeChannelWordings[] = {\n")
    for channel, key, text, arg, number in channels:
        core.write('\t\t{ "%s", "%s", "%s", PcTreeNumber::%s },\n' % (channel, key, arg, number))
    core.write("\t};\n}\n")

    ui = io.StringIO()
    ui.write("#pragma once\n\n// GENERATED by tools/gen_tree_tables.py from %s (BRANCH_COLORS, RAINBOW_BRANCHES, PULSE_BRANCHES): do not\n" % SCRIPT)
    ui.write("// edit. Colours are Godot's Color as it draws on an 8-bit target (to_rgba32), through PawTokens::Srgb like the palette.\n\n")
    ui.write('#include "CoreMinimal.h"\n#include "UI/PawTokens.h"\n\nnamespace PawTree\n{\n')
    ui.write("\tstruct FBranchColour\n\t{\n\t\tconst TCHAR* Id;\n\t\tFLinearColor Colour;\n\t};\n\n")
    ui.write("\t// One colour per trunk, shared by everything growing off it (BRANCH_COLORS); a branch not listed draws white.\n")
    ui.write("\tinline const FBranchColour BranchColours[] = {\n")
    for bid, rgba in colours:
        ui.write('\t\t{ TEXT("%s"), PawTokens::Srgb(%d, %d, %d, %d) },\n' % ((bid,) + ours_bytes(rgba, table)))
    ui.write("\t};\n\n")
    ui.write("\t// The nodes that hand the cat a verb: a rainbow ring in every state (RAINBOW_BRANCHES).\n")
    ui.write("\tinline const TCHAR* const RainbowBranches[] = { %s };\n" % ", ".join('TEXT("%s")' % x for x in rainbow))
    ui.write("\t// Rainbow rings that also breathe (PULSE_BRANCHES).\n")
    ui.write("\tinline const TCHAR* const PulseBranches[] = { %s };\n\n" % ", ".join('TEXT("%s")' % x for x in pulse))
    ui.write("\t// The tooltip's colours (_on_node_hovered's bbcode, two of them not in the palette): the blurb, \"Fully bought.\", the\n")
    ui.write("\t// Next line, why it cannot be bought, and \"You have n.\".\n")
    for name, hexcode in tips:
        hexcode = tip_colour(hexcode, table)
        ui.write("\tinline const FLinearColor %s = PawTokens::Srgb(0x%s, 0x%s, 0x%s);\n" % (name, hexcode[0:2], hexcode[2:4], hexcode[4:6]))
    ui.write("\n\t// %s DIM (a link not walked) and SHUT (the road not taken); %s SHINE_COLOR, a self_modulate MULTIPLIER\n" % (LINKS_SCRIPT, SCRIPT))
    ui.write("\t// (above 1: the node brightens), not a colour.\n")
    for name, (r, g, b, a) in (("LinkDim", dim), ("LinkShut", shut)):
        ui.write("\tinline const FLinearColor %s = PawTokens::Srgb(%d, %d, %d, %d);\n"
                 % ((name,) + ours_bytes((round(r * 255), round(g * 255), round(b * 255), round(a * 255)), table)))
    ui.write("\tinline const FLinearColor ShineTint = FLinearColor(%.4ff, %.4ff, %.4ff, %.4ff);\n" % shine)
    ui.write("}\n")
    return st.getvalue(), core.getvalue(), ui.getvalue(), len(labels), len(cases), len(channels)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--godot", default="C:/Games/pawpocalypse")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    st, core, ui, n_labels, n_cases, n_channels = render(args.godot)
    outs = [(OUT_ST, st), (OUT_CORE, core.replace("\n", "\r\n")), (OUT_UI, ui.replace("\n", "\r\n"))]
    if args.check:
        stale = [p for p, text in outs if not p.exists() or p.read_bytes().decode("utf-8") != text]
        for p in stale:
            print("%s is not what tools/gen_tree_tables.py writes: re-run it" % p.relative_to(ROOT))
        if stale:
            return 1
        print("tree tables current")
        return 0
    for p, text in outs:
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(text.encode("utf-8"))
    print("wrote ST_Tree.csv (%d labels, %d effect phrases, %d channels), PcTreeWording.h, PawTreeTables.h" % (n_labels, n_cases, n_channels))
    return 0


if __name__ == "__main__":
    sys.exit(main())
