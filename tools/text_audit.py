"""The player-facing text, measured against the designer's rule (2026-11-08): "make sure all text is simple and concise in
the ui. Don't overexplain. Use short sentences." And: no grudges, never an "unimpressed" cat; never "prestige".

Reads every string table (Content/Pawpocalypse/Localization/Strings/ST_*.csv) and flags a string that is over budget:
  - a line (between line breaks) over LINE_CHARS characters; a description that wraps (WRAP_KEY) over WRAP_CHARS in all,
  - a sentence over SENTENCE_WORDS words,
  - more than MAX_SENTENCES sentences (MAX_SENTENCES_WRAP for a description),
  - a label (no sentence end) over LABEL_WORDS words,
  - a word off the one-word-per-idea list (VOCAB; designer 2026-11-09: plain, informative, real numbers),
  - a banned word (BANNED).
A draft that loses or invents a {token} is flagged too.
`{token}`s and `<g:...>` prompts count as one word. Numbers stay numbers.

The drafts (the designer approves each line before anything is written into a table): the newest docs/TEXT-DRAFT-*.csv
(table, key, proposed). This reports which flagged strings have no draft yet, and which drafts are still over budget.

    py tools/text_audit.py                # the report -> build/look/text_audit.md (+ the review page's data, JSON)
    py tools/text_audit.py --self-test    # planted faults must be flagged (each rule seen failing)
    py tools/text_audit.py --banned       # exit 1 when any table holds a banned word (a check_all gate since 2026-11-09)
"""
import csv
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TABLES = ROOT / "Content" / "Pawpocalypse" / "Localization" / "Strings"
DRAFTS = max((ROOT / "docs").glob("TEXT-DRAFT-*.csv"), default=ROOT / "docs" / "TEXT-DRAFT-none.csv")  # the newest
OUT = ROOT / "build" / "look"

LINE_CHARS = 60      # a line that does not wrap (labels, buttons, status lines)
WRAP_CHARS = 120     # a description that wraps in its panel (descriptions, blurbs, notes, hints): the whole text
SENTENCE_WORDS = 12
MAX_SENTENCES = 2
MAX_SENTENCES_WRAP = 3
LABEL_WORDS = 4
BANNED = [("grudg", "no grudges (designer 2026-11-08)"), ("unimpress", "never an unimpressed cat (designer 2026-11-08)"),
          ("prestige", "never 'prestige': it is Reawaken (CLAUDE.md)")]
# One word per concept (designer, 2026-11-09: "Use the same words for same concepts. So increase is increase everywhere
# not grows in some."). Each entry: the words that stand in for a concept, and the one word the game uses. Names
# (LABEL_KEY) are exempt; {tokens} are stripped first ({branch} is an argument, not a word).
VOCAB = [
    (r"\bgr[eo]w(s|n|ing)?\b|\brises?\b|\bclimbs?\b|\braises?\b", "increase"),
    (r"\bfrom now on\b|\bfor good\b", "permanently"),
    # designer 2026-11-09: "change mice to enemies since we don't only kill mice" (the input device stays "mouse")
    (r"(?<!keyboard and )(?<!on the )\bmouse('s)?\b|\bmice\b|\bsurvivors?\b", "enemy / enemies"),
    (r"\bwander(s|ed|ing)? off\b|\bleaves? without\b", "escape"),
    (r"\bthis (life|cycle)\b|\byour life\b", "this run"),
    (r"\bswings?\b|\bswung\b", "attack"),
    (r"\baura (size|reach)\b|\bwidens?\b", "aura radius"),
    (r"\bshop lines?\b|\bshop's\b", "upgrade"),
    (r"\bmice caught\b|\bcatch mice\b", "kill"),
    (r"\bnearly[- ]dead\b|\bfinish(es)? off\b|\bput down\b", "kill / health"),
    (r"\brealm list\b", "Realms"),
    (r"\bclosed\b", "locked"),
    (r"\bnodes?\b|\bbranch(es)?\b", "skill"),
    (r"\b\d+(\.\d+)?x\b", "x before the number (x2)"),
]
VOCAB_RE = [(re.compile(p, re.IGNORECASE), want) for p, want in VOCAB]

LABEL_KEY = re.compile(r"(_label|_name|_title|_button)$|^TAB_")  # names, captions, tabs, buttons: the label rule
WRAP_KEY = re.compile(r"_(desc|blurb|note|hint)(_|$)|^TITLE_leaderboard_(rank|offline|before_clock)$")
TOKEN = re.compile(r"\{[^}]*\}|<[^>]*>")
SENTENCE_END = re.compile(r"(?<=[.!?])\s+")


def words(text):
    return TOKEN.sub("X", text).split()


def sentences(text):
    flat = " ".join(text.split("\n"))
    return [s for s in SENTENCE_END.split(flat.strip()) if s]


def problems(text, key=""):
    out = []
    low = text.lower()
    for stem, why in BANNED:
        if stem in low:
            out.append("banned: " + why)
    text = text.replace("\\n", "\n")  # a table's written line break
    wraps = bool(WRAP_KEY.search(key))
    if wraps:
        shown = len(TOKEN.sub("XX", " ".join(text.split())))
        if shown > WRAP_CHARS:
            out.append("text %d chars (> %d)" % (shown, WRAP_CHARS))
    else:
        for line in text.split("\n"):
            shown = len(TOKEN.sub("XX", line))  # a {token} shows as a glyph or a number: about two characters
            if shown > LINE_CHARS:
                out.append("line %d chars (> %d)" % (shown, LINE_CHARS))
                break
    ends = bool(re.search(r"[.!?]\s*$", text.strip()))
    if not ends and "\n" not in text and LABEL_KEY.search(key):
        if len(words(text)) > LABEL_WORDS:
            out.append("label %d words (> %d)" % (len(words(text)), LABEL_WORDS))
    sents = sentences(text)
    longest = max((len(words(s)) for s in sents), default=0)
    if longest > SENTENCE_WORDS:
        out.append("sentence %d words (> %d)" % (longest, SENTENCE_WORDS))
    most = MAX_SENTENCES_WRAP if wraps else MAX_SENTENCES
    if len(sents) > most:
        out.append("%d sentences (> %d)" % (len(sents), most))
    if not LABEL_KEY.search(key):
        bare = TOKEN.sub("9", text)  # a token reads as a number: {v}x is caught, {branch} is not a word
        for rx, want in VOCAB_RE:
            m = rx.search(bare)
            if m:
                out.append("word '%s': say %s" % (m.group(0), want))
    return out


def tokens(text):
    return sorted(TOKEN.findall(text.replace("\\n", "\n")))


def draft_problems(text, draft, key):
    """A draft's own problems, plus any {token} it lost or invented (the code fills them by name)."""
    out = problems(draft, key)
    if tokens(text) != tokens(draft):
        out.append("tokens %s, the table has %s" % (" ".join(tokens(draft)) or "none", " ".join(tokens(text)) or "none"))
    return out


def read_tables():
    rows = []
    for path in sorted(TABLES.glob("ST_*.csv")):
        with path.open(encoding="utf-8", newline="") as f:
            reader = csv.reader(f)
            next(reader, None)
            for r in reader:
                if len(r) >= 2:
                    rows.append({"table": path.stem, "key": r[0], "text": r[1], "comment": r[2] if len(r) > 2 else ""})
    return rows


def read_drafts():
    if not DRAFTS.exists():
        return {}
    with DRAFTS.open(encoding="utf-8", newline="") as f:
        return {(r["table"], r["key"]): r for r in csv.DictReader(f)}


def report():
    rows = read_tables()
    drafts = read_drafts()
    flagged = []
    for r in rows:
        p = problems(r["text"], r["key"])
        d = drafts.get((r["table"], r["key"]))
        if p or d:  # a draft can fix a string that is within budget (vague, or no numbers): it is listed too
            r = dict(r, problems=p, draft=d["proposed"] if d else None, note=d.get("note", "") if d else "",
                     draft_problems=draft_problems(r["text"], d["proposed"], r["key"]) if d and d["proposed"] else [])
            flagged.append(r)
    stale = sorted(set(drafts) - {(r["table"], r["key"]) for r in rows})
    for t, k in stale:
        print("  draft for a key no table has: %s %s" % (t, k))
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "text_audit.json").write_text(json.dumps(flagged, ensure_ascii=False, indent=1), encoding="utf-8")
    lines = ["# Text audit (%d of %d strings over budget or drafted; drafts: %s)" % (len(flagged), len(rows), DRAFTS.name), "",
             "| Table | Key | Now | Problems | Draft | Draft's problems |", "|---|---|---|---|---|---|"]
    cell = lambda s: (s or "").replace("|", "/").replace("\n", " / ")
    for r in flagged:
        lines.append("| %s | %s | %s | %s | %s | %s |" % (r["table"], r["key"], cell(r["text"]), "; ".join(r["problems"]),
                                                       cell(r["draft"]) if r["draft"] is not None else "(none)",
                                                       "; ".join(r["draft_problems"])))
    (OUT / "text_audit.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    undrafted = [r for r in flagged if r["draft"] is None]
    over = [r for r in flagged if r["draft_problems"]]
    by_table = {}
    for r in flagged:
        if r["problems"]:
            by_table[r["table"]] = by_table.get(r["table"], 0) + 1
    print("text_audit: %d of %d strings break a rule %s; %d drafts (%s); %d flagged without a draft; %d drafts still break one"
          % (sum(1 for r in flagged if r["problems"]), len(rows), by_table, len(drafts), DRAFTS.name, len(undrafted), len(over)))
    for r in undrafted:
        print("  no draft: %s %s: %s" % (r["table"], r["key"], "; ".join(r["problems"])))
    for r in over:
        print("  draft over budget: %s %s: %s" % (r["table"], r["key"], "; ".join(r["draft_problems"])))
    print("  -> %s" % (OUT / "text_audit.md"))
    return flagged


def banned_gate():
    bad = [(r["table"], r["key"], p) for r in read_tables() for p in problems(r["text"], r["key"]) if p.startswith("banned")]
    for b in bad:
        print("BANNED %s %s: %s" % b)
    print("text_audit --banned: %s" % ("FAIL" if bad else "PASS"))
    return 1 if bad else 0


def self_test():
    cases = [
        ("a long line", "x" * (LINE_CHARS + 1) + ".", "line"),
        ("a long sentence", " ".join(["word"] * (SENTENCE_WORDS + 1)) + ".", "sentence"),
        ("three sentences", "One. Two. Three.", "sentences"),
        ("a long label", "One two three four five", "label", "SKILL_x_name"),
        ("a grudge", "Hold a grudge.", "banned"),
        ("an unimpressed cat", "The cat is Unimpressed.", "banned"),
        ("prestige", "Prestige now.", "banned"),
        ("a long description", " ".join(["Word five six."] * 12), "text", "TREE_x_blurb"),
        ("four sentences", "One. Two. Three. Four.", "sentences", "TREE_x_blurb"),
        ("grows", "Your aura grows.", "increase"),
        ("for good", "One more card, for good.", "permanently"),
        ("mice", "Signature mice pay more.", "enemies"),
        ("a mouse", "A mouse turns back.", "enemies"),
        ("wander off", "Mice that wander off pay.", "escape"),
        ("this life", "No cards taken this life.", "this run"),
        ("swing", "Every tenth swing crits.", "attack"),
        ("aura size", "x{v} aura size", "aura radius"),
        ("shop line", "The shop line does it.", "upgrade"),
        ("branch", "Buy the Aura branch.", "skill"),
        ("2.5x", "Catnip 2.5x.", "x before"),
        ("{v}x", "Catnip {v}x.", "x before"),
    ]
    ok = True
    for case in cases:
        name, text, want = case[:3]
        got = problems(text, case[3] if len(case) > 3 else "")
        hit = any(want in p for p in got)
        print("  %-20s %s %s" % (name, "FLAGGED" if hit else "MISSED", got))
        ok &= hit
    clean = [("Take one.", ""), ("Paw Damage", "SKILL_x_name"), ("+{v}% damage per card taken this run", "FX_X"),
             ("Close", "SETTINGS_close"), ("Locked — you took {branch}.", "JOBS_closed"), ("Aura Reach", "SKILL_x_name"),
             ("Catnip x2.5. Kitten damage x0.4.", "JOB_x_desc"), ("Keyboard and mouse", "SETTINGS_x"),
             ("Signature enemies give x{v} Catnip.", "CARD_x_desc"), ("{mice} enemies killed.", "TITLE_x"),
             ("Increases damage. +15% to +130% and x1.06 to x1.75 over 10 levels.", "TREE_x_blurb")]
    for text, key in clean:
        got = problems(text, key)
        print("  %-20s %s %s" % ("clean: " + text[:14], "clean" if not got else "FLAGGED (wrong)", got))
        ok &= not got
    lost = draft_problems("Teaches {pounce}.", "Teaches Pounce.", "TREE_x_blurb")
    print("  %-20s %s %s" % ("a lost token", "FLAGGED" if any("tokens" in p for p in lost) else "MISSED", lost))
    ok &= any("tokens" in p for p in lost)
    print("text_audit --self-test: %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    if "--self-test" in sys.argv:
        sys.exit(self_test())
    if "--banned" in sys.argv:
        sys.exit(banned_gate())
    report()
