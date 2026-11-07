#pragma once

// The title screen's rules (docs/05 §6; Godot main_menu.gd and the two pause_menu.gd formatters its slot cards use), as
// plain values the game module turns into words. Proven against the REAL main_menu.tscn (fixture `title`).

#include "PcCore.h"

#include <cstdint>
#include <string>

namespace Pc
{
	struct PcProgress;

	// _refresh(): Continue needs a save on disk or progress in memory; Load needs a save. The status line is "Save found -
	// {n} Reawaken(s), {mice} mice caught." (the game IN MEMORY's counts, whichever slot holds it) or "No save yet".
	struct PcTitleMenu
	{
		bool bContinue = false;
		bool bLoad = false;
		bool bSaveFound = false;
		int64_t Reawakens = 0;
		int64_t Kills = 0;
	};
	PAWCORE_API PcTitleMenu TitleMenu(bool bAnySlotOnDisk, const PcProgress& P);

	// PauseMenu.format_duration(): "{h}h {m}m" (m two digits), "{m}m {s}s" (s two digits), or "{s}s"; negative is 0.
	enum class PcClockForm : uint8_t { HoursMinutes, MinutesSeconds, Seconds };
	struct PcClock
	{
		PcClockForm Form = PcClockForm::Seconds;
		int64_t H = 0;
		int64_t M = 0;
		int64_t S = 0;
	};
	PAWCORE_API PcClock SlotClock(double Seconds);

	// PauseMenu._kills_text(): "{n}M mice" / "{n}K mice" with n as "%.1f", or "{n} mice".
	enum class PcKillsForm : uint8_t { Millions, Thousands, Plain };
	struct PcKills
	{
		PcKillsForm Form = PcKillsForm::Plain;
		std::string Number; // "1.0" / "389": the digits Godot prints
	};
	PAWCORE_API PcKills KillsText(int64_t Kills);

	// MainMenu.format_when(): how long ago a save was written. Unknown for a stamp <= 0; "just now" under a minute (a stamp
	// in the future too); whole minutes, hours, days (truncated); past a week, the UTC date of the stamp.
	enum class PcWhenForm : uint8_t { Unknown, JustNow, Minutes, Hours, Days, Date };
	struct PcWhen
	{
		PcWhenForm Form = PcWhenForm::Unknown;
		int64_t N = 0;
		int64_t Year = 0;
		int64_t Month = 0;
		int64_t Day = 0;
	};
	PAWCORE_API PcWhen WhenSaved(double Unix, double Now);

	// _refresh_leaderboard()'s last line: the first clear's time, a clear from before the clock existed, or none yet.
	enum class PcClearForm : uint8_t { Time, BeforeTheClock, NotYet };
	PAWCORE_API PcClearForm LeaderboardClear(const PcProgress& P);

	// _focus_top() on the slot screen: the card of the last slot used, clamped to the slots there are.
	PAWCORE_API int64_t SlotScreenFocus(int64_t LastSlot, int64_t SlotCount);
}
