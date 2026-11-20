/* Upgrade icons approved on the Missing Art page (D18, S6), in the UI concept's grammar; the game's upgrade id -> its drawing.
   Version 3 (2026-11-18, https://claude.ai/artifact/J265sfqjxSbdsSNsXU3VSh): the designer, "Change shread and shield
   pirce. The lines are too straight and dint convey tearing things apart", then "Shred b, shield a": Shred is a sheet
   ripped in two (it replaces the concept's three straight claw lines), Shield Pierce the shield split along a jagged
   crack with a chunk breaking off. tools/art/sticker_record.js runs this beside tools/art/concept_ui_screens.html and
   records each one; never edit the recorded copy, change this and record again.
   Then "The other missing art is ok" (2026-11-18): the page's six other upgrade icons and Bait, copied below as drawn. */
// The zigzag a tear runs along, top to bottom (shared by both sides of a rip so they fit back together).
const RIP=[[0,-16],[-3,-10],[1,-5],[-4,0],[0,5],[-3,10],[0,16]];
const ripDown=(dx)=>RIP.map(([x,y])=>`L${x+dx} ${y}`).join(' ');
const ripUp=(dx)=>RIP.slice().reverse().map(([x,y])=>`L${x+dx} ${y}`).join(' ');
const APPROVED_ICONS={
  // Shred: a sheet ripped in two along a jagged tear, the halves pulled apart, a scrap between them.
  shred:()=>G(-3,0,1,-8,()=>P(`M-15 -15 L0 -16 ${ripDown(0)} L-15 15 Z`,U.redSoft))
    +G(4,1,1,9,()=>P(`M0 -16 L15 -15 L15 15 L0 16 ${ripUp(0)} Z`,U.redSoft))
    +P('M1 -2 L4 -4 L4 1 Z',U.redSoft,{lw:1.6}),
  // Shield Pierce: the shield split along a jagged crack, the halves forced apart, a chunk breaking off.
  shield_pierce:()=>G(-2,0,1,-6,()=>P(`M0 -17 C -6 -13, -12 -13, -16 -13 C -16 3, -9 12, 0 17 ${ripUp(0)} Z`,L.c.metal))
    +G(3,1,1,7,()=>P(`M0 -17 ${ripDown(0)} C 9 12, 16 3, 16 -13 C 12 -13, 6 -13, 0 -17 Z`,L.c.metal))
    +P('M17 -19 L22 -16 L18 -13 Z',L.c.metalDark,{lw:1.8})+P('M2 -4 L5 -6 L5 -1 Z',L.c.metalDark,{lw:1.6}),
  // aura radius (Missing Art: aura)
  aura_radius:()=>RING(0,0,12,12,4.5,L.c.halo)+P('M17 -5 L24 0 L17 5 Z','INK')+P('M-17 -5 L-24 0 L-17 5 Z','INK'),
  // kitten power (Missing Art: kpower)
  kitten_power:()=>G(-3,3,.86,0,()=>IC.kitten())+P(starN(8,9,4.6,11,-11),L.c.rainbow[0]),
  // kitten speed (Missing Art: kspeed)
  kitten_speed:()=>G(-3,3,.86,0,()=>IC.kitten())+P('M14 -21 L6 -9 L11 -9 L7 1 L18 -13 L12 -13 Z',L.c.gold),
  // overkill (Missing Art: overkill)
  overkill:()=>P(starN(8,14,7,6,-6),L.c.rainbow[1])+P(starN(8,15,7.5,-4,3),L.c.rainbow[0]),
  // pounce power (Missing Art: ppower)
  pounce_power:()=>G(0,3,.9,0,()=>IC.paw())+T('M-17 -13 L-22 -17',2.6,'INK')+T('M17 -13 L22 -17',2.6,'INK')+T('M0 -18 L0 -24',2.6,'INK'),
  // pounce cooldown (Missing Art: precover)
  pounce_cooldown:()=>C(0,0,16,U.paper)+P('M0 0 L0 -12.5 A12.5 12.5 0 1 1 -11.9 -3.9 Z',U.yellow,{noLine:true})+T('M0 0 L0 -11',2.4,'INK')+T('M0 0 L7 4',2.4,'INK')+C(0,0,2.4,'INK')
};
/* Icons by the game's file, not an upgrade (Missing Art's Bait, over the grammar's ui_icon_bait). */
const FILE_ICONS={
  ui_icon_bait:()=>T('M9 -19 L9 -6 C 9 0, 2 1, 1 -4',2.2,'INK')+P('M-15 7 C -9 -2, 4 -2, 9 5 C 4 12, -9 12, -15 7 Z','#FF9B3D')+P('M9 5 L17 -1 L17 11 Z','#FF9B3D')+C(-8,5,1.7,'INK')
};
