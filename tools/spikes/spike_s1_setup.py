"""SPIKE S1 (docs/04 §8) setup, throwaway: delete with Source/Pawpocalypse/Dev/Spikes when the results are recorded.

Imports SourceArt/png/spike_s1/*.png (Godot-rasterised creatures at 2x) as T_S1_<name>, creates the unlit materials
(M_S1_Sprite masked, M_S1_Sprite_A2C masked + alpha-to-coverage, M_S1_Swatch opaque with per-instance colour) and the
map L_SpikeLook holding one ASpikeLookActor. Everything under /Game/Pawpocalypse/Dev/Spikes/S1. Only creates what is
missing.

Headless, editor closed:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=C:/.../tools/spikes/spike_s1_setup.py
      -nullrhi -unattended -nop4 -SCCProvider=None -stdout
"""
import os
import unreal

ROOT = "/Game/Pawpocalypse/Dev/Spikes/S1"
TEX = ROOT + "/Tex"
SRC = os.path.join(unreal.Paths.project_dir(), "SourceArt/png/spike_s1")
MAP = ROOT + "/L_SpikeLook"

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
log = unreal.log_warning


def import_textures():
    tasks = []
    for f in sorted(os.listdir(SRC)):
        if not f.endswith(".png"):
            continue
        name = "T_S1_" + f[:-4]
        if eal.does_asset_exist(TEX + "/" + name):
            continue
        t = unreal.AssetImportTask()
        t.filename = os.path.join(SRC, f)
        t.destination_path = TEX
        t.destination_name = name
        t.automated = True
        t.replace_existing = True
        t.save = False
        tasks.append(t)
    if tasks:
        tools.import_asset_tasks(tasks)
    count = 0
    for f in sorted(os.listdir(SRC)):
        if not f.endswith(".png"):
            continue
        path = TEX + "/T_S1_" + f[:-4]
        tex = eal.load_asset(path)
        if not tex:
            unreal.log_error("S1 setup: import failed for " + f)
            continue
        # Isolate the colour path: uncompressed (compression is spike S3's), sRGB, no streaming (KB 2D/02 §2), mips so a
        # 2x raster drawn at 1x does not alias.
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tex.set_editor_property("srgb", True)
        tex.set_editor_property("never_stream", True)
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
        eal.save_asset(path, only_if_is_dirty=False)
        count += 1
    log("S1 setup: %d textures ready" % count)


def new_material(name):
    path = ROOT + "/" + name
    if eal.does_asset_exist(path):
        log("S1 setup: %s exists, left as is" % path)
        return None
    m = tools.create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("used_with_instanced_static_meshes", True)
    return m


def sprite_material(name, a2c):
    m = new_material(name)
    if not m:
        return
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("use_alpha_to_coverage", a2c)
    tex = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -400, 0)
    tex.set_editor_property("parameter_name", "Tex")
    mel.connect_material_property(tex, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(tex, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    mel.recompile_material(m)
    eal.save_asset(m.get_path_name())
    log("S1 setup: created " + name)


def swatch_material():
    m = new_material("M_S1_Swatch")
    if not m:
        return
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    col = mel.create_material_expression(m, unreal.MaterialExpressionPerInstanceCustomData3Vector, -400, 0)
    col.set_editor_property("data_index", 0)
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    eal.save_asset(m.get_path_name())
    log("S1 setup: created M_S1_Swatch")


def spike_map():
    if eal.does_asset_exist(MAP):
        log("S1 setup: %s exists, left as is" % MAP)
        return
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.new_level(MAP):
        unreal.log_error("S1 setup: new_level failed")
        return
    cls = unreal.load_class(None, "/Script/Pawpocalypse.SpikeLookActor")
    unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(cls, unreal.Vector(0, 0, 0))
    levels.save_current_level()
    log("S1 setup: created " + MAP)


import_textures()
sprite_material("M_S1_Sprite", False)
sprite_material("M_S1_Sprite_A2C", True)
swatch_material()
spike_map()
log("S1 setup: done")
