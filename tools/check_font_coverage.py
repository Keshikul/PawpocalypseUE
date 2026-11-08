"""Every character the game shows, in every language, has a glyph in F_Paw (restyle R2, 2026-11-07; L0 gate).

    py tools/check_font_coverage.py              the gate (exit 1 on any missing character)
    py tools/check_font_coverage.py --self-test  the gate must FAIL on two broken fonts (Thai sub-font gone; a Cyrillic
                                                 letter cut from the text face), and PASS on the real one

The strings: English (Godot's messages.pot msgids and the port's string tables, Content/Pawpocalypse/Localization/Strings),
every Godot locale/*.po msgstr (the translations reach the port in phase 7; the text is Godot's until parity), and the
language picker's names (locales.gd NATIVE_NAMES), shown in EVERY language. Each character (NFC, minus whitespace and
controls) is resolved the way Slate resolves it (tools/fonts.py, the docstring): the culture's sub-fonts, the culture-less
sub-fonts, the default typeface, the fallback. Both text weights are checked (the default typeface's faces).

Rules: every character must reach a face that HAS it (else a box). A language's own letters, digits and marks must not need
the fallback (Godot's rule: a Japanese kanji drawn from the Chinese face is wrong even though it shows); the picker's names
and any symbol may use it (Godot gives every other language SC for those).
"""

import csv
import re
import sys
import unicodedata
from pathlib import Path

from fontTools.ttLib import TTCollection, TTFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fonts  # noqa: E402

GODOT = Path("C:/Games/pawpocalypse")
LOCALE = GODOT / "locale"
LOCALES_GD = GODOT / "scripts" / "systems" / "locales.gd"
ST = fonts.ROOT / "Content" / "Pawpocalypse" / "Localization" / "Strings"


# ---- the strings ----
def po_strings(path, field):
    out, cur, key = [], None, None
    for line in path.read_text(encoding="utf-8").splitlines():
        m = re.match(r'^(msgid|msgstr)\s+"(.*)"$', line)
        if m:
            if key == field and cur is not None:
                out.append(cur)
            key, cur = m.group(1), m.group(2)
        elif line.startswith('"') and cur is not None:
            cur += line[1:-1]
        else:
            if key == field and cur is not None:
                out.append(cur)
            key, cur = None, None
    if key == field and cur is not None:
        out.append(cur)
    return [s.encode("utf-8").decode("unicode_escape").encode("latin-1").decode("utf-8") if "\\" in s else s for s in out]


def chars_of(strings):
    cs = set()
    for s in strings:
        for ch in unicodedata.normalize("NFC", s):
            if ch.isspace() or unicodedata.category(ch).startswith("C"):
                continue
            cs.add(ch)
    return cs


def native_names():
    """The language picker's labels (locales.gd NATIVE_NAMES), each in its own language."""
    text = LOCALES_GD.read_text(encoding="utf-8")
    block = text[text.index("const NATIVE_NAMES"):]
    block = block[:block.index("}")]
    names = re.findall(r'^\s*"[A-Za-z_]+":\s*"([^"]+)",', block, re.M)
    if len(names) < 21:
        raise SystemExit("font coverage: read only %d language names from %s" % (len(names), LOCALES_GD))
    return names


def corpus():
    """culture -> (its own characters, the picker's characters)."""
    en = po_strings(LOCALE / "messages.pot", "msgid")
    for f in sorted(ST.glob("*.csv")):
        with f.open(encoding="utf-8") as fh:
            for row in csv.DictReader(fh):
                en.append(row.get("SourceString") or "")
    picker = chars_of(native_names())
    out = {"en": (chars_of(en), picker)}
    for po in sorted(LOCALE.glob("*.po")):
        own = [s for s in po_strings(po, "msgstr") if s]
        if not own:
            raise SystemExit("font coverage: %s has no translations" % po.name)
        out[fonts.culture(po.stem)] = (chars_of(own), picker)
    return out


# ---- the font, as Slate reads it ----
def load_cmaps():
    cmaps = {}
    for name, (file, _policy) in fonts.FACES.items():
        path = fonts.SOURCE / file
        if path.suffix.lower() in (".ttc", ".otc"):
            for i, f in enumerate(TTCollection(str(path)).fonts):
                cmaps[(name, i)] = set(f.getBestCmap())
        else:
            cmaps[(name, 0)] = set(TTFont(str(path)).getBestCmap())
    return cmaps


def prioritized(culture):
    # FInternationalization::GetPrioritizedCultureNames: "zh-Hans" -> zh-Hans, zh.
    parts = culture.split("-")
    return ["-".join(parts[:i]) for i in range(len(parts), 0, -1)]


def resolve(model, culture, cp, default_face):
    """The face Slate draws `cp` with, and whether it came from the fallback. None: a box."""
    cmaps = model["cmaps"]
    names = prioritized(culture)
    sub = None
    for want_culture in (True, False):
        for _n, cultures, ranges, face in model["subfonts"]:
            listed = [c for c in cultures.split(";") if c]
            if want_culture != bool(listed) or (want_culture and not set(listed) & set(names)):
                continue
            if any(lo <= cp <= hi for lo, hi in ranges):
                sub = "default" if face is fonts.DEFAULT_FACES else face
                break
        if sub:
            break
    if sub == "default":
        sub = default_face  # DEFAULT_FACES: the weight being checked
    if sub and cp in cmaps[sub]:
        return sub, False
    if cp in cmaps[default_face]:
        return default_face, False
    fb = model["fallback"]
    return (fb, True) if cp in cmaps[fb] else (None, True)


def check(model, text):
    problems = []
    weights = sorted({f for _n, f in model["default"]})
    for culture, (own, picker) in text.items():
        for ch in sorted(own | picker):
            for weight in weights:
                face, fell = resolve(model, culture, ord(ch), weight)
                if face is None:
                    problems.append("%s: %s U+%04X has no glyph in any face (%s)" % (culture, ch, ord(ch), weight[0]))
                elif fell and ch in own and unicodedata.category(ch)[0] in "LNM":
                    problems.append("%s: %s U+%04X needs the fallback face (%s)" % (culture, ch, ord(ch), weight[0]))
    return problems


def real_model():
    return {"cmaps": load_cmaps(), "subfonts": list(fonts.SUBFONTS), "default": list(fonts.DEFAULT),
            "fallback": fonts.FALLBACK}


def report(problems, text):
    if problems:
        shown = problems[:40]
        print("font coverage: FAIL, %d missing" % len(problems))
        for p in shown:
            print("  " + p)
        if len(problems) > len(shown):
            print("  ... and %d more" % (len(problems) - len(shown)))
        return 1
    n = sum(len(o | p) for o, p in text.values())
    print("font coverage: PASS (%d languages, %d characters checked per weight)" % (len(text), n))
    return 0


def self_test(text):
    ok = True
    real = check(real_model(), text)
    if real:
        print("self-test: the real font fails, fix that first")
        return 1
    # 1. The Thai sub-font gone: Thai falls to the text face (none) and the fallback (none).
    m = real_model()
    m["subfonts"] = [s for s in m["subfonts"] if s[0] != "Thai"]
    p = [x for x in check(m, text) if x.startswith("th:")]
    print("self-test thai sub-font removed: %s (%d th problems)" % ("FAIL as it must" if p else "PASSED: BROKEN GATE", len(p)))
    ok &= bool(p)
    # 2. One Cyrillic letter Russian uses, cut from both text faces: it would fall to the fallback (Noto SC has Cyrillic),
    #    which the rule forbids for a language's own letters.
    own_ru = text["ru"][0]
    letter = next(ch for ch in sorted(own_ru) if "\u0400" <= ch <= "\u04FF")
    m = real_model()
    for _n, face in m["default"]:
        m["cmaps"][face] = m["cmaps"][face] - {ord(letter)}
    p = [x for x in check(m, text) if x.startswith("ru:") and letter in x]
    print("self-test %s cut from the text face: %s" % (letter, "FAIL as it must" if p else "PASSED: BROKEN GATE"))
    ok &= bool(p)
    print("font coverage self-test: %s" % ("OK" if ok else "BROKEN"))
    return 0 if ok else 1


def main(argv):
    sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")
    text = corpus()
    if "--self-test" in argv:
        return self_test(text)
    return report(check(real_model(), text), text)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
