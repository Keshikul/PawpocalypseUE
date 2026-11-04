"""Creates the game's two maps (docs/03 §4) as empty levels: no lights, no sky, nothing 3D.

L_Title holds the title screen, L_Realm the game. L_Realm also gets its one APawRealmView (docs/03 §3), with the quad and
the field materials (tools/create_field_materials.py, run first) set on the instance so the cook follows them: placed when
missing, and only unset references are filled; nothing else in the map is touched.
Only creates missing maps: new_level on an existing path keeps the old file while reporting success (KB U/17 §2),
and a builder never overwrites a map a human may have edited.

Headless, editor closed:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=C:/abs/path/tools/create_maps.py
      -nullrhi -unattended -nop4 -SCCProvider=None -stdout
"""
import unreal

MAPS = ["/Game/Pawpocalypse/Maps/L_Title", "/Game/Pawpocalypse/Maps/L_Realm"]

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
failed = False
for path in MAPS:
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log_warning(f"create_maps: {path} exists, left as is")
        continue
    if not levels.new_level(path) or not levels.save_current_level():
        unreal.log_error(f"create_maps: FAILED to create {path}")
        failed = True
        continue
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    unreal.log_warning(f"create_maps: created {path} ({len(actors)} actors: {sorted(a.get_class().get_name() for a in actors)})")

REALM = "/Game/Pawpocalypse/Maps/L_Realm"
VIEW_REFS = {
    "quad_mesh": "/Engine/BasicShapes/Plane.Plane",
    "flat_material": "/Game/Pawpocalypse/Materials/M_FieldFlat.M_FieldFlat",
    "soft_material": "/Game/Pawpocalypse/Materials/M_FieldSoft.M_FieldSoft",
    "disc_material": "/Game/Pawpocalypse/Materials/M_FieldDisc.M_FieldDisc",
    "ring_material": "/Game/Pawpocalypse/Materials/M_FieldRing.M_FieldRing",
}


def configure_realm():
    if not levels.load_level(REALM):
        unreal.log_error("create_maps: FAILED to load " + REALM)
        return False
    actors_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    views = [a for a in actors_sub.get_all_level_actors() if isinstance(a, unreal.PawRealmView)]
    if len(views) > 1:
        unreal.log_error("create_maps: %d realm views in %s; expected one (left as is)" % (len(views), REALM))
        return False
    changed = False
    if not views:
        views = [actors_sub.spawn_actor_from_class(unreal.PawRealmView, unreal.Vector(0.0, 0.0, 0.0))]
        views[0].set_actor_label("RealmView")
        changed = True
    view = views[0]
    for name, path in VIEW_REFS.items():
        if view.get_editor_property(name) is None:
            asset = unreal.EditorAssetLibrary.load_asset(path)
            if asset is None:
                unreal.log_error("create_maps: FAILED, %s is missing (py: tools/create_field_materials.py)" % path)
                return False
            view.set_editor_property(name, asset)
            changed = True
    if changed and not levels.save_current_level():
        unreal.log_error("create_maps: FAILED to save " + REALM)
        return False
    unreal.log_warning("create_maps: %s realm view %s" % (REALM, "configured" if changed else "already configured"))
    return True


if not failed and not configure_realm():
    failed = True

unreal.log_warning("create_maps: FAILED" if failed else "create_maps: done")
