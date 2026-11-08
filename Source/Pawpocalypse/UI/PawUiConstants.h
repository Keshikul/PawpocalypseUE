#pragma once

// The HUD's presentation constants (Data/CodeConstantsMap.csv destination "presentation"), each with its Godot source.
// Layout units are the field's logical units: 1080 tall, as wide as the window's aspect makes it (the UI's DPI curve maps
// a 1080-tall viewport to scale 1, so a UMG unit IS a field unit). Font sizes are Godot pixels; Slate takes points
// (PawUi::FontPt converts).

namespace PawUi
{
	// hud.gd
	constexpr double DockWidth = 568.0;              // hud.tscn UpgradeDock: offset_left = -568
	constexpr double EdgeMargin = 20.0;              // EDGE_MARGIN
	constexpr double HudRefreshHz = 15.0;            // HUD_REFRESH_HZ: dirty work spent at 15 Hz; a press refreshes at once
	constexpr double StatusRowWidth = 600.0;         // hud.tscn StatusPanel 20..620
	constexpr double StatusRowTop = 14.0;            // hud.tscn StatusPanel offset_top
	constexpr double CurrencyIconSize = 34.0;        // hud.tscn CatnipIcon custom_minimum_size
	constexpr double CurrencyLabelWidth = 104.0;     // hud.tscn CatnipLabel custom_minimum_size
	constexpr double ZonePlatePadX = 10.0;           // ZONE_PLATE_PAD
	constexpr double ZonePlatePadY = 6.0;
	constexpr double ZoneBarHeight = 14.0;           // _build_zone_progress: custom_minimum_size
	constexpr double WaveBarHeight = 10.0;           // _build_wave_progress
	constexpr double XpBarBottom = 18.0;             // hud.tscn XPBar: offset_bottom = -18, offset_top = -54
	constexpr double XpBarHeight = 24.0;             // XPBar/Bar custom_minimum_size
	constexpr double XpLevelWidth = 52.0;            // XPBar/LevelNow custom_minimum_size
	constexpr double XpDockGap = 16.0;               // _apply_dock_side: inset = _dock_width + 16
	constexpr double RowHeight = 46.0;               // hud.tscn PawDamageButton custom_minimum_size
	constexpr double RowGap = 4.0;                   // UpgradePanel separation
	constexpr double TabHeight = 46.0;               // TAB_ICON_SIZE.y
	constexpr double ReawakenGlowHz = 0.35;          // REAWAKEN_GLOW_HZ: the Reawaken tab's hue cycle while one is suggested
	constexpr double ReawakenGlowSaturation = 0.75;  // REAWAKEN_GLOW_SATURATION
	constexpr double MultiplierButtonWidth = 96.0;   // BuyMultiplierButton custom_minimum_size
	constexpr double MultiplierButtonHeight = 34.0;
	constexpr double LabelPx = 17.0;                 // theme Label/font_sizes
	constexpr double ButtonPx = 16.0;                // theme Button/font_sizes
	constexpr double SmallPx = 16.0;                 // the zone and wave labels (Godot 15; R4b: the 16 px floor)
	constexpr double OverFieldOutline = 2.0;         // _outline_over_playfield's 5 (Godot units): Slate's OutlineSize 5 drew
	                                                 // the 17 px labels as solid ink blocks (seen 2026-11-01); 2 reads alike
	constexpr double TabStarPx = 18.0;               // _set_tab_star
	// The restyle's titles and numbers (R2, the plum spec: "titles and numbers in the heavy weight with a Plum outline and a
	// solid drop shadow"): the shadow straight down, Deep Plum, on text that has an outline (light text: over the field now,
	// on the plum panels from R3). Sized by eye at 1080p; the morning sheet (build/look/night_20261107) shows 2, 3, 4.
	constexpr double HeavyShadowPx = 3.0;
	constexpr double TitleNameOutline = 4.0;     // the title screen's PAWPOCALYPSE over the lawn (R3), by eye

	// dock_navigator.gd: hold-to-buy on shop rows only
	constexpr double PressRepeatDelay = 0.35;        // PRESS_REPEAT_DELAY
	constexpr double PressRepeatStartHz = 4.0;       // PRESS_REPEAT_START_HZ
	constexpr double PressRepeatMaxHz = 12.0;        // PRESS_REPEAT_MAX_HZ
	constexpr double PressRampSec = 1.2;             // PRESS_RAMP_SEC

	// pounce_gauge.gd, and hud.gd's placement of it
	constexpr double PounceDiameter = 72.0;          // DIAMETER
	constexpr double PounceRing = 3.0;               // RING
	constexpr double PounceReadyRing = 5.0;          // READY_RING
	constexpr double PounceReadyHz = 0.9;            // READY_HZ
	constexpr double PounceWedgeAlpha = 0.62;        // WEDGE_ALPHA
	constexpr double PounceMargin = 18.0;            // POUNCE_GAUGE_MARGIN

	// Button prompts (hud.gd _build_dock_prompts, dock_navigator.gd, input_device.gd)
	constexpr double TabPromptPx = 26.0;             // TabPrevPrompt / TabNextPrompt icon_px
	constexpr double TabPromptGap = 4.0;             // the tab bar's separation beside them
	constexpr double DockLegendPromptPx = 22.0;      // dock_legend.icon_px
	constexpr double DockLegendPx = 16.0;            // dock_legend normal_font_size / bold_font_size (Godot 14; R4b: the 16 px floor)
	constexpr double PouncePromptPx = 28.0;          // pounce_prompt.icon_px
	constexpr double PouncePromptGap = 8.0;          // _place_pounce_gauge: -p.x - 8 / size.x + 8
	constexpr double RowPromptPx = 26.0;             // dock_navigator.gd _row_prompt.icon_px
	constexpr double RowPromptGap = 10.0;            // ROW_PROMPT_GAP: outside the row, on the field side
	constexpr double FocusPromptPx = 24.0;           // input_device.gd focus_prompt.icon_px
	constexpr double FocusPromptInset = 10.0;        // FOCUS_PROMPT_INSET: inside the button's right edge

	// tier_controller.gd: the boss panel
	constexpr double BossBannerWidth = 760.0;        // BOSS_BANNER_WIDTH
	constexpr double BossBannerTop = 18.0;           // BOSS_BANNER_TOP
	constexpr double BossBarHeight = 18.0;           // BOSS_BAR_HEIGHT

	// catches_pick_ui.gd: the card pick
	constexpr double CardWidth = 300.0;              // CARD_SIZE.x
	constexpr double CardHeight = 380.0;             // CARD_SIZE.y (a minimum: a long description grows the card)
	constexpr double CardHoverScale = 1.06;          // CARD_HOVER_SCALE
	constexpr double CardHoverSec = 0.09;            // CARD_HOVER_SEC
	constexpr double CardGap = 20.0;                 // _row separation
	constexpr double PickColumnGap = 18.0;           // column separation
	constexpr double PickTitlePx = 40.0;             // _title font_size
	constexpr double PickSubtitlePx = 17.0;          // _subtitle font_size
	constexpr double PickDimAlpha = 0.84;            // dim.color = Color(Palette.SKY["base"], 0.84)
	constexpr double RerollHeight = 46.0;            // _reroll_button custom_minimum_size.y
	constexpr double CardFramePx = 3.0;              // style.set_border_width_all(3)
	constexpr double CardRadius = 6.0;               // style.set_corner_radius_all(6)
	constexpr double CardBandRadius = 4.0;           // band_style.corner_radius_top_*
	constexpr double CardBandPadX = 14.0;            // band_style.content_margin_left/right
	constexpr double CardBandPadY = 8.0;             // band_style.content_margin_top/bottom
	constexpr double CardInnerPadX = 18.0;           // inner margin_left/right
	constexpr double CardBottomPad = 16.0;           // style.content_margin_bottom
	constexpr double CardSeparation = 10.0;          // box / body_box separation
	constexpr double CardFlavourPx = 16.0; // R4b: the 16 px text floor (Godot 13)
	constexpr double CardNamePx = 25.0;
	constexpr double CardRankPx = 16.0; // R4b: the 16 px text floor (Godot 13)
	constexpr double CardBodyPx = 17.0;
	constexpr double CardWarnPx = 16.0; // R4b: the 16 px text floor (Godot 14)
	constexpr double CardTakeHeight = 42.0;          // take custom_minimum_size.y
	constexpr double CardBanishHeight = 28.0;        // burn custom_minimum_size.y
	constexpr double CardBanishPx = 16.0; // R4b: the 16 px text floor (Godot 13)

	// reawaken_ceremony.gd / reawaken_ceremony.tscn
	constexpr double CeremonyFlashInSec = 0.35;      // FLASH_IN_SEC: the flash and the headline fade in
	constexpr double CeremonyPayoutInSec = 0.25;     // play(): the payout's tween_property(..., 0.25), a literal there
	constexpr double CeremonyHoldSec = 1.1;          // HOLD_SEC
	constexpr double CeremonyFadeOutSec = 0.45;      // FADE_OUT_SEC: all three fade out together
	constexpr double CeremonyHeadlinePx = 64.0;      // Headline theme_override_font_sizes/font_size
	constexpr double CeremonyPayoutPx = 28.0;        // Payout font_size
	constexpr double CeremonyTextHalfWidth = 400.0;  // both labels: offset_left = -400, offset_right = 400 about the centre
	constexpr double CeremonyHeadlineTop = -120.0;   // Headline offset_top / offset_bottom about the centre
	constexpr double CeremonyHeadlineBottom = -40.0;
	constexpr double CeremonyPayoutTop = -20.0;      // Payout offset_top / offset_bottom
	constexpr double CeremonyPayoutBottom = 90.0;

	// The theme's WindowPanel (SBT_window: ui_window.svg, PawUi::Chrome Window since phase 6 slice 5a; content margins 22 / 20).
	// Worn by the job picker and the victory screen since F21 (designer 2026-11-03; Tests/Data/victory_look_godot.txt
	// panel.style, jp.panel.style).
	constexpr double WindowPadX = 22.0;
	constexpr double WindowPadY = 20.0;

	// job_picker.gd
	constexpr double JobPanelWidth = 560.0;          // panel.custom_minimum_size.x
	constexpr double JobBodySeparation = 10.0;       // body separation
	constexpr double JobListSeparation = 6.0;        // _list separation
	constexpr double JobTitlePx = 24.0;              // title font_size
	constexpr double JobRowHeight = 62.0;            // btn.custom_minimum_size.y
	constexpr double JobBlurbPx = 16.0;              // desc font_size (Godot 14; R4b: the 16 px floor)

	// victory_screen.gd (the game theme on its own root since F21; the column on a WindowPanel; Tests/Data/victory_look_godot.txt)
	constexpr double VictoryPanelWidth = 640.0;      // panel.custom_minimum_size.x
	constexpr double VictorySeparation = 18.0;       // box separation
	constexpr double VictoryHeadlinePx = 44.0;       // Headline font_size
	constexpr double VictoryTimePx = 28.0;           // TimeLabel font_size
	constexpr double VictoryOkWidth = 200.0;         // OkButton custom_minimum_size
	constexpr double VictoryOkHeight = 52.0;

	// input_device.gd
	constexpr double ModalGuardSec = 0.4;            // MODAL_GUARD_MSEC: a modal ignores presses this long after it opens

	// main_menu.tscn / main_menu.gd (the title; Tests/Data/title_look_godot.txt). Offsets are the scene's, from its anchors.
	constexpr double TitleNamePx = 76.0;             // Title font_size
	constexpr double TitleNameTop = 90.0;            // Title offset_top (anchored top centre, 1000 x 100)
	constexpr double TitleNameWidth = 1000.0;
	constexpr double TitleNameHeight = 100.0;
	constexpr double TitleVersionWidth = 240.0;      // VersionLabel: offset_left -260 .. offset_right -20
	constexpr double TitleVersionHeight = 24.0;      // offset_top -44 .. offset_bottom -20
	constexpr double TitleVersionMargin = 20.0;
	constexpr double TitlePanelLeft = -230.0;        // Panel offsets from the centre: 460 x 440, its top 110 above the middle
	constexpr double TitlePanelTop = -110.0;
	constexpr double TitlePanelWidth = 460.0;
	constexpr double TitlePanelHeight = 440.0;
	constexpr double TitlePanelPadX = 28.0;          // StyleBoxFlat_panel content margins
	constexpr double TitlePanelPadY = 24.0;
	constexpr double TitlePanelRadius = 10.0;        // corner radius, a 3 px ink frame
	constexpr double TitlePanelFrame = 3.0;
	constexpr double TitleBodySeparation = 12.0;     // Body separation
	constexpr double TitleMenuSeparation = 8.0;      // Menu separation
	constexpr double TitleContinueHeight = 48.0;     // ContinueButton custom_minimum_size.y, font 22
	constexpr double TitleContinuePx = 22.0;
	constexpr double TitleButtonHeight = 40.0;       // the other menu buttons
	constexpr double TitleScrimAlpha = 0.8;          // the panels' Scrim: Color(SKY base, 0.8)
	constexpr double TitleSlotScrimAlpha = 0.88;     // SlotPanel/Scrim
	constexpr double TitleWindowTitlePx = 32.0;      // the credits' and leaderboard's Title
	constexpr double TitleWindowWidth = 760.0;       // CreditsPanel / LeaderboardPanel Window
	constexpr double TitleCreditsHeight = 520.0;
	constexpr double TitleLeaderboardHeight = 440.0;
	constexpr double TitleConfirmWidth = 560.0;      // ConfirmPanel Window
	constexpr double TitleConfirmHeight = 240.0;
	constexpr double TitleConfirmSeparation = 16.0;  // its Body
	constexpr double TitleConfirmRowSeparation = 12.0;
	constexpr double TitleConfirmButtonWidth = 120.0; // Yes / Cancel custom_minimum_size
	constexpr double TitleConfirmButtonHeight = 40.0;
	constexpr double TitleSlotBodyWidth = 1680.0;    // SlotPanel/Body: offsets -840 .. 840, -290 .. 290
	constexpr double TitleSlotBodyHeight = 580.0;
	constexpr double TitleSlotSeparation = 24.0;     // Body and Row separation
	constexpr double TitleSlotTitlePx = 40.0;
	constexpr double TitleSlotBackHeight = 52.0;     // BackButton custom_minimum_size.y
	constexpr double SlotCardWidth = 280.0;          // SLOT_CARD_SIZE
	constexpr double SlotCardHeight = 360.0;
	constexpr double SlotTextMargin = 14.0;          // SLOT_TEXT_MARGIN
	constexpr double SlotDeleteHeight = 44.0;        // SLOT_DELETE_HEIGHT: the strip the Delete button sits in
	constexpr double SlotDeleteButtonHeight = 37.0;  // the button's own height there (the theme's minimum: the look file)
	constexpr double SlotNumberPx = 52.0;            // SLOT_NUMBER_FONT
	constexpr double SlotColumnSeparation = 10.0;    // column separation
	constexpr double SlotHoverScale = 1.10;          // SLOT_HOVER_SCALE
	constexpr double SlotHoverSec = 0.12;            // the swell's tween
	constexpr double TitleHintWidth = 1000.0;        // the hint: offsets -500 .. 500, -60 .. -26 from the bottom centre
	constexpr double TitleHintBottom = 26.0;
	constexpr double TitleHintHeight = 34.0;
	constexpr double TitleHintPromptPx = 24.0;       // hint.icon_px
	constexpr double TitleCornerMargin = 24.0;       // the Boba badge and the cluster: 24 from the left and the bottom
	constexpr double BobaBadgeWidth = 167.0;         // as drawn in Godot (the look file): the photo's frame until the photo
	constexpr double BobaBadgeHeight = 183.0;        // arrives (phase 6) keeps its place
	constexpr double BobaBadgePad = 8.0;             // plate content margin
	constexpr double BobaBadgeRadius = 12.0;
	constexpr double BobaPhotoSize = 120.0;
	constexpr double BobaCaptionPx = 16.0; // R4b: the 16 px text floor (Godot 13)
	constexpr double BobaClusterGap = 14.0;          // the cluster starts 14 right of the badge
	constexpr double ClusterButtonSize = 56.0;       // CLUSTER_PX
	constexpr double ClusterGap = 10.0;              // CLUSTER_GAP
	constexpr double ClusterRadius = 10.0;
	constexpr double ClusterPad = 7.0;

	// RESTYLE R4c (2026-11-08): the title after the canvas board "Main menu (plum)" (1600 x 900, drawn here x 1.2 at 1080p).
	// Godot's numbers above stay for what they still size; these place and size the plum title.
	constexpr double R4TitleMargin = 64.0;          // the logo's and the menu's left edge, the corners' margins
	constexpr double R4NameTop = 40.0;              // the logo: top left, tilted, Butter with an ink outline and a shadow
	constexpr double R4NameWidth = 1100.0;
	constexpr double R4NameHeight = 150.0;
	constexpr double R4NamePx = 112.0;
	constexpr double R4NameOutline = 8.0;
	constexpr double R4NameShadow = 8.0;
	constexpr double R4NameAngle = -3.0;            // degrees
	constexpr double R4PanelTop = -290.0;           // the menu: from the middle's height, on the left
	constexpr double R4PanelWidth = 460.0;
	constexpr double R4PanelHeight = 530.0;
	constexpr double R4ContinueHeight = 66.0;       // Continue, the call: the board's 32 px x 1.2
	constexpr double R4ContinuePx = 36.0;
	constexpr double R4MenuButtonHeight = 50.0;     // the rest: the board's 24 px
	constexpr double R4MenuButtonPx = 26.0;
	constexpr double R4MenuSeparation = 10.0;
	constexpr double R4BobaRight = 72.0;            // Boba's card: on the right, tilted the other way
	constexpr double R4BobaTop = 40.0;              // from the middle's height
	constexpr double R4BobaWidth = 300.0;
	constexpr double R4BobaHeight = 360.0;
	constexpr double R4BobaPhoto = 220.0;
	constexpr double R4BobaNamePx = 34.0;
	constexpr double R4BobaAngle = 4.0;
	constexpr double R4HintPadX = 18.0;             // the hint's plate (the small plum plate, a pill on the lawn)
	constexpr double R4HintPadY = 6.0;
	constexpr double R4VersionAbove = 12.0;         // the version: right, above the cluster

	// RESTYLE R4d (2026-11-08): the HUD over the field, after the board "Original, plum menus": the currencies as plum pills
	// (a heavy Bone number on plum), the wave line on its plum plate, the bars with a raised track and a Deep Plum edge.
	constexpr double R4PillPadX = 12.0;
	constexpr double R4PillPadY = 4.0;
	constexpr double R4PillGap = 10.0;             // between the two pills
	constexpr double R4CurrencyPx = 24.0;          // the pills' numbers (the board's 32 px is its 1600-wide mock-up's)
	constexpr double R4WavePadX = 12.0;
	constexpr double R4WavePadY = 6.0;
	constexpr double R4BarEdge = 2.0;              // a bar's Deep Plum edge

	// RESTYLE R4e (2026-11-08): the dock's shop rows after the board: taller, the words on the left, a little bigger. The
	// board's icon tiles and price pills wait for art (R6) and words (the designer): Godot's one line stays.
	constexpr double R4RowHeight = 56.0;           // Godot 46; twelve rows and the header now fill the dock, no void
	constexpr double R4RowPx = 18.0;

	// pause_menu.tscn
	constexpr double PauseWindowWidth = 560.0;       // Window offsets -280 .. 280, -230 .. 230
	constexpr double PauseWindowHeight = 460.0;
	constexpr double PausePadX = 24.0;               // StyleBoxFlat_window content margins
	constexpr double PausePadY = 20.0;
	constexpr double PauseRadius = 10.0;             // corner radius, a 3 px ink frame
	constexpr double PauseFrame = 3.0;
	constexpr double PauseScrimAlpha = 0.8;          // Scrim: Color(SKY base, 0.8)
	constexpr double PauseBodySeparation = 12.0;     // Body separation, MainPage separation
	constexpr double PausePagesHeight = 360.0;       // Pages custom_minimum_size.y
	constexpr double PauseButtonHeight = 54.0;       // the main page's buttons
	constexpr double PauseSlotSeparation = 8.0;      // SlotPage / SlotList separation
	constexpr double PauseSlotHeight = 52.0;         // _build_slots: custom_minimum_size.y
	constexpr double PauseSlotBackHeight = 46.0;     // SlotBackButton

	// settings_dialog.tscn (the scrim, the window's plate and margins are the pause menu's)
	constexpr double SettingsWindowWidth = 1000.0;   // Window offsets -500 .. 500, -490 .. 490
	constexpr double SettingsWindowHeight = 980.0;
	constexpr double SettingsSectionSeparation = 6.0; // SectionBar separation
	constexpr double SettingsPageSeparation = 10.0;  // AudioPage / VideoPage / ControlsPage separation
	constexpr double SettingsSliderHeight = 24.0;    // MusicSlider / SfxSlider custom_minimum_size.y
	constexpr double SettingsRowHeight = 46.0;       // a theme Button's height (the textured stylebox's 16 px margins round a
	                                                 // 16 px label): the boxes, the lists, the section buttons, Close
	constexpr double SettingsCheckPx = 16.0;         // CheckBox / OptionButton font_size
	constexpr double SettingsRuleHeight = 4.0;       // the HSeparator: max(its separation 4, the style's minimum 2 + 2)

	// controls_panel.gd (Settings > Controls, below the prompt and menu-side rows)
	constexpr double ControlsSeparation = 8.0;       // Body / a row's separation
	constexpr double ControlsCellWidth = 132.0;      // CELL_SIZE
	constexpr double ControlsCellHeight = 40.0;
	constexpr double ControlsResetWidth = 92.0;      // RESET_WIDTH
	constexpr double ControlsSmallPx = 16.0;         // the column heads' and Reset's font_size (Godot 14; R4b: the 16 px floor)
	constexpr int32 ControlsIconPx = 26;             // a cell's prompt (PromptIcon icon_px)
	// Godot's panel is a ScrollContainer that fills what the page leaves (size_flags_vertical EXPAND_FILL); here a box that
	// high, which is what the 980 px window leaves under the prompt and menu-side rows (the asset owns it).
	constexpr double ControlsScrollHeight = 480.0;

	// Phase 6 slice 5b: icon sizes
	constexpr double CardArtHeight = 104.0;          // catches_pick_ui.gd ART_HEIGHT (the card's picture)
	constexpr double NodeIconPx = 56.0;              // skill_tree_ui.gd ICON_SIZE (a node's picture)
}
