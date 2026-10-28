"""Write docs/appendix-b-godot-to-unreal-map.md: every Godot script, its size, and where it goes in Unreal.

The map below is hand-written (it is the porting decision); the line counts are read from the Godot repo so the table stays true.
A Godot script missing from MAP fails the run, so a new Godot file cannot be silently left out of the port.

    py tools/make_port_map.py --godot C:/Games/pawpocalypse --out docs/appendix-b-godot-to-unreal-map.md
"""
from __future__ import annotations

import argparse
import os
import sys

# godot path (under scripts/) -> (unreal destination, phase, note)
MAP = {
    "autoload/achievement_manager.gd": ("PcAchievements + UPawAchievementSubsystem", 3, "metrics read existing counters; claims pay Stardust (assert the balance moves)"),
    "autoload/audio_manager.gd": ("UPawAudioSubsystem + DT_AudioEvents", 6, "EVENTS table -> CSV; rate limits; buses"),
    "autoload/boba_cameo.gd": ("UPawBobaSubsystem + cameo widget", 6, "menus only; Suppress() over the tree"),
    "autoload/card_manager.gd": ("PcCards + UPawGameSubsystem facade", 1, "XP curve constants -> CodeConstants.csv"),
    "autoload/combat_resolver.gd": ("PcCombat", 1, "the one damage path; keep every comment"),
    "autoload/game_state.gd": ("PcStats, PcEconomy, PcProgress, PcJobs + UPawGameSubsystem", 1, "2,976 lines: split by area; delegates mirror the signals"),
    "autoload/input_device.gd": ("UPawInputDeviceSubsystem (ULocalPlayerSubsystem)", 5, "family vs cursor source; warp echo; field owner"),
    "autoload/pack_loader.gd": ("- (not ported)", 0, "Unreal mounts paks; chunking in doc 10 §3"),
    "autoload/save_manager.gd": ("UPawSaveSubsystem + PcSave serialiser", 4, "temp/backup/CRC; coalesced request_save; branch split/merge migrations"),
    "autoload/settings_manager.gd": ("UPawGameUserSettings", 4, "per device; bindings via Enhanced Input user settings"),
    "autoload/skill_tree_manager.gd": ("PcTree", 1, "parse keys from the RIGHT; never reset except new game"),
    "data/achievement_data.gd": ("FPawAchievementRow / PcAchievement", 1, ""),
    "data/boss_data.gd": ("FPawBossRow / PcBoss", 1, ""),
    "data/card_data.gd": ("FPawCardRow / PcCard", 1, "Effect, Cost, Flavour enums by name"),
    "data/combat_constants_data.gd": ("FPawCombatConstantsRow / PcCombatConstants", 1, ""),
    "data/enemy_data.gd": ("FPawEnemyRow / PcEnemy", 1, "MovementKind by name"),
    "data/evolution_data.gd": ("FPawEvolutionRow / PcEvolution + UPawVisualDefinition", 1, "texture/tint go to the visual definition"),
    "data/job_data.gd": ("FPawJobRow / PcJob", 1, ""),
    "data/pawpocalypse_save_data.gd": ("FPawSaveState (PcSave)", 4, "doc 08 §1"),
    "data/reawaken_constants_data.gd": ("FPawReawakenConstantsRow / PcReawakenConstants", 1, ""),
    "data/skill_branch_data.gd": ("FPawSkillNodeRow / PcSkillNode", 1, "EffectType by name; extra_effects JSON channels"),
    "data/tier_data.gd": ("FPawTierRow / PcTier", 1, ""),
    "data/upgrade_data.gd": ("FPawUpgradeRow / PcUpgrade", 1, "EffectType, TargetStat by name"),
    "systems/amounts.gd": ("PcFormat::Short + FText wrapper", 1, "suffixes translatable"),
    "systems/anim_sheet.gd": ("sprite material frame index (later)", 6, "no frame animation today"),
    "systems/art.gd": ("UPawVisualDefinition lookup by id", 6, "constructed names -> ids"),
    "systems/aura_ring.gd": ("M_FxShape ring instance", 6, ""),
    "systems/biome_backdrop.gd": ("APawRealmView backdrop: tiled floor material + prop instances", 6, "738 lines of layout; must tile to 32:9"),
    "systems/boss.gd": ("PcField boss + renderer", 2, "phases, shields, time limit from engagement"),
    "systems/boss_add.gd": ("PcField boss adds", 2, "heal the boss"),
    "systems/boss_telegraph.gd": ("M_FxShape telegraph", 6, ""),
    "systems/catches_pick_ui.gd": ("UPawCardPickScreen", 3, "3 cards keep one; reroll"),
    "systems/cheat_overlay.gd": ("UPawCheatOverlay (dev only)", 2, "F1; compiled out of Shipping"),
    "systems/combat_stats.gd": ("PcStats::CombatStats", 1, ""),
    "systems/controls_panel.gd": ("UPawControlsPanel (settings)", 5, "remapping rows, system keys"),
    "systems/currency_icons.gd": ("UPawCurrencyIcon widget", 3, "tooltips; HitTest only on icons"),
    "systems/damage_zone_field.gd": ("PcField zones + M_FxShape", 2, "MAX_DAMAGE_ZONES 64"),
    "systems/dock_navigator.gd": ("UPawDock navigation (explicit nav rules)", 5, "one highlight; hover selects; repeat on shop rows only"),
    "systems/enemy.gd": ("PcField enemy rows + renderer", 2, "verbs, shields, golden flee, take_damage returns excess"),
    "systems/fonts.gd": ("F_Paw composite font", 7, "CJK/Thai faces"),
    "systems/golden_sparkle.gd": ("sprite material glow + Niagara sparkle", 6, ""),
    "systems/ground_shadow.gd": ("shadow instanced component (own Z band)", 2, "one layer: batching"),
    "systems/hud.gd": ("UPawHudScreen + UPawDock + pages", 3, "3,040 lines; dirty flags at 15 Hz; dock side"),
    "systems/input_bindings.gd": ("UPawInputBindings (logical actions over Enhanced Input)", 5, "LOGICAL member lists"),
    "systems/input_glyphs.gd": ("UPawGlyphs + CommonInputBaseControllerData per family", 5, "no Nintendo"),
    "systems/job_picker.gd": ("UPawJobPicker", 3, ""),
    "systems/kitten.gd": ("PcField kittens + renderer", 2, "seek nearest via grid; roam; pool grows to owned"),
    "systems/locales.gd": ("language menu + culture code map", 7, "zh_Hans -> zh-Hans etc."),
    "systems/main_menu.gd": ("UPawMainMenu (L_Title)", 4, "slots, garden, Report a bug, Discord"),
    "systems/menu_focus.gd": ("UPawMenuFocus helper (explicit navigation wiring)", 5, "guard_ok held-Space guard"),
    "systems/palette.gd": ("PawTokens.h (generated by palette.py --cpp)", 0, "never hand-edit"),
    "systems/pause_menu.gd": ("UPawPauseMenu", 4, "save before quit"),
    "systems/player.gd": ("PcField cat + renderer (cat, weapon, hat)", 2, "stick cursor; fit constants; SPRITE_TARGET_PX"),
    "systems/poly_guard.gd": ("- (Godot polygon triangulation guard; not needed)", 0, "SDF shapes replace polygons"),
    "systems/poop_zone_visual.gd": ("M_FxShape poop zone", 6, ""),
    "systems/pounce_gauge.gd": ("UPawPounceClock widget", 3, "disc with wedge, corner opposite the dock"),
    "systems/prompt_icon.gd": ("UPawPromptIcon", 5, ""),
    "systems/prompt_label.gd": ("UPawPromptText (rich text decorator)", 5, "action tokens"),
    "systems/rainbow_bar.gd": ("UI material", 6, ""),
    "systems/rainbow_frame.gd": ("UI material", 6, "Bait ready plate"),
    "systems/reawaken_ceremony.gd": ("UPawReawakenCeremony", 3, ""),
    "systems/resource_dir.gd": ("- (not ported)", 0, "the .remap trap is Godot-only; Unreal's equivalent is the cooker (doc 12)"),
    "systems/settings_dialog.gd": ("UPawSettingsDialog", 4, "Audio / Video / Controls"),
    "systems/skill_tree_connector_layer.gd": ("UPawSkillTreeScreen NativePaint links", 3, "anti-aliased lines"),
    "systems/skill_tree_sky.gd": ("tree background material/widget", 6, ""),
    "systems/skill_tree_ui.gd": ("UPawSkillTreeScreen", 3, "2,396 lines: lattice layout, nav_pick, pan/zoom, states"),
    "systems/sprite_liveliness.gd": ("sprite material vertex pulse", 6, "per-instance phase"),
    "systems/tier_controller.gd": ("PcField + UPawRealmSubsystem + APawRealmView", 2, "3,008 lines: the field sim; split sim from view"),
    "systems/tutorial.gd": ("UPawTutorialOverlay", 6, "reads only; writes tutorial_done"),
    "systems/ui_juice.gd": ("UPawUiJuice (widget transforms/tweens)", 6, ""),
    "systems/unicorn_ceremony.gd": ("UPawUnicornCeremony", 6, "whinny"),
    "systems/upgrade_tooltip.gd": ("UPawUpgradeTooltip", 3, ""),
    "systems/vfx_beam.gd": ("M_FxShape capsule beams", 6, "MAX_SPLASH_BEAMS_PER_FRAME"),
    "systems/vfx_burst.gd": ("Niagara burst (pooled)", 6, "honours effects setting"),
    "systems/vfx_edge_fire.gd": ("Niagara embers + M_FxShape band", 6, "honours effects setting"),
    "systems/vfx_shards.gd": ("Niagara shards", 6, "floor of one when reduced"),
    "systems/vfx_slam.gd": ("M_FxShape ring", 6, ""),
    "systems/vfx_swipe.gd": ("M_FxShape arc + slashes", 6, "ink-edged arc, three claw slashes"),
    "systems/victory_screen.gd": ("UPawVictoryScreen", 3, "first-clear time"),
}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--godot", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    root = os.path.join(a.godot, "scripts")
    found = {}
    for d, _, files in os.walk(root):
        for fn in files:
            if fn.endswith(".gd"):
                rel = os.path.relpath(os.path.join(d, fn), root).replace(os.sep, "/")
                with open(os.path.join(d, fn), encoding="utf-8") as f:
                    found[rel] = sum(1 for _ in f)
    missing = sorted(set(found) - set(MAP))
    stale = sorted(set(MAP) - set(found))
    if missing:
        print("NOT MAPPED (add to MAP):", ", ".join(missing))
        return 1
    lines = [
        "# Appendix B - Godot script -> Unreal destination (generated)",
        "",
        "> Generated by `tools/make_port_map.py`; the destinations are the porting decisions of docs 03-08, the line counts are read",
        "> from the Godot repo. The script fails if a Godot script is not mapped. Phase = the roadmap phase that ports it (doc 11).",
        "> Also port: `ci/*.gd` gates -> doc 09 §3; `tools/*.py` -> docs 06-07 (pawsim, shop_step, palette, art generators kept).",
        "",
        f"{len(found)} scripts, {sum(found.values()):,} lines.",
        "",
        "| Godot script | lines | Unreal destination | phase | note |",
        "|---|---|---|---|---|",
    ]
    for rel in sorted(found):
        dest, phase, note = MAP[rel]
        lines.append(f"| `scripts/{rel}` | {found[rel]} | {dest} | {phase} | {note} |")
    if stale:
        lines += ["", "Mapped but no longer in Godot: " + ", ".join(stale)]
    with open(a.out, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    print(f"{len(found)} scripts mapped -> {a.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
