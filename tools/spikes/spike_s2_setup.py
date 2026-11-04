"""SPIKE S2 (docs/04 §8) setup, throwaway: delete with Source/Pawpocalypse/Dev/Spikes when the results are recorded.

Imports SourceArt/png/spike_s2 (atlas of the 40 S1 sprites in 256 px cells, 8x8 on 2048, plus white shadow/zone/ring
textures) as T_S2_*, builds the materials (M_S2_Atlas masked with a per-instance atlas offset, M_S2_Soft translucent
tinted by per-instance colour, M_S2_Add additive), 43 Paper2D sprites (PS_S2_c00..c39 from the S1 textures in atlas
order, PS_S2_shadow/zone/ring) and the map L_SpikeCrowd holding one ASpikeCrowdActor. Only creates what is missing.
Needs spike S1's assets (T_S1_*). Headless, editor closed, -SCCProvider=None.
"""
import os
import unreal

ROOT = "/Game/Pawpocalypse/Dev/Spikes/S2"
S1TEX = "/Game/Pawpocalypse/Dev/Spikes/S1/Tex"
SRC = os.path.join(unreal.Paths.project_dir(), "SourceArt/png/spike_s2")
S1LIST = os.path.join(unreal.Paths.project_dir(), "SourceArt/png/spike_s1/textures.txt")
MAP = ROOT + "/L_SpikeCrowd"

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
log = unreal.log_warning
failed = []


def import_textures():
    for name in ["atlas", "shadow", "zone", "ring"]:
        path = ROOT + "/T_S2_" + name
        if not eal.does_asset_exist(path):
            t = unreal.AssetImportTask()
            t.filename = os.path.join(SRC, name + ".png")
            t.destination_path = ROOT
            t.destination_name = "T_S2_" + name
            t.automated = True
            t.replace_existing = True
            tools.import_asset_tasks([t])
        tex = eal.load_asset(path)
        if not tex:
            failed.append("import " + name)
            continue
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tex.set_editor_property("srgb", True)
        tex.set_editor_property("never_stream", True)
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
        eal.save_asset(path, only_if_is_dirty=False)
    log("S2 setup: textures ready")


def connect(a, out, b, inp):
    if not mel.connect_material_expressions(a, out, b, inp):
        failed.append("connect %s.%s -> %s.%s" % (a.get_name(), out, b.get_name(), inp))


def prop(expr, out, p):
    if not mel.connect_material_property(expr, out, p):
        failed.append("property %s.%s -> %s" % (expr.get_name(), out, p))


def material(name, blend):
    path = ROOT + "/" + name
    if eal.does_asset_exist(path):
        return None
    m = tools.create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("blend_mode", blend)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("used_with_instanced_static_meshes", True)
    return m


def tex_param(m, default, x):
    t = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, x, 0)
    t.set_editor_property("parameter_name", "Tex")
    t.set_editor_property("texture", eal.load_asset(ROOT + "/" + default))
    return t


def finish(m):
    mel.recompile_material(m)
    eal.save_asset(m.get_path_name())
    log("S2 setup: created " + m.get_name())


def atlas_material():
    m = material("M_S2_Atlas", unreal.BlendMode.BLEND_MASKED)
    if not m:
        return
    # UV = TexCoord * 1/8 + (customdata0, customdata1): the cell's top-left, set once per instance (docs/04 §5.1).
    uv = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1000, 0)
    cell = mel.create_material_expression(m, unreal.MaterialExpressionConstant2Vector, -1000, 150)
    cell.set_editor_property("r", 0.125)
    cell.set_editor_property("g", 0.125)
    mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -800, 0)
    connect(uv, "", mul, "A")
    connect(cell, "", mul, "B")
    u = mel.create_material_expression(m, unreal.MaterialExpressionPerInstanceCustomData, -1000, 300)
    u.set_editor_property("data_index", 0)
    v = mel.create_material_expression(m, unreal.MaterialExpressionPerInstanceCustomData, -1000, 400)
    v.set_editor_property("data_index", 1)
    app = mel.create_material_expression(m, unreal.MaterialExpressionAppendVector, -800, 300)
    connect(u, "", app, "A")
    connect(v, "", app, "B")
    add = mel.create_material_expression(m, unreal.MaterialExpressionAdd, -600, 100)
    connect(mul, "", add, "A")
    connect(app, "", add, "B")
    t = tex_param(m, "T_S2_atlas", -400)
    connect(add, "", t, "UVs")
    prop(t, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    prop(t, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    finish(m)


def soft_material():
    m = material("M_S2_Soft", unreal.BlendMode.BLEND_TRANSLUCENT)
    if not m:
        return
    t = tex_param(m, "T_S2_shadow", -600)
    col = mel.create_material_expression(m, unreal.MaterialExpressionPerInstanceCustomData3Vector, -600, 300)
    col.set_editor_property("data_index", 0)
    a = mel.create_material_expression(m, unreal.MaterialExpressionPerInstanceCustomData, -600, 450)
    a.set_editor_property("data_index", 3)
    rgb = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 0)
    connect(t, "RGB", rgb, "A")
    connect(col, "", rgb, "B")
    op = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 200)
    connect(t, "A", op, "A")
    connect(a, "", op, "B")
    prop(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    prop(op, "", unreal.MaterialProperty.MP_OPACITY)
    finish(m)


def additive_material():
    # Rebuilt in place on every run: the first version used RGB only and drew the white ring texture as a solid square
    # (the ring's shape lives in its alpha). Emissive = RGB * A * per-instance colour.
    path = ROOT + "/M_S2_Add"
    m = eal.load_asset(path) if eal.does_asset_exist(path) else material("M_S2_Add", unreal.BlendMode.BLEND_ADDITIVE)
    mel.delete_all_material_expressions(m)
    t = tex_param(m, "T_S2_ring", -800)
    col = mel.create_material_expression(m, unreal.MaterialExpressionPerInstanceCustomData3Vector, -800, 300)
    col.set_editor_property("data_index", 0)
    shaped = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -500, 0)
    connect(t, "RGB", shaped, "A")
    connect(t, "A", shaped, "B")
    rgb = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 0)
    connect(shaped, "", rgb, "A")
    connect(col, "", rgb, "B")
    prop(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(m)


def sprite(name, texture_path, material):
    path = ROOT + "/" + name
    if eal.does_asset_exist(path):
        # Always (re)set the material: a hard reference from the sprite is what gets it cooked. Loaded by path from C++
        # it was missing in the packaged build (Failed to find object /Paper2D/TranslucentUnlitSpriteMaterial).
        sp = eal.load_asset(path)
        sp.set_editor_property("default_material", eal.load_asset(material))
        eal.save_asset(path, only_if_is_dirty=False)
        return
    # PaperSpriteFactory.InitialTexture is not exposed to Python. Setting SourceTexture through set_editor_property fires
    # UPaperSprite::PostEditChangeProperty, which sizes a brand-new sprite to the texture as if it had been factoried.
    sp = tools.create_asset(name, ROOT, unreal.PaperSprite, unreal.PaperSpriteFactory())
    if not sp:
        failed.append("sprite " + name)
        return
    sp.set_editor_property("source_texture", eal.load_asset(texture_path))
    sp.set_editor_property("default_material", eal.load_asset(material))
    if sp.get_editor_property("source_dimension").x <= 0:
        failed.append("sprite %s has no size" % name)
    eal.save_asset(path)


def sprites():
    cells = [l.strip() for l in open(S1LIST) if l.strip()]
    for i, tex in enumerate(cells):
        sprite("PS_S2_c%02d" % i, S1TEX + "/" + tex, "/Paper2D/MaskedUnlitSpriteMaterial")
    for n in ["shadow", "zone", "ring"]:
        sprite("PS_S2_" + n, ROOT + "/T_S2_" + n, "/Paper2D/TranslucentUnlitSpriteMaterial")
    log("S2 setup: %d Paper2D sprites ready" % (len(cells) + 3))


def spike_map():
    if eal.does_asset_exist(MAP):
        return
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.new_level(MAP):
        failed.append("new_level")
        return
    cls = unreal.load_class(None, "/Script/Pawpocalypse.SpikeCrowdActor")
    unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(cls, unreal.Vector(0, 0, 0))
    levels.save_current_level()
    log("S2 setup: created " + MAP)


import_textures()
atlas_material()
soft_material()
additive_material()
sprites()
spike_map()
log("S2 setup: FAILED " + "; ".join(failed) if failed else "S2 setup: done")
