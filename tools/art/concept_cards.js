/* The card pictures (DRAFTS, 2026-11-19; the designer: "3 yes", draft the card, skill and achievement pictures): one per
   card, in the UI concept's kit grammar (its P, C, E, R, T, G and IC shapes, the Bestiary's colours), so an approved one
   is recorded into the game unchanged (tools/art/sticker_record.js). Each draws in the kit's ±20 box. */
const KC={
  coin:(x,y,r)=>C(x,y,r,L.c.gold)+C(x,y,r*0.6,L.c.goldBelly,{noLine:true}),
  flame:(x,y,s)=>P(`M${x} ${y-14*s} C ${x+10*s} ${y-4*s}, ${x+9*s} ${y+8*s}, ${x} ${y+8*s} C ${x-9*s} ${y+8*s}, ${x-10*s} ${y-4*s}, ${x} ${y-14*s} Z`,L.c.rainbow[1])
    +P(`M${x} ${y-5*s} C ${x+4*s} ${y}, ${x+4*s} ${y+6*s}, ${x} ${y+6*s} C ${x-4*s} ${y+6*s}, ${x-4*s} ${y}, ${x} ${y-5*s} Z`,L.c.rainbow[2],{noLine:true}),
  card:(x,y,rot,fill)=>G(x,y,1,rot,()=>R(-8,-11,16,22,3.5,fill||U.paper)+R(-5,-8,10,9,2,mix(fill||U.paper,'#FFFFFF',.5),{noLine:true})),
  crater:(y)=>E(0,y,17,6.5,L.c.grip)+E(0,y-1,10,3.5,'#5E3F2A',{noLine:true}),
  bomb:(x,y,r)=>C(x,y,r,'#3B3546')+C(x-r*0.35,y-r*0.35,r*0.25,'#6A6478',{noLine:true})+T(`M${x+r*0.6} ${y-r*0.75} C ${x+r} ${y-r*1.4}, ${x+r*1.4} ${y-r*1.2}, ${x+r*1.5} ${y-r*1.55}`,2.4,L.c.grip)
    +P(star4(x+r*1.55,y-r*1.6,4.5),L.c.horn,{lw:1.6}),
  clock:(x,y,r)=>C(x,y,r,U.paper)+T(`M${x} ${y} L${x} ${y-r*0.62}`,2.4,'INK')+T(`M${x} ${y} L${x+r*0.45} ${y+r*0.25}`,2.4,'INK')+C(x,y,1.8,'INK'),
  reroll:(x,y,r)=>T(`M${x+r} ${y} A${r} ${r} 0 1 1 ${x+r*0.3} ${y-r*0.95}`,3,L.c.rainbow[3])+P(`M${x+r*0.1} ${y-r*1.35} L${x+r*0.75} ${y-r*0.9} L${x+r*0.05} ${y-r*0.55} Z`,L.c.rainbow[3]),
  shell:(x,y,r)=>C(x,y,r,L.c.kitten)+T(`M${x} ${y} C ${x+r*0.45} ${y-r*0.1}, ${x+r*0.35} ${y+r*0.5}, ${x-r*0.05} ${y+r*0.45} C ${x-r*0.6} ${y+r*0.4}, ${x-r*0.6} ${y-r*0.45}, ${x} ${y-r*0.55} C ${x+r*0.7} ${y-r*0.6}, ${x+r*0.8} ${y+r*0.25}, ${x+r*0.55} ${y+r*0.6}`,2,L.c.kittenStripe,{noLine:true}),
  snail:(x,y,s)=>P(`M${x-16*s} ${y+7*s} C ${x-18*s} ${y}, ${x-12*s} ${y-2*s}, ${x-10*s} ${y+2*s} L${x+14*s} ${y+2*s} C ${x+18*s} ${y+2*s}, ${x+18*s} ${y+8*s}, ${x+14*s} ${y+8*s} Z`,L.c.mouseBelly)
    +T(`M${x-13*s} ${y} L${x-15*s} ${y-6*s}`,1.8,'INK')+T(`M${x-11*s} ${y} L${x-10*s} ${y-6*s}`,1.8,'INK')+KC.shell(x+3*s,y-3*s,9*s),
  crown:(x,y,s)=>P(`M${x-9*s} ${y+4*s} L${x-10*s} ${y-6*s} L${x-4*s} ${y-1*s} L${x} ${y-8*s} L${x+4*s} ${y-1*s} L${x+10*s} ${y-6*s} L${x+9*s} ${y+4*s} Z`,L.c.crown)+C(x,y+1*s,1.6*s,L.c.plume,{noLine:true}),
  dots:(pts)=>pts.map(([x,y])=>C(x,y,1.6,'INK')).join(''),
};
const CARD={
  // POUNCE
  bloodlust:()=>KC.clock(-2,3,15)+P('M13 -18 C 17 -12, 19 -9, 16 -6 C 14 -4, 10 -5, 10 -9 C 10 -12, 12 -15, 13 -18 Z',L.c.plusRed),
  chain_pounce:()=>G(-12,11,.42,0,()=>IC.paw())+G(0,0,.48,0,()=>IC.paw())+G(13,-12,.55,0,()=>IC.paw())+KC.dots([[-7,4],[-4,1],[6,-5],[8,-8]]),
  feral_focus:()=>KC.crater(10)+KC.flame(0,0,1.05)+P(star4(-14,-12,4),L.c.rainbow[0],{lw:1.6}),
  scorched_landing:()=>KC.crater(11)+G(0,-4,.6,0,()=>IC.paw())+KC.flame(-13,4,.5)+KC.flame(13,4,.5),
  // FORTUNE
  bounty:()=>KC.coin(0,2,17)+G(0,3,.5,0,()=>IC.mouse())+P(star4(14,-14,5),U.white,{lw:1.6}),
  compound_interest:()=>E(-4,12,12,4.5,L.c.gold)+E(-4,6,12,4.5,L.c.gold)+E(-4,0,12,4.5,L.c.gold)+E(-4,0,7,2.4,L.c.goldBelly,{noLine:true})
    +T('M13 8 L13 -14',3,L.c.rainbow[3])+P('M7 -10 L13 -18 L19 -10 Z',L.c.rainbow[3]),
  hoarder:()=>R(-16,-2,32,18,3,L.c.grip)+P('M-16 -2 C -16 -14, 16 -14, 16 -2 Z',mix(L.c.grip,'#FFFFFF',.15))+R(-3,-4,6,8,1.5,L.c.gold)+KC.coin(-9,-13,4.5)+KC.coin(2,-15,4.5)+KC.coin(11,-12,4),
  scavenger:()=>P('M-13 -6 C -16 4, -14 16, 0 16 C 14 16, 16 4, 13 -6 C 8 -9, -8 -9, -13 -6 Z',L.c.clump)+T('M-8 -9 C -4 -14, 4 -14, 8 -9',2.4,L.c.grip)+G(0,4,.42,0,()=>IC.leaf()),
  // SWARM
  catnip_bomb:()=>KC.bomb(-2,4,13)+G(-3,5,.4,0,()=>IC.leaf()),
  gold_rush:()=>E(-1,4,13,9,L.c.gold)+C(-9,-4,4.5,L.c.gold)+C(-2,-6,4.5,L.c.gold)+C(-12,4,1.6,'INK')+T('M11 6 C 18 6, 20 -2, 16 -6',2.4,L.c.ratTail)
    +P(star4(12,-12,5.5),U.white,{lw:1.6})+P(star4(-14,-14,3.5),U.white,{lw:1.4}),
  notoriety:()=>G(-4,4,.85,0,()=>IC.mouse())+P(starN(8,9,5,12,-10),L.c.plusRed)+T('M12 -14 L12 -10',2.4,U.white,{noLine:true})+C(12,-6.5,1.3,U.white,{noLine:true}),
  stampede:()=>KC.bomb(4,4,11)+T('M-19 -4 L-9 -4',2.6,'INK')+T('M-21 3 L-11 3',2.6,'INK')+T('M-18 10 L-8 10',2.6,'INK'),
  // AURA
  close_quarters:()=>RING(0,0,13,13,3.5,L.c.rainbow[4])+P(starN(8,10,5),L.c.rainbow[0])+T('M-19 0 L-15 0',2.4,'INK')+T('M19 0 L15 0',2.4,'INK')+T('M0 -19 L0 -15',2.4,'INK')+T('M0 19 L0 15',2.4,'INK'),
  ember_trail:()=>RING(10,8,9,9,3,L.c.rainbow[4])+KC.flame(-4,2,.55)+KC.flame(-12,-6,.42)+KC.flame(-18,-14,.3),
  rigor:()=>RING(0,4,19,9,2.6,L.c.rainbow[4])+KC.snail(0,0,1.05),
  thunderclap:()=>RING(0,0,17,17,3.5,L.c.rainbow[4])+G(0,0,.8,0,()=>IC.bolt()),
  // DECK
  dilated:()=>KC.card(-9,4,-14,L.c.rainbow[4])+KC.card(9,4,14,L.c.rainbow[3])+E(0,-4,14,8.5,U.white)+C(0,-4,5.5,L.c.rainbow[4])+C(0,-4,2.6,'INK',{noLine:true}),
  fixation:()=>KC.card(-6,2,-10,U.paper)+KC.card(6,0,8,U.paper)+KC.reroll(6,1,7),
  not_that_one:()=>KC.card(0,0,0,U.paper)+T('M-10 -12 L10 12',4,L.c.plusRed)+T('M10 -12 L-10 12',4,L.c.plusRed),
  spite:()=>KC.card(-8,4,-16,U.paper)+KC.card(0,2,0,U.paper)+KC.card(8,4,16,U.paper)+KC.reroll(0,-4,8),
  // KITTEN
  giblets:()=>P(starN(10,14,7,7,-7),L.c.rainbow[1])+G(-4,5,.75,0,()=>IC.kitten()),
  gilded_kitten:()=>G(0,5,.9,0,()=>IC.kitten())+KC.crown(0,-15,.9)+P(star4(-15,-6,4),U.white,{lw:1.4})+P(star4(15,-2,3.5),U.white,{lw:1.4}),
  lone_hunter:()=>G(-5,4,.8,0,()=>IC.kitten())+KC.crown(-5,-14,.7)+G(14,12,.25,0,()=>IC.paw())+G(18,4,.22,0,()=>IC.paw())+G(20,-4,.2,0,()=>IC.paw()),
  peer_pressure:()=>G(-9,7,.62,0,()=>IC.kitten())+G(9,7,.62,0,()=>IC.kitten())+G(0,-10,.45,0,()=>IC.paw())+T('M-6 -4 L-3 -6',2,'INK')+T('M6 -4 L3 -6',2,'INK'),
  // ZONE
  hot_borders:()=>R(-15,-15,30,30,5,'none')+G(0,2,.5,0,()=>IC.mouse())+KC.flame(-15,-13,.42)+KC.flame(15,-13,.42)+KC.flame(-15,17,.42)+KC.flame(15,17,.42),
  lingering_rainbow:()=>E(0,8,18,7,L.c.rainbow[5])+E(0,7,14,5,L.c.rainbow[4],{noLine:true})+E(0,6,10,3.5,L.c.rainbow[3],{noLine:true})+E(0,5,6,2,L.c.rainbow[2],{noLine:true})
    +KC.clock(9,-10,7),
  rainbow_trail:()=>T('M-18 14 C -8 4, 4 2, 18 -12',7,L.c.rainbow[0])+T('M-18 14 C -8 4, 4 2, 18 -12',3,L.c.rainbow[2],{noLine:true})
    +G(-9,-6,.36,-20,()=>IC.paw())+G(4,-13,.36,-20,()=>IC.paw()),
  slow_burn:()=>KC.flame(-9,-2,.95)+KC.snail(5,8,.62),
};
const CARD_WHY={bloodlust:'the Pounce clock and a drop: kills take time off it',chain_pounce:'paw prints hopping from one enemy to the next',
  feral_focus:'a burning crater',scorched_landing:'the paw landing in a burning crater',bounty:'a coin with a signature enemy on it',
  compound_interest:'coins stacking up, rising',hoarder:'a chest of coins',scavenger:'a sack of Catnip picked up after the wanderers',
  catnip_bomb:'a bomb with a Catnip leaf',gold_rush:'a golden enemy, sparkling',notoriety:'an enemy with a star: signature enemies turn up more',
  stampede:'a bomb on the run',close_quarters:'the aura pulled in tight, bursting',ember_trail:'the aura with a burning trail',
  rigor:'a snail in the aura\'s ring: enemies crawl',thunderclap:'the aura with a bolt',dilated:'an eye over the cards: see more',
  fixation:'your own cards going round again',not_that_one:'a card struck out',spite:'the hand rerolled',giblets:'a kitten and a burst',
  gilded_kitten:'a kitten with a crown',lone_hunter:'the crowned kitten, the others\' prints going off',peer_pressure:'two kittens pouncing with you',
  hot_borders:'the edges on fire',lingering_rainbow:'a rainbow puddle and a clock: it stays longer',rainbow_trail:'a rainbow trail behind your paws',
  slow_burn:'burning edges, slow enemies'};
