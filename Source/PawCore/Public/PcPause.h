#pragma once

// The pause menu's rules (Godot pause_menu.gd; proven against the real pause_menu.tscn, fixture `pause`): its pages, the Save
// page's two-press overwrite, the quit button's two-press "Save and quit to menu" with its discard, the status line, and
// what each slot's caption says. The game module draws them (UPawPauseMenu) and does the saving and the leaving.
//
// AS GODOT HAS IT (finding F34, open): going back to the main page does NOT disarm an overwrite; only closing the menu does.

#include "PcCore.h"

#include <cstdint>

namespace Pc
{
	enum class PcPauseStatus : uint8_t { None, Occupied, Saved };

	struct PcPauseState
	{
		bool bOpen = false;
		bool bSlotsPage = false;
		int64_t PendingOverwrite = -1; // the slot armed by a first press, or -1
		bool bExitPending = false;     // the quit button armed: its caption is "Save and quit to menu", the discard shows
		PcPauseStatus Status = PcPauseStatus::None;
		int64_t StatusSlot = 0;
	};

	enum class PcPauseAction : uint8_t
	{
		None,
		SaveToSlot,   // save_game(slot): the slot becomes the active one
		SaveAndLeave, // save_game() then the title
		Leave,        // the title, nothing written
	};

	// open(): the main page, the status cleared.
	PAWCORE_API void PauseOpen(PcPauseState& S);
	// close(): the quit prompt and an armed overwrite disarmed on the way out.
	PAWCORE_API void PauseClose(PcPauseState& S);
	PAWCORE_API void PauseShowMain(PcPauseState& S);
	PAWCORE_API void PauseShowSlots(PcPauseState& S);
	// _on_slot_pressed(): an occupied slot (a file there, readable or not) not yet armed arms; anything else saves there and
	// goes back to the main page saying so.
	PAWCORE_API PcPauseAction PausePressSlot(PcPauseState& S, int64_t Slot, bool bOccupied);
	// _on_main_menu_pressed(): the first press arms, the second saves and leaves.
	PAWCORE_API PcPauseAction PausePressQuit(PcPauseState& S);
	// _on_discard_pressed(): leaves without writing.
	PAWCORE_API PcPauseAction PausePressDiscard(PcPauseState& S);
	// Leaving (_leave_to_menu): the prompt cleared.
	PAWCORE_API void PauseLeave(PcPauseState& S);

	// A slot's caption (_refresh_slots): "Slot n — empty", "Slot n — OVERWRITE? press again" or "Slot n — Reawaken r · time ·
	// mice", " (current)" after the number for the active slot. An unreadable file reads empty.
	enum class PcPauseSlotForm : uint8_t { Empty, Overwrite, Summary };
	struct PcPauseSlotLine
	{
		PcPauseSlotForm Form = PcPauseSlotForm::Empty;
		bool bCurrent = false;
	};
	PAWCORE_API PcPauseSlotLine PauseSlotLine(const PcPauseState& S, int64_t Slot, bool bReadable, int64_t ActiveSlot);

	// The Save page opens on the slot in play (_focus_slots).
	PAWCORE_API int64_t PauseSlotFocus(int64_t ActiveSlot, int64_t SlotCount);
}
