"""Creates the realm renderer's four materials (docs/04 §5.1) under /Game/Pawpocalypse/Materials. No textures: every shape
is computed from the quad's UVs, every colour comes from the instance's custom data (floats 0-2 = linear RGB, 3 = alpha).

  M_FieldFlat  unlit opaque       the backdrop
  M_FieldSoft  unlit translucent  a soft disc (zones, shadows): opacity = alpha x the disc's soft edge
  M_FieldDisc  unlit masked       a solid disc with an ink rim (the bodies, placeholders until phase 6's atlas)
  M_FieldRing  unlit translucent  a thin ring at the rim, opacity = alpha (the effects)
  M_Sprite     unlit masked       an atlas sprite (phase 6): the cell's UVs from custom data 4-7, colour = texel x tint
                                  (custom 0-2), whitened by custom 8 (0 = the art, 1 = a white silhouette); mask = texel alpha
  M_SpriteSoft unlit translucent  the same, opacity = texel alpha x custom 3: the shielded, the illusory, glows, shadows
The two sprite materials sample the texture parameter `Atlas`; the renderer makes one instance per atlas page.
  M_Floor      unlit opaque       the biome's floor (phase 6 slice 2): texture parameter `Floor`, tiled: UV = the quad's
                                  UV x (custom 0, custom 1), the repeats across and down
  M_BackdropFill unlit translucent the backdrop's patches, puddles and rims: vertex colour as sRGB bytes (decoded exactly,
                                  Godot's palette bytes), opacity = vertex alpha; not used with instancing

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


def material(name, blend, instanced=True):
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
    m.set_editor_property("used_with_instanced_static_meshes", instanced)
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


def sprite(name, blend):
    m = material(name, blend)
    if not m:
        return
    def cd(i, x, y):
        return expr(m, unreal.MaterialExpressionPerInstanceCustomData, x, y, data_index=i)
    a = expr(m, unreal.MaterialExpressionAppendVector, -1300, 200)
    connect(cd(4, -1500, 150), "", a, "A")
    connect(cd(5, -1500, 250), "", a, "B")
    b = expr(m, unreal.MaterialExpressionAppendVector, -1300, 400)
    connect(cd(6, -1500, 350), "", b, "A")
    connect(cd(7, -1500, 450), "", b, "B")
    uv = expr(m, unreal.MaterialExpressionTextureCoordinate, -1300, 600)
    cell = expr(m, unreal.MaterialExpressionLinearInterpolate, -1100, 300)
    connect(a, "", cell, "A")
    connect(b, "", cell, "B")
    connect(uv, "", cell, "Alpha")
    tex = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, 300, parameter_name="Atlas",
               texture=unreal.load_asset("/Engine/EngineResources/DefaultTexture.DefaultTexture"))
    connect(cell, "", tex, "UVs")
    white = expr(m, unreal.MaterialExpressionConstant3Vector, -700, 500, constant=unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    whiten = expr(m, unreal.MaterialExpressionLinearInterpolate, -500, 300)
    connect(tex, "RGB", whiten, "A")
    connect(white, "", whiten, "B")
    connect(cd(8, -700, 650), "", whiten, "Alpha")
    tinted = binary(m, unreal.MaterialExpressionMultiply, whiten, colour(m, -500, 0), -300, 200)
    prop(tinted, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if blend == unreal.BlendMode.BLEND_MASKED:
        m.set_editor_property("opacity_mask_clip_value", 0.5)
        prop(tex, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    else:
        op = expr(m, unreal.MaterialExpressionMultiply, -300, 500)
        connect(tex, "A", op, "A")
        connect(alpha(m, -500, 700), "", op, "B")
        prop(op, "", unreal.MaterialProperty.MP_OPACITY)
    finish(m)


def floor():
    m = material("M_Floor", unreal.BlendMode.BLEND_OPAQUE)
    if not m:
        return
    rep_xy = expr(m, unreal.MaterialExpressionAppendVector, -1100, 300)
    connect(expr(m, unreal.MaterialExpressionPerInstanceCustomData, -1300, 250, data_index=0), "", rep_xy, "A")
    connect(expr(m, unreal.MaterialExpressionPerInstanceCustomData, -1300, 350, data_index=1), "", rep_xy, "B")
    uv = binary(m, unreal.MaterialExpressionMultiply, expr(m, unreal.MaterialExpressionTextureCoordinate, -1300, 450), rep_xy,
                -900, 350)
    tex = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -700, 300, parameter_name="Floor",
               texture=unreal.load_asset("/Engine/EngineResources/DefaultTexture.DefaultTexture"))
    connect(uv, "", tex, "UVs")
    prop(tex, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(m)


def backdrop_fill():
    m = material("M_BackdropFill", unreal.BlendMode.BLEND_TRANSLUCENT, instanced=False)
    if not m:
        return
    vc = expr(m, unreal.MaterialExpressionVertexColor, -900, 200)
    dec = expr(m, unreal.MaterialExpressionCustom, -600, 200)
    dec.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    dec.set_editor_property("code", "return float3(C.r <= 0.04045 ? C.r / 12.92 : pow((C.r + 0.055) / 1.055, 2.4),"
                                    " C.g <= 0.04045 ? C.g / 12.92 : pow((C.g + 0.055) / 1.055, 2.4),"
                                    " C.b <= 0.04045 ? C.b / 12.92 : pow((C.b + 0.055) / 1.055, 2.4));")
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", "C")
    dec.set_editor_property("inputs", [ci])
    connect(vc, "", dec, "C")
    prop(dec, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    prop(vc, "A", unreal.MaterialProperty.MP_OPACITY)
    finish(m)


flat()
soft()
disc()
ring()
sprite("M_Sprite", unreal.BlendMode.BLEND_MASKED)
sprite("M_SpriteSoft", unreal.BlendMode.BLEND_TRANSLUCENT)
floor()
backdrop_fill()
unreal.log_warning("create_field_materials: FAILED %s" % failed if failed else "create_field_materials: done")
