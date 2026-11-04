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
	constexpr double MultiplierButtonWidth = 96.0;   // BuyMultiplierButton custom_minimum_size
	constexpr double MultiplierButtonHeight = 34.0;
	constexpr double LabelPx = 17.0;                 // theme Label/font_sizes
	constexpr double ButtonPx = 16.0;                // theme Button/font_sizes
	constexpr double SmallPx = 15.0;                 // the zone and wave labels
	constexpr double OverFieldOutline = 2.0;         // _outline_over_playfield's 5 (Godot units): Slate's OutlineSize 5 drew
	                                                 // the 17 px labels as solid ink blocks (seen 2026-11-01); 2 reads alike
	constexpr double TabLabelPx = 13.0;              // the tab names until the tab icons arrive (phase 6): Godot's tabs are icons
	constexpr double TabStarPx = 18.0;               // _set_tab_star

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
	constexpr double CardFlavourPx = 13.0;
	constexpr double CardNamePx = 25.0;
	constexpr double CardRankPx = 13.0;
	constexpr double CardBodyPx = 17.0;
	constexpr double CardWarnPx = 14.0;
	constexpr double CardTakeHeight = 42.0;          // take custom_minimum_size.y
	constexpr double CardBanishHeight = 28.0;        // burn custom_minimum_size.y
	constexpr double CardBanishPx = 13.0;

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

	// The theme's WindowPanel (SBT_window: ui_window.svg, cream paper, a 3 px ink frame, inner corner radius 16; content margins
	// 22 / 20). Drawn as a rounded brush until the art arrives (phase 6), like the dock. Worn by the job picker and the victory
	// screen since F21 (designer 2026-11-03; Tests/Data/victory_look_godot.txt panel.style, jp.panel.style).
	constexpr double WindowPadX = 22.0;
	constexpr double WindowPadY = 20.0;
	constexpr double WindowRadius = 16.0;
	constexpr double WindowFrame = 3.0;

	// job_picker.gd
	constexpr double JobPanelWidth = 560.0;          // panel.custom_minimum_size.x
	constexpr double JobBodySeparation = 10.0;       // body separation
	constexpr double JobListSeparation = 6.0;        // _list separation
	constexpr double JobTitlePx = 24.0;              // title font_size
	constexpr double JobRowHeight = 62.0;            // btn.custom_minimum_size.y
	constexpr double JobBlurbPx = 14.0;              // desc font_size

	// victory_screen.gd (the game theme on its own root since F21; the column on a WindowPanel; Tests/Data/victory_look_godot.txt)
	constexpr double VictoryPanelWidth = 640.0;      // panel.custom_minimum_size.x
	constexpr double VictorySeparation = 18.0;       // box separation
	constexpr double VictoryHeadlinePx = 44.0;       // Headline font_size
	constexpr double VictoryTimePx = 28.0;           // TimeLabel font_size
	constexpr double VictoryOkWidth = 200.0;         // OkButton custom_minimum_size
	constexpr double VictoryOkHeight = 52.0;

	// input_device.gd
	constexpr double ModalGuardSec = 0.4;            // MODAL_GUARD_MSEC: a modal ignores presses this long after it opens
}
