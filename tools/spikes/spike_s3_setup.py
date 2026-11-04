"""SPIKE S3 (docs/04 §8) setup, throwaway: delete with Source/Pawpocalypse/Dev/Spikes when the results are recorded.

Imports every atlas page of SourceArt/png/spike_s3/<variant>/ twice: T_S3_<variant>_p<n>_bc7 (TC_BC7) and
T_S3_<variant>_p<n>_ui (TC_EDITOR_ICON = UserInterface2D, uncompressed BGRA8); both sRGB, Never Stream, mips. Builds
M_S3_Atlas (unlit masked; UV = TexCoord * (cd2, cd3) + (cd0, cd1)) and the map L_SpikeArt with one ASpikeArtActor. Only
creates what is missing. Run tools/spikes/s3_raster_pack.py first. Headless, editor closed, -SCCProvider=None.
"""
import os
import unreal

ROOT = "/Game/Pawpocalypse/Dev/Spikes/S3"
SRC = os.path.join(unreal.Paths.project_dir(), "SourceArt/png/spike_s3")
MAP = ROOT + "/L_SpikeArt"
COMPRESSION = {"bc7": unreal.TextureCompressionSettings.TC_BC7, "ui": unreal.TextureCompressionSettings.TC_EDITOR_ICON}

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
log = unreal.log_warning
failed = []


def import_pages():
    count = 0
    for variant in ("r1x", "r2x", "godot"):
        folder = os.path.join(SRC, variant)
        for f in sorted(os.listdir(folder)):
            if not f.startswith("page_") or not f.endswith(".png"):
                continue
            page = f[5:-4]
            for comp, setting in COMPRESSION.items():
                name = "T_S3_%s_p%s_%s" % (variant, page, comp)
                path = ROOT + "/" + name
                if not eal.does_asset_exist(path):
                    t = unreal.AssetImportTask()
                    t.filename = os.path.join(folder, f)
                    t.destination_path = ROOT
                    t.destination_name = name
                    t.automated = True
                    t.replace_existing = True
                    tools.import_asset_tasks([t])
                tex = eal.load_asset(path)
                if not tex:
                    failed.append("import " + name)
                    continue
                tex.set_editor_property("compression_settings", setting)
                tex.set_editor_property("srgb", True)
                tex.set_editor_property("never_stream", True)
                tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
                eal.save_asset(path, only_if_is_dirty=False)
                count += 1
    log("S3 setup: %d page textures ready" % count)


def connect(a, out, b, inp):
    if not mel.connect_material_expressions(a, out, b, inp):
        failed.append("connect %s.%s -> %s.%s" % (a.get_name(), out, b.get_name(), inp))


def material():
    path = ROOT + "/M_S3_Atlas"
    if eal.does_asset_exist(path):
        return
    m = tools.create_asset("M_S3_Atlas", ROOT, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("used_with_instanced_static_meshes", True)
    cd = []
    for i in range(4):
        e = mel.create_material_expression(m, unreal.MaterialExpressionPerInstanceCustomData, -1200, i * 120)
        e.set_editor_property("data_index", i)
        cd.append(e)
    offset = mel.create_material_expression(m, unreal.MaterialExpressionAppendVector, -1000, 0)
    connect(cd[0], "", offset, "A")
    connect(cd[1], "", offset, "B")
    scale = mel.create_material_expression(m, unreal.MaterialExpressionAppendVector, -1000, 240)
    connect(cd[2], "", scale, "A")
    connect(cd[3], "", scale, "B")
    uv = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1000, 400)
    mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -800, 300)
    connect(uv, "", mul, "A")
    connect(scale, "", mul, "B")
    add = mel.create_material_expression(m, unreal.MaterialExpressionAdd, -600, 150)
    connect(mul, "", add, "A")
    connect(offset, "", add, "B")
    t = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -400, 0)
    t.set_editor_property("parameter_name", "Tex")
    t.set_editor_property("texture", eal.load_asset(ROOT + "/T_S3_r2x_p0_ui"))
    connect(add, "", t, "UVs")
    if not mel.connect_material_property(t, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        failed.append("emissive")
    if not mel.connect_material_property(t, "A", unreal.MaterialProperty.MP_OPACITY_MASK):
        failed.append("opacity mask")
    mel.recompile_material(m)
    eal.save_asset(path)
    log("S3 setup: created M_S3_Atlas")


def spike_map():
    if eal.does_asset_exist(MAP):
        return
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.new_level(MAP):
        failed.append("new_level")
        return
    cls = unreal.load_class(None, "/Script/Pawpocalypse.SpikeArtActor")
    unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(cls, unreal.Vector(0, 0, 0))
    levels.save_current_level()
    log("S3 setup: created " + MAP)


import_pages()
material()
spike_map()
log("S3 setup: FAILED " + "; ".join(failed) if failed else "S3 setup: done")
