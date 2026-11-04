"""SPIKE S4 (docs/04 §8) setup, throwaway: the map L_SpikeUi holding one ASpikeUiActor (the UI is built in C++).
Only creates what is missing. Headless, editor closed, -SCCProvider=None.
"""
import unreal

MAP = "/Game/Pawpocalypse/Dev/Spikes/S4/L_SpikeUi"
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    unreal.log_warning("S4 setup: map exists")
else:
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    ok = levels.new_level(MAP)
    cls = unreal.load_class(None, "/Script/Pawpocalypse.SpikeUiActor")
    unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(cls, unreal.Vector(0, 0, 0))
    levels.save_current_level()
    unreal.log_warning("S4 setup: created " + MAP if ok else "S4 setup: FAILED new_level")
