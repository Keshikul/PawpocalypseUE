/* The achievement badges (DRAFTS, 2026-11-19; the designer: "3 yes", then "Do drafts"), in the UI concept's kit grammar,
   so an approved one is recorded into the game unchanged (tools/art/sticker_record.js). Runs after the concept's kit,
   tools/art/concept_cards.js, tools/art/concept_icons.js and tools/art/concept_skills.js (SK, SKILL).

   Today's rule kept (tools/art/sweet_ui.py ACHIEVEMENTS): one badge shape for all 39, read down a list, so a family is
   seen before its name is read; the rim is the tier, by rank within the family in thirds (bronze, silver, gold; a family
   of one is gold). The shape is the kit's: a scalloped sticker rosette (scallop), a paper centre, the family's symbol.
     an upgrade's award (up_*)       that upgrade's own icon in the dock (the concept's shop, or the approved one)
     a skill branch's award (br_*)   that skill's picture on a hexagon, the skill tree's node
     the rest                        kills: a defeated enemy; Catnip: a pile of leaves; Reawaken: its icon;
                                     the cat's stage: the cat and the arrow up; a boss: the skull with its crown
   Colours only from the kit (L.c, U) or mix() of them. */
const BADGE_TIER=[mix(L.c.gooseBeak,L.c.grip,.45),L.c.metal,L.c.gold];
const BADGE_TIER_NAME=['bronze','silver','gold'];
const BADGE_SYMBOL={
  kills:()=>C(-9,-7,6.5,L.c.mouseFur)+C(9,-7,6.5,L.c.mouseFur)+E(0,5,12,10,L.c.mouseFur)
    +T('M-7 0 L-2 5',2,'INK')+T('M-2 0 L-7 5',2,'INK')+T('M2 0 L7 5',2,'INK')+T('M7 0 L2 5',2,'INK')+C(0,10,2.2,L.c.pink,{noLine:true}),
  catnip:()=>SK.leaf(-8,6,.62,-35)+SK.leaf(8,6,.62,50)+SK.leaf(0,-4,.75,0),
  reawaken:()=>IC.reawaken(),
  stage:()=>P('M-14 -1 L-13 -17 L-4 -9 Z',L.c.catFur)+P('M12 -1 L11 -17 L2 -9 Z',L.c.catFur)+E(-1,4,14,11,L.c.catFur)
    +T('M-4 -6 L-4 -2',2,L.c.catStripe,{noLine:true})+T('M2 -6 L2 -2',2,L.c.catStripe,{noLine:true})
    +C(-6,3,3.2,L.c.eye,{noLine:true})+C(4,3,3.2,L.c.eye,{noLine:true})+E(-6,3,1,2.6,'INK')+E(4,3,1,2.6,'INK')
    +T('M-10 -1 L-3 1',1.8,'INK')+T('M8 -1 L1 1',1.8,'INK')+SK.up(14,-11,.62),
  boss:()=>SK.skull(0,5,.95)+KC.crown(0,-12,.85),
  br_instinct:()=>SKILL.instinct(),
  br_unicorn:()=>SKILL.unlock_unicorn(),
  br_vitality:()=>SKILL.vitality(),
  up_paw_damage:()=>IC.paw(),
  up_aura_radius:()=>APPROVED_ICONS.aura_radius(),
  up_currency_find:()=>IC.leaf(),
  up_kitten_count:()=>IC.kitten(),
  up_kitten_power:()=>APPROVED_ICONS.kitten_power(),
  up_kitten_speed:()=>APPROVED_ICONS.kitten_speed(),
  up_overkill:()=>APPROVED_ICONS.overkill(),
  up_pounce_power:()=>APPROVED_ICONS.pounce_power(),
  up_shield_pierce:()=>APPROVED_ICONS.shield_pierce(),
  up_spawn_rate:()=>IC.mouse(),
};
const BADGE_WHY={kills:'a defeated enemy',catnip:'a pile of Catnip',reawaken:'the Reawaken icon',stage:'the cat, one stage up',
  boss:'the skull with its crown: a boss',br_instinct:'the Instinct node',br_unicorn:'the Unicorn node',br_vitality:'the Vitality node',
  up_paw_damage:'the Paw Damage upgrade',up_aura_radius:'the Aura Radius upgrade',up_currency_find:'the Currency Find upgrade',
  up_kitten_count:'the Kitten Count upgrade',up_kitten_power:'the Kitten Power upgrade',up_kitten_speed:'the Kitten Speed upgrade',
  up_overkill:'the Overkill upgrade',up_pounce_power:'the Pounce Power upgrade',up_shield_pierce:'the Shield Pierce upgrade',
  up_spawn_rate:'the Spawn Rate upgrade'};
// The plate the symbol sits on: a skill branch's award on the skill tree's hexagon, the rest on a round paper centre.
const badgePlate=family=>family.startsWith('br_')
  ? P('M0 -16 L13.86 -8 L13.86 8 L0 16 L-13.86 8 L-13.86 -8 Z',U.paper2)
  : C(0,0,15,U.paper);
const badge=(family,tier)=>()=>P(scallop(0,0,18,18,3.4),BADGE_TIER[tier])+badgePlate(family)+G(0,0,.68,0,BADGE_SYMBOL[family]);
