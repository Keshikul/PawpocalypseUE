/* The skill pictures (DRAFTS, 2026-11-19; the designer: "3 yes" to drafting the card, skill and achievement pictures,
   then "Do drafts"): one per skill node (77; the ten job nodes show their approved hats), in the UI concept's kit
   grammar, so an approved one is recorded into the game unchanged (tools/art/sticker_record.js, as concept_icons.js is).
   Runs after the concept's kit, tools/art/concept_cards.js (KC) and tools/art/concept_icons.js (APPROVED_ICONS,
   FILE_ICONS). Each draws in the kit's ±20 box; the tree draws it at 0.7 inside its hexagon (the concept's treeNode).

   One mark per idea, the same everywhere:
     the aura      the gold ring of the approved Aura Radius icon (RING in L.c.halo)
     Catnip        the leaf (the HUD's Catnip icon); Stardust the star; Bait the approved fish
     damage        three curved red scratches (SK.claws; small, top right: SK.dmg). Not the concept's three straight white
                   lines: the designer on Shred, 2026-11-18, "The lines are too straight and dint convey tearing things apart"
     crit          the Crit Chance upgrade's burst (IC.burst)
     increase      the green arrow up (SK.up); one more: the green plus (SK.plus)
     automatic     the gear, small, bottom right (SK.gear)
     enemies       the kit's mouse; fliers the moth
     the beam      a purple bar with a white core (SK.beam)
   A skill named as an approved upgrade icon shows that icon (Pounce Power, Aura Reach, Overkill).
   Colours only from the kit (L.c, U) or mix() of them: the recorder names every colour. */
const SK={
  ring:(x,y,r,w,col)=>RING(x,y,r,r,w||4.5,col||L.c.halo),
  scratch:(dx,dy)=>P(`M${dx-5} ${dy-15} C ${dx+6} ${dy-8}, ${dx+10} ${dy+4}, ${dx+3} ${dy+15} C ${dx-2} ${dy+4}, ${dx-6} ${dy-6}, ${dx-5} ${dy-15} Z`,L.c.plusRed),
  claws:(x,y,s)=>G(x,y,s||1,0,()=>SK.scratch(-9,1)+SK.scratch(0,-2)+SK.scratch(9,1)),
  dmg:(x,y,s)=>SK.claws(x,y,s||.6),
  fish:(x,y,s)=>G(x,y,s,0,()=>P('M-14 0 C -8 -9, 5 -9, 10 -2 C 5 6, -8 7, -14 0 Z',L.c.gooseBeak)+P('M10 -2 L18 -8 L18 5 Z',L.c.gooseBeak)+C(-7,-2,1.7,'INK')),
  gear:(x,y,s)=>G(x,y,s||.42,0,()=>IC.gear()),
  plus:(x,y,s)=>G(x,y,s||1,0,()=>P('M-3.2 -9 L3.2 -9 L3.2 -3.2 L9 -3.2 L9 3.2 L3.2 3.2 L3.2 9 L-3.2 9 L-3.2 3.2 L-9 3.2 L-9 -3.2 L-3.2 -3.2 Z',U.green)),
  up:(x,y,s)=>G(x,y,s||1,0,()=>P('M-4.5 12 L-4.5 1 L-10 1 L0 -12 L10 1 L4.5 1 L4.5 12 Z',L.c.rainbow[3])),
  head:(x,y,deg,col,h)=>{h=h||6.5;const a=deg*Math.PI/180,ux=Math.cos(a),uy=Math.sin(a),bx=x-ux*h,by=y-uy*h,px=-uy*h*.72,py=ux*h*.72;
    return P(`M${r2(x+ux*1.5)} ${r2(y+uy*1.5)} L${r2(bx+px)} ${r2(by+py)} L${r2(bx-px)} ${r2(by-py)} Z`,col);},
  arrow:(x1,y1,x2,y2,col,w)=>{const a=Math.atan2(y2-y1,x2-x1),h=6.5;
    return T(`M${r2(x1)} ${r2(y1)} L${r2(x2-Math.cos(a)*h)} ${r2(y2-Math.sin(a)*h)}`,w||3,col)+SK.head(x2,y2,a*180/Math.PI,col);},
  carrow:(d,x2,y2,deg,col,w)=>T(d,w||3,col)+SK.head(x2,y2,deg,col),
  beam:d=>T(d,6,L.c.rainbow[5])+T(d,2.2,U.white,{noLine:true}),
  leaf:(x,y,s,rot)=>G(x,y,s,rot||0,()=>IC.leaf()),
  mouse:(x,y,s)=>G(x,y,s,0,()=>IC.mouse()),
  kitten:(x,y,s)=>G(x,y,s,0,()=>IC.kitten()),
  paw:(x,y,s,rot)=>G(x,y,s,rot||0,()=>IC.paw()),
  burst:(x,y,s)=>G(x,y,s,0,()=>IC.burst()),
  bait:(x,y,s)=>G(x,y,s,0,()=>FILE_ICONS.ui_icon_bait()),
  star:(x,y,r,col)=>P(star4(x,y,r),col||L.c.gold,{lw:1.6}),
  skull:(x,y,s)=>G(x,y,s,0,()=>R(-7,3,14,10,3,L.c.catBelly)+C(0,-3,12,L.c.catBelly)+C(-4.6,-3,3.4,'INK')+C(4.6,-3,3.4,'INK')
    +P('M0 2.5 L-2 6 L2 6 Z','INK')+T('M-3 9.5 L-3 13',1.6,'INK')+T('M0 9.5 L0 13',1.6,'INK')+T('M3 9.5 L3 13',1.6,'INK')),
  moth:(x,y,s)=>G(x,y,s,0,()=>E2(-8,-3,9,6,-25,L.c.mothWing)+E2(8,-3,9,6,25,L.c.mothWing)+E2(-6,6,6,4,25,L.c.mothWing2)+E2(6,6,6,4,-25,L.c.mothWing2)
    +E(0,1,3.6,9.5,L.c.mothBody)+T('M-1.5 -8 L-5 -14',1.6,'INK')+T('M1.5 -8 L5 -14',1.6,'INK')),
  // the net of a snare, over whatever is under it
  net:()=>T('M-17 -8 L8 17',1.8,L.c.grip)+T('M-8 -17 L17 8',1.8,L.c.grip)+T('M17 -8 L-8 17',1.8,L.c.grip)+T('M8 -17 L-17 8',1.8,L.c.grip),
  ripple:(x,y)=>T(`M${x} ${y} l3 0`,2.4,'INK'),
  poop:(x,y,s)=>G(x,y,s,0,()=>E(0,12,16,5.5,L.c.rainbow[0])+E(0,6,12.5,5,L.c.rainbow[1])+E(0,0.5,9,4.5,L.c.rainbow[2])+E(0,-4.5,6,3.6,L.c.rainbow[3])
    +P('M-2 -7 C -1 -12, 2 -15, 5 -16 C 5 -12, 3.5 -9, 2.5 -7.5 Z',L.c.rainbow[4])),
  shard:(x,y,s,col)=>P(`M${x} ${y-7*s} L${r2(x+4.2*s)} ${y} L${x} ${y+7*s} L${r2(x-4.2*s)} ${y} Z`,col),
  card:(x,y,rot,fill)=>KC.card(x,y,rot,fill),
};
const SKILL={
  // ---- Claws (damage, the paw, the Pounce, overkill) ----
  claws:()=>SK.claws(0,0,1.15),
  butchers_rhythm:()=>G(-1,-6,.82,0,()=>IC.cleaver())+C(-15,13,2,'INK')+C(-9,13,2,'INK')+C(-3,13,2,'INK')+SK.burst(10,11,.42),
  anvil_paw:()=>P('M-19 1 L15 1 L15 7 L6 7 L9 17 L-9 17 L-6 7 L-9 7 C -13 7, -17 5, -19 1 Z',L.c.metalDark)
    +SK.paw(-7,-9,.46)+SK.paw(7,-11,.46),
  dent:()=>G(-4,3,.82,0,()=>APPROVED_ICONS.shred())+SK.up(13,-9,.62),
  hammerfall:()=>SK.mouse(-12,12,.4)+SK.mouse(0,12,.4)+SK.mouse(12,12,.4)
    +SK.paw(0,-8,.6)+T('M-11 -1 L-15 2',2.2,'INK')+T('M11 -1 L15 2',2.2,'INK'),
  crash_landing:()=>E(0,14,17,5.5,L.c.grip)+E(0,13.5,10,3,mix(L.c.grip,L.ink,.4),{noLine:true})+KC.flame(-13,11,.45)+KC.flame(13,11,.45)
    +SK.paw(2,-3,.62,20)+T('M-17 -17 L-11 -11',2.4,'INK')+T('M-10 -20 L-5 -15',2.4,'INK'),
  momentum:()=>KC.clock(-7,2,12)+P('M5 -7 L13 2 L5 11 Z',L.c.rainbow[3])+P('M12 -7 L20 2 L12 11 Z',L.c.rainbow[3]),
  nine_lives_insurance:()=>R(-13,-17,26,34,3,U.paper)+T('M-8 -10 L8 -10',2,'INK')+T('M-8 -5 L8 -5',2,'INK')+T('M-8 0 L3 0',2,'INK')
    +SK.paw(5,10,.42),
  swarm_logic:()=>SK.mouse(-12,4,.4)+SK.mouse(12,4,.4)+SK.mouse(0,0,.4)+SK.mouse(-6,13,.4)+SK.mouse(6,13,.4)+SK.claws(0,-11,.55),
  last_stand:()=>SK.claws(-5,-3,.9)+SK.mouse(12,12,.44),
  unlock_overkill:()=>APPROVED_ICONS.overkill(),
  overkill:()=>P(starN(8,12,6,-9,3),L.c.rainbow[1])+SK.mouse(-9,6,.5)
    +SK.carrow('M-3 -9 C 3 -16, 11 -12, 12 -4',12,-2,90,L.c.rainbow[0])+SK.mouse(12,10,.44),
  overflow:()=>G(-9,-9,.55,0,()=>APPROVED_ICONS.overkill())+KC.bomb(4,6,11)+SK.leaf(3,7,.38),
  unlock_pounce:()=>SK.paw(4,2,.85,-15)+T('M-19 -7 L-11 -4',2.6,'INK')+T('M-19 1 L-10 2',2.6,'INK')+T('M-18 9 L-10 9',2.6,'INK'),
  pounce:()=>APPROVED_ICONS.pounce_power(),
  unlock_reflex:()=>SK.paw(-3,0,.82)+SK.gear(12,12),
  // ---- Fortune (Catnip, Bait, Reawaken, the automatic skills) ----
  fortune:()=>SK.leaf(-2,3,1.05)+SK.star(13,-13,6),
  compound_interest:()=>SK.leaf(-5,5,.9)+KC.clock(10,-10,8),
  compound:()=>SK.leaf(-5,5,.9)+SK.up(12,-6,.72),
  deep_pockets:()=>SK.fish(1,-8,.85)+P('M-15 -4 L15 -4 L15 9 L0 18 L-15 9 Z',L.c.rainbow[4])
    +T('M-11 0 L11 0 L11 7 L0 13 L-11 7 Z',1.4,mix(L.c.rainbow[4],'#FFFFFF',.55),{noLine:true}),
  audit:()=>SK.leaf(-11,-9,.55)+SK.arrow(-6,-2,1,5,'INK',2.6)+SK.bait(7,8,.62),
  levy:()=>C(-16,14,2,'INK')+C(-10,14,2,'INK')+C(-4,14,2,'INK')+SK.leaf(6,-2,.95),
  long_lunch:()=>T('M9 1 C 17 1, 17 11, 7 11',3,U.paper)+P('M-14 -3 L11 -3 L9 12 C 8 16, 5 17, 1 17 L-5 17 C -9 17, -12 16, -13 12 Z',U.paper)
    +E(-1.5,-3,12.5,3,mix(L.c.leafy,'#FFFFFF',.35))+SK.leaf(-2,-11,.45),
  severance:()=>G(-3,3,.88,0,()=>IC.reawaken())+SK.dmg(12,-12),
  union_rules:()=>G(-6,5,.8,0,()=>IC.hat())+G(10,-7,.55,0,()=>IC.hat()),
  legacy:()=>T('M11 -11 C 4 -18, -12 -15, -14 -2 C -15 10, -4 17, 8 13',4.5,U.stardust)+P('M6 -21 L17 -11 L4 -6 Z',U.stardust)+SK.leaf(1,0,.5),
  auto_card:()=>G(-3,-1,1,0,()=>IC.cards())+SK.gear(12,12),
  auto_travel:()=>G(-2,-1,1,0,()=>IC.flag())+SK.gear(11,12),
  auto_bait:()=>SK.bait(-3,0,.9)+SK.gear(12,12),
  cartography:()=>P('M-17 -12 L-6 -16 L6 -12 L17 -16 L17 12 L6 16 L-6 12 L-17 16 Z',U.paper)
    +T('M-6 -16 L-6 12',1.6,'INK')+T('M6 -12 L6 16',1.6,'INK')+C(-12,9,1.6,'INK')+C(-8,5,1.6,'INK')+C(-3,3,1.6,'INK')+SK.leaf(8,-3,.42),
  second_helping:()=>P(star4(-6,5,13),U.stardust)+C(4,-5,1.6,'INK')+C(7,-8,1.6,'INK')+P(star4(13,-12,7.5),mix(U.stardust,'#FFFFFF',.55)),
  // ---- Instinct (kittens, the run's counters, the Cat Burglar and the Sphinx) ----
  instinct:()=>SK.kitten(0,2,1.15),
  vitality:()=>SK.kitten(-6,6,.75)+SK.kitten(10,10,.55)+SK.plus(11,-10,.8),
  kitten_foreman:()=>SK.kitten(-8,8,.7)+SK.mouse(9,-14,.32)+SK.mouse(16,-7,.32)+SK.mouse(8,-5,.32)+SK.arrow(-2,-2,3,-6,'INK',2.4),
  whisker_trade:()=>SK.paw(-11,-10,.48)+SK.burst(1,-8,.36)+SK.kitten(7,8,.72),
  sixth_sense:()=>P('M0 19 C -6 9, -14 3, -14 -5 C -14 -13, -8 -18, 0 -18 C 8 -18, 14 -13, 14 -5 C 14 3, 6 9, 0 19 Z',L.c.plusRed)+eye(0,-5,7,0,0),
  reputation:()=>P('M-7 1 L-12 18 L-6 15 L-2 19 L0 3 Z',L.c.rainbow[4])+P('M7 1 L12 18 L6 15 L2 19 L0 3 Z',L.c.rainbow[4])
    +C(0,-3,11,L.c.gold)+P(starN(5,6.5,2.8,0,-3),L.c.goldBelly,{lw:1.6})+SK.dmg(12,-13),
  body_count:()=>R(-17,-13,28,30,3,U.paper)+T('M-12 -7 L-12 11',2.6,'INK')+T('M-7 -7 L-7 11',2.6,'INK')+T('M-2 -7 L-2 11',2.6,'INK')
    +T('M3 -7 L3 11',2.6,'INK')+T('M-15 8 L6 -4',2.6,L.c.plusRed,{noLine:true})+SK.dmg(12,-12),
  grudge_ledger:()=>SK.card(-6,5,-12,U.paper)+SK.card(3,3,8,U.paper)+SK.dmg(12,-12),
  cat_burglars_cut:()=>G(-7,-7,.62,0,()=>APPROVED_ICONS.overkill())+SK.leaf(6,6,.72),
  fence:()=>SK.leaf(-14,12,.36)+SK.leaf(-5,6,.52)+SK.leaf(8,-3,.78),
  pickpocket:()=>SK.mouse(6,-5,.62)+T('M-9 -12 L-4 -12',2.4,'INK')+T('M-10 -5 L-5 -5',2.4,'INK')+T('M-9 2 L-4 2',2.4,'INK')+SK.leaf(-9,12,.45),
  inscrutable:()=>R(-15,-10,24,24,5,U.paper)+C(-9,-4,2.2,'INK')+C(-3,2,2.2,'INK')+C(3,8,2.2,'INK')
    +T('M-17 16 L11 -12',3.4,L.c.plusRed)+SK.burst(12,-12,.36),
  patience:()=>P('M-11 -14 L5 -14 L5 -11 C 5 -5, -1 -3, -1 0 C -1 3, 5 5, 5 11 L5 14 L-11 14 L-11 11 C -11 5, -5 3, -5 0 C -5 -3, -11 -5, -11 -11 Z',U.paper)
    +P('M-9 13 C -9 8, -5 6, -3 4 C -1 6, 3 8, 3 13 Z',L.c.gold,{noLine:true})+R(-14,-18,22,5,2.5,L.c.grip)+R(-14,13,22,5,2.5,L.c.grip)+SK.burst(12,-10,.38),
  riddle:()=>SK.burst(-12,-10,.38)+SK.arrow(-6,-5,-1,0,'INK',2.4)+SK.skull(7,6,.85),
  // ---- Aura (the ring; the Magician and the Town Crier) ----
  unlock_aura:()=>SK.ring(0,0,15)+SK.burst(0,0,.55),
  aura_damage:()=>SK.ring(0,0,15)+SK.claws(0,0,.55),
  aura_reach:()=>APPROVED_ICONS.aura_radius(),
  standing_wave:()=>SK.ring(0,0,13)+KC.clock(0,0,7)+T('M-14 -14 L-18 -18',2.4,'INK')+T('M14 -14 L18 -18',2.4,'INK')+T('M-14 14 L-18 18',2.4,'INK')+T('M14 14 L18 18',2.4,'INK'),
  grand_finale:()=>SK.kitten(-6,8,.72)+G(10,-9,.85,-35,()=>T('M-6 -9 L-6 0 C -6 9, 6 9, 6 0 L6 -9',5.5,L.c.plusRed)+R(-9.5,-14,7,5,1.5,L.c.metal)+R(2.5,-14,7,5,1.5,L.c.metal)),
  misdirection:()=>SK.mouse(-11,9,.5)+SK.carrow('M-6 -1 C -4 -10, 1 -12, 5 -11',5,-11,0,'INK',2.4)+SK.paw(12,-9,.5),
  sleight_of_hand:()=>P(scallop(0,3,11,8,5),U.white)+SK.star(-14,-12,5)+SK.star(14,-14,4)+SK.star(14,13,3.5),
  carry:()=>SK.ring(0,0,13)+SK.paw(0,0,.46)+P('M17 -5 L24 0 L17 5 Z','INK')+P('M-17 -5 L-24 0 L-17 5 Z','INK'),
  crowd_work:()=>SK.ring(-2,2,15)+SK.mouse(-8,-1,.3)+SK.mouse(4,-3,.3)+SK.mouse(-2,8,.3)+SK.dmg(13,-13),
  echo:()=>SK.ring(-6,4,11,4,mix(L.c.halo,'#FFFFFF',.6))+SK.ring(6,-3,11),
  // ---- Snare (slow, the run's cards; the Engineer and the Ratcatcher) ----
  unlock_snare:()=>SK.net()+SK.mouse(0,3,.85),
  deadweight:()=>T('M-15 -15 L-2 -2',2.6,L.c.metalDark)+RING(-14,-14,3.4,3.4,1.8,L.c.metal)+RING(-9,-9,3.4,3.4,1.8,L.c.metal)+RING(-4,-4,3.4,3.4,1.8,L.c.metal)
    +C(6,7,11,'#3B3546')+C(2.5,3.5,2.8,'#6A6478',{noLine:true}),
  second_opinion:()=>KC.reroll(-3,3,12)+SK.up(13,-10,.6),
  curiosity:()=>SK.card(-8,5,-16,U.paper)+SK.card(0,3,0,U.paper)+SK.card(8,5,16,U.paper)+SK.plus(12,-12,.72),
  funnel:()=>SK.mouse(-2,9,.6)+SK.carrow('M-12 -2 L-12 -9 C -12 -18, 8 -18, 8 -9 L8 -8',8,-3,90,L.c.rainbow[4],3.2),
  swarm:()=>SK.mouse(-10,9,.48)+SK.mouse(4,11,.44)+SK.mouse(-4,-3,.44)+SK.up(13,-8,.7),
  gridlock:()=>SK.net()+SK.moth(0,0,1.1),
  blueprint:()=>R(-17,-15,34,30,3,L.c.rainbow[4])+T('M-17 -5 L17 -5',1,mix(L.c.rainbow[4],'#FFFFFF',.35),{noLine:true})+T('M-17 5 L17 5',1,mix(L.c.rainbow[4],'#FFFFFF',.35),{noLine:true})
    +T('M-7 -15 L-7 15',1,mix(L.c.rainbow[4],'#FFFFFF',.35),{noLine:true})+T('M7 -15 L7 15',1,mix(L.c.rainbow[4],'#FFFFFF',.35),{noLine:true})
    +T('M-6 -10 L6 -10 L6 10 L-6 10 Z',2.2,U.white,{noLine:true})+T('M-3 -5 L3 -5',2.2,U.white,{noLine:true}),
  overtime:()=>KC.clock(-4,4,12)+KC.reroll(12,-11,6),
  prototype:()=>SK.card(-5,4,-8,U.paper)+SK.up(11,-6,.75),
  infestation:()=>E(0,13,17,6,mix(L.c.grip,L.ink,.35))+E(0,13,10,3,'INK')+SK.mouse(-6,1,.5)+SK.mouse(8,-6,.5),
  pied_piper:()=>SK.paw(0,0,.45)+[[0,-15],[14,-5],[9,12],[-9,12],[-14,-5]].map(([x,y])=>SK.mouse(x,y,.3)).join(''),
  traps:()=>P('M-18 -3 C -18 -17, 18 -17, 18 -3 L13 -3 L10.5 -9 L8 -3 L5.5 -9 L3 -3 L0.5 -9 L-2 -3 L-4.5 -9 L-7 -3 L-9.5 -9 L-12 -3 Z',L.c.metal)
    +P('M-18 3 C -18 17, 18 17, 18 3 L13 3 L10.5 9 L8 3 L5.5 9 L3 3 L0.5 9 L-2 3 L-4.5 9 L-7 3 L-9.5 9 L-12 3 Z',L.c.metal)
    +C(-19,0,3.2,L.c.metalDark)+C(19,0,3.2,L.c.metalDark),
  play_with_food:()=>R(-16,-17,26,8,4,U.track)+R(-16,-17,7,8,4,U.redSoft,{noLine:true})+SK.mouse(-5,6,.68)+SK.leaf(13,7,.5),
  // ---- Unicorn (the horn, Rainbow Poop, the beam) ----
  unlock_unicorn:()=>G(0,0,1.05,0,()=>IC.horn()),
  unlock_poop:()=>SK.poop(0,1,1)+SK.star(-15,-10,4,U.white)+SK.star(14,-14,3.5,U.white),
  unicorn:()=>SK.poop(-3,3,.95)+SK.dmg(12,-12),
  unlock_prism:()=>G(-12,11,.62,45,()=>IC.horn())+SK.beam('M-5 4 L7 -8')+SK.mouse(12,-11,.46),
  refraction:()=>SK.beam('M-19 -12 L-12 8 L0 -8 L12 8')+SK.mouse(-12,8,.4)+SK.mouse(0,-8,.4)+SK.mouse(12,8,.4),
  spectrum:()=>SK.beam('M-18 15 L1 -1')+SK.leaf(8,-6,.72),
  unlock_full_spectrum:()=>SK.beam('M-15 15 L12 -10')+SK.carrow('M17 -2 C 16 6, 12 11, 6 14',4,15,160,'INK',2.4)+C(-15,15,4,L.c.horn),
  critical_mass:()=>P(starN(8,16,8),U.white)+SK.shard(-7,2,1.1,L.c.rainbow[4])+SK.shard(2,-3,1.25,L.c.rainbow[5])+SK.shard(9,5,1,L.c.rainbow[2]),
};
const SKILL_WHY={
  claws:'the claw marks (the concept\'s Claws node)',butchers_rhythm:'the cleaver, the count and the crit',anvil_paw:'two paws on the anvil: hits twice',
  dent:'a dented plate',hammerfall:'one hit, three enemies in a row',crash_landing:'the paw crashing into a burning crater',
  momentum:'the clock, fast-forward: the lock runs out sooner',nine_lives_insurance:'the contract and the paw that signed it',
  swarm_logic:'claws over a crowd',last_stand:'claws, one enemy left',unlock_overkill:'the approved Overkill icon',
  overkill:'the overkill jumps to the nearest enemy',overflow:'overkill drops a Catnip bomb',unlock_pounce:'the paw, leaping',
  pounce:'the approved Pounce Power icon',unlock_reflex:'the paw and the gear: automatic',
  fortune:'Catnip, and the golden sparkle',compound_interest:'Catnip and the clock: every minute',compound:'Catnip, increasing',
  deep_pockets:'the Bait kept in your pocket',audit:'Catnip buys the Bait',levy:'every 100th kill: a lot of Catnip',
  long_lunch:'a cup of Catnip tea: standing still pays',severance:'Reawaken, and damage',union_rules:'two hats: a second job',
  legacy:'Catnip carried through the Reawaken',auto_card:'the cards and the gear: automatic',auto_travel:'the biome flag and the gear: automatic',
  auto_bait:'the Bait and the gear: automatic',cartography:'the map shows the Catnip',second_helping:'the Stardust in two halves',
  instinct:'the kitten (the concept\'s Instinct node)',vitality:'one more kitten',kitten_foreman:'the kitten goes for the crowd',
  whisker_trade:'your crit passes to the kittens',sixth_sense:'the pin that sees where it will spawn',reputation:'the medal, and damage',
  body_count:'the kill count, and damage',grudge_ledger:'cards taken, and damage',cat_burglars_cut:'overkill turned into Catnip',
  fence:'Catnip increasing with each kill',pickpocket:'the escaping enemy drops its Catnip',inscrutable:'no dice: crits on a count',
  patience:'the hourglass and the crit',riddle:'after a crit, the kill',
  unlock_aura:'the aura (the concept\'s Aura node: the burst, in the ring)',aura_damage:'the aura, and damage',aura_reach:'the approved Aura Radius icon',
  standing_wave:'the aura pulsing',grand_finale:'the kitten with a magnet: it pulls',misdirection:'the enemy drifts toward you',
  sleight_of_hand:'a puff and sparkles: gone',carry:'standing in the aura, it widens',crowd_work:'enemies in the aura, and damage',
  echo:'a second aura behind the first',
  unlock_snare:'the enemy caught in a net',deadweight:'the ball and chain',second_opinion:'the reroll, and a level up',
  curiosity:'one more card',funnel:'the enemy turns back',swarm:'more enemies',gridlock:'the flier caught in the net',
  blueprint:'the next card on a blueprint',overtime:'the clock and a reroll: start with more',prototype:'a card, one level higher',
  infestation:'enemies out of the burrow',pied_piper:'enemies in a ring around you',traps:'a trap',play_with_food:'low health, more Catnip',
  unlock_unicorn:'the horn (the concept\'s Unicorn node)',unlock_poop:'Rainbow Poop',unicorn:'Rainbow Poop, and damage',
  unlock_prism:'the horn fires the beam',refraction:'the beam jumps from enemy to enemy',spectrum:'the beam, and Catnip',
  unlock_full_spectrum:'the beam sweeps',critical_mass:'the shards burst'};
