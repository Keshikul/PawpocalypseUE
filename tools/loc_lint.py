"""The translations' wording rules a machine can hold (phase 7, the wording review of 2026-11-20; HANDOFF-LOG (132)).

    py tools/loc_lint.py              the gate: every rule over every culture's files (exit 1 on any finding)
    py tools/loc_lint.py --self-test  each rule must catch a fault planted in a copy of today's rows, and pass the real ones

Reads the English tables and the translations as tools/loc_status.py does (only rows in use: Source = today's English).
A line is held to:
- numbers: every number the English writes (outside {placeholders}) is in the translation. A thousands mark may be the
  language's own ("10,000" -> "10.000", "10 000"); a translation may add numbers (Japanese counters: "1 回").
  The text rules want real numbers (CLAUDE.md, 2026-11-09): "every 100th kill" is not "every hundredth".
- line breaks: as many as the English, except the lines a layout fix split on purpose (SPLIT).
- untranslated: a line of three words or more left as the English ("created by keshi").
- enemies, never mice (designer, 2026-11-09; F55): no mouse or rat word where the English has none (Chinese named
  Swarm, Exterminator and Infestation after rats).
- the port's fixes to Godot's translations (D16, doc 01 §5): F53 (Traditional Chinese "browser refresh" for spawning),
  F54 (Czech "Duhový Hovínko").
- percent: one way per language ("40%", or Turkish "%40"; never both).
"""
import argparse
import copy
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import loc_status as L  # noqa: E402

NUM = re.compile(r"\d+(?:\.\d+)?")
# Bounded by digits, not \b: Korean writes "10,000처치", and a Hangul letter is a word character.
GROUPED_EN = re.compile(r"(?<![\d.,])\d{1,3}(?:,\d{3})+(?![\d.,]\d)")                     # 10,000
GROUPED_ANY = re.compile(r"(?<![\d.,])\d{1,3}(?:[.,   ]\d{3})+(?![\d.,]\d)")     # 10.000, 10 000, 10,000

# Lines a layout fix split on purpose (HANDOFF-LOG (130): the locked Kitten Speed row on two lines).
SPLIT = {"SHOP_kitten_speed_locked"}

MOUSE = {  # cs/pl: a rat exterminator too (deratizátor, deratyzator), 2026-11-20
    "cs": r"\bmyš|\bmyši|\bderatiz", "da": r"\bmus\b|\bmusen\b|\bmusene\b", "de": r"\bmaus\b|\bmäuse", "es": r"\bratón|\bratones|\bratas?\b",
    "es-419": r"\bratón|\bratones|\bratas?\b", "fr": r"\bsouris\b|\brats?\b", "hu": r"\begér|\begere", "it": r"\btop[oi]\b|\bratt",
    "ja": r"ネズミ|鼠", "ko": r"쥐", "nl": r"\bmuis\b|\bmuizen|\bratten?\b", "pl": r"\bmysz|\bszczur|\bderatyz", "pt-BR": r"\bratos?\b",
    "pt-PT": r"\bratos?\b", "ru": r"мыш|крыс", "sv": r"\bmöss\b|\bråttor?\b", "th": r"หนู", "tr": r"\bfare", "uk": r"\bмиш|\bщур|\bпацюк",
    "vi": r"chuột", "zh-Hans": r"鼠", "zh-Hant": r"鼠",
}
# The English names a rat or mouse itself here (the job Ratcatcher, the Pied Piper, Rodent Apocalypse) or the input device.
# Whole words, on the English without its placeholders: "rat" inside "spawn rate" / "crater" and the {mice} count exempted
# a line until 2026-11-20.
MOUSE_EN = re.compile(r"\bmouse\b|\bmice\b|\brats?\b|\bratcatcher|\brodents?\b|\bpied piper\b", re.I)

FIXED = [  # (culture, pattern, the fault): the port's fixes to Godot's translations stay fixed
    ("zh-Hant", "重新整理", "F53: spawning called a browser's refresh"),
    ("cs", "Duhový Hovínko", "F54: a masculine adjective on a neuter noun"),
    ("zh-Hant", "臨界品質", "F56: Critical Mass called critical quality (质量 converted as 品質)"),
]

PCT_AFTER = re.compile(r"(?:\d|\})%")
PCT_BEFORE = re.compile(r"%(?:\d|\{)")


def grouped(s, rx):
    return {re.sub(r"\D", "", m.group(0)) for m in rx.finditer(s)}


def lost_numbers(eng, text):
    e, t = L.TOKEN.sub(" ", eng), L.TOKEN.sub(" ", text)
    big = grouped(e, GROUPED_EN)
    t_big = grouped(t, GROUPED_ANY)
    e = GROUPED_EN.sub(" ", e)
    t = GROUPED_ANY.sub(lambda m: " " if re.sub(r"\D", "", m.group(0)) in big else m.group(0), t)
    want, have = NUM.findall(e), NUM.findall(t)
    lost = [n for n in big if n not in t_big]
    for n in want:
        if n in have:
            have.remove(n)
        else:
            lost.append(n)
    return lost


def words(s):
    return re.findall(r"[^\W\d_]+", L.TOKEN.sub(" ", s))


def lint(eng_tables, trans_by_culture):
    """Every finding: (culture, table, key, rule, detail)."""
    found = []
    for c, tables in trans_by_culture.items():
        styles = {}
        for t, rows in tables.items():
            for k, eng_cell in eng_tables[t].items():
                row = rows.get(k)
                if not row or row[1] != eng_cell or not L.needs(eng_cell):
                    continue
                eng, text = L.unescape(eng_cell), L.unescape(row[0])
                lost = lost_numbers(eng, text)
                if lost:
                    found.append((c, t, k, "numbers", "lost %s: %s" % (" ".join(lost), text)))
                if eng.count("\n") != text.count("\n") and k not in SPLIT:
                    found.append((c, t, k, "line breaks", "%d, the English %d" % (text.count("\n"), eng.count("\n"))))
                if len(words(eng)) >= 3 and text.strip() == eng.strip():
                    found.append((c, t, k, "untranslated", text))
                if not MOUSE_EN.search(L.TOKEN.sub(" ", eng)) and re.search(MOUSE[c], L.TOKEN.sub(" ", text), re.I):
                    found.append((c, t, k, "mice", text))
                for culture, bad, why in FIXED:
                    if c == culture and bad in text:
                        found.append((c, t, k, "fixed fault back", why + ": " + text))
                if PCT_AFTER.search(text):
                    styles.setdefault("40%", (t, k))
                if PCT_BEFORE.search(text):
                    styles.setdefault("%40", (t, k))
        if len(styles) > 1:
            found.append((c, styles["%40"][0], styles["%40"][1], "percent",
                          "both 40%% (%s) and %%40 (%s)" % (styles["40%"][1], styles["%40"][1])))
    return found


def load():
    return L.english(), {c: L.translations(c) for c in L.CULTURES}


def self_test():
    eng, trans = load()
    base = lint(eng, trans)
    ok = True
    if base:
        print("loc_lint self-test: today's rows already fail (%d); run the gate" % len(base))
        return 1

    def plant(culture, table, key, text, rule):
        nonlocal ok
        t2 = copy.deepcopy(trans)
        t2[culture][table][key] = (text, eng[table][key])
        got = [f for f in lint(eng, t2) if f[3] == rule and f[2] == key]
        print("%-7s %-16s %s" % ("CAUGHT" if got else "MISSED", rule, key))
        ok = ok and bool(got)

    def key_with(table, rx):
        return next(k for k, v in eng[table].items() if re.search(rx, L.unescape(v)))

    levy = key_with("ST_Tree", r"100th kill")
    plant("ru", "ST_Tree", levy, L.unescape(trans["ru"]["ST_Tree"][levy][0]).replace("100-е", "сотое"), "numbers")
    plant("de", "ST_Tree", key_with("ST_Tree", r"10,000"), "+1% bis +10% Schaden pro zehntausend Abschüsse.", "numbers")
    two = next(k for k, v in eng["ST_UI"].items() if "\\n" in v and k not in SPLIT)
    plant("fr", "ST_UI", two, L.unescape(trans["fr"]["ST_UI"][two][0]).replace("\n", " "), "line breaks")
    plant("ja", "ST_UI", "TITLE_credits_text", "created by keshi", "untranslated")
    plant("zh-Hans", "ST_Data", "SKILL_swarm_name", "鼠群", "mice")
    plant("de", "ST_UI", key_with("ST_UI", r"enemies"), "Mäuse überall.", "mice")
    plant("cs", "ST_Data", "ACH_kills_1000_name", "Deratizátor", "mice")
    # lines the old English test skipped: "rate" holds "rat", and a {mice} placeholder
    plant("pl", "ST_Data", key_with("ST_Data", r"^Spawn Rate$"), "Tempo szczurów", "mice")
    plant("es", "ST_UI", key_with("ST_UI", r"\{mice\} enemies killed"), "Partida encontrada: {n} Renaceres, {mice} ratones.", "mice")
    plant("zh-Hant", "ST_Data", key_with("ST_Data", r"^Spawn Rate$"), "重新整理速度", "fixed fault back")
    plant("cs", "ST_Data", "SKILL_unlock_poop_name", "Duhový Hovínko", "fixed fault back")
    plant("zh-Hant", "ST_Tree", "TREE_critical_mass_label", "臨界品質", "fixed fault back")
    plant("sv", "ST_UI", "SETTINGS_master", "Huvudvolym  %{n}", "percent")
    # And what must pass: a thousands mark of the language's own, a counter added, a planned split.
    ok_rows = copy.deepcopy(trans)
    body = key_with("ST_Tree", r"10,000")
    ok_rows["fr"]["ST_Tree"][body] = (L.unescape(trans["fr"]["ST_Tree"][body][0]).replace("10 000", "10 000"), eng["ST_Tree"][body])
    passed = not lint(eng, ok_rows)
    print("%-7s %s" % ("PASS" if passed else "FAILED", "a narrow no-break space as the thousands mark passes"))
    ok = ok and passed
    print("loc_lint self-test: %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--self-test", action="store_true")
    a = ap.parse_args()
    if a.self_test:
        return self_test()
    found = lint(*load())
    for c, t, k, rule, detail in found:
        print("%-7s %-16s %-8s %-34s %s" % (c, rule, t, k, detail.replace("\n", " / ")[:140]))
    print("loc_lint: %s (%d findings, %d cultures)" % ("PASS" if not found else "FAIL", len(found), len(L.CULTURES)))
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
