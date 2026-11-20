/* The jobs' hats (D18, S5), approved 2026-11-18 on the Missing Art page, version 2 (https://claude.ai/artifact/J265sfqjxSbdsSNsXU3VSh):
   designer, "The hats are good. The ones that are specific. Not the top hats" (the seven drafts below, and option B for the
   job icons: each job's own hat). Three hats are the UI concept's own (its hatFig: cap, beanie, tophat); the seven below
   were drawn for the page in the concept's grammar, in the concept's units (the Tiny Kitten's frame: its head E(-17, -58,
   29, 23)). Version 2 took the designer's notes: "Engineer needs to have yellow hard hat. Change daredevil to other color".
   tools/art/sticker_record.js runs this beside tools/art/concept_ui_screens.html and records each hat; never edit the
   recorded copy, change this and record again. */
const NEWHAT={
  helmet:()=>P('M-48 -72 C -48 -106, 14 -106, 14 -72 Z',L.c.metal)+R(-51,-79,68,10,5,L.c.metalDark)+C(-40,-74,1.8,'INK')+C(-17,-74,1.8,'INK')+C(6,-74,1.8,'INK')+C(-17,-106,4.5,L.c.metalDark),
  knit:()=>P('M-47 -72 C -47 -103, 13 -103, 13 -72 Z','#3B3546')+R(-49,-80,64,10,5,'#4A4458')+T('M-40 -86 L-26 -96',2.2,'#6A6478',{noLine:true})+T('M-30 -84 L-14 -98',2.2,'#6A6478',{noLine:true})+T('M-18 -84 L-2 -97',2.2,'#6A6478',{noLine:true}),
  stunt:()=>P('M-50 -70 C -52 -108, 16 -108, 14 -70 Z','#C93323')+R(-23,-106,12,36,4,U.white,{noLine:true})+T('M-50 -79 L14 -79',4,'INK')+C(-38,-81,6.5,L.c.visor)+C(-24,-82,6.5,L.c.visor),
  hardhat:()=>E(-17,-74,36,6,'#FFC83D')+P('M-45 -75 C -45 -104, 11 -104, 11 -75 Z','#FFC83D')+R(-21,-104,8,30,4,mix('#FFC83D','#FFFFFF',.45),{noLine:true}),
  wizard:()=>E(-17,-75,34,7,'#5E3F8E')+P('M-40 -76 L-14 -126 L6 -76 Z','#5E3F8E')+P(star4(-17,-96,5),L.c.horn,{lw:1.8})+P(star4(-8,-84,3.4),L.c.horn,{lw:1.6}),
  nemes:()=>P('M-50 -66 C -56 -100, 22 -100, 16 -66 L20 -40 L8 -44 L8 -66 C 0 -72, -34 -72, -42 -66 L-42 -44 L-54 -40 Z',L.c.horn)+T('M-44 -84 L10 -84',3,'#A88BFF',{noLine:true})+T('M-46 -76 L12 -76',3,'#A88BFF',{noLine:true})+T('M-48 -56 L-48 -46',3,'#A88BFF',{noLine:true})+T('M14 -56 L14 -46',3,'#A88BFF',{noLine:true}),
  tricorn:()=>P('M-56 -76 C -44 -70, 10 -70, 22 -76 C 14 -100, -44 -102, -56 -76 Z','#58B6FF')+T('M-54 -77 C -42 -72, 8 -72, 20 -77',3,L.c.horn,{noLine:true})+P('M-17 -98 C -12 -104, -22 -104, -17 -98 Z',L.c.horn)
};
/* Each job's hat: the game's job id -> the hat (NEWHAT's, or the concept's hatFig kind). */
const JOB_HATS={ratcatcher:'cap',unemployed:'beanie',tax_collector:'tophat',blacksmith:'helmet',cat_burglar:'knit',
  daredevil:'stunt',engineer:'hardhat',magician:'wizard',sphinx:'nemes',town_crier:'tricorn'};
const hatOf=k=>NEWHAT[k]||(()=>hatFig(k));
