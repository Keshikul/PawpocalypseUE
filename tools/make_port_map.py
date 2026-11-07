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
    "autoload/audio_manager.gd": ("UPawAudioSubsystem + DA_AudioBank (Data/Audio*.csv)", 6, "built: rate limits, round robin, buses, crossfade; F40"),
    "autoload/boba_cameo.gd": ("UPawBobaSubsystem + cameo widget", 6, "menus only; Suppress() over the tree"),
    "autoload/card_manager.gd": ("PcCards + UPawGameSubsystem facade", 1, "XP curve constants -> CodeConstants.csv"),
    "autoload/combat_resolver.gd": ("PcCombat", 1, "the one damage path; keep every comment"),
    "autoload/game_state.gd": ("PcStats, PcEconomy, PcProgress, PcJobs + UPawGameSubsystem", 1, "2,976 lines: split by area; delegates mirror the signals"),
    "autoload/input_device.gd": ("UPawInputDeviceSubsystem (ULocalPlayerSubsystem)", 5, "family vs cursor source; warp echo; field owner"),
    "autoload/pack_loader.gd": ("- (not ported)", 0, "Unreal mounts paks; chunking in doc 10 §3"),
    "autoload/save_manager.gd": ("UPawSaveSubsystem + PcSave serialiser", 4, "temp/backup/CRC; coalesced request_save; branch split/merge migrations"),
    "autoload/settings_manager.gd": ("UPawGameUserSettings", 4, "per device; bindings via Enhanced Input user settings"),
    "autoload/skill_tree_manager.gd": ("PcTree", 1, "ported 2026-10-29; nodes keyed by branch id alone (no key parsing since 2026-08-21); never reset except new game"),
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
    "systems/art.gd": ("UPawSpriteAtlas::Find (by stem)", 6, "built: constructed names -> atlas cells"),
    "systems/aura_ring.gd": ("FPawVfx (aura ring)", 6, "built: Over band behind the cat; the Unicorn rainbow"),
    "systems/biome_backdrop.gd": ("PcBackdrop + APawRealmView::DrawBackdrop; UPawGarden (the title)", 6, "built: exact vs Godot (fixture backdrop)"),
    "systems/boss.gd": ("PcField boss + renderer", 2, "phases, shields, time limit from engagement"),
    "systems/boss_add.gd": ("PcField boss adds", 2, "heal the boss"),
    "systems/boss_telegraph.gd": ("FPawVfx (telegraph)", 6, "built: z 5"),
    "systems/catches_pick_ui.gd": ("UPawCardPickScreen", 3, "3 cards keep one; reroll"),
    "systems/cheat_overlay.gd": ("PcCheats + paw.Cheat.* commands; UPawCheatOverlay panel (dev only)", 2, "F1 panel in phase 3; compiled out of Shipping"),
    "systems/combat_stats.gd": ("PcStats::CombatStats", 1, ""),
    "systems/controls_panel.gd": ("UPawControlsPanel (settings) over PcControls", 5, "remapping rows, listening, system keys"),
    "systems/currency_icons.gd": ("PawUi::Icon + UPawIconText (wrap_label)", 6, "built (slice 5b): the icon beside every amount"),
    "systems/damage_zone_field.gd": ("PcField zones + FPawVfx (Under band)", 2, "MAX_DAMAGE_ZONES 64; drawn since phase 6"),
    "systems/dock_navigator.gd": ("UPawDock navigation (explicit nav rules)", 5, "one highlight; hover selects; repeat on shop rows only"),
    "systems/enemy.gd": ("PcField enemy rows + renderer", 2, "verbs, shields, golden flee, take_damage returns excess"),
    "systems/fonts.gd": ("F_Paw composite font", 7, "CJK/Thai faces"),
    "systems/golden_sparkle.gd": ("FPawVfx (sparkle)", 6, "built"),
    "systems/ground_shadow.gd": ("UPawSpriteRenderer shadow components (own Z band); PawLook::Shadow", 2, "one layer: batching; Godot art since phase 6"),
    "systems/hud.gd": ("UPawHudScreen + UPawDock + pages", 3, "3,040 lines; dirty flags at 15 Hz; dock side"),
    "systems/input_bindings.gd": ("PcBindings (PawCore) + PawInput::ApplyBindings (the live mapping context)", 5, "LOGICAL member lists, swaps, settings.cfg [bindings]"),
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
    "systems/poop_zone_visual.gd": ("FPawVfx (poop zone)", 6, "built: Under band"),
    "systems/pounce_gauge.gd": ("UPawPounceClock widget", 3, "disc with wedge, corner opposite the dock"),
    "systems/prompt_icon.gd": ("UPawPromptIcon", 5, ""),
    "systems/prompt_label.gd": ("UPawPromptText (rich text decorator)", 5, "action tokens"),
    "systems/rainbow_bar.gd": ("PcRainbowBar + UPawRainbowBar (the Reawaken page's bar)", 6, "built (slice 5d): exact vs Godot (fixture rainbow); Slate paint, no material"),
    "systems/rainbow_frame.gd": ("PcRainbow (RainbowFrameOutline) + UPawRainbowFrame", 6, "built (slice 5d): the zone plate's frame (Bait / a biome to go to); the Reawaken banner's waits for the banner"),
    "systems/reawaken_ceremony.gd": ("UPawReawakenCeremony", 3, ""),
    "systems/resource_dir.gd": ("- (not ported)", 0, "the .remap trap is Godot-only; Unreal's equivalent is the cooker (doc 12)"),
    "systems/settings_dialog.gd": ("UPawSettingsDialog", 4, "Audio / Video / Controls"),
    "systems/skill_tree_connector_layer.gd": ("UPawSkillTreeScreen NativePaint links", 3, "anti-aliased lines"),
    "systems/skill_tree_sky.gd": ("PcSky (PcBackdrop) + UPawTreeSky", 6, "built (slice 5d): exact to the float vs Godot (fixture rainbow)"),
    "systems/skill_tree_ui.gd": ("UPawSkillTreeScreen", 3, "2,396 lines: lattice layout, nav_pick, pan/zoom, states"),
    "systems/sprite_liveliness.gd": ("PawLook::PulseY / Face (CPU, per frame)", 6, "built"),
    "systems/tier_controller.gd": ("PcField + UPawRealmSubsystem + APawRealmView", 2, "3,008 lines: the field sim; split sim from view"),
    "systems/tutorial.gd": ("UPawTutorialOverlay", 6, "reads only; writes tutorial_done"),
    "systems/ui_juice.gd": ("PcJuice (+ TweenInterpolate) + FPawJuice (one per screen)", 6, "built (slice 5c): exact vs Godot (fixture juice); render transform only; F42"),
    "systems/unicorn_ceremony.gd": ("PcUnicornCeremony (+ PcTween) + UPawUnicornCeremony (the tree's top part)", 6, "built (slice 6a): exact vs Godot (fixture unicorn)"),
    "systems/upgrade_tooltip.gd": ("UPawUpgradeTooltip", 3, ""),
    "systems/vfx_beam.gd": ("FPawVfx::Beam", 6, "built: MAX_SPLASH_BEAMS_PER_FRAME"),
    "systems/vfx_burst.gd": ("FPawVfx::Burst", 6, "built: honours effects setting; F39"),
    "systems/vfx_edge_fire.gd": ("FPawVfx (Edge Burn, Ground band)", 6, "built: embers from Godot's seed"),
    "systems/vfx_shards.gd": ("FPawVfx::Shards", 6, "built: floor of one when reduced"),
    "systems/vfx_slam.gd": ("FPawVfx::Slam", 6, "built"),
    "systems/vfx_swipe.gd": ("FPawVfx::Swipe", 6, "built: ink-edged arc, three claw slashes"),
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
