"""Import the rasterised art (SourceArt/png/<group>/*.png, from tools/raster_svg.py) as textures (docs/07 §4).

    py tools/import_art.py --run [--group input]

Runs INSIDE the editor (headless, editor closed); --run finds the editor and refuses while one of this project's editors runs.
Each PNG becomes T_<stem> under the group's folder; the textures are GENERATED (the PNG is the source, the SVG its source):
an existing one is re-imported over, never edited by hand. A texture whose PNG is gone is reported, not deleted.

UI art (the input glyphs; since phase 6 slice 5a the chrome, the theme's nine-patches, in /Game/Pawpocalypse/Art/UI/Chrome,
drawn as Box brushes by PawUi::Chrome; since 5b the icons, /Game/Pawpocalypse/Art/UI/Icons, plus DA_Icons (UPawIconSet: every
stem -> its texture, rewritten whole each time, read by PawUi::Icon)): UserInterface2D (TC_EDITOR_ICON, uncompressed), sRGB, no mips, Never Stream, the UI texture group:
drawn 1:1 in the UI (docs/07 §4).

The field atlas (group `atlas`, phase 6; tools/pack_atlas.py): SourceArt/png/atlas/field_<n>.png -> T_Atlas_Field_<n> (BC7, sRGB,
mips, Never Stream, the World group: drawn smaller than rasterised, docs/07 §2.2 / §4) and DA_FieldAtlas (UPawSpriteAtlas: the
pages and every cell of field.csv), all in /Game/Pawpocalypse/Art/Field. The data asset is rewritten whole each time.

The biome floors (group `floors`, phase 6 slice 2): SourceArt/svg/backdrops/backdrop_tier_<n>.png (the generator's own PNGs,
512 px seamless surfaces) -> T_backdrop_tier_<n> in /Game/Pawpocalypse/Art/Backdrops: BC7, sRGB, mips (Godot imports them
with mipmaps and draws them at half size), WRAP, Never Stream, the World group.
"""

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EDITOR_CMD = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

# group -> the content folder its textures go to
GROUPS = {
    "input": "/Game/Pawpocalypse/Art/UI/Input",
    "chrome": "/Game/Pawpocalypse/Art/UI/Chrome",
    "icons": "/Game/Pawpocalypse/Art/UI/Icons",
    "atlas": "/Game/Pawpocalypse/Art/Field",
    "floors": "/Game/Pawpocalypse/Art/Backdrops",
}
ATLAS = "DA_FieldAtlas"


def import_atlas(dest, failed):
    import csv
    import unreal
    eal = unreal.EditorAssetLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    src = os.path.join(unreal.Paths.project_dir(), "SourceArt", "png", "atlas")
    pages = sorted((f for f in os.listdir(src) if f.startswith("field_") and f.endswith(".png")), key=lambda f: int(f[6:-4]))
    tasks = []
    for f in pages:
        t = unreal.AssetImportTask()
        t.filename = os.path.join(src, f)
        t.destination_path = dest
        t.destination_name = "T_Atlas_Field_" + f[6:-4]
        t.automated = True
        t.replace_existing = True
        t.save = False
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    textures = []
    for f in pages:
        path = dest + "/T_Atlas_Field_" + f[6:-4]
        tex = eal.load_asset(path)
        if not tex:
            failed.append("import " + path)
            continue
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_BC7)
        tex.set_editor_property("srgb", True)
        tex.set_editor_property("never_stream", True)
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
        tex.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
        tex.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
        if not eal.save_asset(path, only_if_is_dirty=False):
            failed.append("save " + path)
        textures.append(tex)
    path = dest + "/" + ATLAS
    atlas = eal.load_asset(path) if eal.does_asset_exist(path) else None
    if atlas is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.PawSpriteAtlas)
        atlas = tools.create_asset(ATLAS, dest, unreal.PawSpriteAtlas, factory)
    cells = []
    with open(os.path.join(src, "field.csv"), encoding="utf-8") as fh:
        for r in csv.DictReader(fh):
            c = unreal.PawAtlasCell()
            c.set_editor_property("id", r["id"])
            c.set_editor_property("page", int(r["page"]))
            for k in ("u0", "v0", "u1", "v1"):
                c.set_editor_property(k, float(r[k]))
            c.set_editor_property("native_width", float(r["native_w"]))
            c.set_editor_property("native_height", float(r["native_h"]))
            if r.get("rig_of"):                    # a rig layer (pack_atlas.py RIG_COLUMNS)
                c.set_editor_property("rig_of", r["rig_of"])
                c.set_editor_property("rig_order", int(r["rig_order"]))
                if r["motion"]:
                    c.set_editor_property("pivot_u", float(r["pivot_u"]))
                    c.set_editor_property("pivot_v", float(r["pivot_v"]))
                    c.set_editor_property("motion", r["motion"])
                    c.set_editor_property("amplitude", float(r["amp"]))
                    c.set_editor_property("hz", float(r["hz"]))
                    c.set_editor_property("phase", float(r["phase"]))
            cells.append(c)
    atlas.set_editor_property("pages", textures)
    atlas.set_editor_property("cells", cells)
    if not eal.save_asset(path, only_if_is_dirty=False):
        failed.append("save " + path)
    wanted = {"T_Atlas_Field_" + f[6:-4] for f in pages} | {ATLAS}
    for p in eal.list_assets(dest, recursive=False):
        if p.split(".")[-1] not in wanted:
            unreal.log_warning("IMPORT stray (no PNG): " + p)
    unreal.log_warning("IMPORT atlas: %d pages, %d cells in %s" % (len(textures), len(cells), dest))


def inside_editor(groups):
    import unreal
    eal = unreal.EditorAssetLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    log = unreal.log_warning
    failed = []
    for group in groups:
        dest = GROUPS[group]
        if group == "atlas":
            import_atlas(dest, failed)
            continue
        floors = group == "floors"
        src = os.path.join(unreal.Paths.project_dir(), "SourceArt", *(("svg", "backdrops") if floors else ("png", group)))
        pngs = sorted(f for f in os.listdir(src) if f.endswith(".png"))
        tasks = []
        for f in pngs:
            t = unreal.AssetImportTask()
            t.filename = os.path.join(src, f)
            t.destination_path = dest
            t.destination_name = "T_" + f[:-4]
            t.automated = True
            t.replace_existing = True
            t.save = False
            tasks.append(t)
        tools.import_asset_tasks(tasks)
        wanted = set()
        for f in pngs:
            path = dest + "/T_" + f[:-4]
            wanted.add("T_" + f[:-4])
            tex = eal.load_asset(path)
            if not tex:
                failed.append("import " + path)
                continue
            tex.set_editor_property("srgb", True)
            tex.set_editor_property("never_stream", True)
            if floors:
                tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_BC7)
                tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
                tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
                tex.set_editor_property("address_x", unreal.TextureAddress.TA_WRAP)
                tex.set_editor_property("address_y", unreal.TextureAddress.TA_WRAP)
            else:
                tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
                tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
                tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
            if not eal.save_asset(path, only_if_is_dirty=False):
                failed.append("save " + path)
        if group == "icons":
            path = dest + "/DA_Icons"
            wanted.add("DA_Icons")
            icons = eal.load_asset(path) if eal.does_asset_exist(path) else None
            if icons is None:
                factory = unreal.DataAssetFactory()
                factory.set_editor_property("data_asset_class", unreal.PawIconSet)
                icons = tools.create_asset("DA_Icons", dest, unreal.PawIconSet, factory)
            names = [f[:-4] for f in pngs]
            textures = [eal.load_asset(dest + "/T_" + n) for n in names]
            icons.set_editor_property("names", names)
            icons.set_editor_property("textures", textures)
            if not eal.save_asset(path, only_if_is_dirty=False):
                failed.append("save " + path)
        stray = [p for p in eal.list_assets(dest, recursive=False) if p.split(".")[-1] not in wanted]
        for p in stray:
            log("IMPORT stray (no PNG): " + p)
        log("IMPORT %s: %d textures in %s" % (group, len(pngs), dest))
    for f in failed:
        log("IMPORT FAILED " + f)
    log("IMPORT FAILED" if failed else "IMPORT DONE")


def from_shell(groups):
    sys.path.insert(0, str(ROOT / "tools"))
    import unreal_procs
    if unreal_procs.blocking():
        print("import_art: an Unreal process of this project is running; close it first")
        return 2
    script = (ROOT / "tools" / "import_art.py").as_posix()
    cmd = [EDITOR_CMD, str(ROOT / "Pawpocalypse.uproject"), "-run=pythonscript", "-script=%s %s" % (script, " ".join(groups)),
           "-nullrhi", "-unattended", "-nop4", "-SCCProvider=None", "-stdout"]
    r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    lines = [l[l.index("IMPORT "):] for l in r.stdout.splitlines() if "IMPORT " in l]
    for l in lines:
        print(l)
    if not any(l == "IMPORT DONE" for l in lines):
        log = ROOT / "build" / "import_art.log"
        log.parent.mkdir(exist_ok=True)
        log.write_text(r.stdout + "\n--- stderr ---\n" + r.stderr, encoding="utf-8")
        print("import_art: FAILED (log: %s)" % log)
        return 1
    return 0


if __name__ == "__main__":
    args = sys.argv[1:]
    groups = [a for a in args if a in GROUPS] or sorted(GROUPS)
    if "--run" in args:
        sys.exit(from_shell(groups))
    inside_editor(groups)
