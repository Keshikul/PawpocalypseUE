// Records the approved Bestiary's cast as Python (D18, S5; 2026-11-18): tools/art/sticker_cast.py.
//
//     node tools/art/sticker_record.js            (writes tools/art/sticker_cast.py and tools/art/sticker_colours.json)
//     node tools/art/sticker_record.js --check    (exit 1 when either differs from what the concept records)
//
// tools/art/concept_bestiary.html is the approved concept (the Bestiary artifact, version 5: "The enemies all seem good",
// designer, 2026-11-18). Its drawing code is run here as it runs in the browser, with its drawing primitives replaced by
// recorders: every ellipse, circle, path, tube, rect, ring, group, eye, spark and blob it draws becomes one call of the same
// primitive in tools/art/sticker_draw.py, with the numbers it was given and its colours by their names in the concept's
// colour table (L.c) or as the mix() that made them. So the game draws exactly what the designer approved, and a change to
// the concept is a re-record, never a hand edit of sticker_cast.py.
'use strict';
const fs = require('fs');
const path = require('path');

const HERE = __dirname;
const CHECK = process.argv.includes('--check');
const html = fs.readFileSync(path.join(HERE, 'concept_bestiary.html'), 'utf8');
const script = html.slice(html.lastIndexOf('<script>') + 8, html.lastIndexOf('</script>'));
const head = script.slice(0, script.indexOf('const BIOMES=['));

// ---- the recorder ----
let OUT = [], IND = 1, NAMES = null;
const num = v => {
  if (typeof v !== 'number' && !(v instanceof Number)) throw new Error('not a number: ' + v);
  const s = String(+v);
  return s === '-0' ? '0' : s;
};
// Colours: the concept's L.c entries become tagged strings (their name rides along); mix() results carry their recipe.
function tag(hex, expr) { const s = new String(hex); s.expr = expr; return s; }
const LITERALS = {};   // a hex written in the drawing code itself, not from L.c: named here (sticker_colours.json)
const EXTRA_NAMES = {'#45404F': 'pigeonBeak', '#F4F2F6': 'pigeonBeakShine', '#E04F3A': 'gullMouth', '#3A3448': 'parrotBeak',
  '#57506A': 'parrotBeakShine', '#120E24': 'voidMouth',
  // the jobs' hats (tools/art/concept_hats.js, the UI concept's hatFig)
  '#8A5A3A': 'hatCap', '#6E4529': 'hatCapBrim', '#3B3546': 'hatTop', '#4A4458': 'hatKnitBand', '#6A6478': 'hatKnitRib',
  '#5E3F8E': 'hatWizard', '#A88BFF': 'hatNemesStripe', '#58B6FF': 'hatTricorn', '#C93323': 'hatRed', '#FFC83D': 'hatYellow',
  // the UI concept's controller icon (its IC.pad; the toast)
  '#5B4A72': 'uiPad'};
function colour(f) {
  if (f === 'INK') return '"INK"';
  if (f instanceof String && f.expr) return f.expr;
  const hex = String(f).toUpperCase();
  if (!/^#[0-9A-F]{6}$/.test(hex)) throw new Error('not a colour: ' + f);
  if (hex === '#2A2131') return 'INK';   // L.ink, as a mix() end (sticker_draw.INK)
  if (NAMES[hex]) return 'c["' + NAMES[hex] + '"]';
  if (EXTRA_NAMES[hex]) { LITERALS[EXTRA_NAMES[hex]] = hex; return 'c["' + EXTRA_NAMES[hex] + '"]'; }
  throw new Error('a literal colour with no name: ' + hex + ' (add it to EXTRA_NAMES)');
}
function opts(o) {
  if (!o) return '';
  const keys = Object.keys(o).filter(k => o[k] !== undefined);
  if (!keys.length) return '';
  if (keys.length === 1 && keys[0] === 'noLine' && o.noLine) return ', NO';
  const parts = keys.map(k => {
    if (k === 'lw') {
      const v = +o.lw;
      for (const [m, s] of [[1, 'EYE_LW'], [0.6, 'EYE_LW * 0.6'], [0.7, 'EYE_LW * 0.7'], [0.8, 'EYE_LW * 0.8']])
        if (v === 3 * m) return '"lw": ' + s;
      return '"lw": ' + num(v);
    }
    if (k === 'noLine') return '"noLine": ' + (o.noLine ? 'True' : 'False');
    throw new Error('an option the recorder does not know: ' + k);
  });
  return ', {' + parts.join(', ') + '}';
}
const emit = line => { OUT.push('    '.repeat(IND) + line); return ''; };
const pystr = s => JSON.stringify(String(s));

const RECORDERS = `
function S(tag,a,fill,f={}){
  if(tag==='ellipse'){if(a.transform){const m=/rotate\\(([-\\d.]+) /.exec(a.transform);return __emit('E2('+[a.cx,a.cy,a.rx,a.ry,+m[1]].map(__num).join(', ')+', '+__colour(fill)+__opts(f)+')');}
    return __emit('E('+[a.cx,a.cy,a.rx,a.ry].map(__num).join(', ')+', '+__colour(fill)+__opts(f)+')');}
  if(tag==='circle')return __emit('C('+[a.cx,a.cy,a.r].map(__num).join(', ')+', '+__colour(fill)+__opts(f)+')');
  if(tag==='path')return __emit('P('+__pystr(a.d)+', '+__colour(fill)+__opts(f)+')');
  if(tag==='rect'){if(a['fill-opacity']!==undefined)return __emit('GLASS('+[a.x,a.y,a.width,a.height,a.rx,a['fill-opacity']].map(__num).join(', ')+', '+__colour(fill)+__opts(f)+')');
    return __emit('R('+[a.x,a.y,a.width,a.height,a.rx].map(__num).join(', ')+', '+__colour(fill)+__opts(f)+')');}
  throw new Error('S: '+tag);}
function T(d,w,fill,f={}){return __emit('T('+__pystr(d)+', '+__num(w)+', '+__colour(fill)+__opts(f)+')');}
function RING(cx,cy,rx,ry,w,col){return __emit('RING('+[cx,cy,rx,ry,w].map(__num).join(', ')+', '+__colour(col)+')');}
function G(tx,ty,s,rot,fn,flip){__emit('with at('+[tx,ty,s,rot].map(__num).join(', ')+(flip?', flip=True':'')+'):');
  const n=__out().length;__ind(1);fn();if(__out().length===n)__emit('pass');__ind(-1);return '';}
function mix(a,b,t){const A=hex2rgb(String(a)),B=hex2rgb(String(b));const hex=rgb2hex(A.map((v,i)=>v+(B[i]-v)*t));
  return __tag(hex,'mix('+__colour(a)+', '+__colour(b)+', '+__num(t)+')');}
function eye(x,y,r,px=0,py=0){return __emit('eye('+[x,y,r,px,py].map(__num).join(', ')+')');}
function spark(x,y,r,col){return __emit('spark('+[x,y,r].map(__num).join(', ')+', '+__colour(col)+')');}
function blob(x,y,s,col){return __emit('blob('+[x,y,s].map(__num).join(', ')+', '+__colour(col)+')');}
function bossEyes(x1,y1,x2,y2,r){return __emit('bossEyes('+[x1,y1,x2,y2,r].map(__num).join(', ')+')');}
`;

const stub = new Proxy(function () {}, { get: (t, p) => (p === Symbol.toPrimitive ? () => '' : stub), apply: () => stub, set: () => true });
const recArgs = ['document', '__emit', '__num', '__colour', '__opts', '__pystr', '__tag', '__out', '__ind'];
const recVals = () => [stub, emit, num, colour, opts, pystr, tag, () => OUT, d => { IND += d; }];
const run = new Function('document', '__emit', '__num', '__colour', '__opts', '__pystr', '__tag', '__out', '__ind',
  head + RECORDERS +
  '\nreturn {L, FIG, catFig, riderFig, catStageFn, gristleFig, tyrantFig, championFig, voidFig, foremanFig, kingFig, addFig};');
const B = run(stub, emit, num, colour, opts, pystr, tag, () => OUT, d => { IND += d; });

// The colour table: every L.c entry by name (the rainbow as rainbow0..5), each a tagged string so its name rides along.
const COLOURS = {};
NAMES = {};
for (const [k, v] of Object.entries(B.L.c)) {
  if (Array.isArray(v)) {
    v.forEach((h, i) => { COLOURS[k + i] = h.toUpperCase(); B.L.c[k][i] = tag(h, 'c["' + k + i + '"]'); });
  } else {
    COLOURS[k] = v.toUpperCase();
    B.L.c[k] = tag(v, 'c["' + k + '"]');
    if (!NAMES[v.toUpperCase()]) NAMES[v.toUpperCase()] = k;   // a literal hex takes the first name that has it
  }
}
B.L.c.rainbow.forEach((h, i) => { if (!NAMES[String(h).toUpperCase()]) NAMES[String(h).toUpperCase()] = 'rainbow' + i; });

// The kitten's colours are the UI Screens concept's (tools/art/concept_ui_screens.html, version 5; the Bestiary draws no
// kitten): its L.c kitten, kittenBelly, kittenStripe, drawn with the Bestiary's own catFig.
const ui = fs.readFileSync(path.join(HERE, 'concept_ui_screens.html'), 'utf8');
for (const k of ['kitten', 'kittenBelly', 'kittenStripe']) {
  const m = new RegExp(k + ":'(#[0-9A-Fa-f]{6})'").exec(ui);
  if (!m) throw new Error('the UI Screens concept has no ' + k);
  COLOURS[k] = m[1].toUpperCase();
  B.L.c[k] = tag(m[1], 'c["' + k + '"]');
}

// ---- the cast: the Bestiary's key -> the game's file stem(s) (SourceArt/svg) and how it is called ----
const GRISTLE_STEMS = ['backyard', 'alley', 'park', 'rooftop', 'sewer', 'turf', 'portal', 'mythic', 'kitchen', 'vet',
  'factory', 'litterbox'];
const CAST = [
  // [python name, stems, the figure, what it is]
  ['mouse', ['enemy_backyard_mouse'], B.FIG.mouse[1], 'the Backyard mouse'],
  ['vole', ['enemy_alley_vole'], B.FIG.vole[1], 'the Alley vole'],
  ['rat', ['enemy_park_rat'], B.FIG.rat[1], 'the Park rat'],
  ['pigeon', ['enemy_rooftop_pigeon'], B.FIG.pigeon[1], 'the Rooftops pigeon'],
  ['sewer_rat', ['enemy_sewer_rat'], B.FIG.sewerrat[1], 'the sewer rat (a plaster on its head)'],
  ['slime_rat', ['enemy_turf_slime_rat'], B.FIG.slimerat[1], 'the slime rat (melting)'],
  ['ghost', ['enemy_portal_ghost'], B.FIG.ghost[1], 'the Portal ghost'],
  ['imp', ['enemy_mythic_imp'], B.FIG.imp[1], 'the Mythic imp'],
  ['ant', ['enemy_kitchen_ant'], B.FIG.ant[1], 'the Kitchen ant'],
  ['hamster', ['enemy_vet_hamster'], B.FIG.hamster[1], "the Vet's hamster"],
  ['factory_rat', ['enemy_factory_rat'], B.FIG.facrat[1], 'the factory rat (goggles)'],
  ['dust_bunny', ['enemy_litterbox_dust_bunny'], B.FIG.dust[1], 'the Litterbox dust bunny'],
  ['moth', ['enemy_backyard_moth'], B.FIG.moth[1], 'signature: the moth'],
  ['dog', ['enemy_alley_dog'], B.FIG.dog[1], 'signature: the dog'],
  ['goose', ['enemy_park_goose'], B.FIG.goose[1], 'signature: the goose'],
  ['seagull', ['enemy_rooftop_seagull'], B.FIG.seagull[1], 'signature: the seagull'],
  ['crocodile', ['enemy_sewer_crocodile'], B.FIG.croc[1], 'signature: the crocodile'],
  ['toad', ['enemy_turf_toad'], B.FIG.toad[1], 'signature: the toad'],
  ['hound', ['enemy_portal_hound'], B.FIG.hound[1], 'signature: the hound'],
  ['crow', ['enemy_mythic_crow'], B.FIG.crow[1], 'signature: the crow'],
  ['cockroach', ['enemy_kitchen_cockroach'], B.FIG.roach[1], 'signature: the cockroach'],
  ['parrot', ['enemy_vet_parrot'], B.FIG.parrot[1], 'signature: the parrot'],
  ['robot_mouse', ['enemy_factory_robot_mouse'], B.FIG.robot[1], 'signature: the robot mouse'],
  ['sludge_cat', ['enemy_litterbox_sludge_cat'], B.FIG.sludge[1], 'signature: the sludge cat'],
  ['flying_mouse', ['enemy_mouse_flying'], B.FIG.flymouse[1], 'the flying mouse (Alley, Park)'],
  ['bat', ['enemy_bat'], B.FIG.bat[1], 'the bat (the flier from the Rooftops on)'],
  ['golden_mouse', ['enemy_mouse_golden'], B.FIG.goldenplain[1], 'the golden mouse (the game sparkles it: EnemyData.sparkle)'],
];
for (let t = 1; t <= 12; t++)
  CAST.push(['gristle_' + t, ['enemy_' + GRISTLE_STEMS[t - 1] + '_gristle'], () => B.gristleFig((t - 1) / 11),
    'the gristle of tier ' + t + ' (its spikes, lumps and scars grow with the tier)']);
CAST.push(['park_tyrant', ['boss_park_tyrant'], B.tyrantFig, 'boss: The Park Tyrant'],
  ['rival_champion', ['boss_rival_champion'], B.championFig, 'boss: The Rival Champion'],
  ['void_god', ['boss_void_god'], B.voidFig, 'boss: The Void God'],
  ['foreman', ['boss_factory_foreman'], B.foremanFig, 'boss: The Foreman'],
  ['litter_king', ['boss_litter_king'], B.kingFig, 'boss: The Litter King'],
  ['add', ['boss_add'], B.addFig, "a boss's add (the plus: it heals the boss)"]);

const lines = [];
const fns = [];
for (const [name, stems, fn, what] of CAST) {
  OUT = []; IND = 1;
  fn();
  fns.push('def ' + name + '():\n    """' + what.charAt(0).toUpperCase() + what.slice(1) + '."""\n' + OUT.join('\n') + '\n');
}
// The cat (sticker_art fits each group of stages in one frame, so the hat and the weapon sit on every stage of it). The
// weapon is its own sprite in the game (drawn over the cat): cut out of the concept's figure, the block it draws last
// (catFig: the sword, the visor gun) or the lance's group (riderFig).
function rec(fn) { OUT = []; IND = 1; fn(); return OUT.slice(); }
const player = [];
for (let n = 1; n <= 3; n++) {
  const bare = rec(() => B.catFig({stage: n, noWeapon: true})), full = rec(() => B.catFig({stage: n}));
  if (full.slice(0, bare.length).join('\n') !== bare.join('\n')) throw new Error('stage ' + n + ': the weapon is not drawn last');
  player.push(['cat_stage_' + n, bare, 'the cat, stage ' + n + ' (no weapon: its weapon is its own sprite)']);
  if (full.length > bare.length) player.push(['weapon_stage_' + n, full.slice(bare.length), 'the weapon of stage ' + n]);
}
const LANCE = '    with at(30, -70, 1, 30):';
for (let n = 4; n <= 7; n++) {
  const all = rec(() => B.riderFig(n)), i = all.indexOf(LANCE);
  if (i < 0) throw new Error('stage ' + n + ': no lance at ' + LANCE.trim());
  let j = i + 1;
  while (j < all.length && all[j].startsWith('        ')) j++;
  player.push(['cat_stage_' + n, all.slice(0, i).concat(all.slice(j)), 'the cat, stage ' + n + ', mounted (no lance: the lance is its own sprite)']);
  player.push(['weapon_stage_' + n, all.slice(i, j), 'the lance of stage ' + n]);
}
player.push(['kitten', rec(() => B.catFig({fur: B.L.c.kitten, belly: B.L.c.kittenBelly, stripe: B.L.c.kittenStripe, noWeapon: true})),
  'a kitten (the cat drawn in the kitten colours, no weapon)']);
for (const [name, body, what] of player)
  fns.push('def ' + name + '():\n    """' + what.charAt(0).toUpperCase() + what.slice(1) + '."""\n' + body.join('\n') + '\n');
// The icons (S6, first part): the UI Screens concept's kit icons (its IC table, drawn with the concept's icon line, 2.6, and
// no rim), recorded the same way from tools/art/concept_ui_screens.html, the game's icon files they replace beside them.
// The concept's UI colours (its U table) are named "ui" + the key; its creature colours are the Bestiary's.
const uiHtml = fs.readFileSync(path.join(HERE, 'concept_ui_screens.html'), 'utf8');
const uiScript = uiHtml.slice(uiHtml.lastIndexOf('<script>') + 8, uiHtml.indexOf('/* ---------- the page ---------- */'));
// The held paw is drawn inside the concept's kit (kitSVG's closedPaw): its statement is lifted out and run beside IC.
const held = /const closedPaw=[\s\S]*?\{noLine:true\}\);/.exec(uiScript);
if (!held) throw new Error("the UI concept's kit has no closedPaw");
// The jobs' hats (approved 2026-11-18, tools/art/concept_hats.js) run beside the concept: three are its hatFig's, seven drawn in
// its grammar on the Missing Art page.
const hatsJs = fs.readFileSync(path.join(HERE, 'concept_hats.js'), 'utf8');
// The upgrade icons approved on the Missing Art page (tools/art/concept_icons.js): they replace the concept's or fill a gap.
const iconsJs = fs.readFileSync(path.join(HERE, 'concept_icons.js'), 'utf8');
const UIC = new Function(...recArgs, uiScript + RECORDERS + '\n' + held[0] + '\n' + hatsJs + '\n' + iconsJs
  + '\nreturn {L, U, IC, SHOP, closedPaw, hatOf, JOB_HATS, APPROVED_ICONS, FILE_ICONS};')(...recVals());
for (const [k, v] of Object.entries(UIC.L.c)) {
  if (Array.isArray(v)) { v.forEach((h, i) => { UIC.L.c[k][i] = tag(h, 'c["' + k + i + '"]'); }); continue; }
  if (COLOURS[k] && COLOURS[k] !== v.toUpperCase()) throw new Error('the two concepts disagree on ' + k);
  COLOURS[k] = v.toUpperCase();
  UIC.L.c[k] = tag(v, 'c["' + k + '"]');
}
for (const [k, v] of Object.entries(UIC.U)) {
  const name = 'ui' + k.charAt(0).toUpperCase() + k.slice(1);
  COLOURS[name] = v.toUpperCase();
  UIC.U[k] = tag(v, 'c["' + name + '"]');
}
const ICON_FILES = [
  ['ui_tab_upgrades', 'paw'], ['ui_tab_reawaken', 'reawaken'], ['ui_tab_realms', 'flag'], ['ui_tab_catches', 'cards'],
  ['ui_tab_awards', 'trophy'], ['ui_tab_jobs', 'hat'], ['ui_quick_settings', 'gear'], ['ui_quick_skills', 'tree'],
  ['ui_quick_save', 'save'], ['ui_quick_quit', 'quit'], ['ui_menu_bug', 'bug'], ['ui_menu_discord', 'chat'],
  ['ui_menu_language', 'globe'], ['ui_icon_catnip', 'leaf'], ['ui_icon_stardust', 'star'], ['ui_tick', 'check'],
  ['ui_close', 'close'], ['ui_lock', 'lock'],
  ['ui_icon_pad', 'pad']];  // D18 (S4): the controller toast's icon (the concept's scrUpgrades toast)
const iconRows = [], upgradeRows = [], recorded = new Set();
function recordIcon(key) {
  if (!UIC.IC[key]) throw new Error('the UI concept has no icon ' + key);
  if (recorded.has(key)) return;
  recorded.add(key);
  fns.push('def icon_' + key + '():\n    """The ' + key + ' icon (the UI concept\'s kit)."""\n' + rec(() => UIC.IC[key]()).join('\n') + '\n');
}
for (const [file, key] of ICON_FILES) {
  recordIcon(key);
  iconRows.push('    "' + file + '": icon_' + key + ',');
}
// The upgrades the concept's shop draws (its SHOP rows: an icon beside each name): ui_icon_upgrade_<the upgrade's id>.
for (const [key, name] of UIC.SHOP) {
  recordIcon(key);
  upgradeRows.push('    "ui_icon_upgrade_' + String(name).toLowerCase().replace(/ /g, '_') + '": icon_' + key + ',');
}
// The approved upgrade icons (Missing Art, tools/art/concept_icons.js) over the shop's: ui_icon_upgrade_<the upgrade's id>.
for (const [id, fn] of Object.entries(UIC.APPROVED_ICONS)) {
  const file = '"ui_icon_upgrade_' + id + '"';
  for (let i = upgradeRows.length - 1; i >= 0; i--) if (upgradeRows[i].trim().startsWith(file + ':')) upgradeRows.splice(i, 1);
  fns.push('def icon_upgrade_' + id + '():\n    """The ' + id.replace(/_/g, ' ') + ' upgrade\'s icon (approved on Missing Art, 2026-11-18)."""\n'
    + rec(() => fn()).join('\n') + '\n');
  upgradeRows.push('    ' + file + ': icon_upgrade_' + id + ',');
}
// Approved icons by the game's file (Missing Art's Bait): over the grammar's drawing of that file.
for (const [file, fn] of Object.entries(UIC.FILE_ICONS)) {
  fns.push('def icon_file_' + file + '():\n    """' + file + ' (approved on Missing Art, 2026-11-18)."""\n' + rec(() => fn()).join('\n') + '\n');
  iconRows.push('    "' + file + '": icon_file_' + file + ',');
}
// The jobs' hats, in the Tiny Kitten's frame (the game draws a hat as its own sprite over the cat: sticker_art places it).
const hatRows = [];
for (const [job, key] of Object.entries(UIC.JOB_HATS)) {
  fns.push('def hat_' + job + '():\n    """The ' + job.replace(/_/g, ' ') + ' hat (' + key + '; approved 2026-11-18)."""\n'
    + rec(() => UIC.hatOf(key)()).join('\n') + '\n');
  hatRows.push('    "' + job + '": hat_' + job + ',');
}
// The cursors: the kit's open paw (its paw icon) and held paw, drawn as the kit draws them (line 2.8, the white rim).
fns.push('def cursor_paw_held():\n    """The held paw: the toes closed as arcs (the UI concept\'s kit)."""\n' + rec(() => UIC.closedPaw()).join('\n') + '\n');
for (const [k, v] of Object.entries(LITERALS)) COLOURS[k] = v;

const py = `"""The approved Bestiary's cast in the generator grammar (D18, S5): RECORDED, never edited by hand.

Made by \`node tools/art/sticker_record.js\` from tools/art/concept_bestiary.html, the approved concept (the Bestiary
artifact, version 5; designer, 2026-11-18: "The enemies all seem good"). Each function below is one character's drawing as
the concept's own code drew it: the same primitives (tools/art/sticker_draw.py), the same numbers, the colours by their
names in the concept's table (palette.STICKER_CREATURES, written from tools/art/sticker_colours.json) or as the mix() that
made them. To change a drawing, change the concept and record again. The kitten's colours, the UI icons and the cursors
come the same way from tools/art/concept_ui_screens.html (the UI Screens artifact, version 5): its kit.

Units are the concept's: feet on y = 0, facing left; sticker_draw.fit() puts each drawing and its rim in the 128 box.
"""
# flake8: noqa  (recorded code: one call per line, as the concept draws)
from __future__ import annotations

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from sticker_draw import C, E, E2, EYE_LW, GLASS, INK, NO, P, R, RING, T, at, blob, bossEyes, eye, mix, spark  # noqa: E402
import palette as PAL  # noqa: E402

c = PAL.STICKER_CREATURES


` + fns.join('\n\n') + `

# The cast: python name -> (the game's file stems, the figure). Stems follow SourceArt/svg.
CAST = {
${CAST.map(([name, stems]) => '    "' + name + '": ((' + stems.map(s => '"' + s + '"').join(', ') + ',), ' + name + '),').join('\n')}
    "kitten": (("kitten_basic",), kitten),
    "cat_base": (("player_cat_base",), cat_stage_1),        # the base drawing the grammar keeps (no stage ships it)
}

# The cat: each group of stages shares one frame (sticker_art), and each stage's weapon is its own sprite in that frame.
STANDING = (("player_cat_stage_1", cat_stage_1), ("player_cat_stage_2", cat_stage_2), ("player_cat_stage_3", cat_stage_3))
STANDING_WEAPONS = (("player_weapon_stage_2", weapon_stage_2), ("player_weapon_stage_3", weapon_stage_3))
MOUNTED = (("player_cat_stage_4", cat_stage_4), ("player_cat_stage_5", cat_stage_5), ("player_cat_stage_6", cat_stage_6),
           ("player_cat_stage_7", cat_stage_7))
MOUNTED_WEAPONS = (("player_weapon_stage_4", weapon_stage_4), ("player_weapon_stage_5", weapon_stage_5),
                   ("player_weapon_stage_6", weapon_stage_6), ("player_weapon_stage_7", weapon_stage_7))

# The UI icons the concept's kit draws: the game's file (SourceArt/svg/ui) -> its figure, drawn at the icon line (2.6).
ICONS = {
${iconRows.join('\n')}
}

# The upgrades' icons the concept's shop draws (new files: the grammar drew none): the game's file -> its figure.
UPGRADE_ICONS = {
${upgradeRows.join('\n')}
}

# The jobs' hats (approved 2026-11-18, tools/art/concept_hats.js): the game's job id -> its hat, in the Tiny Kitten's frame. The
# game's files: hat_<id> (the field's sprite, sticker_art places it on the cat) and job_<id> (its icon: the hat alone).
JOB_HATS = {
${hatRows.join('\n')}
}

# The mouse cursor over menus (the game's file -> its figure), drawn at the kit's cursor line (2.8) with the rim.
CURSORS = {
    "ui_cursor_paw_open": icon_paw,
    "ui_cursor_paw_closed": cursor_paw_held,
}
`;
const json = JSON.stringify(COLOURS, null, 1) + '\n';
const outPy = path.join(HERE, 'sticker_cast.py'), outJson = path.join(HERE, 'sticker_colours.json');
if (CHECK) {
  const same = f => fs.existsSync(f[0]) && fs.readFileSync(f[0], 'utf8') === f[1];
  const bad = [[outPy, py], [outJson, json]].filter(f => !same(f)).map(f => path.basename(f[0]));
  if (bad.length) { console.log('sticker_record: differs from the concept: ' + bad.join(', ')); process.exit(1); }
  console.log('sticker_record: OK (' + CAST.length + ' characters)');
} else {
  fs.writeFileSync(outPy, py);
  fs.writeFileSync(outJson, json);
  console.log('sticker_record: ' + CAST.length + ' characters -> sticker_cast.py; ' + Object.keys(COLOURS).length + ' colours -> sticker_colours.json');
}
