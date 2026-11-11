#include "PcPause.h"

#include <algorithm>

namespace Pc
{
	void PauseOpen(PcPauseState& S)
	{
		S.bOpen = true;
		S.bSlotsPage = false;
		S.Status = PcPauseStatus::None;
	}

	void PauseLeave(PcPauseState& S)
	{
		// _clear_exit_prompt()
		S.PendingOverwrite = -1;
		S.bExitPending = false;
	}

	void PauseClose(PcPauseState& S)
	{
		// Disarm on the way out, or reopening finds it still armed and the next single press quits.
		PauseLeave(S);
		S.bOpen = false;
	}

	void PauseShowMain(PcPauseState& S)
	{
		// Leaving the Save page disarms its overwrite, as Godot's comment on _pending_overwrite promises: Godot left it armed,
		// so one press on the slot overwrote it (F34, a Godot fault the port does not copy: D16; the `pause` fixture is made
		// with the same fix, tools/godot_fixes.py).
		S.PendingOverwrite = -1;
		S.bSlotsPage = false;
	}

	void PauseShowSlots(PcPauseState& S)
	{
		S.bSlotsPage = true;
	}

	PcPauseAction PausePressSlot(PcPauseState& S, int64_t Slot, bool bOccupied)
	{
		// Saving into an OCCUPIED slot asks first (designer, 2026-08-23); an empty one does not.
		if (bOccupied && S.PendingOverwrite != Slot)
		{
			S.PendingOverwrite = Slot;
			S.Status = PcPauseStatus::Occupied;
			S.StatusSlot = Slot;
			return PcPauseAction::None;
		}
		S.PendingOverwrite = -1;
		PauseShowMain(S);
		S.Status = PcPauseStatus::Saved;
		S.StatusSlot = Slot;
		return PcPauseAction::SaveToSlot;
	}

	PcPauseAction PausePressQuit(PcPauseState& S)
	{
		if (!S.bExitPending)
		{
			S.bExitPending = true;
			return PcPauseAction::None;
		}
		return PcPauseAction::SaveAndLeave;
	}

	PcPauseAction PausePressDiscard(PcPauseState& S)
	{
		(void)S;
		return PcPauseAction::Leave;
	}

	PcPauseSlotLine PauseSlotLine(const PcPauseState& S, int64_t Slot, bool bReadable, int64_t ActiveSlot)
	{
		PcPauseSlotLine Out;
		Out.bCurrent = Slot == ActiveSlot;
		Out.Form = !bReadable ? PcPauseSlotForm::Empty : Slot == S.PendingOverwrite ? PcPauseSlotForm::Overwrite : PcPauseSlotForm::Summary;
		return Out;
	}

	int64_t PauseSlotFocus(int64_t ActiveSlot, int64_t SlotCount)
	{
		return std::clamp<int64_t>(ActiveSlot, 0, std::max<int64_t>(SlotCount - 1, 0));
	}
}
