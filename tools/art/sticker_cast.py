"""The approved Bestiary's cast in the generator grammar (D18, S5): RECORDED, never edited by hand.

Made by `node tools/art/sticker_record.js` from tools/art/concept_bestiary.html, the approved concept (the Bestiary
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


def mouse():
    """The Backyard mouse."""
    T("M26 -12 C 46 -8, 54 -28, 42 -38", 3.8, c["pink"])
    C(-12, -50, 11.5, c["mouseFur"])
    C(7, -47, 10.5, c["mouseFur"])
    C(-12, -50, 6.4, c["pink"], NO)
    C(7, -47, 5.8, c["pink"], NO)
    E(6, -21, 26, 21, c["mouseFur"])
    E(-14, -31, 19, 17, c["mouseFur"])
    E(-2, -13, 13, 8, c["mouseBelly"], NO)
    E(-31, -27, 9, 7, c["mouseFur"])
    C(-39.5, -28.5, 3.8, c["pink"])
    eye(-22, -36, 7.5, -0.7, -0.6)
    eye(-8, -38, 7, 0.75, 0.45)
    E(-26, -21.5, 2.6, 3.4, "INK")
    E(-12, -2.5, 7, 4.5, c["mouseFur"])
    E(14, -2.5, 7, 4.5, c["mouseFur"])


def vole():
    """The Alley vole."""
    T("M30 -12 L41 -16", 5, c["vole"])
    E(16, -3, 8, 4.5, c["pink"])
    P("M-31 -15 C -35 -35, -14 -47, 6 -46 C 27 -45, 39 -32, 36 -16 C 34 -4, 20 0, 0 0 C -18 0, -29 -4, -31 -15 Z", c["vole"])
    E(-2, -12, 17, 8, c["voleBelly"], NO)
    E(-34, -21, 10, 8.5, c["vole"])
    C(-43, -23, 4.4, c["pink"])
    T("M-36 -16 L-48 -14", 1.5, "INK")
    T("M-36 -19 L-49 -20", 1.5, "INK")
    eye(-22, -30, 6.5, -0.7, -0.5)
    eye(-10, -33, 6, 0.7, 0.5)
    E(-26, -12.5, 2.2, 3, "INK")
    E(-23, -2.5, 11, 5, c["pink"])
    T("M-31 -1 L-33 2", 1.6, "INK")
    T("M-26 0 L-27 3", 1.6, "INK")
    T("M-21 0 L-21 3", 1.6, "INK")


def rat():
    """The Park rat."""
    T("M30 -10 C 52 -6, 62 -20, 58 -36", 6, c["ratTail"])
    T("M44 -12 L45 -6", 1.4, "INK")
    T("M53 -18 L58 -15", 1.4, "INK")
    T("M57 -28 L62 -28", 1.4, "INK")
    E(20, -3, 9, 4.5, c["rat"])
    C(-10, -44, 6.5, c["rat"])
    C(-10, -44, 3.6, c["pink"], NO)
    P("M-14 -10 C -20 -30, -2 -50, 18 -44 C 36 -38, 38 -14, 30 -6 C 22 0, -6 0, -14 -10 Z", c["rat"])
    E(4, -12, 14, 8, c["ratBelly"], NO)
    P("M-6 -42 C -18 -48, -36 -38, -50 -28 C -44 -20, -26 -18, -8 -22 Z", c["rat"])
    C(-50, -28, 3.6, c["pink"])
    R(-45, -25, 5, 6, 1.2, c["eye"], {"lw": 1.3})
    eye(-26, -35, 6, -0.7, -0.5)
    eye(-15, -38, 5.6, 0.7, 0.5)
    T("M-40 -27 L-52 -24", 1.4, "INK")
    T("M-40 -24 L-51 -19", 1.4, "INK")
    E(-12, -3, 7, 4, c["rat"])


def pigeon():
    """The Rooftops pigeon."""
    T("M-4 -12 L-6 -2", 3, c["footP"])
    T("M8 -12 L8 -2", 3, c["footP"])
    T("M-12 -1 L0 -1", 3, c["footP"])
    T("M2 -1 L14 -1", 3, c["footP"])
    P("M26 -24 L48 -20 L44 -11 L24 -13 Z", c["pigeonWing"])
    E(4, -24, 28, 20, c["pigeon"])
    E(-18, -39, 15, 14, c["pigeon"])
    E(-9, -30, 11, 7, c["neckGreen"], NO)
    E(-6, -27, 7, 4.5, c["neckPurple"], NO)
    E(12, -25, 17, 11, c["pigeonWing"])
    T("M6 -26 L22 -26", 2.4, mix(c["pigeonWing"], INK, 0.45), NO)
    T("M8 -20 L24 -20", 2.4, mix(c["pigeonWing"], INK, 0.45), NO)
    P("M-31 -42 L-41 -38 L-31 -35 Z", c["pigeonBeak"], {"lw": EYE_LW * 0.7})
    E(-31, -41.5, 2.6, 1.8, c["pigeonBeakShine"], NO)
    eye(-23, -44, 5.6, -0.6, -0.6)
    eye(-14, -46, 5.4, 0.6, 0.5)


def sewer_rat():
    """The sewer rat (a plaster on its head)."""
    T("M30 -10 C 52 -6, 62 -20, 58 -36", 6, c["ratTail"])
    T("M44 -12 L45 -6", 1.4, "INK")
    T("M53 -18 L58 -15", 1.4, "INK")
    T("M57 -28 L62 -28", 1.4, "INK")
    E(20, -3, 9, 4.5, c["sewerRat"])
    C(-10, -44, 6.5, c["sewerRat"])
    C(-10, -44, 3.6, c["pink"], NO)
    P("M-14 -10 C -20 -30, -2 -50, 18 -44 C 36 -38, 38 -14, 30 -6 C 22 0, -6 0, -14 -10 Z", c["sewerRat"])
    E(4, -12, 14, 8, c["sewerBelly"], NO)
    P("M-6 -42 C -18 -48, -36 -38, -50 -28 C -44 -20, -26 -18, -8 -22 Z", c["sewerRat"])
    C(-50, -28, 3.6, c["pink"])
    R(-45, -25, 5, 6, 1.2, c["eye"], {"lw": 1.3})
    eye(-26, -35, 6, -0.7, -0.5)
    eye(-15, -38, 5.6, 0.7, 0.5)
    T("M-40 -27 L-52 -24", 1.4, "INK")
    T("M-40 -24 L-51 -19", 1.4, "INK")
    E(-12, -3, 7, 4, c["sewerRat"])
    with at(4, -44, 1, 24):
        R(-7, -3, 14, 6, 2, c["plaster"], {"lw": 1.6})
    C(24, -22, 2.4, mix(c["sewerRat"], INK, 0.25), NO)


def slime_rat():
    """The slime rat (melting)."""
    E(6, -3, 38, 6, c["slime"])
    T("M30 -10 C 52 -6, 62 -20, 58 -36", 6, c["slime"])
    T("M44 -12 L45 -6", 1.4, "INK")
    T("M53 -18 L58 -15", 1.4, "INK")
    T("M57 -28 L62 -28", 1.4, "INK")
    C(-10, -44, 6.5, c["slime"])
    C(-10, -44, 3.6, c["pink"], NO)
    P("M-14 -10 C -20 -30, -2 -50, 18 -44 C 36 -38, 38 -14, 30 -6 C 22 0, -6 0, -14 -10 Z", c["slime"])
    E(4, -12, 14, 8, c["slimeBelly"], NO)
    P("M-6 -42 C -18 -48, -36 -38, -50 -28 C -44 -20, -26 -18, -8 -22 Z", c["slime"])
    C(-50, -28, 3.6, c["pink"])
    R(-45, -25, 5, 6, 1.2, c["eye"], {"lw": 1.3})
    eye(-26, -35, 6, -0.7, -0.5)
    eye(-15, -38, 5.6, 0.7, 0.5)
    T("M-40 -27 L-52 -24", 1.4, "INK")
    T("M-40 -24 L-51 -19", 1.4, "INK")
    blob(-16, -6, 0.75, c["slime"])
    blob(12, -4, 0.65, c["slime"])
    blob(30, -7, 0.55, c["slime"])
    C(14, -26, 2.6, mix(c["slime"], INK, 0.2), NO)
    C(4, -34, 2, mix(c["slime"], INK, 0.2), NO)


def ghost():
    """The Portal ghost."""
    T("M22 -26 C 34 -28, 40 -38, 34 -46", 3.4, c["ghost"])
    C(-12, -66, 10, c["ghost"])
    C(9, -64, 9, c["ghost"])
    C(-12, -66, 5.5, c["ghostInner"], NO)
    C(9, -64, 5, c["ghostInner"], NO)
    P("M-26 -12 C -30 -44, -20 -70, 0 -70 C 20 -70, 30 -44, 26 -12 Q 20 -4, 14 -12 Q 8 -20, 2 -12 Q -4 -4, -10 -12 Q -16 -20, -20 -12 Q -24 -4, -26 -12 Z", c["ghost"])
    eye(-10, -46, 8, -0.6, -0.6)
    eye(8, -47, 8.5, 0.6, 0.5)
    E(-1, -31, 3, 4.2, "INK")


def imp():
    """The Mythic imp."""
    T("M14 -16 C 30 -12, 38 -24, 34 -36", 3.6, c["imp"])
    P("M34 -46 L41 -36 L34 -31 L27 -36 Z", c["imp"])
    P("M6 -40 C 12 -58, 28 -66, 40 -60 C 34 -56, 36 -52, 30 -50 C 30 -46, 24 -44, 16 -44 Z", c["impWing"])
    T("M-6 -18 C -10 -10, -4 -8, -8 -3", 7, c["imp"])
    T("M8 -18 C 12 -10, 6 -8, 10 -3", 7, c["imp"])
    R(-13, -5, 10, 5, 2.5, c["wheel"])
    R(5, -5, 10, 5, 2.5, c["wheel"])
    E(0, -28, 16, 15, c["imp"])
    E(-2, -25, 9, 8, c["impBelly"], NO)
    P("M6 -56 L22 -64 L12 -47 Z", c["imp"])
    P("M-17 -61 C -22 -71, -18 -78, -13 -81 C -13 -73, -11 -67, -8 -63 Z", c["bone"])
    P("M3 -63 C 4 -71, 8 -77, 13 -78 C 11 -71, 10 -66, 8 -61 Z", c["bone"])
    E(-5, -50, 17, 15, c["imp"])
    eye(-13, -52, 6.4, -0.6, -0.5)
    eye(-1, -53, 6, 0.6, 0.5)
    E(-8, -40, 2.3, 3, "INK")
    E(-14, -30, 4, 3.4, c["imp"])
    E(10, -30, 4, 3.4, c["imp"])


def ant():
    """The Kitchen ant."""
    T("M-2 -20 L-10 -12 L-14 -2", 2.4, "INK")
    T("M2 -20 L4 -10 L0 -2", 2.4, "INK")
    T("M4 -20 L16 -12 L20 -2", 2.4, "INK")
    E(22, -20, 17, 13, c["ant"])
    E(24, -26, 7, 3, c["antShine"], NO)
    E(0, -22, 9, 8, c["ant"])
    T("M-22 -38 L-26 -52 L-36 -56", 2.4, "INK")
    T("M-14 -39 L-14 -54 L-4 -60", 2.4, "INK")
    E(-18, -28, 13, 12, c["ant"])
    P("M-29 -22 C -36 -24, -38 -18, -34 -15 L-30 -19 Z", mix(c["ant"], INK, 0.3), {"lw": EYE_LW * 0.6})
    eye(-22, -31, 5.6, -0.6, -0.6)
    eye(-13, -32, 5.6, 0.6, 0.5)
    T("M-4 -18 L-14 -10 L-20 -2", 2.8, "INK")
    T("M0 -16 L-2 -8 L-6 -2", 2.8, "INK")
    T("M4 -18 L12 -10 L14 -2", 2.8, "INK")


def hamster():
    """The Vet's hamster."""
    C(-13, -55, 6, c["hamster"])
    C(13, -55, 6, c["hamster"])
    C(-13, -55, 3.2, c["pink"], NO)
    C(13, -55, 3.2, c["pink"], NO)
    E(-10, -2.5, 7, 4, c["pink"])
    E(10, -2.5, 7, 4, c["pink"])
    E(0, -26, 25, 25, c["hamster"])
    E(0, -18, 15, 15, c["hamsterBelly"], NO)
    E(0, -45, 20, 15, c["hamster"])
    E(-15, -38, 10, 8.5, c["hamsterBelly"])
    E(15, -38, 10, 8.5, c["hamsterBelly"])
    E(0, -27, 5, 6.5, c["seed"], {"lw": EYE_LW * 0.7})
    E(-6, -29, 4, 3.4, c["pink"])
    E(6, -29, 4, 3.4, c["pink"])
    eye(-7, -49, 5.6, -0.6, -0.6)
    eye(7, -49, 6, 0.6, 0.5)
    C(0, -42, 2.2, c["pink"], NO)
    E(0, -37.5, 1.8, 2.4, "INK")


def factory_rat():
    """The factory rat (goggles)."""
    T("M30 -10 C 52 -6, 62 -20, 58 -36", 6, c["ratTail"])
    T("M44 -12 L45 -6", 1.4, "INK")
    T("M53 -18 L58 -15", 1.4, "INK")
    T("M57 -28 L62 -28", 1.4, "INK")
    E(20, -3, 9, 4.5, c["facRat"])
    C(-10, -44, 6.5, c["facRat"])
    C(-10, -44, 3.6, c["pink"], NO)
    P("M-14 -10 C -20 -30, -2 -50, 18 -44 C 36 -38, 38 -14, 30 -6 C 22 0, -6 0, -14 -10 Z", c["facRat"])
    E(4, -12, 14, 8, c["facBelly"], NO)
    P("M-6 -42 C -18 -48, -36 -38, -50 -28 C -44 -20, -26 -18, -8 -22 Z", c["facRat"])
    C(-50, -28, 3.6, c["pink"])
    R(-45, -25, 5, 6, 1.2, c["eye"], {"lw": 1.3})
    eye(-26, -35, 6, -0.7, -0.5)
    eye(-15, -38, 5.6, 0.7, 0.5)
    T("M-40 -27 L-52 -24", 1.4, "INK")
    T("M-40 -24 L-51 -19", 1.4, "INK")
    E(-12, -3, 7, 4, c["facRat"])
    T("M-24 -46 L-4 -45", 3, "INK")
    C(-19, -47, 5, c["goggle"])
    C(-9, -46.5, 5, c["goggle"])
    C(-20, -48.5, 1.6, c["eye"], NO)


def dust_bunny():
    """The Litterbox dust bunny."""
    E2(-10, -66, 6.5, 17, -12, c["dustFur"])
    E2(18, -58, 6, 15, 64, c["dustFur"])
    E2(-10, -66, 3, 11, -12, c["pink"], NO)
    E2(18, -58, 2.8, 10, 64, c["pink"], NO)
    E(-10, -3, 7, 4, c["dustFur"])
    E(10, -3, 7, 4, c["dustFur"])
    P("M26 -34 A6.4 6.4 0 0 1 23.43 -22.72 A6.4 6.4 0 0 1 16.21 -13.67 A6.4 6.4 0 0 1 5.79 -8.65 A6.4 6.4 0 0 1 -5.79 -8.65 A6.4 6.4 0 0 1 -16.21 -13.67 A6.4 6.4 0 0 1 -23.43 -22.72 A6.4 6.4 0 0 1 -26 -34 A6.4 6.4 0 0 1 -23.43 -45.28 A6.4 6.4 0 0 1 -16.21 -54.33 A6.4 6.4 0 0 1 -5.79 -59.35 A6.4 6.4 0 0 1 5.79 -59.35 A6.4 6.4 0 0 1 16.21 -54.33 A6.4 6.4 0 0 1 23.43 -45.28 A6.4 6.4 0 0 1 26 -34 Z", c["dustFur"])
    C(-33, -54, 2.2, c["dustDark"], NO)
    C(31, -14, 2, c["dustDark"], NO)
    C(35, -48, 1.6, c["dustDark"], NO)
    C(-35, -22, 1.8, c["dustDark"], NO)
    eye(-9, -38, 8.5, -0.6, -0.6)
    eye(9.5, -39, 9, 0.65, 0.5)
    E(0, -24, 2.5, 3.3, "INK")


def moth():
    """Signature: the moth."""
    T("M-5 -54 C -8 -66, -14 -72, -21 -72", 2.4, "INK")
    T("M5 -54 C 8 -66, 14 -72, 21 -72", 2.4, "INK")
    C(-22, -72, 3.8, c["mothWing2"])
    C(22, -72, 3.8, c["mothWing2"])
    E2(-19, -48, 21, 14, -32, c["mothWing"])
    E2(19, -48, 21, 14, 32, c["mothWing"])
    E2(-16, -22, 14, 10, 24, c["mothWing2"])
    E2(16, -22, 14, 10, -24, c["mothWing2"])
    C(-24, -52, 5, c["mothWing2"], NO)
    C(24, -52, 5, c["mothWing2"], NO)
    E(-6, -3, 5.5, 3.5, c["mothBody"])
    E(6, -3, 5.5, 3.5, c["mothBody"])
    E(0, -30, 15, 24, c["mothBody"])
    E(0, -46, 11, 5.5, mix(c["mothBody"], c["eye"], 0.5), NO)
    eye(-6, -36, 6.5, -0.6, 0.5)
    eye(6.5, -37, 6, 0.6, -0.6)
    E(0, -23, 2.3, 3.2, "INK")


def dog():
    """Signature: the dog."""
    T("M36 -30 C 46 -38, 50 -48, 45 -57", 7, c["dogFur"])
    R(-2, -22, 10, 22, 5, mix(c["dogFur"], INK, 0.14))
    R(30, -22, 10, 22, 5, mix(c["dogFur"], INK, 0.14))
    E(12, -25, 30, 18, c["dogFur"])
    E(20, -31, 11, 7, c["dogSpot"], NO)
    R(-12, -18, 11, 18, 5.5, c["dogFur"])
    R(20, -18, 11, 18, 5.5, c["dogFur"])
    E(-36, -27, 4, 6, c["pink"], {"lw": EYE_LW * 0.6})
    E(-17, -43, 21, 18, c["dogFur"])
    E(-35, -36, 13, 9.5, mix(c["dogFur"], c["eye"], 0.55))
    E(-46.5, -40, 4.6, 3.6, "INK")
    P("M-12 -60 C 0 -64, 7 -54, 3 -38 C -1 -33, -8 -37, -8 -46 Z", c["dogSpot"])
    eye(-27, -49, 6.5, -0.6, 0.5)
    eye(-13.5, -51, 7, 0.7, -0.45)


def goose():
    """Signature: the goose."""
    T("M2 -10 L2 -3", 3.6, c["gooseBeak"])
    T("M22 -10 L22 -3", 3.6, c["gooseBeak"])
    E(-1, -2, 8, 3.4, c["gooseBeak"])
    E(19, -2, 8, 3.4, c["gooseBeak"])
    P("M36 -36 L54 -46 L48 -28 Z", c["gooseBody"])
    E(16, -25, 30, 19, c["gooseBody"])
    E(22, -27, 17, 10, c["gooseWing"])
    T("M-2 -32 C -10 -46, -2 -58, -10 -70", 13, c["gooseBody"])
    E(-12, -73, 12.5, 11, c["gooseBody"])
    P("M-22 -78 L-41 -78 L-24 -71 Z", c["gooseBeak"])
    P("M-23 -69 L-38 -65 L-22 -64 Z", c["gooseBeak"])
    eye(-17, -78, 5.4, -0.6, -0.5)
    eye(-7.5, -79, 5.8, 0.6, 0.5)


def seagull():
    """Signature: the seagull."""
    P("M-12 -46 C -28 -62, -50 -64, -66 -54 C -58 -50, -56 -46, -48 -44 C -46 -38, -34 -36, -18 -36 Z", c["gullWing"])
    P("M-66 -54 C -60 -58, -54 -58, -50 -56 C -54 -52, -58 -50, -62 -51 Z", c["gullTip"], NO)
    with at(0, 0, 1, 0, flip=True):
        P("M-12 -46 C -28 -62, -50 -64, -66 -54 C -58 -50, -56 -46, -48 -44 C -46 -38, -34 -36, -18 -36 Z", c["gullWing"])
        P("M-66 -54 C -60 -58, -54 -58, -50 -56 C -54 -52, -58 -50, -62 -51 Z", c["gullTip"], NO)
    T("M-6 -14 L-7 -3", 3.4, c["footO"])
    T("M6 -14 L7 -3", 3.4, c["footO"])
    P("M-15 -2 L-7 -6 L1 -2 Z", c["footO"])
    P("M-1 -2 L7 -6 L15 -2 Z", c["footO"])
    E(0, -30, 18, 20, c["seagull"])
    E(0, -56, 14, 12.5, c["seagull"])
    eye(-6, -60, 5, -0.6, -0.6)
    eye(6, -60, 5.4, 0.6, 0.5)
    P("M-5 -53 L5 -53 L0 -41 Z", c["beakY"], {"lw": EYE_LW * 0.8})
    C(0, -45.5, 1.6, c["gullMouth"], NO)


def crocodile():
    """Signature: the crocodile."""
    T("M20 -16 C 38 -16, 46 -28, 44 -42", 12, c["croc"])
    T("M44 -42 C 43 -52, 35 -55, 31 -48", 7, c["croc"])
    P("M-10 -29 L-6 -37 L-2 -29 Z", mix(c["croc"], INK, 0.15))
    P("M0 -30.5 L4 -38.5 L8 -30.5 Z", mix(c["croc"], INK, 0.15))
    P("M10 -29 L14 -37 L18 -29 Z", mix(c["croc"], INK, 0.15))
    P("M20 -24 L24 -32 L28 -24 Z", mix(c["croc"], INK, 0.15))
    P("M32 -26 L36 -34 L40 -26 Z", mix(c["croc"], INK, 0.15))
    P("M38 -36 L42 -44 L46 -36 Z", mix(c["croc"], INK, 0.15))
    E(-12, -6, 8, 5, mix(c["croc"], INK, 0.14))
    E(18, -6, 8, 5, mix(c["croc"], INK, 0.14))
    E(4, -18, 26, 16, c["croc"])
    E(2, -8, 18, 5.5, c["crocBelly"], NO)
    E(-18, -4, 9, 5, c["croc"])
    E(14, -4, 9, 5, c["croc"])
    T("M-25 -2 L-27 1", 1.5, "INK")
    T("M-20 -1 L-21 2", 1.5, "INK")
    T("M8 -2 L6 1", 1.5, "INK")
    T("M13 -1 L12 2", 1.5, "INK")
    C(-24, -36, 8.5, c["croc"])
    C(-10, -37, 8.5, c["croc"])
    P("M-8 -34 C -24 -40, -46 -34, -56 -24 C -58 -14, -48 -8, -30 -8 C -18 -8, -8 -14, -8 -22 Z", c["croc"])
    P("M-52 -17 L-50 -13 L-47 -17 L-44 -13 L-41 -17 L-38 -13 L-35 -17 L-32 -13 L-29 -17 L-26 -13 L-23 -17 Z", c["eye"], {"lw": 1.3})
    C(-52, -28, 1.6, "INK")
    C(-48, -30, 1.6, "INK")
    eye(-24, -37.5, 6.4, -0.6, -0.6)
    eye(-10, -38.5, 6.4, 0.6, 0.5)


def toad():
    """Signature: the toad."""
    E(22, -9, 15, 10, c["toad"])
    E(30, -2.5, 11, 4, c["toad"])
    E(4, -17, 30, 17, c["toad"])
    E(-2, -7, 19, 6.5, c["toadBelly"], NO)
    C(12, -24, 3, c["toadWart"], NO)
    C(22, -18, 2.4, c["toadWart"], NO)
    C(4, -28, 2.2, c["toadWart"], NO)
    C(26, -28, 2.6, c["toadWart"], NO)
    C(-24, -36, 8.5, c["toad"])
    C(-8, -37, 8.5, c["toad"])
    E(-14, -25, 21, 13, c["toad"])
    eye(-24, -37, 6.6, -0.6, -0.6)
    eye(-8, -38, 6.6, 0.6, 0.5)
    T("M-33 -22 Q -20 -14 -4 -20", 2.5, "INK")
    E(-22, -3, 9, 4, c["toad"])


def hound():
    """Signature: the hound."""
    T("M28 -40 C 38 -46, 40 -56, 36 -62", 4, c["hound"])
    T("M-10 -24 L-13 -2", 6, mix(c["hound"], INK, 0.15))
    T("M22 -30 L26 -2", 6, mix(c["hound"], INK, 0.15))
    P("M-22 -42 C -26 -26, -16 -18, -2 -24 C 10 -28, 22 -30, 30 -36 C 34 -46, 24 -52, 10 -50 C -2 -50, -16 -52, -22 -42 Z", c["hound"])
    T("M-16 -26 L-19 -2", 7, c["hound"])
    T("M18 -32 L16 -2", 7, c["hound"])
    E(-21, -2, 6, 3.4, c["hound"])
    E(14, -2, 6, 3.4, c["hound"])
    C(6, -40, 3.4, c["houndSpot"], NO)
    C(18, -38, 2.6, c["houndSpot"], NO)
    C(-8, -44, 2.4, c["houndSpot"], NO)
    P("M-20 -62 L-14 -76 L-10 -60 Z", c["hound"])
    E(-24, -56, 13, 11, c["hound"])
    P("M-32 -60 C -44 -60, -56 -56, -60 -50 C -56 -45, -44 -45, -32 -48 Z", c["hound"])
    E(-60, -51, 3.6, 3, "INK")
    eye(-28, -60, 5.4, -0.6, -0.6)
    eye(-19, -61, 5.2, 0.6, 0.5)
    E(-44, -44, 2, 2.8, "INK")


def crow():
    """Signature: the crow."""
    T("M-2 -10 L-4 -2", 3, c["hoof"])
    T("M8 -10 L8 -2", 3, c["hoof"])
    T("M-10 -1 L2 -1", 3, c["hoof"])
    T("M2 -1 L14 -1", 3, c["hoof"])
    P("M24 -26 L48 -32 L46 -18 L26 -12 Z", c["crow"])
    E(4, -26, 26, 20, c["crow"])
    P("M-2 -34 L30 -38 L34 -22 L6 -16 Z", c["crowWing"])
    P("M-24 -56 L-20 -66 L-16 -56 Z", c["crow"])
    P("M-16 -57 L-10 -66 L-8 -55 Z", c["crow"])
    E(-18, -44, 15, 14, c["crow"])
    P("M-30 -48 L-52 -42 L-30 -37 Z", c["crowBeak"], {"lw": EYE_LW * 0.8})
    eye(-22, -46, 5.6, -0.6, -0.6)
    eye(-12, -47, 6, 0.6, 0.5)
    eye(-17, -56, 3.6, 0, -0.7)
    spark(-40, -62, 4.4, c["voidStar"])
    spark(32, -44, 3.6, c["voidStar"])


def cockroach():
    """Signature: the cockroach."""
    T("M3 -12 L7 -1", 2.4, "INK")
    T("M17 -12 L21 -1", 2.4, "INK")
    T("M31 -12 L35 -1", 2.4, "INK")
    T("M-30 -26 C -44 -40, -42 -56, -26 -62", 2, "INK")
    T("M-25 -29 C -34 -46, -26 -58, -12 -62", 2, "INK")
    E(8, -20, 30, 15, c["roachShell"])
    T("M-14 -24 C 4 -34, 22 -34, 37 -22", 2.2, mix(c["roachShell"], INK, 0.35), NO)
    E(12, -28, 10, 3, mix(c["roachShell"], c["eye"], 0.35), NO)
    T("M-2 -8 L-7 -1", 2.8, "INK")
    T("M12 -8 L7 -1", 2.8, "INK")
    T("M26 -8 L21 -1", 2.8, "INK")
    E(-22, -19, 12.5, 11.5, c["roachBelly"])
    eye(-27, -23, 5.6, -0.6, -0.5)
    eye(-17.5, -24.5, 5.9, 0.6, 0.5)
    E(-30, -13, 1.9, 2.5, "INK")


def parrot():
    """Signature: the parrot."""
    P("M16 -22 L40 -6 L33 1 L10 -14 Z", c["parrotWing"])
    T("M-4 -8 L-6 -2", 3, c["hoof"])
    T("M6 -8 L6 -2", 3, c["hoof"])
    T("M-12 -1 L0 -1", 3, c["hoof"])
    T("M0 -1 L12 -1", 3, c["hoof"])
    E(4, -26, 18, 21, c["parrot"])
    E(-2, -20, 10, 13, c["parrotBelly"], NO)
    E(11, -26, 10, 15, c["parrotWing"])
    P("M-2 -60 C 0 -74, 8 -80, 14 -78 C 8 -72, 6 -66, 4 -60 Z", c["parrotCrest"])
    P("M2 -60 C 8 -70, 16 -72, 20 -68 C 14 -66, 10 -62, 8 -58 Z", c["parrotCrest"])
    E(-6, -50, 14, 13, c["parrot"])
    P("M-17 -55 C -27 -55, -29 -44, -23 -38 C -23 -44, -19 -46, -15 -47 Z", c["parrotBeak"])
    E(-15, -47, 4, 3, c["parrotBeakShine"])
    eye(-9, -54, 5.4, -0.6, -0.6)
    eye(0, -55, 5.2, 0.6, 0.5)


def robot_mouse():
    """Signature: the robot mouse."""
    T("M22 -26 L34 -26", 3, c["robotDark"])
    P("M34 -34 C 44 -38, 46 -24, 36 -24 C 46 -24, 44 -12, 34 -16 Z", c["robot"])
    C(-16, -44, 10, c["robot"])
    C(6, -46, 9, c["robot"])
    C(-16, -44, 3, c["robotDark"])
    C(6, -46, 2.6, c["robotDark"])
    C(-13, -7, 7.5, c["wheel"])
    C(13, -7, 7.5, c["wheel"])
    R(-28, -40, 52, 32, 8, c["robot"])
    T("M-24 -22 L20 -22", 1.6, mix(c["robot"], INK, 0.35), NO)
    C(-23, -35, 1.6, "INK")
    C(19, -35, 1.6, "INK")
    C(-23, -13, 1.6, "INK")
    C(19, -13, 1.6, "INK")
    P("M-28 -32 L-42 -26 L-28 -20 Z", c["robot"])
    C(-43, -26, 4.2, c["light"])
    C(-13, -7, 2.6, c["robot"], NO)
    C(13, -7, 2.6, c["robot"], NO)
    eye(-17, -30, 6, -0.6, -0.6)
    eye(-4, -31, 6.4, 0.6, 0.5)


def sludge_cat():
    """Signature: the sludge cat."""
    E(4, -6, 42, 8, c["sludge"])
    T("M26 -10 C 40 -18, 44 -34, 36 -44", 7, c["sludge"])
    P("M-20 -50 L-23 -70 L-9 -56 Z", c["sludge"])
    P("M1 -56 C 9 -59, 15 -55, 18 -46 L 5 -50 Z", c["sludge"])
    P("M-30 -6 C -34 -28, -27 -54, -8 -57 C 12 -59, 26 -36, 30 -6 Z", c["sludge"])
    C(12, -28, 4, c["sludgeDark"], NO)
    C(18, -16, 3, c["sludgeDark"], NO)
    C(-16, -15, 3.4, c["sludgeDark"], NO)
    C(-2, -47, 2.4, c["sludgeDark"], NO)
    blob(-34, -16, 0.7, c["sludge"])
    eye(-14, -41, 7, -0.5, -0.6)
    eye(1, -37, 6, 0.5, 0.6)
    E(-7, -27, 2.4, 3.4, "INK")


def flying_mouse():
    """The flying mouse (Alley, Park)."""
    T("M26 -12 C 46 -8, 54 -28, 42 -38", 3.8, c["pink"])
    with at(-12, -2, 0.92, -16):
        P("M4 -34 C 4 -58, 24 -74, 46 -68 C 41 -62, 44 -56, 37 -54 C 38 -47, 31 -44, 26 -45 C 24 -38, 15 -34, 8 -32 Z", mix(c["flierWing"], INK, 0.18))
    C(-12, -50, 11.5, c["flierFur"])
    C(7, -47, 10.5, c["flierFur"])
    C(-12, -50, 6.4, c["pink"], NO)
    C(7, -47, 5.8, c["pink"], NO)
    E(6, -21, 26, 21, c["flierFur"])
    E(-14, -31, 19, 17, c["flierFur"])
    E(-2, -13, 13, 8, c["mouseBelly"], NO)
    E(-31, -27, 9, 7, c["flierFur"])
    C(-39.5, -28.5, 3.8, c["pink"])
    eye(-22, -36, 7.5, -0.7, -0.6)
    eye(-8, -38, 7, 0.75, 0.45)
    E(-26, -21.5, 2.6, 3.4, "INK")
    E(-12, -2.5, 7, 4.5, c["flierFur"])
    E(14, -2.5, 7, 4.5, c["flierFur"])
    P("M4 -34 C 4 -58, 24 -74, 46 -68 C 41 -62, 44 -56, 37 -54 C 38 -47, 31 -44, 26 -45 C 24 -38, 15 -34, 8 -32 Z", c["flierWing"])


def bat():
    """The bat (the flier from the Rooftops on)."""
    P("M-10 -44 C -26 -60, -48 -64, -60 -52 C -54 -48, -52 -44, -48 -42 C -46 -36, -40 -34, -34 -36 C -30 -30, -22 -30, -12 -34 Z", c["batWing"])
    T("M-12 -42 L-58 -52", 1.5, mix(c["batWing"], INK, 0.5), NO)
    T("M-12 -40 L-47 -42", 1.5, mix(c["batWing"], INK, 0.5), NO)
    T("M-12 -38 L-34 -36", 1.5, mix(c["batWing"], INK, 0.5), NO)
    with at(0, 0, 1, 0, flip=True):
        P("M-10 -44 C -26 -60, -48 -64, -60 -52 C -54 -48, -52 -44, -48 -42 C -46 -36, -40 -34, -34 -36 C -30 -30, -22 -30, -12 -34 Z", c["batWing"])
        T("M-12 -42 L-58 -52", 1.5, mix(c["batWing"], INK, 0.5), NO)
        T("M-12 -40 L-47 -42", 1.5, mix(c["batWing"], INK, 0.5), NO)
        T("M-12 -38 L-34 -36", 1.5, mix(c["batWing"], INK, 0.5), NO)
    P("M-12 -54 L-14 -72 L-2 -58 Z", c["bat"])
    P("M12 -54 L14 -72 L2 -58 Z", c["bat"])
    P("M-11 -57 L-12 -67 L-5 -59 Z", c["pink"], NO)
    P("M11 -57 L12 -67 L5 -59 Z", c["pink"], NO)
    E(0, -38, 13, 15, c["bat"])
    E(0, -34, 7, 8, mix(c["bat"], c["eye"], 0.3), NO)
    eye(-5.5, -44, 5, -0.6, -0.6)
    eye(5.5, -44, 5.4, 0.6, 0.5)
    E(0, -34.5, 2.4, 3, "INK")
    P("M-4 -32 L-3 -28 L-2 -32 Z", c["eye"], {"lw": 1})
    P("M2 -32 L3 -28 L4 -32 Z", c["eye"], {"lw": 1})
    T("M-4 -24 L-5 -21", 2.4, c["bat"])
    T("M4 -24 L5 -21", 2.4, c["bat"])


def golden_mouse():
    """The golden mouse (the game sparkles it: EnemyData.sparkle)."""
    T("M26 -12 C 46 -8, 54 -28, 42 -38", 3.8, c["pink"])
    C(-12, -50, 11.5, c["gold"])
    C(7, -47, 10.5, c["gold"])
    C(-12, -50, 6.4, c["pink"], NO)
    C(7, -47, 5.8, c["pink"], NO)
    E(6, -21, 26, 21, c["gold"])
    E(-14, -31, 19, 17, c["gold"])
    E(-2, -13, 13, 8, c["goldBelly"], NO)
    E(-31, -27, 9, 7, c["gold"])
    C(-39.5, -28.5, 3.8, c["pink"])
    eye(-22, -36, 7.5, -0.7, -0.6)
    eye(-8, -38, 7, 0.75, 0.45)
    E(-26, -21.5, 2.6, 3.4, "INK")
    E(-12, -2.5, 7, 4.5, c["gold"])
    E(14, -2.5, 7, 4.5, c["gold"])


def gristle_1():
    """The gristle of tier 1 (its spikes, lumps and scars grow with the tier)."""
    P("M-16.85 -63.66 L-16.88 -76.86 L-6.86 -68.27 Z", c["bone"])
    P("M32.61 -50.05 L45.81 -49.95 L37.11 -40.02 Z", c["bone"])
    T("M-4 -52 C -4 -36, 0 -22, 0 -8", 13, mix(mix(c["gristle"], c["gristleDeep"], 0), INK, 0.22))
    E(0, -6, 9, 6, mix(mix(c["gristle"], c["gristleDeep"], 0), INK, 0.22))
    E(24, -12, 9, 7, mix(mix(c["gristle"], c["gristleDeep"], 0), INK, 0.22))
    E(26, -3, 9, 4, mix(mix(c["gristle"], c["gristleDeep"], 0), INK, 0.22))
    P("M-30 -42 C -32 -64, -10 -76, 8 -72 C 26 -68, 40 -52, 42 -34 C 44 -20, 36 -12, 26 -12 C 16 -12, 8 -20, -2 -22 C -16 -24, -28 -30, -30 -42 Z", mix(c["gristle"], c["gristleDeep"], 0))
    E(-14, -34, 13, 11, c["gristleBelly"], NO)
    T("M2 -62 L14 -52", 1.8, "INK")
    T("M5 -55 L10 -59", 1.4, "INK")
    E(32, -18, 11, 10, mix(c["gristle"], c["gristleDeep"], 0))
    E(36, -4, 11, 5, mix(c["gristle"], c["gristleDeep"], 0))
    E(-38, -40, 12, 11, mix(c["gristle"], c["gristleDeep"], 0))
    with at(-38, -51, 1, -20):
        R(-7, -2.5, 14, 5, 2, c["plaster"], {"lw": 1.5})
    eye(-44, -42, 4.8, -0.6, -0.5)
    eye(-35, -43, 4.6, 0.6, 0.5)
    E(-41, -33, 2, 2.6, "INK")
    P("M-47 -30 L-46 -35 L-44 -30 Z", c["eye"], {"lw": 1})
    P("M-38 -29.5 L-37 -34.5 L-35 -29.5 Z", c["eye"], {"lw": 1})
    T("M-14 -54 C -24 -40, -26 -22, -22 -8", 16, mix(c["gristle"], c["gristleDeep"], 0))
    E(-23, -6, 11, 7, mix(c["gristle"], c["gristleDeep"], 0))
    T("M-29 -7 L-29 -3", 1.4, "INK")
    T("M-24 -8 L-24 -3", 1.4, "INK")
    T("M-19 -7 L-19 -3", 1.4, "INK")


def gristle_2():
    """The gristle of tier 2 (its spikes, lumps and scars grow with the tier)."""
    P("M-16.85 -63.66 L-17.22 -77.61 L-6.86 -68.27 Z", c["bone"])
    P("M32.61 -50.05 L46.55 -50.29 L37.11 -40.02 Z", c["bone"])
    T("M-4 -52 C -4 -36, 0 -22, 0 -8", 13, mix(mix(c["gristle"], c["gristleDeep"], 0.09090909090909091), INK, 0.22))
    E(0, -6, 9, 6, mix(mix(c["gristle"], c["gristleDeep"], 0.09090909090909091), INK, 0.22))
    E(24, -12, 9, 7, mix(mix(c["gristle"], c["gristleDeep"], 0.09090909090909091), INK, 0.22))
    E(26, -3, 9, 4, mix(mix(c["gristle"], c["gristleDeep"], 0.09090909090909091), INK, 0.22))
    P("M-30 -42 C -32 -64, -10 -76, 8 -72 C 26 -68, 40 -52, 42 -34 C 44 -20, 36 -12, 26 -12 C 16 -12, 8 -20, -2 -22 C -16 -24, -28 -30, -30 -42 Z", mix(c["gristle"], c["gristleDeep"], 0.09090909090909091))
    E(-14, -34, 13, 11, c["gristleBelly"], NO)
    T("M2 -62 L14 -52", 1.8, "INK")
    T("M5 -55 L10 -59", 1.4, "INK")
    E(32, -18, 11, 10, mix(c["gristle"], c["gristleDeep"], 0.09090909090909091))
    E(36, -4, 11, 5, mix(c["gristle"], c["gristleDeep"], 0.09090909090909091))
    E(-38, -40, 12, 11, mix(c["gristle"], c["gristleDeep"], 0.09090909090909091))
    with at(-38, -51, 1, -20):
        R(-7, -2.5, 14, 5, 2, c["plaster"], {"lw": 1.5})
    eye(-44, -42, 4.8, -0.6, -0.5)
    eye(-35, -43, 4.6, 0.6, 0.5)
    E(-41, -33, 2, 2.6, "INK")
    P("M-47 -30 L-46 -35 L-44 -30 Z", c["eye"], {"lw": 1})
    P("M-38 -29.5 L-37 -34.5 L-35 -29.5 Z", c["eye"], {"lw": 1})
    T("M-14 -54 C -24 -40, -26 -22, -22 -8", 16, mix(c["gristle"], c["gristleDeep"], 0.09090909090909091))
    E(-23, -6, 11, 7, mix(c["gristle"], c["gristleDeep"], 0.09090909090909091))
    T("M-29 -7 L-29 -3", 1.4, "INK")
    T("M-24 -8 L-24 -3", 1.4, "INK")
    T("M-19 -7 L-19 -3", 1.4, "INK")


def gristle_3():
    """The gristle of tier 3 (its spikes, lumps and scars grow with the tier)."""
    P("M-16.85 -63.66 L-17.56 -78.35 L-6.86 -68.27 Z", c["bone"])
    P("M4.46 -69 L13.65 -80.48 L15 -65.84 Z", c["bone"])
    P("M32.61 -50.05 L47.3 -50.62 L37.11 -40.02 Z", c["bone"])
    C(13.15, -68.89, 5.55, mix(c["gristle"], c["gristleDeep"], 0.18181818181818182))
    T("M-4 -52 C -4 -36, 0 -22, 0 -8", 13, mix(mix(c["gristle"], c["gristleDeep"], 0.18181818181818182), INK, 0.22))
    E(0, -6, 9, 6, mix(mix(c["gristle"], c["gristleDeep"], 0.18181818181818182), INK, 0.22))
    E(24, -12, 9, 7, mix(mix(c["gristle"], c["gristleDeep"], 0.18181818181818182), INK, 0.22))
    E(26, -3, 9, 4, mix(mix(c["gristle"], c["gristleDeep"], 0.18181818181818182), INK, 0.22))
    P("M-30 -42 C -32 -64, -10 -76, 8 -72 C 26 -68, 40 -52, 42 -34 C 44 -20, 36 -12, 26 -12 C 16 -12, 8 -20, -2 -22 C -16 -24, -28 -30, -30 -42 Z", mix(c["gristle"], c["gristleDeep"], 0.18181818181818182))
    E(-14, -34, 13, 11, c["gristleBelly"], NO)
    T("M2 -62 L14 -52", 1.8, "INK")
    T("M5 -55 L10 -59", 1.4, "INK")
    E(32, -18, 11, 10, mix(c["gristle"], c["gristleDeep"], 0.18181818181818182))
    E(36, -4, 11, 5, mix(c["gristle"], c["gristleDeep"], 0.18181818181818182))
    E(-38, -40, 12, 11, mix(c["gristle"], c["gristleDeep"], 0.18181818181818182))
    with at(-38, -51, 1, -20):
        R(-7, -2.5, 14, 5, 2, c["plaster"], {"lw": 1.5})
    eye(-44, -42, 4.8, -0.6, -0.5)
    eye(-35, -43, 4.6, 0.6, 0.5)
    E(-41, -33, 2, 2.6, "INK")
    P("M-47 -30 L-46 -35 L-44 -30 Z", c["eye"], {"lw": 1})
    P("M-38 -29.5 L-37 -34.5 L-35 -29.5 Z", c["eye"], {"lw": 1})
    T("M-14 -54 C -24 -40, -26 -22, -22 -8", 16, mix(c["gristle"], c["gristleDeep"], 0.18181818181818182))
    E(-23, -6, 11, 7, mix(c["gristle"], c["gristleDeep"], 0.18181818181818182))
    T("M-29 -7 L-29 -3", 1.4, "INK")
    T("M-24 -8 L-24 -3", 1.4, "INK")
    T("M-19 -7 L-19 -3", 1.4, "INK")


def gristle_4():
    """The gristle of tier 4 (its spikes, lumps and scars grow with the tier)."""
    P("M-16.85 -63.66 L-17.9 -79.09 L-6.86 -68.27 Z", c["bone"])
    P("M4.46 -69 L13.89 -81.26 L15 -65.84 Z", c["bone"])
    P("M32.61 -50.05 L48.05 -50.96 L37.11 -40.02 Z", c["bone"])
    C(13.15, -68.89, 5.82, mix(c["gristle"], c["gristleDeep"], 0.2727272727272727))
    T("M-4 -52 C -4 -36, 0 -22, 0 -8", 13, mix(mix(c["gristle"], c["gristleDeep"], 0.2727272727272727), INK, 0.22))
    E(0, -6, 9, 6, mix(mix(c["gristle"], c["gristleDeep"], 0.2727272727272727), INK, 0.22))
    E(24, -12, 9, 7, mix(mix(c["gristle"], c["gristleDeep"], 0.2727272727272727), INK, 0.22))
    E(26, -3, 9, 4, mix(mix(c["gristle"], c["gristleDeep"], 0.2727272727272727), INK, 0.22))
    P("M-30 -42 C -32 -64, -10 -76, 8 -72 C 26 -68, 40 -52, 42 -34 C 44 -20, 36 -12, 26 -12 C 16 -12, 8 -20, -2 -22 C -16 -24, -28 -30, -30 -42 Z", mix(c["gristle"], c["gristleDeep"], 0.2727272727272727))
    E(-14, -34, 13, 11, c["gristleBelly"], NO)
    T("M2 -62 L14 -52", 1.8, "INK")
    T("M5 -55 L10 -59", 1.4, "INK")
    E(32, -18, 11, 10, mix(c["gristle"], c["gristleDeep"], 0.2727272727272727))
    E(36, -4, 11, 5, mix(c["gristle"], c["gristleDeep"], 0.2727272727272727))
    E(-38, -40, 12, 11, mix(c["gristle"], c["gristleDeep"], 0.2727272727272727))
    with at(-38, -51, 1, -20):
        R(-7, -2.5, 14, 5, 2, c["plaster"], {"lw": 1.5})
    eye(-44, -42, 4.8, -0.6, -0.5)
    eye(-35, -43, 4.6, 0.6, 0.5)
    E(-41, -33, 2, 2.6, "INK")
    P("M-47 -30 L-46 -35 L-44 -30 Z", c["eye"], {"lw": 1})
    P("M-38 -29.5 L-37 -34.5 L-35 -29.5 Z", c["eye"], {"lw": 1})
    T("M-14 -54 C -24 -40, -26 -22, -22 -8", 16, mix(c["gristle"], c["gristleDeep"], 0.2727272727272727))
    E(-23, -6, 11, 7, mix(c["gristle"], c["gristleDeep"], 0.2727272727272727))
    T("M-29 -7 L-29 -3", 1.4, "INK")
    T("M-24 -8 L-24 -3", 1.4, "INK")
    T("M-19 -7 L-19 -3", 1.4, "INK")


def gristle_5():
    """The gristle of tier 5 (its spikes, lumps and scars grow with the tier)."""
    P("M-16.85 -63.66 L-18.24 -79.84 L-6.86 -68.27 Z", c["bone"])
    P("M-3.66 -68.91 L2.29 -84.01 L7.33 -68.59 Z", c["bone"])
    P("M15.57 -65.54 L28.64 -75.16 L24.71 -59.41 Z", c["bone"])
    P("M32.61 -50.05 L48.79 -51.29 L37.11 -40.02 Z", c["bone"])
    C(13.15, -68.89, 6.09, mix(c["gristle"], c["gristleDeep"], 0.36363636363636365))
    T("M-4 -52 C -4 -36, 0 -22, 0 -8", 13, mix(mix(c["gristle"], c["gristleDeep"], 0.36363636363636365), INK, 0.22))
    E(0, -6, 9, 6, mix(mix(c["gristle"], c["gristleDeep"], 0.36363636363636365), INK, 0.22))
    E(24, -12, 9, 7, mix(mix(c["gristle"], c["gristleDeep"], 0.36363636363636365), INK, 0.22))
    E(26, -3, 9, 4, mix(mix(c["gristle"], c["gristleDeep"], 0.36363636363636365), INK, 0.22))
    P("M-30 -42 C -32 -64, -10 -76, 8 -72 C 26 -68, 40 -52, 42 -34 C 44 -20, 36 -12, 26 -12 C 16 -12, 8 -20, -2 -22 C -16 -24, -28 -30, -30 -42 Z", mix(c["gristle"], c["gristleDeep"], 0.36363636363636365))
    E(-14, -34, 13, 11, c["gristleBelly"], NO)
    T("M2 -62 L14 -52", 1.8, "INK")
    T("M5 -55 L10 -59", 1.4, "INK")
    T("M22 -46 L32 -38", 1.8, "INK")
    T("M25 -40 L29 -44", 1.4, "INK")
    E(32, -18, 11, 10, mix(c["gristle"], c["gristleDeep"], 0.36363636363636365))
    E(36, -4, 11, 5, mix(c["gristle"], c["gristleDeep"], 0.36363636363636365))
    E(-38, -40, 12, 11, mix(c["gristle"], c["gristleDeep"], 0.36363636363636365))
    with at(-38, -51, 1, -20):
        R(-7, -2.5, 14, 5, 2, c["plaster"], {"lw": 1.5})
    eye(-44, -42, 4.8, -0.6, -0.5)
    eye(-35, -43, 4.6, 0.6, 0.5)
    E(-41, -33, 2, 2.6, "INK")
    P("M-47 -30 L-46 -35 L-44 -30 Z", c["eye"], {"lw": 1})
    P("M-38 -29.5 L-37 -34.5 L-35 -29.5 Z", c["eye"], {"lw": 1})
    T("M-14 -54 C -24 -40, -26 -22, -22 -8", 16, mix(c["gristle"], c["gristleDeep"], 0.36363636363636365))
    E(-23, -6, 11, 7, mix(c["gristle"], c["gristleDeep"], 0.36363636363636365))
    T("M-29 -7 L-29 -3", 1.4, "INK")
    T("M-24 -8 L-24 -3", 1.4, "INK")
    T("M-19 -7 L-19 -3", 1.4, "INK")


def gristle_6():
    """The gristle of tier 6 (its spikes, lumps and scars grow with the tier)."""
    P("M-16.85 -63.66 L-18.59 -80.58 L-6.86 -68.27 Z", c["bone"])
    P("M-3.66 -68.91 L2.31 -84.83 L7.33 -68.59 Z", c["bone"])
    P("M15.57 -65.54 L29.1 -75.84 L24.71 -59.41 Z", c["bone"])
    P("M32.61 -50.05 L49.54 -51.63 L37.11 -40.02 Z", c["bone"])
    C(13.15, -68.89, 6.36, mix(c["gristle"], c["gristleDeep"], 0.45454545454545453))
    T("M-4 -52 C -4 -36, 0 -22, 0 -8", 13, mix(mix(c["gristle"], c["gristleDeep"], 0.45454545454545453), INK, 0.22))
    E(0, -6, 9, 6, mix(mix(c["gristle"], c["gristleDeep"], 0.45454545454545453), INK, 0.22))
    E(24, -12, 9, 7, mix(mix(c["gristle"], c["gristleDeep"], 0.45454545454545453), INK, 0.22))
    E(26, -3, 9, 4, mix(mix(c["gristle"], c["gristleDeep"], 0.45454545454545453), INK, 0.22))
    P("M-30 -42 C -32 -64, -10 -76, 8 -72 C 26 -68, 40 -52, 42 -34 C 44 -20, 36 -12, 26 -12 C 16 -12, 8 -20, -2 -22 C -16 -24, -28 -30, -30 -42 Z", mix(c["gristle"], c["gristleDeep"], 0.45454545454545453))
    E(-14, -34, 13, 11, c["gristleBelly"], NO)
    T("M2 -62 L14 -52", 1.8, "INK")
    T("M5 -55 L10 -59", 1.4, "INK")
    T("M22 -46 L32 -38", 1.8, "INK")
    T("M25 -40 L29 -44", 1.4, "INK")
    E(32, -18, 11, 10, mix(c["gristle"], c["gristleDeep"], 0.45454545454545453))
    E(36, -4, 11, 5, mix(c["gristle"], c["gristleDeep"], 0.45454545454545453))
    E(-38, -40, 12, 11, mix(c["gristle"], c["gristleDeep"], 0.45454545454545453))
    with at(-38, -51, 1, -20):
        R(-7, -2.5, 14, 5, 2, c["plaster"], {"lw": 1.5})
    eye(-44, -42, 4.8, -0.6, -0.5)
    eye(-35, -43, 4.6, 0.6, 0.5)
    E(-41, -33, 2, 2.6, "INK")
    P("M-47 -30 L-46 -35 L-44 -30 Z", c["eye"], {"lw": 1})
    P("M-38 -29.5 L-37 -34.5 L-35 -29.5 Z", c["eye"], {"lw": 1})
    T("M-14 -54 C -24 -40, -26 -22, -22 -8", 16, mix(c["gristle"], c["gristleDeep"], 0.45454545454545453))
    E(-23, -6, 11, 7, mix(c["gristle"], c["gristleDeep"], 0.45454545454545453))
    T("M-29 -7 L-29 -3", 1.4, "INK")
    T("M-24 -8 L-24 -3", 1.4, "INK")
    T("M-19 -7 L-19 -3", 1.4, "INK")


def gristle_7():
    """The gristle of tier 7 (its spikes, lumps and scars grow with the tier)."""
    P("M-16.85 -63.66 L-18.93 -81.32 L-6.86 -68.27 Z", c["bone"])
    P("M-7.16 -68.17 L-3.15 -85.49 L3.8 -69.12 Z", c["bone"])
    P("M4.46 -69 L14.59 -83.61 L15 -65.84 Z", c["bone"])
    P("M20.61 -62.6 L36 -71.5 L28.79 -55.25 Z", c["bone"])
    P("M32.61 -50.05 L50.28 -51.96 L37.11 -40.02 Z", c["bone"])
    C(13.15, -68.89, 6.64, mix(c["gristle"], c["gristleDeep"], 0.5454545454545454))
    C(25.3, -61.71, 6.64, mix(c["gristle"], c["gristleDeep"], 0.5454545454545454))
    T("M-4 -52 C -4 -36, 0 -22, 0 -8", 13, mix(mix(c["gristle"], c["gristleDeep"], 0.5454545454545454), INK, 0.22))
    E(0, -6, 9, 6, mix(mix(c["gristle"], c["gristleDeep"], 0.5454545454545454), INK, 0.22))
    E(24, -12, 9, 7, mix(mix(c["gristle"], c["gristleDeep"], 0.5454545454545454), INK, 0.22))
    E(26, -3, 9, 4, mix(mix(c["gristle"], c["gristleDeep"], 0.5454545454545454), INK, 0.22))
    P("M-30 -42 C -32 -64, -10 -76, 8 -72 C 26 -68, 40 -52, 42 -34 C 44 -20, 36 -12, 26 -12 C 16 -12, 8 -20, -2 -22 C -16 -24, -28 -30, -30 -42 Z", mix(c["gristle"], c["gristleDeep"], 0.5454545454545454))
    E(-14, -34, 13, 11, c["gristleBelly"], NO)
    T("M2 -62 L14 -52", 1.8, "INK")
    T("M5 -55 L10 -59", 1.4, "INK")
    T("M22 -46 L32 -38", 1.8, "INK")
    T("M25 -40 L29 -44", 1.4, "INK")
    E(32, -18, 11, 10, mix(c["gristle"], c["gristleDeep"], 0.5454545454545454))
    E(36, -4, 11, 5, mix(c["gristle"], c["gristleDeep"], 0.5454545454545454))
    E(-38, -40, 12, 11, mix(c["gristle"], c["gristleDeep"], 0.5454545454545454))
    with at(-38, -51, 1, -20):
        R(-7, -2.5, 14, 5, 2, c["plaster"], {"lw": 1.5})
    eye(-44, -42, 4.8, -0.6, -0.5)
    eye(-35, -43, 4.6, 0.6, 0.5)
    E(-41, -33, 2, 2.6, "INK")
    P("M-47 -30 L-46 -35 L-44 -30 Z", c["eye"], {"lw": 1})
    P("M-38 -29.5 L-37 -34.5 L-35 -29.5 Z", c["eye"], {"lw": 1})
    T("M-14 -54 C -24 -40, -26 -22, -22 -8", 16, mix(c["gristle"], c["gristleDeep"], 0.5454545454545454))
    E(-23, -6, 11, 7, mix(c["gristle"], c["gristleDeep"], 0.5454545454545454))
    T("M-29 -7 L-29 -3", 1.4, "INK")
    T("M-24 -8 L-24 -3", 1.4, "INK")
    T("M-19 -7 L-19 -3", 1.4, "INK")


def gristle_8():
    """The gristle of tier 8 (its spikes, lumps and scars grow with the tier)."""
    P("M-16.85 -63.66 L-19.27 -82.06 L-6.86 -68.27 Z", c["bone"])
    P("M-7.16 -68.17 L-3.22 -86.31 L3.8 -69.12 Z", c["bone"])
    P("M4.46 -69 L14.83 -84.4 L15 -65.84 Z", c["bone"])
    P("M20.61 -62.6 L36.54 -72.11 L28.79 -55.25 Z", c["bone"])
    P("M32.61 -50.05 L51.03 -52.3 L37.11 -40.02 Z", c["bone"])
    C(13.15, -68.89, 6.91, mix(c["gristle"], c["gristleDeep"], 0.6363636363636364))
    C(25.3, -61.71, 6.91, mix(c["gristle"], c["gristleDeep"], 0.6363636363636364))
    T("M-4 -52 C -4 -36, 0 -22, 0 -8", 13, mix(mix(c["gristle"], c["gristleDeep"], 0.6363636363636364), INK, 0.22))
    E(0, -6, 9, 6, mix(mix(c["gristle"], c["gristleDeep"], 0.6363636363636364), INK, 0.22))
    E(24, -12, 9, 7, mix(mix(c["gristle"], c["gristleDeep"], 0.6363636363636364), INK, 0.22))
    E(26, -3, 9, 4, mix(mix(c["gristle"], c["gristleDeep"], 0.6363636363636364), INK, 0.22))
    P("M-30 -42 C -32 -64, -10 -76, 8 -72 C 26 -68, 40 -52, 42 -34 C 44 -20, 36 -12, 26 -12 C 16 -12, 8 -20, -2 -22 C -16 -24, -28 -30, -30 -42 Z", mix(c["gristle"], c["gristleDeep"], 0.6363636363636364))
    E(-14, -34, 13, 11, c["gristleBelly"], NO)
    T("M2 -62 L14 -52", 1.8, "INK")
    T("M5 -55 L10 -59", 1.4, "INK")
    T("M22 -46 L32 -38", 1.8, "INK")
    T("M25 -40 L29 -44", 1.4, "INK")
    E(32, -18, 11, 10, mix(c["gristle"], c["gristleDeep"], 0.6363636363636364))
    E(36, -4, 11, 5, mix(c["gristle"], c["gristleDeep"], 0.6363636363636364))
    E(-38, -40, 12, 11, mix(c["gristle"], c["gristleDeep"], 0.6363636363636364))
    with at(-38, -51, 1, -20):
        R(-7, -2.5, 14, 5, 2, c["plaster"], {"lw": 1.5})
    eye(-44, -42, 4.8, -0.6, -0.5)
    eye(-35, -43, 4.6, 0.6, 0.5)
    E(-41, -33, 2, 2.6, "INK")
    P("M-47 -30 L-46 -35 L-44 -30 Z", c["eye"], {"lw": 1})
    P("M-38 -29.5 L-37 -34.5 L-35 -29.5 Z", c["eye"], {"lw": 1})
    T("M-14 -54 C -24 -40, -26 -22, -22 -8", 16, mix(c["gristle"], c["gristleDeep"], 0.6363636363636364))
    E(-23, -6, 11, 7, mix(c["gristle"], c["gristleDeep"], 0.6363636363636364))
    T("M-29 -7 L-29 -3", 1.4, "INK")
    T("M-24 -8 L-24 -3", 1.4, "INK")
    T("M-19 -7 L-19 -3", 1.4, "INK")


def gristle_9():
    """The gristle of tier 9 (its spikes, lumps and scars grow with the tier)."""
    P("M-16.85 -63.66 L-19.61 -82.81 L-6.86 -68.27 Z", c["bone"])
    P("M-9.22 -67.54 L-6.66 -86.71 L1.65 -69.24 Z", c["bone"])
    P("M-0.84 -69.22 L6.95 -86.93 L10.07 -67.84 Z", c["bone"])
    P("M11.27 -67.33 L24.63 -81.32 L21.05 -62.31 Z", c["bone"])
    P("M23.41 -60.5 L40.68 -69.22 L30.95 -52.5 Z", c["bone"])
    P("M32.61 -50.05 L51.78 -52.63 L37.11 -40.02 Z", c["bone"])
    C(13.15, -68.89, 7.18, mix(c["gristle"], c["gristleDeep"], 0.7272727272727273))
    C(25.3, -61.71, 7.18, mix(c["gristle"], c["gristleDeep"], 0.7272727272727273))
    T("M-4 -52 C -4 -36, 0 -22, 0 -8", 13, mix(mix(c["gristle"], c["gristleDeep"], 0.7272727272727273), INK, 0.22))
    E(0, -6, 9, 6, mix(mix(c["gristle"], c["gristleDeep"], 0.7272727272727273), INK, 0.22))
    E(24, -12, 9, 7, mix(mix(c["gristle"], c["gristleDeep"], 0.7272727272727273), INK, 0.22))
    E(26, -3, 9, 4, mix(mix(c["gristle"], c["gristleDeep"], 0.7272727272727273), INK, 0.22))
    P("M-30 -42 C -32 -64, -10 -76, 8 -72 C 26 -68, 40 -52, 42 -34 C 44 -20, 36 -12, 26 -12 C 16 -12, 8 -20, -2 -22 C -16 -24, -28 -30, -30 -42 Z", mix(c["gristle"], c["gristleDeep"], 0.7272727272727273))
    E(-14, -34, 13, 11, c["gristleBelly"], NO)
    T("M2 -62 L14 -52", 1.8, "INK")
    T("M5 -55 L10 -59", 1.4, "INK")
    T("M22 -46 L32 -38", 1.8, "INK")
    T("M25 -40 L29 -44", 1.4, "INK")
    T("M10 -40 L16 -30", 1.8, "INK")
    T("M11 -34 L15 -36", 1.4, "INK")
    E(32, -18, 11, 10, mix(c["gristle"], c["gristleDeep"], 0.7272727272727273))
    E(36, -4, 11, 5, mix(c["gristle"], c["gristleDeep"], 0.7272727272727273))
    E(-38, -40, 12, 11, mix(c["gristle"], c["gristleDeep"], 0.7272727272727273))
    with at(-38, -51, 1, -20):
        R(-7, -2.5, 14, 5, 2, c["plaster"], {"lw": 1.5})
    eye(-44, -42, 4.8, -0.6, -0.5)
    eye(-35, -43, 4.6, 0.6, 0.5)
    E(-41, -33, 2, 2.6, "INK")
    P("M-47 -30 L-46 -35 L-44 -30 Z", c["eye"], {"lw": 1})
    P("M-38 -29.5 L-37 -34.5 L-35 -29.5 Z", c["eye"], {"lw": 1})
    T("M-14 -54 C -24 -40, -26 -22, -22 -8", 16, mix(c["gristle"], c["gristleDeep"], 0.7272727272727273))
    E(-23, -6, 11, 7, mix(c["gristle"], c["gristleDeep"], 0.7272727272727273))
    T("M-29 -7 L-29 -3", 1.4, "INK")
    T("M-24 -8 L-24 -3", 1.4, "INK")
    T("M-19 -7 L-19 -3", 1.4, "INK")


def gristle_10():
    """The gristle of tier 10 (its spikes, lumps and scars grow with the tier)."""
    P("M-16.85 -63.66 L-19.96 -83.55 L-6.86 -68.27 Z", c["bone"])
    P("M-9.22 -67.54 L-6.78 -87.52 L1.65 -69.24 Z", c["bone"])
    P("M-0.84 -69.22 L7.05 -87.74 L10.07 -67.84 Z", c["bone"])
    P("M11.27 -67.33 L25 -82.05 L21.05 -62.31 Z", c["bone"])
    P("M23.41 -60.5 L41.27 -69.78 L30.95 -52.5 Z", c["bone"])
    P("M32.61 -50.05 L52.52 -52.97 L37.11 -40.02 Z", c["bone"])
    C(13.15, -68.89, 7.45, mix(c["gristle"], c["gristleDeep"], 0.8181818181818182))
    C(25.3, -61.71, 7.45, mix(c["gristle"], c["gristleDeep"], 0.8181818181818182))
    T("M-4 -52 C -4 -36, 0 -22, 0 -8", 13, mix(mix(c["gristle"], c["gristleDeep"], 0.8181818181818182), INK, 0.22))
    E(0, -6, 9, 6, mix(mix(c["gristle"], c["gristleDeep"], 0.8181818181818182), INK, 0.22))
    E(24, -12, 9, 7, mix(mix(c["gristle"], c["gristleDeep"], 0.8181818181818182), INK, 0.22))
    E(26, -3, 9, 4, mix(mix(c["gristle"], c["gristleDeep"], 0.8181818181818182), INK, 0.22))
    P("M-30 -42 C -32 -64, -10 -76, 8 -72 C 26 -68, 40 -52, 42 -34 C 44 -20, 36 -12, 26 -12 C 16 -12, 8 -20, -2 -22 C -16 -24, -28 -30, -30 -42 Z", mix(c["gristle"], c["gristleDeep"], 0.8181818181818182))
    E(-14, -34, 13, 11, c["gristleBelly"], NO)
    T("M2 -62 L14 -52", 1.8, "INK")
    T("M5 -55 L10 -59", 1.4, "INK")
    T("M22 -46 L32 -38", 1.8, "INK")
    T("M25 -40 L29 -44", 1.4, "INK")
    T("M10 -40 L16 -30", 1.8, "INK")
    T("M11 -34 L15 -36", 1.4, "INK")
    E(32, -18, 11, 10, mix(c["gristle"], c["gristleDeep"], 0.8181818181818182))
    E(36, -4, 11, 5, mix(c["gristle"], c["gristleDeep"], 0.8181818181818182))
    E(-38, -40, 12, 11, mix(c["gristle"], c["gristleDeep"], 0.8181818181818182))
    with at(-38, -51, 1, -20):
        R(-7, -2.5, 14, 5, 2, c["plaster"], {"lw": 1.5})
    eye(-44, -42, 4.8, -0.6, -0.5)
    eye(-35, -43, 4.6, 0.6, 0.5)
    E(-41, -33, 2, 2.6, "INK")
    P("M-47 -30 L-46 -35 L-44 -30 Z", c["eye"], {"lw": 1})
    P("M-38 -29.5 L-37 -34.5 L-35 -29.5 Z", c["eye"], {"lw": 1})
    T("M-14 -54 C -24 -40, -26 -22, -22 -8", 16, mix(c["gristle"], c["gristleDeep"], 0.8181818181818182))
    E(-23, -6, 11, 7, mix(c["gristle"], c["gristleDeep"], 0.8181818181818182))
    T("M-29 -7 L-29 -3", 1.4, "INK")
    T("M-24 -8 L-24 -3", 1.4, "INK")
    T("M-19 -7 L-19 -3", 1.4, "INK")


def gristle_11():
    """The gristle of tier 11 (its spikes, lumps and scars grow with the tier)."""
    P("M-16.85 -63.66 L-20.3 -84.29 L-6.86 -68.27 Z", c["bone"])
    P("M-10.57 -67.04 L-9.21 -87.92 L0.21 -69.24 Z", c["bone"])
    P("M-3.66 -68.91 L2.43 -88.92 L7.33 -68.59 Z", c["bone"])
    P("M4.46 -69 L15.53 -86.75 L15 -65.84 Z", c["bone"])
    P("M15.57 -65.54 L31.38 -79.24 L24.71 -59.41 Z", c["bone"])
    P("M25.18 -58.97 L44.15 -67.78 L32.27 -50.57 Z", c["bone"])
    P("M32.61 -50.05 L53.27 -53.3 L37.11 -40.02 Z", c["bone"])
    C(13.15, -68.89, 7.73, mix(c["gristle"], c["gristleDeep"], 0.9090909090909091))
    C(25.3, -61.71, 7.73, mix(c["gristle"], c["gristleDeep"], 0.9090909090909091))
    C(34.5, -51.04, 7.73, mix(c["gristle"], c["gristleDeep"], 0.9090909090909091))
    T("M-4 -52 C -4 -36, 0 -22, 0 -8", 13, mix(mix(c["gristle"], c["gristleDeep"], 0.9090909090909091), INK, 0.22))
    E(0, -6, 9, 6, mix(mix(c["gristle"], c["gristleDeep"], 0.9090909090909091), INK, 0.22))
    E(24, -12, 9, 7, mix(mix(c["gristle"], c["gristleDeep"], 0.9090909090909091), INK, 0.22))
    E(26, -3, 9, 4, mix(mix(c["gristle"], c["gristleDeep"], 0.9090909090909091), INK, 0.22))
    P("M-30 -42 C -32 -64, -10 -76, 8 -72 C 26 -68, 40 -52, 42 -34 C 44 -20, 36 -12, 26 -12 C 16 -12, 8 -20, -2 -22 C -16 -24, -28 -30, -30 -42 Z", mix(c["gristle"], c["gristleDeep"], 0.9090909090909091))
    E(-14, -34, 13, 11, c["gristleBelly"], NO)
    T("M2 -62 L14 -52", 1.8, "INK")
    T("M5 -55 L10 -59", 1.4, "INK")
    T("M22 -46 L32 -38", 1.8, "INK")
    T("M25 -40 L29 -44", 1.4, "INK")
    T("M10 -40 L16 -30", 1.8, "INK")
    T("M11 -34 L15 -36", 1.4, "INK")
    E(32, -18, 11, 10, mix(c["gristle"], c["gristleDeep"], 0.9090909090909091))
    E(36, -4, 11, 5, mix(c["gristle"], c["gristleDeep"], 0.9090909090909091))
    E(-38, -40, 12, 11, mix(c["gristle"], c["gristleDeep"], 0.9090909090909091))
    with at(-38, -51, 1, -20):
        R(-7, -2.5, 14, 5, 2, c["plaster"], {"lw": 1.5})
    eye(-44, -42, 4.8, -0.6, -0.5)
    eye(-35, -43, 4.6, 0.6, 0.5)
    E(-41, -33, 2, 2.6, "INK")
    P("M-47 -30 L-46 -35 L-44 -30 Z", c["eye"], {"lw": 1})
    P("M-38 -29.5 L-37 -34.5 L-35 -29.5 Z", c["eye"], {"lw": 1})
    T("M-14 -54 C -24 -40, -26 -22, -22 -8", 16, mix(c["gristle"], c["gristleDeep"], 0.9090909090909091))
    E(-23, -6, 11, 7, mix(c["gristle"], c["gristleDeep"], 0.9090909090909091))
    T("M-29 -7 L-29 -3", 1.4, "INK")
    T("M-24 -8 L-24 -3", 1.4, "INK")
    T("M-19 -7 L-19 -3", 1.4, "INK")


def gristle_12():
    """The gristle of tier 12 (its spikes, lumps and scars grow with the tier)."""
    P("M-16.85 -63.66 L-20.64 -85.04 L-6.86 -68.27 Z", c["bone"])
    P("M-10.57 -67.04 L-9.37 -88.72 L0.21 -69.24 Z", c["bone"])
    P("M-3.66 -68.91 L2.46 -89.74 L7.33 -68.59 Z", c["bone"])
    P("M4.46 -69 L15.77 -87.53 L15 -65.84 Z", c["bone"])
    P("M15.57 -65.54 L31.83 -79.92 L24.71 -59.41 Z", c["bone"])
    P("M25.18 -58.97 L44.77 -68.31 L32.27 -50.57 Z", c["bone"])
    P("M32.61 -50.05 L54.02 -53.64 L37.11 -40.02 Z", c["bone"])
    C(13.15, -68.89, 8, mix(c["gristle"], c["gristleDeep"], 1))
    C(25.3, -61.71, 8, mix(c["gristle"], c["gristleDeep"], 1))
    C(34.5, -51.04, 8, mix(c["gristle"], c["gristleDeep"], 1))
    T("M-4 -52 C -4 -36, 0 -22, 0 -8", 13, mix(mix(c["gristle"], c["gristleDeep"], 1), INK, 0.22))
    E(0, -6, 9, 6, mix(mix(c["gristle"], c["gristleDeep"], 1), INK, 0.22))
    E(24, -12, 9, 7, mix(mix(c["gristle"], c["gristleDeep"], 1), INK, 0.22))
    E(26, -3, 9, 4, mix(mix(c["gristle"], c["gristleDeep"], 1), INK, 0.22))
    P("M-30 -42 C -32 -64, -10 -76, 8 -72 C 26 -68, 40 -52, 42 -34 C 44 -20, 36 -12, 26 -12 C 16 -12, 8 -20, -2 -22 C -16 -24, -28 -30, -30 -42 Z", mix(c["gristle"], c["gristleDeep"], 1))
    E(-14, -34, 13, 11, c["gristleBelly"], NO)
    T("M2 -62 L14 -52", 1.8, "INK")
    T("M5 -55 L10 -59", 1.4, "INK")
    T("M22 -46 L32 -38", 1.8, "INK")
    T("M25 -40 L29 -44", 1.4, "INK")
    T("M10 -40 L16 -30", 1.8, "INK")
    T("M11 -34 L15 -36", 1.4, "INK")
    E(32, -18, 11, 10, mix(c["gristle"], c["gristleDeep"], 1))
    E(36, -4, 11, 5, mix(c["gristle"], c["gristleDeep"], 1))
    E(-38, -40, 12, 11, mix(c["gristle"], c["gristleDeep"], 1))
    with at(-38, -51, 1, -20):
        R(-7, -2.5, 14, 5, 2, c["plaster"], {"lw": 1.5})
    eye(-44, -42, 4.8, -0.6, -0.5)
    eye(-35, -43, 4.6, 0.6, 0.5)
    E(-41, -33, 2, 2.6, "INK")
    P("M-47 -30 L-46 -35 L-44 -30 Z", c["eye"], {"lw": 1})
    P("M-38 -29.5 L-37 -34.5 L-35 -29.5 Z", c["eye"], {"lw": 1})
    T("M-14 -54 C -24 -40, -26 -22, -22 -8", 16, mix(c["gristle"], c["gristleDeep"], 1))
    E(-23, -6, 11, 7, mix(c["gristle"], c["gristleDeep"], 1))
    T("M-29 -7 L-29 -3", 1.4, "INK")
    T("M-24 -8 L-24 -3", 1.4, "INK")
    T("M-19 -7 L-19 -3", 1.4, "INK")


def park_tyrant():
    """Boss: The Park Tyrant."""
    with at(-26, 6, 0.85, -14):
        P("M14 -62 C 4 -108, 50 -132, 86 -104 C 76 -100, 80 -92, 70 -88 C 76 -80, 68 -74, 60 -74 C 62 -66, 50 -60, 40 -60 Z", mix(mix(c["tyrant"], INK, 0.18), INK, 0.1))
    P("M14 -62 C 4 -108, 50 -132, 86 -104 C 76 -100, 80 -92, 70 -88 C 76 -80, 68 -74, 60 -74 C 62 -66, 50 -60, 40 -60 Z", mix(c["tyrant"], INK, 0.18))
    T("M30 -76 L62 -98", 2, mix(mix(c["tyrant"], INK, 0.18), INK, 0.4), NO)
    T("M34 -68 L66 -84", 2, mix(mix(c["tyrant"], INK, 0.18), INK, 0.4), NO)
    T("M-6 -22 L-14 -4", 5, c["gooseBeak"])
    T("M30 -22 L40 -4", 5, c["gooseBeak"])
    P("M-28 -1 L-14 -8 L-4 -1 Z", c["gooseBeak"])
    P("M30 -1 L40 -8 L52 -1 Z", c["gooseBeak"])
    E(14, -42, 42, 28, c["tyrant"])
    E(22, -44, 24, 14, mix(c["tyrant"], INK, 0.18))
    T("M-18 -50 C -36 -56, -52 -46, -64 -42", 18, c["tyrant"])
    E(-72, -44, 17, 14, c["tyrant"])
    P("M-84 -52 L-114 -48 L-86 -42 Z", c["gooseBeak"])
    P("M-84 -38 L-108 -30 L-82 -31 Z", c["gooseBeak"])
    E(-96, -38.5, 8, 3, c["pink"], NO)
    bossEyes(-80, -51, -67, -52, 6)


def rival_champion():
    """Boss: The Rival Champion."""
    T("M34 -16 C 56 -18, 60 -44, 47 -58", 11, c["champ"])
    E(10, -27, 34, 27, c["champ"])
    T("M29 -50 C 34 -46, 36 -40, 33 -35", 5, c["champStripe"], NO)
    T("M40 -42 C 44 -38, 45 -33, 43 -29", 5, c["champStripe"], NO)
    E(-6, -19, 16, 14, c["champBelly"], NO)
    E(-19, -4.5, 9.5, 6, c["champ"])
    E(1, -4, 9.5, 6, c["champ"])
    P("M-42 -63 L-47 -96 L-37 -85 L-31 -94 L-21 -77 Z", c["champ"])
    P("M-9 -77 L-4 -87 L0 -84 L4 -97 L9 -68 Z", c["champ"])
    P("M-37.5 -70 L-37 -87 L-27 -77 Z", c["pink"], NO)
    P("M-2 -77 L3 -89 L6.5 -71 Z", c["pink"], NO)
    T("M-34 -40 L-20 -24", 4.5, c["robe"])
    T("M-4 -40 L-16 -24", 4.5, c["robe"])
    C(-18, -21, 7.5, c["halo"])
    P("M-18 -25.2 Q-17.16 -21.84 -13.8 -21 Q-17.16 -20.16 -18 -16.8 Q-18.84 -20.16 -22.2 -21 Q-18.84 -21.84 -18 -25.2 Z", c["eye"], NO)
    E(-17, -58, 29, 23, c["champ"])
    T("M-23 -79 L-22.5 -73", 4, c["champStripe"], NO)
    T("M-15 -80 L-15 -73.5", 4, c["champStripe"], NO)
    T("M-7 -78.5 L-7.5 -72.5", 4, c["champStripe"], NO)
    E(-24, -47, 13.5, 8.5, c["champBelly"], NO)
    C(-33, -60, 9.5, c["eye"], {"lw": EYE_LW})
    C(-12, -61, 10, c["eye"], {"lw": EYE_LW})
    C(-29.5, -59, 2.9, "INK")
    C(-15.5, -59.5, 2.9, "INK")
    T("M-44 -74 L-28 -68.5", 4.4, "INK")
    T("M-18 -69 L-1 -75", 4.4, "INK")
    E(-17, -40, 3.6, 4.4, c["pink"], {"lw": EYE_LW * 0.6})
    P("M-28 -52.5 L-20 -52.5 L-24 -47.5 Z", c["pink"], {"lw": EYE_LW * 0.7})
    T("M-36 -45 Q -24 -36 -11 -46", 2.7, "INK")
    P("M-30.5 -42.3 L-28.6 -37.6 L-26.8 -41.4 Z", c["eye"], {"lw": 1.5})
    T("M-40 -50 L-55 -53", 1.7, "INK")
    T("M-40 -46.5 L-54 -44.5", 1.7, "INK")


def void_god():
    """Boss: The Void God."""
    T("M-30 -40 C -40 -20, -30 -6, -42 6", 9, mix(c["voidBody"], c["eye"], 0.1))
    T("M-12 -36 C -16 -16, -4 -4, -10 12", 10, mix(c["voidBody"], c["eye"], 0.1))
    T("M8 -36 C 10 -14, 22 -6, 18 10", 10, mix(c["voidBody"], c["eye"], 0.1))
    T("M26 -40 C 36 -22, 30 -8, 40 4", 9, mix(c["voidBody"], c["eye"], 0.1))
    T("M40 -50 C 54 -40, 56 -24, 62 -18", 8, mix(c["voidBody"], c["eye"], 0.1))
    with at(-36, -85.2, 1, -22):
        P("M-6 6 L0 -24 L6 6 Z", c["voidHorn"])
    with at(-20, -90, 1, -9):
        P("M-6 6 L0 -34 L6 6 Z", c["voidHorn"])
    with at(-2, -95.4, 1, 0):
        P("M-6 6 L0 -44 L6 6 Z", c["voidHorn"])
    with at(16, -91.2, 1, 10):
        P("M-6 6 L0 -30 L6 6 Z", c["voidHorn"])
    with at(32, -86.4, 1, 22):
        P("M-6 6 L0 -22 L6 6 Z", c["voidHorn"])
    with at(44, -82.8, 1, 32):
        P("M-6 6 L0 -16 L6 6 Z", c["voidHorn"])
    P("M-46 -60 C -58 -92, -20 -110, 6 -104 C 34 -110, 62 -88, 56 -60 C 66 -40, 44 -24, 20 -30 C 6 -22, -18 -26, -28 -34 C -48 -30, -56 -44, -46 -60 Z", c["voidBody"])
    P("M-30 -85.4 Q-29.32 -82.68 -26.6 -82 Q-29.32 -81.32 -30 -78.6 Q-30.68 -81.32 -33.4 -82 Q-30.68 -82.68 -30 -85.4 Z", c["voidStar"], NO)
    P("M40 -86.8 Q40.56 -84.56 42.8 -84 Q40.56 -83.44 40 -81.2 Q39.44 -83.44 37.2 -84 Q39.44 -84.56 40 -86.8 Z", c["voidStar"], NO)
    P("M32 -49.2 Q32.64 -46.64 35.2 -46 Q32.64 -45.36 32 -42.8 Q31.36 -45.36 28.8 -46 Q31.36 -46.64 32 -49.2 Z", c["voidStar"], NO)
    P("M-38 -50.6 Q-37.48 -48.52 -35.4 -48 Q-37.48 -47.48 -38 -45.4 Q-38.52 -47.48 -40.6 -48 Q-38.52 -48.52 -38 -50.6 Z", c["voidStar"], NO)
    P("M10 -99.4 Q10.48 -97.48 12.4 -97 Q10.48 -96.52 10 -94.6 Q9.52 -96.52 7.6 -97 Q9.52 -97.48 10 -99.4 Z", c["voidStar"], NO)
    P("M-14 -42.8 Q-13.44 -40.56 -11.2 -40 Q-13.44 -39.44 -14 -37.2 Q-14.56 -39.44 -16.8 -40 Q-14.56 -40.56 -14 -42.8 Z", c["voidStar"], NO)
    P("M46 -68.2 Q46.44 -66.44 48.2 -66 Q46.44 -65.56 46 -63.8 Q45.56 -65.56 43.8 -66 Q45.56 -66.44 46 -68.2 Z", c["voidStar"], NO)
    C(2, -72, 20, c["eye"], {"lw": EYE_LW})
    C(-4, -70, 10.5, c["voidIris"], NO)
    C(-5, -70, 5, "INK")
    C(-1, -74.5, 2.2, c["eye"], NO)
    eye(-30, -64, 5, 0.6, -0.5)
    eye(34, -72, 6, -0.7, 0.4)
    eye(-18, -92, 4, 0.4, 0.6)
    eye(28, -94, 4.4, -0.5, -0.6)
    eye(46, -54, 3.6, 0.6, 0.6)
    P("M-16 -42 C -6 -32, 14 -32, 22 -44 C 14 -37, -4 -37, -16 -42 Z", c["voidMouth"], {"lw": EYE_LW * 0.8})
    P("M-10 -40.6 L-8 -37 L-6 -39.6 L-3 -36.4 L0 -39.2 L3 -36.4 L6 -39.4 L9 -37 L12 -40.4 Z", c["eye"], {"lw": 1})


def foreman():
    """Boss: The Foreman."""
    T("M30 -20 C 50 -18, 58 -30, 56 -44", 12, c["gristle"])
    R(46, -30, 10, 8, 2, c["metalDark"])
    R(-22, -24, 18, 24, 8, mix(c["gristle"], INK, 0.2))
    R(10, -24, 18, 24, 8, mix(c["gristle"], INK, 0.2))
    E(-14, -3, 12, 5, mix(c["gristle"], INK, 0.2))
    E(19, -3, 12, 5, mix(c["gristle"], INK, 0.2))
    E(0, -56, 38, 40, c["gristle"])
    E(2, -44, 22, 22, c["gristleBelly"], NO)
    R(-10, -64, 30, 18, 4, c["metal"])
    C(-6, -60, 1.8, c["metalDark"])
    C(16, -60, 1.8, c["metalDark"])
    C(-6, -50, 1.8, c["metalDark"])
    C(16, -50, 1.8, c["metalDark"])
    T("M8 -78 L24 -66", 2, "INK")
    T("M12 -72 L16 -76", 1.5, "INK")
    T("M17 -68 L21 -72", 1.5, "INK")
    with at(22, -82, 1, 35):
        R(-4.5, -58, 9, 66, 4.5, c["metalDark"])
        P("M-12 -60 C -12 -54, 12 -54, 12 -60 L12 -70 C 12 -76, 8 -80, 4 -80 L4 -70 L-4 -70 L-4 -80 C -8 -80, -12 -76, -12 -70 Z", c["metal"])
    E(22, -80, 11, 10, c["gristle"])
    E(-10, -92, 22, 18, c["gristle"])
    P("M-34 -100 C -34 -126, 14 -126, 14 -100 Z", c["hazard"])
    T("M-24 -104 L-18 -116", 3.4, "INK")
    T("M-12 -104 L-6 -118", 3.4, "INK")
    T("M0 -104 L5 -113", 3.4, "INK")
    R(-38, -103, 56, 6, 3, c["hazard"])
    C(-30, -109, 5, c["voidStar"])
    bossEyes(-20, -91, -6, -92, 6.5)
    T("M-24 -79 Q -12 -73 0 -79", 2.6, "INK")
    P("M-20 -78 L-18 -73 L-16 -77.4 Z", c["eye"], {"lw": 1})
    P("M-6 -77.6 L-4 -73 L-2 -78.4 Z", c["eye"], {"lw": 1})


def litter_king():
    """Boss: The Litter King."""
    P("M56 -66 A12 12 0 0 1 51.74 -44.57 A12 12 0 0 1 39.6 -26.4 A12 12 0 0 1 21.43 -14.26 A12 12 0 0 1 0 -10 A12 12 0 0 1 -21.43 -14.26 A12 12 0 0 1 -39.6 -26.4 A12 12 0 0 1 -51.74 -44.57 A12 12 0 0 1 -56 -66 A12 12 0 0 1 -51.74 -87.43 A12 12 0 0 1 -39.6 -105.6 A12 12 0 0 1 -21.43 -117.74 A12 12 0 0 1 0 -122 A12 12 0 0 1 21.43 -117.74 A12 12 0 0 1 39.6 -105.6 A12 12 0 0 1 51.74 -87.43 A12 12 0 0 1 56 -66 Z", c["clump"])
    P("M-32 -10 A5 5 0 0 1 -35.51 -1.51 A5 5 0 0 1 -44 2 A5 5 0 0 1 -52.49 -1.51 A5 5 0 0 1 -56 -10 A5 5 0 0 1 -52.49 -18.49 A5 5 0 0 1 -44 -22 A5 5 0 0 1 -35.51 -18.49 A5 5 0 0 1 -32 -10 Z", c["clump"])
    P("M-10 -6 A5 5 0 0 1 -13.51 2.49 A5 5 0 0 1 -22 6 A5 5 0 0 1 -30.49 2.49 A5 5 0 0 1 -34 -6 A5 5 0 0 1 -30.49 -14.49 A5 5 0 0 1 -22 -18 A5 5 0 0 1 -13.51 -14.49 A5 5 0 0 1 -10 -6 Z", c["clump"])
    P("M34 -6 A5 5 0 0 1 30.49 2.49 A5 5 0 0 1 22 6 A5 5 0 0 1 13.51 2.49 A5 5 0 0 1 10 -6 A5 5 0 0 1 13.51 -14.49 A5 5 0 0 1 22 -18 A5 5 0 0 1 30.49 -14.49 A5 5 0 0 1 34 -6 Z", c["clump"])
    P("M56 -10 A5 5 0 0 1 52.49 -1.51 A5 5 0 0 1 44 2 A5 5 0 0 1 35.51 -1.51 A5 5 0 0 1 32 -10 A5 5 0 0 1 35.51 -18.49 A5 5 0 0 1 44 -22 A5 5 0 0 1 52.49 -18.49 A5 5 0 0 1 56 -10 Z", c["clump"])
    P("M-40 -60 C -56 -30, -52 -6, -40 -2 L40 -2 C 52 -6, 56 -30, 40 -60 Z", c["robe"])
    E(-18, -4, 12, 6, c["kingBody"])
    E(18, -4, 12, 6, c["kingBody"])
    E(0, -38, 36, 30, c["kingBody"])
    E(0, -30, 24, 17, mix(c["kingBody"], c["eye"], 0.45), NO)
    P("M-34 -62 C -20 -50, 20 -50, 34 -62 C 30 -70, -30 -70, -34 -62 Z", c["eye"])
    E(-22, -60, 1.6, 2.6, "INK")
    E(-8, -57, 1.6, 2.6, "INK")
    E(8, -57, 1.6, 2.6, "INK")
    E(22, -60, 1.6, 2.6, "INK")
    P("M-6 -50 L-2 -41 L2 -41 L6 -50 Z", c["robe"], {"lw": 1.5})
    C(0, -36, 6, c["crown"])
    P("M0 -39.4 Q0.68 -36.68 3.4 -36 Q0.68 -35.32 0 -32.6 Q-0.68 -35.32 -3.4 -36 Q-0.68 -36.68 0 -39.4 Z", c["eye"], NO)
    P("M-26 -92 L-25 -112 L-11 -101 Z", c["kingBody"])
    P("M26 -92 L25 -112 L11 -101 Z", c["kingBody"])
    E(0, -84, 28, 23, c["kingBody"])
    P("M-22 -102 L-27 -130 L-12 -112 L0 -134 L12 -112 L27 -130 L22 -102 Z", c["crown"])
    C(0, -115, 3.8, c["rainbow0"])
    C(-15, -109, 3, c["rainbow4"])
    C(15, -109, 3, c["rainbow3"])
    bossEyes(-10, -86, 10, -87, 8)
    T("M-10 -71 Q 0 -66 10 -71", 2.6, "INK")
    with at(46, -46, 1, 22):
        T("M0 8 L0 -42", 5, c["grip"])
        P("M-13 -42 L13 -42 L11 -66 L-11 -66 Z", c["rainbow4"])
        T("M-5 -48 L-4 -61", 2, "INK")
        T("M0 -48 L0 -61", 2, "INK")
        T("M5 -48 L4 -61", 2, "INK")
    E(42, -46, 7, 6, c["kingBody"])


def add():
    """A boss's add (the plus: it heals the boss)."""
    E(-8, -3, 6, 4, c["add"])
    E(8, -3, 6, 4, c["add"])
    E(0, -20, 19, 17, c["add"])
    E(0, -14, 11, 8, mix(c["add"], c["eye"], 0.5), NO)
    P("M12 -46 L17 -46 L17 -41 L22 -41 L22 -36 L17 -36 L17 -31 L12 -31 L12 -36 L7 -36 L7 -41 L12 -41 Z", c["plusRed"], {"lw": 1.6})
    eye(-6, -24, 5, -0.6, -0.6)
    eye(5, -25, 5.4, 0.6, 0.5)
    E(0, -15, 2, 2.8, "INK")


def cat_stage_1():
    """The cat, stage 1 (no weapon: its weapon is its own sprite)."""
    T("M34 -16 C 56 -18, 60 -44, 47 -58", 11, c["catFur"])
    E(10, -27, 34, 27, c["catFur"])
    T("M29 -50 C 34 -46, 36 -40, 33 -35", 5, c["catStripe"], NO)
    T("M40 -42 C 44 -38, 45 -33, 43 -29", 5, c["catStripe"], NO)
    E(-6, -19, 16, 14, c["catBelly"], NO)
    E(-19, -4.5, 9.5, 6, c["catFur"])
    E(1, -4, 9.5, 6, c["catFur"])
    P("M-42 -63 L-41 -95 L-21 -77 Z", c["catFur"])
    P("M-9 -77 L-4 -87 L0 -84 L4 -97 L9 -68 Z", c["catFur"])
    P("M-37.5 -70 L-37 -87 L-27 -77 Z", c["pink"], NO)
    P("M-2 -77 L3 -89 L6.5 -71 Z", c["pink"], NO)
    E(-17, -58, 29, 23, c["catFur"])
    T("M-23 -79 L-22.5 -73", 4, c["catStripe"], NO)
    T("M-15 -80 L-15 -73.5", 4, c["catStripe"], NO)
    T("M-7 -78.5 L-7.5 -72.5", 4, c["catStripe"], NO)
    E(-24, -47, 13.5, 8.5, c["catBelly"], NO)
    C(-33, -60, 9.5, c["eye"], {"lw": EYE_LW})
    C(-12, -61, 10, c["eye"], {"lw": EYE_LW})
    C(-29.5, -59, 2.9, "INK")
    C(-15.5, -59.5, 2.9, "INK")
    T("M-44 -74 L-28 -68.5", 4.4, "INK")
    T("M-18 -69 L-1 -75", 4.4, "INK")
    E(-17, -40, 3.6, 4.4, c["pink"], {"lw": EYE_LW * 0.6})
    P("M-28 -52.5 L-20 -52.5 L-24 -47.5 Z", c["pink"], {"lw": EYE_LW * 0.7})
    T("M-36 -45 Q -24 -36 -11 -46", 2.7, "INK")
    P("M-30.5 -42.3 L-28.6 -37.6 L-26.8 -41.4 Z", c["eye"], {"lw": 1.5})
    T("M-40 -50 L-55 -53", 1.7, "INK")
    T("M-40 -46.5 L-54 -44.5", 1.7, "INK")


def cat_stage_2():
    """The cat, stage 2 (no weapon: its weapon is its own sprite)."""
    T("M34 -16 C 56 -18, 60 -44, 47 -58", 11, c["catFur"])
    E(10, -27, 34, 27, c["catFur"])
    T("M29 -50 C 34 -46, 36 -40, 33 -35", 5, c["catStripe"], NO)
    T("M40 -42 C 44 -38, 45 -33, 43 -29", 5, c["catStripe"], NO)
    E(-6, -19, 16, 14, c["catBelly"], NO)
    E(-19, -4.5, 9.5, 6, c["catFur"])
    E(1, -4, 9.5, 6, c["catFur"])
    P("M-40 -80 L-45 -101 L-29 -90 Z", c["metal"])
    P("M-3 -90 L6 -104 L10 -84 Z", c["metal"])
    E(-17, -58, 29, 23, c["catFur"])
    E(-24, -47, 13.5, 8.5, c["catBelly"], NO)
    T("M-17 -92 C -14 -104, -3 -109, 6 -105", 7, c["plume"])
    P("M-49 -60 C -52 -100, 16 -100, 14 -62 C 4 -70, -6 -72, -17 -72 C -30 -72, -40 -68, -49 -60 Z", c["metal"])
    C(-40, -67, 1.9, "INK")
    C(6, -68, 1.9, "INK")
    C(-17, -75, 1.9, "INK")
    C(-33, -60, 9.5, c["eye"], {"lw": EYE_LW})
    C(-12, -61, 10, c["eye"], {"lw": EYE_LW})
    C(-29.5, -59, 2.9, "INK")
    C(-15.5, -59.5, 2.9, "INK")
    T("M-44 -74 L-28 -68.5", 4.4, "INK")
    T("M-18 -69 L-1 -75", 4.4, "INK")
    E(-17, -40, 3.6, 4.4, c["pink"], {"lw": EYE_LW * 0.6})
    P("M-28 -52.5 L-20 -52.5 L-24 -47.5 Z", c["pink"], {"lw": EYE_LW * 0.7})
    T("M-36 -45 Q -24 -36 -11 -46", 2.7, "INK")
    P("M-30.5 -42.3 L-28.6 -37.6 L-26.8 -41.4 Z", c["eye"], {"lw": 1.5})
    T("M-40 -50 L-55 -53", 1.7, "INK")
    T("M-40 -46.5 L-54 -44.5", 1.7, "INK")


def weapon_stage_2():
    """The weapon of stage 2."""
    with at(37, -24, 1, 32):
        P("M-5 -5 L-5 -40 L0 -48 L5 -40 L5 -5 Z", c["metal"])
        R(-3, 0, 6, 15, 3, c["grip"])
        R(-12, -6, 24, 6, 3, c["horn"])


def cat_stage_3():
    """The cat, stage 3 (no weapon: its weapon is its own sprite)."""
    T("M34 -16 C 56 -18, 60 -44, 47 -58", 11, c["catFur"])
    E(10, -27, 34, 27, c["catFur"])
    T("M29 -50 C 34 -46, 36 -40, 33 -35", 5, c["catStripe"], NO)
    T("M40 -42 C 44 -38, 45 -33, 43 -29", 5, c["catStripe"], NO)
    E(-6, -19, 16, 14, c["catBelly"], NO)
    E(-19, -4.5, 9.5, 6, c["catFur"])
    E(1, -4, 9.5, 6, c["catFur"])
    P("M-42 -63 L-41 -95 L-21 -77 Z", c["catFur"])
    P("M-9 -77 L-4 -87 L0 -84 L4 -97 L9 -68 Z", c["catFur"])
    P("M-37.5 -70 L-37 -87 L-27 -77 Z", c["pink"], NO)
    P("M-2 -77 L3 -89 L6.5 -71 Z", c["pink"], NO)
    E(-17, -58, 29, 23, c["catFur"])
    T("M-23 -79 L-22.5 -73", 4, c["catStripe"], NO)
    T("M-15 -80 L-15 -73.5", 4, c["catStripe"], NO)
    T("M-7 -78.5 L-7.5 -72.5", 4, c["catStripe"], NO)
    E(-24, -47, 13.5, 8.5, c["catBelly"], NO)
    C(-33, -60, 9.5, c["eye"], {"lw": EYE_LW})
    C(-12, -61, 10, c["eye"], {"lw": EYE_LW})
    C(-29.5, -59, 2.9, "INK")
    C(-15.5, -59.5, 2.9, "INK")
    GLASS(-47, -70, 50, 18, 9, 0.55, c["visor"])
    T("M-13 -80 L-7 -98", 2.6, "INK")
    C(-6.5, -100, 4.6, c["visor"])
    T("M-44 -74 L-28 -68.5", 4.4, "INK")
    T("M-18 -69 L-1 -75", 4.4, "INK")
    E(-17, -40, 3.6, 4.4, c["pink"], {"lw": EYE_LW * 0.6})
    P("M-28 -52.5 L-20 -52.5 L-24 -47.5 Z", c["pink"], {"lw": EYE_LW * 0.7})
    T("M-36 -45 Q -24 -36 -11 -46", 2.7, "INK")
    P("M-30.5 -42.3 L-28.6 -37.6 L-26.8 -41.4 Z", c["eye"], {"lw": 1.5})
    T("M-40 -50 L-55 -53", 1.7, "INK")
    T("M-40 -46.5 L-54 -44.5", 1.7, "INK")


def weapon_stage_3():
    """The weapon of stage 3."""
    with at(38, -26, 1, 35):
        R(-4, -38, 8, 24, 3, c["visor"])
        E(0, -10, 13, 9, c["metal"])
        R(-7, -42, 14, 6, 3, c["metalDark"])
        C(0, -10, 3, c["plume"])


def cat_stage_4():
    """The cat, stage 4, mounted (no lance: the lance is its own sprite)."""
    T("M48 -54 C 68 -54, 74 -32, 64 -14", 9, c["rainbow0"])
    T("M50 -48 C 64 -46, 66 -30, 58 -18", 6.5, c["rainbow2"])
    T("M50 -43 C 58 -40, 58 -30, 53 -24", 4.5, c["rainbow4"])
    R(-10, -34, 12, 34, 6, mix(c["uniWhite"], INK, 0.14))
    R(26, -34, 12, 34, 6, mix(c["uniWhite"], INK, 0.14))
    E(14, -45, 40, 23, c["uniWhite"])
    R(-22, -30, 13, 30, 6.5, c["uniWhite"])
    R(32, -30, 13, 30, 6.5, c["uniWhite"])
    R(-22, -7, 13, 7, 3, c["hoof"])
    R(32, -7, 13, 7, 3, c["hoof"])
    with at(22, -63, 0.6, 0):
        T("M34 -16 C 56 -18, 60 -44, 47 -58", 11, c["catFur"])
        E(10, -27, 34, 27, c["catFur"])
        T("M29 -50 C 34 -46, 36 -40, 33 -35", 5, c["catStripe"], NO)
        T("M40 -42 C 44 -38, 45 -33, 43 -29", 5, c["catStripe"], NO)
        E(-6, -19, 16, 14, c["catBelly"], NO)
        E(-19, -4.5, 9.5, 6, c["catFur"])
        E(1, -4, 9.5, 6, c["catFur"])
        P("M-42 -63 L-41 -95 L-21 -77 Z", c["catFur"])
        P("M-9 -77 L-4 -87 L0 -84 L4 -97 L9 -68 Z", c["catFur"])
        P("M-37.5 -70 L-37 -87 L-27 -77 Z", c["pink"], NO)
        P("M-2 -77 L3 -89 L6.5 -71 Z", c["pink"], NO)
        E(-17, -58, 29, 23, c["catFur"])
        T("M-23 -79 L-22.5 -73", 4, c["catStripe"], NO)
        T("M-15 -80 L-15 -73.5", 4, c["catStripe"], NO)
        T("M-7 -78.5 L-7.5 -72.5", 4, c["catStripe"], NO)
        E(-24, -47, 13.5, 8.5, c["catBelly"], NO)
        C(-33, -60, 9.5, c["eye"], {"lw": EYE_LW})
        C(-12, -61, 10, c["eye"], {"lw": EYE_LW})
        C(-29.5, -59, 2.9, "INK")
        C(-15.5, -59.5, 2.9, "INK")
        T("M-44 -74 L-28 -68.5", 4.4, "INK")
        T("M-18 -69 L-1 -75", 4.4, "INK")
        E(-17, -40, 3.6, 4.4, c["pink"], {"lw": EYE_LW * 0.6})
        P("M-28 -52.5 L-20 -52.5 L-24 -47.5 Z", c["pink"], {"lw": EYE_LW * 0.7})
        T("M-36 -45 Q -24 -36 -11 -46", 2.7, "INK")
        P("M-30.5 -42.3 L-28.6 -37.6 L-26.8 -41.4 Z", c["eye"], {"lw": 1.5})
        T("M-40 -50 L-55 -53", 1.7, "INK")
        T("M-40 -46.5 L-54 -44.5", 1.7, "INK")
    P("M-30 -102 L-24 -119 L-19 -99 Z", c["uniWhite"])
    P("M-30 -52 C -40 -68, -42 -84, -32 -92 L -12 -88 C -14 -74, -10 -62, 0 -55 Z", c["uniWhite"])
    E(-38, -91, 19, 15, c["uniWhite"])
    C(-21, -97, 8, c["rainbow0"])
    C(-14, -85, 8, c["rainbow1"])
    C(-9, -72, 7.5, c["rainbow2"])
    C(-6, -61, 6.5, c["rainbow4"])
    E(-52, -83, 12.5, 10, c["uniSnout"])
    C(-59, -86, 1.9, "INK")
    P("M-43 -104 L-51 -131 L-33 -106 Z", c["horn"])
    T("M-46 -112 L-38 -114", 1.6, "INK")
    T("M-48 -120 L-42 -121.5", 1.6, "INK")
    eye(-41, -94, 7, -0.3, -0.9)
    E(-49, -74, 3.2, 5, c["pink"], {"lw": EYE_LW * 0.6})


def weapon_stage_4():
    """The lance of stage 4."""
    with at(30, -70, 1, 30):
        T("M0 6 L0 -66", 5, c["grip"])
        P("M0 -58 L22 -52 L0 -46 Z", c["rainbow0"])
        P("M-4.5 -64 L0 -80 L4.5 -64 Z", c["metal"])


def cat_stage_5():
    """The cat, stage 5, mounted (no lance: the lance is its own sprite)."""
    T("M48 -54 C 68 -54, 74 -32, 64 -14", 9, c["rainbow0"])
    T("M50 -48 C 64 -46, 66 -30, 58 -18", 6.5, c["rainbow2"])
    T("M50 -43 C 58 -40, 58 -30, 53 -24", 4.5, c["rainbow4"])
    R(-10, -34, 12, 34, 6, mix(c["uniWhite"], INK, 0.14))
    R(26, -34, 12, 34, 6, mix(c["uniWhite"], INK, 0.14))
    E(14, -45, 40, 23, c["uniWhite"])
    R(-22, -30, 13, 30, 6.5, c["uniWhite"])
    R(32, -30, 13, 30, 6.5, c["uniWhite"])
    R(-22, -7, 13, 7, 3, c["hoof"])
    R(32, -7, 13, 7, 3, c["hoof"])
    P("M10 -62 C 14 -100, 50 -126, 86 -116 C 80 -110, 84 -103, 75 -100 C 79 -93, 72 -88, 64 -88 C 64 -80, 54 -72, 40 -68 Z", c["uniWhite"])
    with at(22, -63, 0.6, 0):
        T("M34 -16 C 56 -18, 60 -44, 47 -58", 11, c["catFur"])
        E(10, -27, 34, 27, c["catFur"])
        T("M29 -50 C 34 -46, 36 -40, 33 -35", 5, c["catStripe"], NO)
        T("M40 -42 C 44 -38, 45 -33, 43 -29", 5, c["catStripe"], NO)
        E(-6, -19, 16, 14, c["catBelly"], NO)
        E(-19, -4.5, 9.5, 6, c["catFur"])
        E(1, -4, 9.5, 6, c["catFur"])
        P("M-42 -63 L-41 -95 L-21 -77 Z", c["catFur"])
        P("M-9 -77 L-4 -87 L0 -84 L4 -97 L9 -68 Z", c["catFur"])
        P("M-37.5 -70 L-37 -87 L-27 -77 Z", c["pink"], NO)
        P("M-2 -77 L3 -89 L6.5 -71 Z", c["pink"], NO)
        E(-17, -58, 29, 23, c["catFur"])
        T("M-23 -79 L-22.5 -73", 4, c["catStripe"], NO)
        T("M-15 -80 L-15 -73.5", 4, c["catStripe"], NO)
        T("M-7 -78.5 L-7.5 -72.5", 4, c["catStripe"], NO)
        E(-24, -47, 13.5, 8.5, c["catBelly"], NO)
        C(-33, -60, 9.5, c["eye"], {"lw": EYE_LW})
        C(-12, -61, 10, c["eye"], {"lw": EYE_LW})
        C(-29.5, -59, 2.9, "INK")
        C(-15.5, -59.5, 2.9, "INK")
        T("M-44 -74 L-28 -68.5", 4.4, "INK")
        T("M-18 -69 L-1 -75", 4.4, "INK")
        E(-17, -40, 3.6, 4.4, c["pink"], {"lw": EYE_LW * 0.6})
        P("M-28 -52.5 L-20 -52.5 L-24 -47.5 Z", c["pink"], {"lw": EYE_LW * 0.7})
        T("M-36 -45 Q -24 -36 -11 -46", 2.7, "INK")
        P("M-30.5 -42.3 L-28.6 -37.6 L-26.8 -41.4 Z", c["eye"], {"lw": 1.5})
        T("M-40 -50 L-55 -53", 1.7, "INK")
        T("M-40 -46.5 L-54 -44.5", 1.7, "INK")
    P("M-30 -102 L-24 -119 L-19 -99 Z", c["uniWhite"])
    P("M-30 -52 C -40 -68, -42 -84, -32 -92 L -12 -88 C -14 -74, -10 -62, 0 -55 Z", c["uniWhite"])
    E(-38, -91, 19, 15, c["uniWhite"])
    C(-21, -97, 8, c["rainbow0"])
    C(-14, -85, 8, c["rainbow1"])
    C(-9, -72, 7.5, c["rainbow2"])
    C(-6, -61, 6.5, c["rainbow4"])
    E(-52, -83, 12.5, 10, c["uniSnout"])
    C(-59, -86, 1.9, "INK")
    P("M-43 -104 L-51 -131 L-33 -106 Z", c["horn"])
    T("M-46 -112 L-38 -114", 1.6, "INK")
    T("M-48 -120 L-42 -121.5", 1.6, "INK")
    eye(-41, -94, 7, -0.3, -0.9)
    E(-49, -74, 3.2, 5, c["pink"], {"lw": EYE_LW * 0.6})


def weapon_stage_5():
    """The lance of stage 5."""
    with at(30, -70, 1, 30):
        T("M0 6 L0 -66", 5, c["grip"])
        P("M0 -58 L22 -52 L0 -46 Z", c["rainbow0"])
        P("M-4.5 -64 L0 -80 L4.5 -64 Z", c["metal"])


def cat_stage_6():
    """The cat, stage 6, mounted (no lance: the lance is its own sprite)."""
    T("M48 -54 C 68 -54, 74 -32, 64 -14", 9, c["voidMane"])
    T("M50 -48 C 64 -46, 66 -30, 58 -18", 6.5, c["rainbow5"])
    T("M50 -43 C 58 -40, 58 -30, 53 -24", 4.5, c["voidStar"])
    R(-10, -34, 12, 34, 6, mix(c["voidBody"], INK, 0.14))
    R(26, -34, 12, 34, 6, mix(c["voidBody"], INK, 0.14))
    E(14, -45, 40, 23, c["voidBody"])
    R(-22, -30, 13, 30, 6.5, c["voidBody"])
    R(32, -30, 13, 30, 6.5, c["voidBody"])
    R(-22, -7, 13, 7, 3, c["hoof"])
    R(32, -7, 13, 7, 3, c["hoof"])
    P("M10 -62 C 14 -100, 50 -126, 86 -116 C 80 -110, 84 -103, 75 -100 C 79 -93, 72 -88, 64 -88 C 64 -80, 54 -72, 40 -68 Z", mix(c["voidBody"], c["eye"], 0.24))
    with at(22, -63, 0.6, 0):
        T("M34 -16 C 56 -18, 60 -44, 47 -58", 11, c["catFur"])
        E(10, -27, 34, 27, c["catFur"])
        T("M29 -50 C 34 -46, 36 -40, 33 -35", 5, c["catStripe"], NO)
        T("M40 -42 C 44 -38, 45 -33, 43 -29", 5, c["catStripe"], NO)
        E(-6, -19, 16, 14, c["catBelly"], NO)
        E(-19, -4.5, 9.5, 6, c["catFur"])
        E(1, -4, 9.5, 6, c["catFur"])
        P("M-42 -63 L-41 -95 L-21 -77 Z", c["catFur"])
        P("M-9 -77 L-4 -87 L0 -84 L4 -97 L9 -68 Z", c["catFur"])
        P("M-37.5 -70 L-37 -87 L-27 -77 Z", c["pink"], NO)
        P("M-2 -77 L3 -89 L6.5 -71 Z", c["pink"], NO)
        E(-17, -58, 29, 23, c["catFur"])
        T("M-23 -79 L-22.5 -73", 4, c["catStripe"], NO)
        T("M-15 -80 L-15 -73.5", 4, c["catStripe"], NO)
        T("M-7 -78.5 L-7.5 -72.5", 4, c["catStripe"], NO)
        E(-24, -47, 13.5, 8.5, c["catBelly"], NO)
        C(-33, -60, 9.5, c["eye"], {"lw": EYE_LW})
        C(-12, -61, 10, c["eye"], {"lw": EYE_LW})
        C(-29.5, -59, 2.9, "INK")
        C(-15.5, -59.5, 2.9, "INK")
        T("M-44 -74 L-28 -68.5", 4.4, "INK")
        T("M-18 -69 L-1 -75", 4.4, "INK")
        E(-17, -40, 3.6, 4.4, c["pink"], {"lw": EYE_LW * 0.6})
        P("M-28 -52.5 L-20 -52.5 L-24 -47.5 Z", c["pink"], {"lw": EYE_LW * 0.7})
        T("M-36 -45 Q -24 -36 -11 -46", 2.7, "INK")
        P("M-30.5 -42.3 L-28.6 -37.6 L-26.8 -41.4 Z", c["eye"], {"lw": 1.5})
        T("M-40 -50 L-55 -53", 1.7, "INK")
        T("M-40 -46.5 L-54 -44.5", 1.7, "INK")
    P("M-30 -102 L-24 -119 L-19 -99 Z", c["voidBody"])
    P("M-30 -52 C -40 -68, -42 -84, -32 -92 L -12 -88 C -14 -74, -10 -62, 0 -55 Z", c["voidBody"])
    E(-38, -91, 19, 15, c["voidBody"])
    C(-21, -97, 8, c["voidMane"])
    C(-14, -85, 8, c["rainbow5"])
    C(-9, -72, 7.5, c["voidMane"])
    C(-6, -61, 6.5, c["rainbow5"])
    E(-52, -83, 12.5, 10, mix(c["voidBody"], c["eye"], 0.32))
    C(-59, -86, 1.9, "INK")
    P("M-43 -104 L-51 -131 L-33 -106 Z", c["horn"])
    T("M-46 -112 L-38 -114", 1.6, "INK")
    T("M-48 -120 L-42 -121.5", 1.6, "INK")
    eye(-41, -94, 7, -0.3, -0.9)
    E(-49, -74, 3.2, 5, c["pink"], {"lw": EYE_LW * 0.6})


def weapon_stage_6():
    """The lance of stage 6."""
    with at(30, -70, 1, 30):
        T("M0 6 L0 -66", 5, c["grip"])
        P("M0 -58 L22 -52 L0 -46 Z", c["rainbow0"])
        P("M-4.5 -64 L0 -80 L4.5 -64 Z", c["metal"])


def cat_stage_7():
    """The cat, stage 7, mounted (no lance: the lance is its own sprite)."""
    T("M48 -54 C 68 -54, 74 -32, 64 -14", 9, c["halo"])
    T("M50 -48 C 64 -46, 66 -30, 58 -18", 6.5, c["rainbow0"])
    T("M50 -43 C 58 -40, 58 -30, 53 -24", 4.5, c["uniWhite"])
    R(-10, -34, 12, 34, 6, mix(c["lastBody"], INK, 0.14))
    R(26, -34, 12, 34, 6, mix(c["lastBody"], INK, 0.14))
    E(14, -45, 40, 23, c["lastBody"])
    R(-22, -30, 13, 30, 6.5, c["lastBody"])
    R(32, -30, 13, 30, 6.5, c["lastBody"])
    R(-22, -7, 13, 7, 3, c["hoof"])
    R(32, -7, 13, 7, 3, c["hoof"])
    P("M10 -62 C 14 -100, 50 -126, 86 -116 C 80 -110, 84 -103, 75 -100 C 79 -93, 72 -88, 64 -88 C 64 -80, 54 -72, 40 -68 Z", c["uniWhite"])
    with at(22, -63, 0.6, 0):
        T("M34 -16 C 56 -18, 60 -44, 47 -58", 11, c["catFur"])
        E(10, -27, 34, 27, c["catFur"])
        T("M29 -50 C 34 -46, 36 -40, 33 -35", 5, c["catStripe"], NO)
        T("M40 -42 C 44 -38, 45 -33, 43 -29", 5, c["catStripe"], NO)
        E(-6, -19, 16, 14, c["catBelly"], NO)
        E(-19, -4.5, 9.5, 6, c["catFur"])
        E(1, -4, 9.5, 6, c["catFur"])
        P("M-42 -63 L-41 -95 L-21 -77 Z", c["catFur"])
        P("M-9 -77 L-4 -87 L0 -84 L4 -97 L9 -68 Z", c["catFur"])
        P("M-37.5 -70 L-37 -87 L-27 -77 Z", c["pink"], NO)
        P("M-2 -77 L3 -89 L6.5 -71 Z", c["pink"], NO)
        E(-17, -58, 29, 23, c["catFur"])
        T("M-23 -79 L-22.5 -73", 4, c["catStripe"], NO)
        T("M-15 -80 L-15 -73.5", 4, c["catStripe"], NO)
        T("M-7 -78.5 L-7.5 -72.5", 4, c["catStripe"], NO)
        E(-24, -47, 13.5, 8.5, c["catBelly"], NO)
        C(-33, -60, 9.5, c["eye"], {"lw": EYE_LW})
        C(-12, -61, 10, c["eye"], {"lw": EYE_LW})
        C(-29.5, -59, 2.9, "INK")
        C(-15.5, -59.5, 2.9, "INK")
        T("M-44 -74 L-28 -68.5", 4.4, "INK")
        T("M-18 -69 L-1 -75", 4.4, "INK")
        E(-17, -40, 3.6, 4.4, c["pink"], {"lw": EYE_LW * 0.6})
        P("M-28 -52.5 L-20 -52.5 L-24 -47.5 Z", c["pink"], {"lw": EYE_LW * 0.7})
        T("M-36 -45 Q -24 -36 -11 -46", 2.7, "INK")
        P("M-30.5 -42.3 L-28.6 -37.6 L-26.8 -41.4 Z", c["eye"], {"lw": 1.5})
        T("M-40 -50 L-55 -53", 1.7, "INK")
        T("M-40 -46.5 L-54 -44.5", 1.7, "INK")
    P("M-30 -102 L-24 -119 L-19 -99 Z", c["lastBody"])
    P("M-30 -52 C -40 -68, -42 -84, -32 -92 L -12 -88 C -14 -74, -10 -62, 0 -55 Z", c["lastBody"])
    E(-38, -91, 19, 15, c["lastBody"])
    C(-21, -97, 8, c["halo"])
    C(-14, -85, 8, c["rainbow0"])
    C(-9, -72, 7.5, c["halo"])
    C(-6, -61, 6.5, c["rainbow0"])
    E(-52, -83, 12.5, 10, c["uniSnout"])
    C(-59, -86, 1.9, "INK")
    P("M-43 -104 L-51 -131 L-33 -106 Z", c["horn"])
    T("M-46 -112 L-38 -114", 1.6, "INK")
    T("M-48 -120 L-42 -121.5", 1.6, "INK")
    eye(-41, -94, 7, -0.3, -0.9)
    E(-49, -74, 3.2, 5, c["pink"], {"lw": EYE_LW * 0.6})
    RING(-38, -122, 15, 4.5, 3.6, c["halo"])


def weapon_stage_7():
    """The lance of stage 7."""
    with at(30, -70, 1, 30):
        T("M0 6 L0 -66", 5, c["grip"])
        P("M0 -58 L22 -52 L0 -46 Z", c["rainbow0"])
        P("M-4.5 -64 L0 -80 L4.5 -64 Z", c["metal"])


def kitten():
    """A kitten (the cat drawn in the kitten colours, no weapon)."""
    T("M34 -16 C 56 -18, 60 -44, 47 -58", 11, c["kitten"])
    E(10, -27, 34, 27, c["kitten"])
    T("M29 -50 C 34 -46, 36 -40, 33 -35", 5, c["kittenStripe"], NO)
    T("M40 -42 C 44 -38, 45 -33, 43 -29", 5, c["kittenStripe"], NO)
    E(-6, -19, 16, 14, c["kittenBelly"], NO)
    E(-19, -4.5, 9.5, 6, c["kitten"])
    E(1, -4, 9.5, 6, c["kitten"])
    P("M-42 -63 L-41 -95 L-21 -77 Z", c["kitten"])
    P("M-9 -77 L-4 -87 L0 -84 L4 -97 L9 -68 Z", c["kitten"])
    P("M-37.5 -70 L-37 -87 L-27 -77 Z", c["pink"], NO)
    P("M-2 -77 L3 -89 L6.5 -71 Z", c["pink"], NO)
    E(-17, -58, 29, 23, c["kitten"])
    T("M-23 -79 L-22.5 -73", 4, c["kittenStripe"], NO)
    T("M-15 -80 L-15 -73.5", 4, c["kittenStripe"], NO)
    T("M-7 -78.5 L-7.5 -72.5", 4, c["kittenStripe"], NO)
    E(-24, -47, 13.5, 8.5, c["kittenBelly"], NO)
    C(-33, -60, 9.5, c["eye"], {"lw": EYE_LW})
    C(-12, -61, 10, c["eye"], {"lw": EYE_LW})
    C(-29.5, -59, 2.9, "INK")
    C(-15.5, -59.5, 2.9, "INK")
    T("M-44 -74 L-28 -68.5", 4.4, "INK")
    T("M-18 -69 L-1 -75", 4.4, "INK")
    E(-17, -40, 3.6, 4.4, c["pink"], {"lw": EYE_LW * 0.6})
    P("M-28 -52.5 L-20 -52.5 L-24 -47.5 Z", c["pink"], {"lw": EYE_LW * 0.7})
    T("M-36 -45 Q -24 -36 -11 -46", 2.7, "INK")
    P("M-30.5 -42.3 L-28.6 -37.6 L-26.8 -41.4 Z", c["eye"], {"lw": 1.5})
    T("M-40 -50 L-55 -53", 1.7, "INK")
    T("M-40 -46.5 L-54 -44.5", 1.7, "INK")


def icon_paw():
    """The paw icon (the UI concept's kit)."""
    E(0, 7, 10.5, 8.5, c["catFur"])
    C(-11.5, -4, 4.8, c["catFur"])
    C(-4.5, -11, 4.8, c["catFur"])
    C(4.5, -11, 4.8, c["catFur"])
    C(11.5, -4, 4.8, c["catFur"])
    E(0, 8, 5.5, 4, c["pink"], NO)


def icon_reawaken():
    """The reawaken icon (the UI concept's kit)."""
    T("M11 -11 C 4 -18, -12 -15, -14 -2 C -15 10, -4 17, 8 13", 4.5, c["uiStardust"])
    P("M6 -21 L17 -11 L4 -6 Z", c["uiStardust"])
    P("M3 -6 Q4.2 -1.2 9 0 Q4.2 1.2 3 6 Q1.8 1.2 -3 0 Q1.8 -1.2 3 -6 Z", c["gold"])


def icon_flag():
    """The flag icon (the UI concept's kit)."""
    T("M-10 17 L-10 -17", 3.6, "INK")
    P("M-9 -17 L15 -10 L-9 -1 Z", c["rainbow0"])


def icon_cards():
    """The cards icon (the UI concept's kit)."""
    with at(-5, 2, 1, -12):
        R(-9, -13, 18, 24, 3, c["eye"])
    with at(5, 0, 1, 10):
        R(-9, -13, 18, 24, 3, c["rainbow4"])


def icon_trophy():
    """The trophy icon (the UI concept's kit)."""
    T("M-11 -10 C -19 -10, -18 2, -8 1", 3, c["gold"])
    T("M11 -10 C 19 -10, 18 2, 8 1", 3, c["gold"])
    P("M-11 -15 L11 -15 C 11 -1, 6 5, 0 5 C -6 5, -11 -1, -11 -15 Z", c["gold"])
    R(-3, 4, 6, 7, 1.5, c["gold"])
    R(-9, 10, 18, 6, 2.5, c["gold"])


def icon_hat():
    """The hat icon (the UI concept's kit)."""
    E(0, 9, 17, 5, c["hat"])
    R(-10, -16, 20, 25, 4, c["hat"])
    R(-10, -1, 20, 5, 0, c["plume"], NO)


def icon_gear():
    """The gear icon (the UI concept's kit)."""
    P("M0 -17 L4.78 -11.55 L12.02 -12.02 L11.55 -4.78 L17 0 L11.55 4.78 L12.02 12.02 L4.78 11.55 L0 17 L-4.78 11.55 L-12.02 12.02 L-11.55 4.78 L-17 0 L-11.55 -4.78 L-12.02 -12.02 L-4.78 -11.55 Z", c["metal"])
    C(0, 0, 6, c["uiPaper"])


def icon_tree():
    """The tree icon (the UI concept's kit)."""
    R(-3, 2, 6, 15, 2.5, c["grip"])
    C(-7, -4, 8, c["leafy"])
    C(7, -4, 8, c["leafy"])
    C(0, -11, 9, c["leafy"])


def icon_save():
    """The save icon (the UI concept's kit)."""
    R(-14, -14, 28, 28, 4, c["rainbow4"])
    R(-8, -14, 16, 10, 1, c["uiPaper"])
    R(-9, 3, 18, 9, 1.5, c["uiPaper"])


def icon_quit():
    """The quit icon (the UI concept's kit)."""
    R(-14, -14, 18, 28, 3, c["uiPaper2"])
    T("M-2 0 L14 0", 4, c["rainbow0"])
    P("M10 -7 L18 0 L10 7 Z", c["rainbow0"])


def icon_bug():
    """The bug icon (the UI concept's kit)."""
    T("M-12 -6 L-4 -2", 2.4, "INK")
    T("M12 -6 L4 -2", 2.4, "INK")
    T("M-13 6 L-4 4", 2.4, "INK")
    T("M13 6 L4 4", 2.4, "INK")
    E(0, 3, 9, 12, c["rainbow0"])
    C(0, -11, 6, c["wheel"])
    T("M0 -8 L0 14", 1.6, "INK")


def icon_chat():
    """The chat icon (the UI concept's kit)."""
    P("M-15 -12 C -15 -16, -12 -17, -8 -17 L8 -17 C 12 -17, 15 -16, 15 -12 L15 4 C 15 8, 12 9, 8 9 L-2 9 L-9 16 L-8 9 C -12 9, -15 8, -15 4 Z", c["rainbow5"])
    C(-6, -4, 2.2, "INK")
    C(0, -4, 2.2, "INK")
    C(6, -4, 2.2, "INK")


def icon_globe():
    """The globe icon (the UI concept's kit)."""
    C(0, 0, 16, c["rainbow4"])
    P("M-6 -14 C -2 -8, -10 -4, -4 2 C 0 6, -4 10, -2 15 C -12 12, -16 4, -15 -2 C -12 -10, -10 -12, -6 -14 Z", c["leafy"], NO)
    P("M8 -12 C 12 -6, 6 -2, 10 4 C 14 2, 16 -4, 14 -8 Z", c["leafy"], NO)


def icon_leaf():
    """The leaf icon (the UI concept's kit)."""
    P("M-15 13 C -17 -6, -3 -17, 15 -15 C 15 3, 3 15, -15 13 Z", c["uiCatnip"])
    T("M-12 10 L9 -9", 2, "INK")


def icon_star():
    """The star icon (the UI concept's kit)."""
    P("M0 -17 Q3.6 -2.6 18 1 Q3.6 4.6 0 19 Q-3.6 4.6 -18 1 Q-3.6 -2.6 0 -17 Z", c["uiStardust"])
    P("M13 -17 Q14 -13 18 -12 Q14 -11 13 -7 Q12 -11 8 -12 Q12 -13 13 -17 Z", c["gold"])


def icon_check():
    """The check icon (the UI concept's kit)."""
    T("M-10 0 L-3 8 L11 -9", 5, "INK")


def icon_close():
    """The close icon (the UI concept's kit)."""
    T("M-9 -9 L9 9", 4.5, "INK")
    T("M9 -9 L-9 9", 4.5, "INK")


def icon_lock():
    """The lock icon (the UI concept's kit)."""
    T("M-6 -2 L-6 -8 C -6 -16, 6 -16, 6 -8 L6 -2", 3.4, "INK")
    R(-11, -3, 22, 17, 4, c["metal"])
    C(0, 5, 2.4, "INK")


def icon_pad():
    """The pad icon (the UI concept's kit)."""
    P("M-18 -6 C -18 -12, -12 -13, -6 -13 L6 -13 C 12 -13, 18 -12, 18 -6 L20 8 C 20 14, 14 15, 10 10 L6 6 L-6 6 L-10 10 C -14 15, -20 14, -20 8 Z", c["uiPad"])
    T("M-11 -5 L-11 1", 2.6, c["uiPaper"])
    T("M-14 -2 L-8 -2", 2.6, c["uiPaper"])
    C(9, -6, 2.4, c["rainbow3"], NO)
    C(13, -2, 2.4, c["rainbow0"], NO)


def icon_bolt():
    """The bolt icon (the UI concept's kit)."""
    P("M5 -19 L-11 3 L-1 3 L-6 19 L11 -5 L1 -5 Z", c["gold"])


def icon_burst():
    """The burst icon (the UI concept's kit)."""
    P("M0 -18 L3.44 -8.31 L12.73 -12.73 L8.31 -3.44 L18 0 L8.31 3.44 L12.73 12.73 L3.44 8.31 L0 18 L-3.44 8.31 L-12.73 12.73 L-8.31 3.44 L-18 0 L-8.31 -3.44 L-12.73 -12.73 L-3.44 -8.31 Z", c["rainbow0"])


def icon_claw():
    """The claw icon (the UI concept's kit)."""
    T("M-12 -14 L-5 14", 4.5, c["eye"])
    T("M-2 -16 L5 12", 4.5, c["eye"])
    T("M8 -14 L14 8", 4.5, c["eye"])


def icon_cleaver():
    """The cleaver icon (the UI concept's kit)."""
    R(6, -4, 14, 7, 3.5, c["grip"])
    P("M-16 -12 L8 -12 L8 8 L-10 8 C -14 8, -16 5, -16 1 Z", c["metal"])
    C(-9, -5, 2.2, "INK")


def icon_mouse():
    """The mouse icon (the UI concept's kit)."""
    C(-9, -7, 6.5, c["mouseFur"])
    C(9, -7, 6.5, c["mouseFur"])
    E(0, 5, 12, 10, c["mouseFur"])
    C(-4, 3, 2, "INK")
    C(4, 3, 2, "INK")
    C(0, 9, 2.2, c["pink"], NO)


def icon_kitten():
    """The kitten icon (the UI concept's kit)."""
    P("M-13 -2 L-12 -18 L-3 -9 Z", c["kitten"])
    P("M13 -2 L12 -18 L3 -9 Z", c["kitten"])
    E(0, 3, 14, 11, c["kitten"])
    C(-5, 2, 3.2, c["eye"], NO)
    C(5, 2, 3.2, c["eye"], NO)
    C(-5, 2, 1.6, "INK")
    C(5, 2, 1.6, "INK")


def icon_upgrade_shred():
    """The shred upgrade's icon (approved on Missing Art, 2026-11-18)."""
    with at(-3, 0, 1, -8):
        P("M-15 -15 L0 -16 L0 -16 L-3 -10 L1 -5 L-4 0 L0 5 L-3 10 L0 16 L-15 15 Z", c["uiRedSoft"])
    with at(4, 1, 1, 9):
        P("M0 -16 L15 -15 L15 15 L0 16 L0 16 L-3 10 L0 5 L-4 0 L1 -5 L-3 -10 L0 -16 Z", c["uiRedSoft"])
    P("M1 -2 L4 -4 L4 1 Z", c["uiRedSoft"], {"lw": 1.6})


def icon_upgrade_shield_pierce():
    """The shield pierce upgrade's icon (approved on Missing Art, 2026-11-18)."""
    with at(-2, 0, 1, -6):
        P("M0 -17 C -6 -13, -12 -13, -16 -13 C -16 3, -9 12, 0 17 L0 16 L-3 10 L0 5 L-4 0 L1 -5 L-3 -10 L0 -16 Z", c["metal"])
    with at(3, 1, 1, 7):
        P("M0 -17 L0 -16 L-3 -10 L1 -5 L-4 0 L0 5 L-3 10 L0 16 C 9 12, 16 3, 16 -13 C 12 -13, 6 -13, 0 -17 Z", c["metal"])
    P("M17 -19 L22 -16 L18 -13 Z", c["metalDark"], {"lw": 1.8})
    P("M2 -4 L5 -6 L5 -1 Z", c["metalDark"], {"lw": 1.6})


def icon_upgrade_aura_radius():
    """The aura radius upgrade's icon (approved on Missing Art, 2026-11-18)."""
    RING(0, 0, 12, 12, 4.5, c["halo"])
    P("M17 -5 L24 0 L17 5 Z", "INK")
    P("M-17 -5 L-24 0 L-17 5 Z", "INK")


def icon_upgrade_kitten_power():
    """The kitten power upgrade's icon (approved on Missing Art, 2026-11-18)."""
    with at(-3, 3, 0.86, 0):
        P("M-13 -2 L-12 -18 L-3 -9 Z", c["kitten"])
        P("M13 -2 L12 -18 L3 -9 Z", c["kitten"])
        E(0, 3, 14, 11, c["kitten"])
        C(-5, 2, 3.2, c["eye"], NO)
        C(5, 2, 3.2, c["eye"], NO)
        C(-5, 2, 1.6, "INK")
        C(5, 2, 1.6, "INK")
    P("M11 -20 L12.76 -15.25 L17.36 -17.36 L15.25 -12.76 L20 -11 L15.25 -9.24 L17.36 -4.64 L12.76 -6.75 L11 -2 L9.24 -6.75 L4.64 -4.64 L6.75 -9.24 L2 -11 L6.75 -12.76 L4.64 -17.36 L9.24 -15.25 Z", c["rainbow0"])


def icon_upgrade_kitten_speed():
    """The kitten speed upgrade's icon (approved on Missing Art, 2026-11-18)."""
    with at(-3, 3, 0.86, 0):
        P("M-13 -2 L-12 -18 L-3 -9 Z", c["kitten"])
        P("M13 -2 L12 -18 L3 -9 Z", c["kitten"])
        E(0, 3, 14, 11, c["kitten"])
        C(-5, 2, 3.2, c["eye"], NO)
        C(5, 2, 3.2, c["eye"], NO)
        C(-5, 2, 1.6, "INK")
        C(5, 2, 1.6, "INK")
    P("M14 -21 L6 -9 L11 -9 L7 1 L18 -13 L12 -13 Z", c["gold"])


def icon_upgrade_overkill():
    """The overkill upgrade's icon (approved on Missing Art, 2026-11-18)."""
    P("M6 -20 L8.68 -12.47 L15.9 -15.9 L12.47 -8.68 L20 -6 L12.47 -3.32 L15.9 3.9 L8.68 0.47 L6 8 L3.32 0.47 L-3.9 3.9 L-0.47 -3.32 L-8 -6 L-0.47 -8.68 L-3.9 -15.9 L3.32 -12.47 Z", c["rainbow1"])
    P("M-4 -12 L-1.13 -3.93 L6.61 -7.61 L2.93 0.13 L11 3 L2.93 5.87 L6.61 13.61 L-1.13 9.93 L-4 18 L-6.87 9.93 L-14.61 13.61 L-10.93 5.87 L-19 3 L-10.93 0.13 L-14.61 -7.61 L-6.87 -3.93 Z", c["rainbow0"])


def icon_upgrade_pounce_power():
    """The pounce power upgrade's icon (approved on Missing Art, 2026-11-18)."""
    with at(0, 3, 0.9, 0):
        E(0, 7, 10.5, 8.5, c["catFur"])
        C(-11.5, -4, 4.8, c["catFur"])
        C(-4.5, -11, 4.8, c["catFur"])
        C(4.5, -11, 4.8, c["catFur"])
        C(11.5, -4, 4.8, c["catFur"])
        E(0, 8, 5.5, 4, c["pink"], NO)
    T("M-17 -13 L-22 -17", 2.6, "INK")
    T("M17 -13 L22 -17", 2.6, "INK")
    T("M0 -18 L0 -24", 2.6, "INK")


def icon_upgrade_pounce_cooldown():
    """The pounce cooldown upgrade's icon (approved on Missing Art, 2026-11-18)."""
    C(0, 0, 16, c["uiPaper"])
    P("M0 0 L0 -12.5 A12.5 12.5 0 1 1 -11.9 -3.9 Z", c["uiYellow"], NO)
    T("M0 0 L0 -11", 2.4, "INK")
    T("M0 0 L7 4", 2.4, "INK")
    C(0, 0, 2.4, "INK")


def icon_file_ui_icon_bait():
    """ui_icon_bait (approved on Missing Art, 2026-11-18)."""
    T("M9 -19 L9 -6 C 9 0, 2 1, 1 -4", 2.2, "INK")
    P("M-15 7 C -9 -2, 4 -2, 9 5 C 4 12, -9 12, -15 7 Z", c["gooseBeak"])
    P("M9 5 L17 -1 L17 11 Z", c["gooseBeak"])
    C(-8, 5, 1.7, "INK")


def hat_ratcatcher():
    """The ratcatcher hat (cap; approved 2026-11-18)."""
    P("M-46 -74 C -46 -98, 8 -98, 8 -74 Z", c["grip"])
    E(-50, -75, 15, 5, c["hatCapBrim"])
    T("M-20 -96 L-18 -90", 3, c["hatCapBrim"], NO)


def hat_unemployed():
    """The unemployed hat (beanie; approved 2026-11-18)."""
    P("M-46 -72 C -46 -102, 12 -102, 12 -72 Z", c["plume"])
    R(-48, -78, 62, 10, 5, c["hatRed"])
    C(-17, -104, 7, c["eye"])


def hat_tax_collector():
    """The tax collector hat (tophat; approved 2026-11-18)."""
    E(-17, -76, 32, 6.5, c["wheel"])
    R(-36, -118, 38, 42, 4, c["wheel"])
    R(-36, -88, 38, 8, 0, c["hatRed"])


def hat_blacksmith():
    """The blacksmith hat (helmet; approved 2026-11-18)."""
    P("M-48 -72 C -48 -106, 14 -106, 14 -72 Z", c["metal"])
    R(-51, -79, 68, 10, 5, c["metalDark"])
    C(-40, -74, 1.8, "INK")
    C(-17, -74, 1.8, "INK")
    C(6, -74, 1.8, "INK")
    C(-17, -106, 4.5, c["metalDark"])


def hat_cat_burglar():
    """The cat burglar hat (knit; approved 2026-11-18)."""
    P("M-47 -72 C -47 -103, 13 -103, 13 -72 Z", c["wheel"])
    R(-49, -80, 64, 10, 5, c["hatKnitBand"])
    T("M-40 -86 L-26 -96", 2.2, c["hatKnitRib"], NO)
    T("M-30 -84 L-14 -98", 2.2, c["hatKnitRib"], NO)
    T("M-18 -84 L-2 -97", 2.2, c["hatKnitRib"], NO)


def hat_daredevil():
    """The daredevil hat (stunt; approved 2026-11-18)."""
    P("M-50 -70 C -52 -108, 16 -108, 14 -70 Z", c["hatRed"])
    R(-23, -106, 12, 36, 4, c["uiWhite"], NO)
    T("M-50 -79 L14 -79", 4, "INK")
    C(-38, -81, 6.5, c["visor"])
    C(-24, -82, 6.5, c["visor"])


def hat_engineer():
    """The engineer hat (hardhat; approved 2026-11-18)."""
    E(-17, -74, 36, 6, c["gold"])
    P("M-45 -75 C -45 -104, 11 -104, 11 -75 Z", c["gold"])
    R(-21, -104, 8, 30, 4, mix(c["gold"], c["eye"], 0.45), NO)


def hat_magician():
    """The magician hat (wizard; approved 2026-11-18)."""
    E(-17, -75, 34, 7, c["hatWizard"])
    P("M-40 -76 L-14 -126 L6 -76 Z", c["hatWizard"])
    P("M-17 -101 Q-16 -97 -12 -96 Q-16 -95 -17 -91 Q-18 -95 -22 -96 Q-18 -97 -17 -101 Z", c["horn"], {"lw": 1.8})
    P("M-8 -87.4 Q-7.32 -84.68 -4.6 -84 Q-7.32 -83.32 -8 -80.6 Q-8.68 -83.32 -11.4 -84 Q-8.68 -84.68 -8 -87.4 Z", c["horn"], {"lw": 1.6})


def hat_sphinx():
    """The sphinx hat (nemes; approved 2026-11-18)."""
    P("M-50 -66 C -56 -100, 22 -100, 16 -66 L20 -40 L8 -44 L8 -66 C 0 -72, -34 -72, -42 -66 L-42 -44 L-54 -40 Z", c["horn"])
    T("M-44 -84 L10 -84", 3, c["rainbow5"], NO)
    T("M-46 -76 L12 -76", 3, c["rainbow5"], NO)
    T("M-48 -56 L-48 -46", 3, c["rainbow5"], NO)
    T("M14 -56 L14 -46", 3, c["rainbow5"], NO)


def hat_town_crier():
    """The town crier hat (tricorn; approved 2026-11-18)."""
    P("M-56 -76 C -44 -70, 10 -70, 22 -76 C 14 -100, -44 -102, -56 -76 Z", c["rainbow4"])
    T("M-54 -77 C -42 -72, 8 -72, 20 -77", 3, c["horn"], NO)
    P("M-17 -98 C -12 -104, -22 -104, -17 -98 Z", c["horn"])


def cursor_paw_held():
    """The held paw: the toes closed as arcs (the UI concept's kit)."""
    T("M-15.3 -1 C -15.7 -9, -7.3 -9, -7.7 -1", 3, "INK")
    T("M-8.3 -8 C -8.7 -16, -0.2999999999999998 -16, -0.7000000000000002 -8", 3, "INK")
    T("M0.7000000000000002 -8 C 0.2999999999999998 -16, 8.7 -16, 8.3 -8", 3, "INK")
    T("M7.7 -1 C 7.3 -9, 15.7 -9, 15.3 -1", 3, "INK")
    E(0, 7, 10.5, 8.5, c["catFur"])
    E(0, 8, 5.5, 4, c["pink"], NO)


# The cast: python name -> (the game's file stems, the figure). Stems follow SourceArt/svg.
CAST = {
    "mouse": (("enemy_backyard_mouse",), mouse),
    "vole": (("enemy_alley_vole",), vole),
    "rat": (("enemy_park_rat",), rat),
    "pigeon": (("enemy_rooftop_pigeon",), pigeon),
    "sewer_rat": (("enemy_sewer_rat",), sewer_rat),
    "slime_rat": (("enemy_turf_slime_rat",), slime_rat),
    "ghost": (("enemy_portal_ghost",), ghost),
    "imp": (("enemy_mythic_imp",), imp),
    "ant": (("enemy_kitchen_ant",), ant),
    "hamster": (("enemy_vet_hamster",), hamster),
    "factory_rat": (("enemy_factory_rat",), factory_rat),
    "dust_bunny": (("enemy_litterbox_dust_bunny",), dust_bunny),
    "moth": (("enemy_backyard_moth",), moth),
    "dog": (("enemy_alley_dog",), dog),
    "goose": (("enemy_park_goose",), goose),
    "seagull": (("enemy_rooftop_seagull",), seagull),
    "crocodile": (("enemy_sewer_crocodile",), crocodile),
    "toad": (("enemy_turf_toad",), toad),
    "hound": (("enemy_portal_hound",), hound),
    "crow": (("enemy_mythic_crow",), crow),
    "cockroach": (("enemy_kitchen_cockroach",), cockroach),
    "parrot": (("enemy_vet_parrot",), parrot),
    "robot_mouse": (("enemy_factory_robot_mouse",), robot_mouse),
    "sludge_cat": (("enemy_litterbox_sludge_cat",), sludge_cat),
    "flying_mouse": (("enemy_mouse_flying",), flying_mouse),
    "bat": (("enemy_bat",), bat),
    "golden_mouse": (("enemy_mouse_golden",), golden_mouse),
    "gristle_1": (("enemy_backyard_gristle",), gristle_1),
    "gristle_2": (("enemy_alley_gristle",), gristle_2),
    "gristle_3": (("enemy_park_gristle",), gristle_3),
    "gristle_4": (("enemy_rooftop_gristle",), gristle_4),
    "gristle_5": (("enemy_sewer_gristle",), gristle_5),
    "gristle_6": (("enemy_turf_gristle",), gristle_6),
    "gristle_7": (("enemy_portal_gristle",), gristle_7),
    "gristle_8": (("enemy_mythic_gristle",), gristle_8),
    "gristle_9": (("enemy_kitchen_gristle",), gristle_9),
    "gristle_10": (("enemy_vet_gristle",), gristle_10),
    "gristle_11": (("enemy_factory_gristle",), gristle_11),
    "gristle_12": (("enemy_litterbox_gristle",), gristle_12),
    "park_tyrant": (("boss_park_tyrant",), park_tyrant),
    "rival_champion": (("boss_rival_champion",), rival_champion),
    "void_god": (("boss_void_god",), void_god),
    "foreman": (("boss_factory_foreman",), foreman),
    "litter_king": (("boss_litter_king",), litter_king),
    "add": (("boss_add",), add),
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
    "ui_tab_upgrades": icon_paw,
    "ui_tab_reawaken": icon_reawaken,
    "ui_tab_realms": icon_flag,
    "ui_tab_catches": icon_cards,
    "ui_tab_awards": icon_trophy,
    "ui_tab_jobs": icon_hat,
    "ui_quick_settings": icon_gear,
    "ui_quick_skills": icon_tree,
    "ui_quick_save": icon_save,
    "ui_quick_quit": icon_quit,
    "ui_menu_bug": icon_bug,
    "ui_menu_discord": icon_chat,
    "ui_menu_language": icon_globe,
    "ui_icon_catnip": icon_leaf,
    "ui_icon_stardust": icon_star,
    "ui_tick": icon_check,
    "ui_close": icon_close,
    "ui_lock": icon_lock,
    "ui_icon_pad": icon_pad,
    "ui_icon_bait": icon_file_ui_icon_bait,
}

# The upgrades' icons the concept's shop draws (new files: the grammar drew none): the game's file -> its figure.
UPGRADE_ICONS = {
    "ui_icon_upgrade_paw_damage": icon_paw,
    "ui_icon_upgrade_attack_speed": icon_bolt,
    "ui_icon_upgrade_crit_chance": icon_burst,
    "ui_icon_upgrade_execute": icon_cleaver,
    "ui_icon_upgrade_currency_find": icon_leaf,
    "ui_icon_upgrade_spawn_rate": icon_mouse,
    "ui_icon_upgrade_kitten_count": icon_kitten,
    "ui_icon_upgrade_shred": icon_upgrade_shred,
    "ui_icon_upgrade_shield_pierce": icon_upgrade_shield_pierce,
    "ui_icon_upgrade_aura_radius": icon_upgrade_aura_radius,
    "ui_icon_upgrade_kitten_power": icon_upgrade_kitten_power,
    "ui_icon_upgrade_kitten_speed": icon_upgrade_kitten_speed,
    "ui_icon_upgrade_overkill": icon_upgrade_overkill,
    "ui_icon_upgrade_pounce_power": icon_upgrade_pounce_power,
    "ui_icon_upgrade_pounce_cooldown": icon_upgrade_pounce_cooldown,
}

# The jobs' hats (approved 2026-11-18, tools/art/concept_hats.js): the game's job id -> its hat, in the Tiny Kitten's frame. The
# game's files: hat_<id> (the field's sprite, sticker_art places it on the cat) and job_<id> (its icon: the hat alone).
JOB_HATS = {
    "ratcatcher": hat_ratcatcher,
    "unemployed": hat_unemployed,
    "tax_collector": hat_tax_collector,
    "blacksmith": hat_blacksmith,
    "cat_burglar": hat_cat_burglar,
    "daredevil": hat_daredevil,
    "engineer": hat_engineer,
    "magician": hat_magician,
    "sphinx": hat_sphinx,
    "town_crier": hat_town_crier,
}

# The mouse cursor over menus (the game's file -> its figure), drawn at the kit's cursor line (2.8) with the rim.
CURSORS = {
    "ui_cursor_paw_open": icon_paw,
    "ui_cursor_paw_closed": cursor_paw_held,
}
