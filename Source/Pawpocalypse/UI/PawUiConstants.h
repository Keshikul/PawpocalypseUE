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
	constexpr double ZoneBarHeight = 12.0;           // D18 (S4): the concept's 10 at 1600 (Godot _build_zone_progress: 14)
	constexpr double WaveBarHeight = 10.0;           // _build_wave_progress
	constexpr double XpBarBottom = 18.0;             // hud.tscn XPBar: offset_bottom = -18, offset_top = -54
	constexpr double XpBarHeight = 16.0;             // D18 (S4): the concept's 14 at 1600 (Godot XPBar/Bar: 24)
	constexpr double XpLevelWidth = 52.0;            // XPBar/LevelNow custom_minimum_size
	constexpr double XpDockGap = 16.0;               // _apply_dock_side: inset = _dock_width + 16
	constexpr double RowHeight = 46.0;               // hud.tscn PawDamageButton custom_minimum_size
	constexpr double RowGap = 4.0;                   // UpgradePanel separation
	constexpr double TabHeight = 46.0;               // TAB_ICON_SIZE.y
	constexpr double ReawakenGlowHz = 0.35;          // REAWAKEN_GLOW_HZ: the Reawaken tab's hue cycle while one is suggested
	constexpr double ReawakenGlowSaturation = 0.75;  // REAWAKEN_GLOW_SATURATION
	// D18 (S4, the UI concept's dock, scrUpgrades): each page's title heavy and on the left (Godot: centred, the label's 17);
	// the buy amount on its own row, a muted "Buy amount" and a switch of four (x1, x5, x10, Max) on a raised plate, the
	// chosen one yellow (Godot: one button that cycles, BuyMultiplierButton 96 x 34); the quick bar's buttons on plates.
	constexpr double DockTitlePx = 30.0;
	constexpr double AmountLabelPx = 16.0;
	constexpr double AmountButtonWidth = 64.0;
	constexpr double AmountButtonHeight = 40.0;
	constexpr double AmountGap = 4.0;
	constexpr double AmountPlatePad = 4.0;
	constexpr double AmountPlateRadius = 12.0;
	constexpr double AmountPlateEdgePx = 2.5;
	constexpr double QuickPlateHeight = 56.0;
	constexpr double QuickPlateIconPx = 34.0;
	constexpr double QuickPlateGap = 8.0;
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
	// D18 (S4, the UI concept's scrUpgrades): "Pounce" and its key on a plate beside the clock (Godot: the key alone).
	constexpr double PouncePlateHeight = 52.0;
	constexpr double PouncePlatePadX = 12.0;
	constexpr double PounceLabelPx = 20.0;
	constexpr double PounceLabelGap = 8.0;
	constexpr double RowPromptPx = 26.0;             // dock_navigator.gd _row_prompt.icon_px
	constexpr double RowPromptGap = 10.0;            // ROW_PROMPT_GAP: outside the row, on the field side
	constexpr double FocusPromptPx = 24.0;           // input_device.gd focus_prompt.icon_px
	constexpr double FocusPromptInset = 10.0;        // FOCUS_PROMPT_INSET: inside the button's right edge

	// tier_controller.gd: the boss panel
	constexpr double BossBannerWidth = 760.0;        // BOSS_BANNER_WIDTH
	constexpr double BossBannerTop = 18.0;           // BOSS_BANNER_TOP
	// D18 (S4, the UI concept's scrBoss): beside the status panel where at least BossFitWidth fits between it and the dock,
	// under it where not; BossGap from both. The SHIELDED tag a red pill; the bar the concept's.
	constexpr double BossGap = 16.0;
	constexpr double BossFitWidth = 480.0;
	constexpr double BossTagPadX = 12.0;
	constexpr double BossTagPadY = 3.0;
	constexpr double BossTagPx = 16.0;
	constexpr double BossBarRadius = 7.0;
	constexpr double BossBarHeight = 18.0;           // BOSS_BAR_HEIGHT

	// catches_pick_ui.gd: the card pick
	constexpr double CardWidth = 300.0;              // CARD_SIZE.x
	constexpr double CardHeight = 380.0;             // CARD_SIZE.y (a minimum: a long description grows the card)
	constexpr double CardHoverScale = 1.06;          // CARD_HOVER_SCALE
	constexpr double CardHoverSec = 0.09;            // CARD_HOVER_SEC
	constexpr double CardGap = 20.0;                 // _row separation
	constexpr double PickColumnGap = 18.0;           // column separation
	constexpr double PickEdgePad = 16.0;             // the column kept this far off the screen's edges when it shrinks to fit
	constexpr double PickTitlePx = 56.0;             // D18 (S4): LEVEL n as the concept's sticker title (Godot 40)
	constexpr double PickSubtitlePx = 18.0;          // D18 (S4): ink on the title panel (Godot 17, muted on the dim)
	constexpr double PickTitlePadX = 40.0;           // D18 (S4): the title panel's margins (the concept's 480 x 108 plate)
	constexpr double PickTitlePadY = 12.0;
	constexpr double PickTitleRadius = 22.0;
	constexpr double PickDimAlpha = 0.84;            // dim.color = Color(Palette.SKY["base"], 0.84)
	constexpr double RerollHeight = 46.0;            // _reroll_button custom_minimum_size.y
	constexpr double RerollIconPx = 26.0;            // D18 (S4): the Reawaken arrow on Reroll (the concept's)
	constexpr double CardFramePx = 3.0;              // style.set_border_width_all(3)
	constexpr double CardRadius = 20.0;              // D18 (S4): the concept's card corner (Godot 6)
	constexpr double CardBandRadius = 17.0;          // D18 (S4): the card's corner inside its 3 px frame (Godot 4)
	constexpr double CardBandPadX = 17.0;            // D18 (S4): the flavour 20 px in from the card's edge (Godot 14)
	constexpr double CardBandPadY = 10.0;            // D18 (S4): the concept's 44 px band (Godot 8)
	constexpr double CardInnerPadX = 13.0;           // D18 (S4): the art panel 16 px in from the card's edge (Godot 18)
	constexpr double CardBottomPad = 16.0;           // style.content_margin_bottom
	constexpr double CardSeparation = 10.0;          // box / body_box separation
	constexpr double CardFlavourPx = 16.0; // R4b: the 16 px text floor (Godot 13)
	constexpr int32 CardFlavourSpacing = 100;        // D18 (S4): the flavour word spaced out (the concept's 1.6 px at 15 px)
	constexpr double CardNamePx = 28.0;              // D18 (S4): under the art, as the concept's (Godot 25, on the band)
	// D18 (S4): the picture on a panel tinted with the flavour (the concept's: the band's colour 55 % of the way to white).
	constexpr double CardArtTint = 0.55;
	constexpr double CardArtPanelRadius = 12.0;
	constexpr double CardArtPanelEdgePx = 2.5;
	constexpr double CardArtPanelPadY = 22.0;        // 104 px of picture in a 150 px panel
	// D18 (S4): the level as dots beside "LEVEL n of max", filled to the level taking the card gives (the concept's).
	constexpr double CardDotPx = 12.0;
	constexpr double CardDotGap = 6.0;
	constexpr double CardDotEdgePx = 2.0;
	// D18 (S4): NEW on the concept's red starburst over the band's right end (centre 28 px in from the top and the right).
	constexpr double CardNewPx = 68.0;
	constexpr double CardNewInset = 28.0;
	constexpr double CardNewTextPx = 16.0;
	constexpr int32 CardNewTextOutline = 2;
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
	// Sticker Box (D18, S4, 2026-11-18): placed as the UI concept's scrCeremony (its 1600 x 900 frame x 1.2): the title at
	// the top, the cat in the middle, the payout on a panel below. Godot's: the headline 64 px at -120..-40, the payout
	// 28 px at -20..90, both 800 wide, on a flat flash.
	constexpr double CeremonyHeadlinePx = 96.0;      // a sticker title (the concept's 92 at its scale)
	constexpr double CeremonyPayoutPx = 28.0;        // Payout font_size
	constexpr double CeremonyTextHalfWidth = 420.0;  // the headline's box about the centre
	constexpr double CeremonyHeadlineTop = -430.0;   // Headline offsets about the centre
	constexpr double CeremonyHeadlineBottom = -300.0;
	constexpr double CeremonyPayoutHalfWidth = 290.0; // the payout's panel (the concept's 480 wide)
	constexpr double CeremonyPayoutTop = 250.0;      // Payout offsets about the centre
	constexpr double CeremonyPayoutBottom = 420.0;
	constexpr double CeremonyCatPx = 380.0;          // the cat's picture, square (the concept's stage-2 cat at 1.9)
	constexpr double CeremonyCatTop = -190.0;
	// The stage the three stand on, about the screen's centre: the headline's box wide, its top's distance from the centre
	// twice over tall, 20 to spare each way. Smaller screens (150 % UI: 720 tall) shrink it to fit; it never grows.
	constexpr double CeremonyStageWidth = 2.0 * CeremonyTextHalfWidth + 40.0;
	constexpr double CeremonyStageHeight = 2.0 * (-CeremonyHeadlineTop) + 40.0;
	constexpr int32 CeremonyTitleOutline = 5;        // the title's ink outline

	// The theme's WindowPanel (SBT_window: ui_window.svg, PawUi::Chrome Window since phase 6 slice 5a; content margins 22 / 20).
	// Worn by the job picker and the victory screen since F21 (designer 2026-11-03; Tests/Data/victory_look_godot.txt
	// panel.style, jp.panel.style).
	constexpr double WindowPadX = 22.0;
	constexpr double WindowPadY = 20.0;

	// job_picker.gd
	// D18 (S4, the UI concept's scrJobPick): a card per job, up to five in a row, the cat in the job's hat on its photo;
	// the panel shrinks to fit a small screen (the UI at 150 %). Godot: a 560 wide list of buttons.
	constexpr int32 JobCardsPerRow = 5;
	constexpr double JobCardWidth = 236.0;
	constexpr double JobCardGap = 16.0;
	constexpr double JobCardPad = 12.0;
	constexpr double JobCardRadius = 20.0;
	constexpr double JobPhotoPadY = 6.0;
	constexpr double JobPhotoRadius = 14.0;
	constexpr double JobCatPx = 132.0;
	constexpr double JobFitMargin = 24.0;
	constexpr double JobPanelWidth = JobCardsPerRow * JobCardWidth + (JobCardsPerRow - 1) * JobCardGap + 2.0 * (WindowPadX + 12.0);
	constexpr double JobBodySeparation = 10.0;       // body separation
	constexpr double JobListSeparation = 6.0;        // _list separation
	constexpr double JobTitlePx = 24.0;              // title font_size
	constexpr double JobRowHeight = 62.0;            // btn.custom_minimum_size.y
	constexpr double JobBlurbPx = 16.0;              // desc font_size (Godot 14; R4b: the 16 px floor)

	// victory_screen.gd (the game theme on its own root since F21; the column on a WindowPanel; Tests/Data/victory_look_godot.txt)
	constexpr double VictoryPanelWidth = 640.0;      // panel.custom_minimum_size.x
	constexpr double VictorySeparation = 18.0;       // box separation
	constexpr double VictoryHeadlinePx = 84.0;       // D18 (S4): the UI concept's sticker title (Godot: Headline font_size 44)
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
	constexpr double TitleScrollBarRoom = 12.0;      // a window's text wraps this much short of its width: the scroll bar's
	                                                 // place once it shows (the language pass, 2026-11-20: Dutch, 6 px over)
	constexpr double TitleCreditsHeight = 520.0;
	constexpr double TitleLeaderboardHeight = 440.0;
	constexpr double TitleConfirmWidth = 560.0;      // ConfirmPanel Window
	constexpr double TitleConfirmHeight = 240.0;
	constexpr double TitleConfirmSeparation = 16.0;  // its Body
	constexpr double TitleConfirmRowSeparation = 12.0;
	constexpr double TitleConfirmButtonWidth = 120.0; // Yes / Cancel custom_minimum_size
	constexpr double TitleConfirmButtonHeight = 40.0;
	// SlotPanel/Body: Godot's offsets -840 .. 840, -290 .. 290. The width is the row's (5 cards of 280, 4 gaps of 24; the row
	// is centred in Godot's 1680, so nothing moves): a screen narrower than it shrinks the slots to fit by the row, not by
	// 1680 (the language pass, 2026-11-20: at 1280 x 800 and 150 % UI the fifth slot was off the screen).
	constexpr double TitleSlotBodyWidth = 1496.0;
	constexpr double TitleSlotBodyHeight = 580.0;
	constexpr double TitleSlotSeparation = 24.0;     // Body and Row separation
	constexpr double TitleSlotTitlePx = 40.0;
	constexpr double TitleSlotBackHeight = 52.0;     // BackButton custom_minimum_size.y
	constexpr double SlotCardWidth = 280.0;          // SLOT_CARD_SIZE
	constexpr double SlotCardHeight = 380.0;         // D18 (S4): + the slot's number inside it (Godot 360, the number under it)
	constexpr double SlotTextMargin = 14.0;          // SLOT_TEXT_MARGIN
	constexpr double SlotDeleteHeight = 44.0;        // SLOT_DELETE_HEIGHT: the strip the Delete button sits in
	constexpr double SlotDeleteButtonHeight = 37.0;  // the button's own height there (the theme's minimum: the look file)
	constexpr double SlotNumberPx = 24.0;            // D18 (S4): inside the card at its top, the concept's (Godot 52, under it)
	constexpr double SlotNumberGap = 6.0;
	// D18 (S4, the UI concept's slotCard): the save's cat at its stage in a photo well at the card's top (grass for a save,
	// card stock for an empty slot, the off plate for one that cannot be read); the summary under it.
	constexpr double SlotPhotoPadY = 6.0;
	constexpr double SlotPhotoRadius = 12.0;
	constexpr double SlotPhotoEdgePx = 2.5;
	constexpr double SlotPhotoGap = 10.0;
	constexpr double SlotCatPx = 104.0;
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
	// main_menu.gd _open_language_menu (phase 7): a paper list above the Language button, one row per language.
	constexpr double LanguageMenuGap = 8.0;          // its foot 8 above the cluster
	constexpr double LanguageMenuPad = 6.0;          // the plate's content margin
	constexpr double LanguageRowWidth = 220.0;       // a row's custom_minimum_size (the list grows to its longest name)
	constexpr double LanguageRowHeight = 40.0;       // Godot 34; the port's 18 px label and the kit's row
	constexpr double LanguageRowSeparation = 2.0;    // the list's separation
	constexpr double LanguageTickPx = 20.0;          // the current language's tick (the check box's tick, CheckTickPx)

	// RESTYLE R4c (2026-11-08): the title after the canvas board "Main menu (plum)" (1600 x 900, drawn here x 1.2 at 1080p).
	// Godot's numbers above stay for what they still size; these place and size the plum title.
	constexpr double R4TitleMargin = 64.0;          // the logo's and the menu's left edge, the corners' margins
	constexpr double R4NameTop = 40.0;              // the logo: top left, tilted, Butter with an ink outline and a shadow
	constexpr double R4NameWidth = 1000.0;          // R4h: was 1100; the logo's letters are ~935 (an empty box past them left a 1153-wide screen)
	constexpr double R4NameHeight = 150.0;
	constexpr double R4NamePx = 112.0;
	constexpr double R4NameOutline = 8.0;
	constexpr double R4NameShadow = 8.0;
	constexpr double R4NameAngle = -3.0;            // degrees (R4c; D18 sets the logo upright: TitleLogoAngle)
	// D18 (S4, the UI concept's scrTitle): the logo centred at the top and upright, a sticker; the menu keeps its place.
	constexpr double TitleLogoTop = 40.0;             // R4NameTop: lower (the concept's) meets the menu at 150 % UI (Layout.Title)
	constexpr double TitleLogoAngle = 0.0;
	// D18 (S4): the concept's cast on the garden, the Tiny Kitten and three mice, each placed by its feet: offsets from the
	// screen's middle at 1080p (the concept's 1600 x 900 places x 1.2), the picture's size, and whether it is mirrored. The
	// two right-hand mice sit closer in than the concept's (552 and 384): at 1280 x 800 and 150 % UI the screen is 1153 wide.
	constexpr double TitleCatPx = 280.0;
	constexpr double TitleCatX = 24.0;
	constexpr double TitleCatY = 132.0;
	constexpr double TitleMousePx = 92.0;
	constexpr double TitleMouseX[3] = { 360.0, 500.0, 216.0 };
	constexpr double TitleMouseY[3] = { 84.0, 12.0, 180.0 };
	constexpr bool TitleMouseFlip[3] = { true, true, false };
	constexpr double R4PanelTop = -290.0;           // the menu: from the middle's height, on the left
	constexpr double R4PanelWidth = 460.0;
	constexpr double R4PanelHeight = 560.0;         // the shipped asset's (measured, R4h); the code said 530
	constexpr double R4MenuBottom = 115.0;          // R4h: the menu's room ends this far above the bottom (the hint's top is 103 up)
	constexpr double R4BobaRoomTop = 200.0;         // R4h: Boba's card's room: under the logo's row ...
	constexpr double R4BobaBottom = 140.0;          // ... to this far above the bottom (the version and the cluster below)
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
	constexpr double R4CurrencyPx = 30.0;          // the pills' numbers (D18, S4: the UI concept's 32 at 1600; R4: 24)
	// D18 (S4, the UI concept's HUD): the status panel under the pills (the biome's name, the next biome's line, the wave
	// line; Godot: two plates), the concept's bars, the level along the bottom on a plate with "Level n" before its bar.
	constexpr double FieldPlatePadX = 14.0;
	constexpr double FieldPlatePadY = 8.0;
	constexpr double FieldNamePx = 24.0;
	constexpr double FieldRowGap = 6.0;
	constexpr double StatusBarEdge = 2.0;
	constexpr double StatusBarRadius = 6.0;
	constexpr double XpPlatePadX = 14.0;
	constexpr double XpPlatePadY = 6.0;
	constexpr double XpLabelPx = 20.0;
	// D18 (S4, the UI concept's scrReawaken): the payout in its own box (the heading muted and spaced, "+n" big and purple
	// beside the Stardust star, the total beside it), the button yellow when it can be pressed.
	constexpr double PayoutPadX = 16.0;
	constexpr double PayoutPadY = 12.0;
	constexpr double PayoutRadius = 14.0;
	constexpr double PayoutEdgePx = 3.0;
	constexpr double PayoutHeadPx = 16.0;
	constexpr int32 PayoutHeadSpacing = 100;
	constexpr double PayoutNumberPx = 48.0;
	constexpr double PayoutIconPx = 48.0;
	// D18 (S4, the UI concept's scrAwards): each achievement on its plate, a progress bar before the numbers.
	constexpr double AwardPlatePadX = 10.0;
	constexpr double AwardPlatePadY = 6.0;
	constexpr double AwardPlateRadius = 10.0;
	constexpr double AwardPlateEdgePx = 2.5;
	constexpr double AwardBarHeight = 10.0;
	constexpr double AwardBarGap = 10.0;
	constexpr double AwardIconFaded = 0.45;   // the trophy of a row not yet earned
	constexpr double R4WavePadX = 12.0;
	constexpr double R4WavePadY = 6.0;
	constexpr double R4BarEdge = 2.0;              // a bar's Deep Plum edge

	// RESTYLE R4e (2026-11-08): the dock's shop rows after the board: taller, the words on the left, a little bigger. The
	// board's icon tiles and price pills wait for art (R6) and words (the designer): Godot's one line stays.
	constexpr double R4RowHeight = 56.0;           // Godot 46; twelve rows and the header now fill the dock, no void
	constexpr double R4RowPx = 18.0;
	constexpr double ShopIconPx = 30.0;            // D18 (S4): an upgrade's icon before its name (the concept's .72 x 36, at 1080p)

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
	constexpr double PauseTitlePx = 48.0;            // D18 (S4): PAUSED as the concept's sticker title
	constexpr double SettingsTitlePx = 40.0;         // D18 (S4): SETTINGS as the concept's sticker title (Godot: the label's 17)
	constexpr double PauseIconPx = 28.0;             // D18 (S4): each button's icon (the concept's .62 of 36, at 1080p)
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
	// D18 (S4, the UI concept's scrSettings): the table's head on a raised band, in muted, spaced capitals; every other row
	// on a faint band (the raised colour at 55 %).
	constexpr double ControlsHeadPadX = 8.0;
	constexpr double ControlsHeadPadY = 6.0;
	constexpr double ControlsHeadRadius = 10.0;
	constexpr double ControlsHeadEdgePx = 2.5;
	constexpr int32 ControlsHeadSpacing = 100;
	constexpr double ControlsRowBandAlpha = 0.55;
	constexpr double ControlsRowPadX = 8.0;
	constexpr int32 ControlsIconPx = 26;             // a cell's prompt (PromptIcon icon_px)
	// Godot's panel is a ScrollContainer that fills what the page leaves (size_flags_vertical EXPAND_FILL); here a box that
	// high, which is what the 980 px window leaves under the prompt and menu-side rows (the asset owns it).
	constexpr double ControlsScrollHeight = 480.0;

	// Phase 6 slice 5b: icon sizes
	constexpr double CardArtHeight = 104.0;          // catches_pick_ui.gd ART_HEIGHT (the card's picture)
	constexpr double NodeIconPx = 56.0;              // skill_tree_ui.gd ICON_SIZE (a node's picture)
}
