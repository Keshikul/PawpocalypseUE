#include "PcTitle.h"

#include "PcFormat.h"
#include "PcStats.h"

#include <algorithm>
#include <cmath>

namespace Pc
{
	PcTitleMenu TitleMenu(bool bAnySlotOnDisk, const PcProgress& P, PcTitleNotice Notice)
	{
		PcTitleMenu Out;
		const bool bProgress = P.LifetimeCatnipEver > 0.0 || P.ReawakenCount > 0;
		Out.bContinue = bAnySlotOnDisk || bProgress;
		Out.bLoad = bAnySlotOnDisk;
		Out.bSaveFound = bAnySlotOnDisk;
		Out.Reawakens = P.ReawakenCount;
		Out.Kills = P.LifetimeKills;
		Out.Notice = Notice;
		if (Notice == PcTitleNotice::NewerBuild)
		{
			// The game in memory is not that slot's, and every write to the slot is refused: playing it would be a session
			// that is never saved (CR2).
			Out.bContinue = false;
		}
		else if (Notice == PcTitleNotice::Unreadable)
		{
			// F47: memory holds a fresh game, not the slot's; the player chooses (that slot fresh, or another).
			Out.bContinue = true;
			Out.bContinueOpensSlots = true;
		}
		return Out;
	}

	PcClock SlotClock(double Seconds)
	{
		// int(maxf(seconds, 0.0)): toward zero.
		const int64_t Total = static_cast<int64_t>(std::max(Seconds, 0.0));
		PcClock Out;
		Out.H = Total / 3600;
		Out.M = (Total % 3600) / 60;
		Out.S = Total % 60;
		Out.Form = Out.H > 0 ? PcClockForm::HoursMinutes : Out.M > 0 ? PcClockForm::MinutesSeconds : PcClockForm::Seconds;
		return Out;
	}

	PcKills KillsText(int64_t Kills)
	{
		PcKills Out;
		if (Kills >= 1000000)
		{
			Out.Form = PcKillsForm::Millions;
			Out.Number = FixedLikeGodot(static_cast<double>(Kills) / 1000000.0, 1);
		}
		else if (Kills >= 1000)
		{
			Out.Form = PcKillsForm::Thousands;
			Out.Number = FixedLikeGodot(static_cast<double>(Kills) / 1000.0, 1);
		}
		else
		{
			Out.Form = PcKillsForm::Plain;
			Out.Number = std::to_string(Kills);
		}
		return Out;
	}

	namespace
	{
		// Days since 1970-01-01 to a proleptic Gregorian (year, month, day): Howard Hinnant's civil_from_days, the
		// arithmetic Godot's Time.get_datetime_dict_from_unix_time() does (UTC).
		void CivilFromDays(int64_t Z, int64_t& Y, int64_t& M, int64_t& D)
		{
			Z += 719468;
			const int64_t Era = (Z >= 0 ? Z : Z - 146096) / 146097;
			const int64_t Doe = Z - Era * 146097;
			const int64_t Yoe = (Doe - Doe / 1460 + Doe / 36524 - Doe / 146096) / 365;
			const int64_t Doy = Doe - (365 * Yoe + Yoe / 4 - Yoe / 100);
			const int64_t Mp = (5 * Doy + 2) / 153;
			D = Doy - (153 * Mp + 2) / 5 + 1;
			M = Mp < 10 ? Mp + 3 : Mp - 9;
			Y = Yoe + Era * 400 + (M <= 2 ? 1 : 0);
		}
	}

	PcWhen WhenSaved(double Unix, double Now)
	{
		PcWhen Out;
		if (!(Unix > 0.0))
		{
			Out.Form = PcWhenForm::Unknown;
			return Out;
		}
		const double Elapsed = Now - Unix;
		if (Elapsed < 60.0)
		{
			Out.Form = PcWhenForm::JustNow;
		}
		else if (Elapsed < 3600.0)
		{
			Out.Form = PcWhenForm::Minutes;
			Out.N = static_cast<int64_t>(Elapsed / 60.0);
		}
		else if (Elapsed < 86400.0)
		{
			Out.Form = PcWhenForm::Hours;
			Out.N = static_cast<int64_t>(Elapsed / 3600.0);
		}
		else if (Elapsed < 604800.0)
		{
			Out.Form = PcWhenForm::Days;
			Out.N = static_cast<int64_t>(Elapsed / 86400.0);
		}
		else
		{
			Out.Form = PcWhenForm::Date;
			const int64_t Seconds = static_cast<int64_t>(Unix); // int(unix)
			const int64_t Days = Seconds >= 0 ? Seconds / 86400 : (Seconds - 86399) / 86400;
			CivilFromDays(Days, Out.Year, Out.Month, Out.Day);
		}
		return Out;
	}

	PcClearForm LeaderboardClear(const PcProgress& P)
	{
		if (P.FinalBossSeconds > 0.0)
		{
			return PcClearForm::Time;
		}
		// Cleared, but before the clock existed (final_boss_seconds arrived 2026-08-20): telling such a player they have not
		// cleared it would be a lie.
		return P.bFinalBossDefeated ? PcClearForm::BeforeTheClock : PcClearForm::NotYet;
	}

	int64_t SlotScreenFocus(int64_t LastSlot, int64_t SlotCount)
	{
		return std::clamp<int64_t>(LastSlot, 0, std::max<int64_t>(SlotCount - 1, 0));
	}
}
