"""Creates the realm renderer's four materials (docs/04 §5.1) under /Game/Pawpocalypse/Materials. No textures: every shape
is computed from the quad's UVs, every colour comes from the instance's custom data (floats 0-2 = linear RGB, 3 = alpha).

  M_FieldFlat  unlit opaque       the backdrop
  M_FieldSoft  unlit translucent  a soft disc (zones, shadows): opacity = alpha x the disc's soft edge
  M_FieldDisc  unlit masked       a solid disc with an ink rim (the bodies, placeholders until phase 6's atlas)
  M_FieldRing  unlit translucent  a thin ring at the rim, opacity = alpha (the effects)

Rebuilds a material's graph only when it is missing or --force is given (they are generated, never hand-edited).
Headless, editor closed:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="C:/abs/path/tools/create_field_materials.py" -nullrhi
      -unattended -nop4 -SCCProvider=None -stdout
"""
import sys

import unreal

ROOT = "/Game/Pawpocalypse/Materials"
FORCE = "--force" in sys.argv

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
log = unreal.log_warning
failed = []


def connect(a, out, b, inp):
    if not mel.connect_material_expressions(a, out, b, inp):
        failed.append("connect %s.%s -> %s.%s" % (a.get_name(), out, b.get_name(), inp))


def prop(expr, out, p):
    if not mel.connect_material_property(expr, out, p):
        failed.append("property %s.%s -> %s" % (expr.get_name(), out, p))


def material(name, blend):
    path = ROOT + "/" + name
    if eal.does_asset_exist(path):
        if not FORCE:
            log("create_field_materials: %s exists, left as is" % name)
            return None
        m = eal.load_asset(path)
        mel.delete_all_material_expressions(m)
    else:
        m = tools.create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("blend_mode", blend)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("used_with_instanced_static_meshes", True)
    return m


def expr(m, cls, x, y, **props):
    e = mel.create_material_expression(m, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def binary(m, cls, a, b, x, y, a_out="", b_out=""):
    e = expr(m, cls, x, y)
    connect(a, a_out, e, "A")
    connect(b, b_out, e, "B")
    return e


def colour(m, x=-900, y=0):
    return expr(m, unreal.MaterialExpressionPerInstanceCustomData3Vector, x, y, data_index=0)


def alpha(m, x=-900, y=200):
    return expr(m, unreal.MaterialExpressionPerInstanceCustomData, x, y, data_index=3)


def radius01(m):
    """0 at the quad's centre, 1 at the middle of each edge."""
    uv = expr(m, unreal.MaterialExpressionTextureCoordinate, -1300, 400)
    centre = expr(m, unreal.MaterialExpressionConstant2Vector, -1300, 520, r=0.5, g=0.5)
    dist = binary(m, unreal.MaterialExpressionDistance, uv, centre, -1100, 450)
    two = expr(m, unreal.MaterialExpressionConstant, -1100, 560, r=2.0)
    return binary(m, unreal.MaterialExpressionMultiply, dist, two, -950, 450)


def scaled_saturate(m, value, scale, offset, x, y):
    """saturate((value + offset) * scale)."""
    off = expr(m, unreal.MaterialExpressionConstant, x - 150, y + 100, r=offset)
    add = binary(m, unreal.MaterialExpressionAdd, value, off, x, y)
    k = expr(m, unreal.MaterialExpressionConstant, x, y + 100, r=scale)
    mul = binary(m, unreal.MaterialExpressionMultiply, add, k, x + 150, y)
    sat = expr(m, unreal.MaterialExpressionSaturate, x + 300, y)
    connect(mul, "", sat, "")
    return sat


def finish(m):
    mel.recompile_material(m)
    if not eal.save_asset(m.get_path_name(), only_if_is_dirty=False):
        failed.append("save " + m.get_name())
    log("create_field_materials: built " + m.get_name())


def flat():
    m = material("M_FieldFlat", unreal.BlendMode.BLEND_OPAQUE)
    if m:
        prop(colour(m), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        finish(m)


def soft():
    m = material("M_FieldSoft", unreal.BlendMode.BLEND_TRANSLUCENT)
    if not m:
        return
    d = radius01(m)
    neg = expr(m, unreal.MaterialExpressionOneMinus, -800, 450)
    connect(d, "", neg, "")
    edge = scaled_saturate(m, neg, 3.0, 0.0, -650, 450)  # full inside 2/3 of the radius, fading to the rim
    op = binary(m, unreal.MaterialExpressionMultiply, edge, alpha(m), -200, 300)
    prop(colour(m), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    prop(op, "", unreal.MaterialProperty.MP_OPACITY)
    finish(m)


def disc():
    m = material("M_FieldDisc", unreal.BlendMode.BLEND_MASKED)
    if not m:
        return
    m.set_editor_property("opacity_mask_clip_value", 0.5)
    d = radius01(m)
    neg = expr(m, unreal.MaterialExpressionOneMinus, -800, 450)
    connect(d, "", neg, "")
    inside = scaled_saturate(m, neg, 1000.0, 0.0005, -650, 450)  # 1 inside the disc, 0 outside, a hard edge
    rim = scaled_saturate(m, d, 1000.0, -0.86, -650, 650)         # the outer 14 % of the radius
    ink = expr(m, unreal.MaterialExpressionConstant3Vector, -650, 850, constant=unreal.LinearColor(0.0097, 0.0097, 0.0097, 1.0))
    lerp = expr(m, unreal.MaterialExpressionLinearInterpolate, -200, 0)
    connect(colour(m), "", lerp, "A")
    connect(ink, "", lerp, "B")
    connect(rim, "", lerp, "Alpha")
    prop(lerp, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    prop(inside, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    finish(m)


def ring():
    # Translucent, not additive: the floor is light paper, and adding light to it shows nothing (measured 2026-10-31).
    m = material("M_FieldRing", unreal.BlendMode.BLEND_TRANSLUCENT)
    if not m:
        return
    d = radius01(m)
    off = expr(m, unreal.MaterialExpressionConstant, -800, 550, r=-0.88)
    shifted = binary(m, unreal.MaterialExpressionAdd, d, off, -650, 450)
    ab = expr(m, unreal.MaterialExpressionAbs, -500, 450)
    connect(shifted, "", ab, "")
    neg = expr(m, unreal.MaterialExpressionOneMinus, -350, 450)
    k = expr(m, unreal.MaterialExpressionConstant, -500, 550, r=10.0)
    scaled = binary(m, unreal.MaterialExpressionMultiply, ab, k, -350, 550)
    connect(scaled, "", neg, "")
    band = expr(m, unreal.MaterialExpressionSaturate, -200, 450)
    connect(neg, "", band, "")
    strength = binary(m, unreal.MaterialExpressionMultiply, band, alpha(m), -100, 300)
    prop(colour(m), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    prop(strength, "", unreal.MaterialProperty.MP_OPACITY)
    finish(m)


flat()
soft()
disc()
ring()
unreal.log_warning("create_field_materials: FAILED %s" % failed if failed else "create_field_materials: done")
