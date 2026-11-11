// THE PAD-ONLY PLAYTHROUGH (docs/09 L4 `padplay`; Godot ci/check_padplay.gd, gate 18): the Steam Deck rule "all content is
// reachable with the default controller configuration", as an assertion. From the title screen, with nothing but pad buttons
// and a trigger sent THROUGH SLATE (FSlateApplication: the path XInput's events take, so focus, navigation, Common UI and the
// device subsystem all see them), it starts a new game, buys in the shop (a press, a held trigger, the buy amount), takes a
// card, travels, summons the first boss with Bait, Reawakens, buys in the tree, visits Settings through the pause menu, saves
// and quits to the title, which then offers Continue. At every screen it walks the selection with the D-pad and checks it is
// never lost (THE FOCUS TOUR).
//
// WHAT IS SEEDED: as Godot's, the economy (Catnip, kills, the XP that deals a hand, a run worth Reawakening) is set directly
// where earning it would take an hour; every DECISION is made with the pad.
//
//   UnrealEditor.exe <uproject> -game -windowed -PawSaveDir=<sandbox> -ExecCmds="paw.PadPlay"     (tools/padplay_tour.py)
//
// Each check logs "PADPLAY PASS|FAIL <label>"; the end logs "PADPLAY RESULT PASS|FAIL <n> checks" and quits.

#include "Containers/Ticker.h"
#include "Core/PawGameSubsystem.h"
#include "Core/PawLog.h"
#include "Core/PawPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "Input/PawInputDeviceSubsystem.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "PcCardDraw.h"
#include "PcData.h"
#include "PcField.h"
#include "PcHud.h"
#include "PcStats.h"
#include "Realm/PawRealmSubsystem.h"
#include "Save/PawSaveSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawCardPickScreen.h"
#include "UI/PawHudScreen.h"
#include "UI/PawMainMenu.h"
#include "UI/PawPauseMenu.h"
#include "UI/PawPrompts.h"
#include "UI/PawRootLayout.h"
#include "UI/PawSettingsDialog.h"
#include "UI/PawSkillTreeScreen.h"
#include "UnrealClient.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#if !UE_BUILD_SHIPPING

namespace
{
	constexpr int32 Pad = 0;
	constexpr int32 PressFrames = 3;   // a button held this many frames, then let go
	constexpr int32 SettleFrames = 6;  // after a press: Slate's focus and the screens catch up
	constexpr int32 SceneFrames = 900; // a map load or a screen opening: up to ~15 s at 60 fps

	APawPlayerController* PC()
	{
		UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
		return World ? Cast<APawPlayerController>(World->GetFirstPlayerController()) : nullptr;
	}

	UPawHudScreen* Hud() { APawPlayerController* P = PC(); return P ? P->GetHud() : nullptr; }
	UPawMainMenu* Title() { APawPlayerController* P = PC(); return P ? P->GetMainMenu() : nullptr; }

	UPawRealmSubsystem* Realm()
	{
		UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
		return World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
	}

	UPawGameSubsystem* Game() { UPawRealmSubsystem* R = Realm(); return R ? R->GetGame() : nullptr; }

	UGameInstance* GI()
	{
		UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
		return World ? World->GetGameInstance() : nullptr;
	}

	bool Focused(const UWidget* W)
	{
		return W && (W->HasAnyUserFocus() || W->HasFocusedDescendants());
	}

	// Whatever has the pad's focus, if anything (the focus tour's question: never nothing).
	bool SomethingFocused()
	{
		return FSlateApplication::IsInitialized() && FSlateApplication::Get().GetUserFocusedWidget(0).IsValid();
	}

	void SendKey(const FKey& Key, bool bDown)
	{
		const FKeyEvent E(Key, FModifierKeysState(), FInputDeviceId::CreateFromInternalId(Pad), false, 0, 0, TOptional<int32>(0));
		if (bDown)
		{
			FSlateApplication::Get().ProcessKeyDownEvent(E);
		}
		else
		{
			FSlateApplication::Get().ProcessKeyUpEvent(E);
		}
	}

	void SendAxis(const FKey& Axis, float Value)
	{
		FSlateApplication::Get().ProcessAnalogInputEvent(FAnalogInputEvent(Axis, FModifierKeysState(),
			FInputDeviceId::CreateFromInternalId(Pad), false, 0, 0, Value, TOptional<int32>(0)));
	}

	// One step: Run is called every frame until it returns true (or its frame budget runs out: a FAIL, and the run stops).
	struct FStep
	{
		FString Name;
		TFunction<bool()> Run;
		int32 Budget = 1;
	};

	struct FPadPlay
	{
		TArray<FStep> Steps;
		int32 Index = 0;
		int32 Frames = 0;      // frames spent in the current step
		int32 Hold = 0;        // frames to wait before the next step
		int32 Checks = 0;
		TArray<FString> Failures;
		bool bDone = false;
		// scratch the steps share
		int64 Level = 0;
		int64 Count = 0;
		int32 Presses = 0;
		double HeldSeconds = 0.0;
		std::string Biome;

		void Check(const FString& Label, bool bOk, const FString& Detail = FString())
		{
			++Checks;
			UE_LOG(LogPaw, Display, TEXT("PADPLAY %s %s%s"), bOk ? TEXT("PASS") : TEXT("FAIL"), *Label, Detail.IsEmpty() ? TEXT("") : *(TEXT(" - ") + Detail));
			if (!bOk)
			{
				Failures.Add(Label + (Detail.IsEmpty() ? FString() : TEXT(" - ") + Detail));
			}
		}

		// --- the step builders ---
		void Do(const FString& Name, TFunction<void()> Fn, int32 Wait = SettleFrames)
		{
			Steps.Add({ Name, [this, Fn = MoveTemp(Fn), Wait]() { Fn(); Hold = Wait; return true; }, 1 });
		}

		void Until(const FString& Name, TFunction<bool()> Cond, int32 Budget = SceneFrames)
		{
			Steps.Add({ Name, MoveTemp(Cond), Budget });
		}

		void Press(const FKey& Key, int32 Wait = SettleFrames)
		{
			Do(TEXT("press ") + Key.ToString(), [Key]() { SendKey(Key, true); }, PressFrames);
			Do(TEXT("let go ") + Key.ToString(), [Key]() { SendKey(Key, false); }, Wait);
		}

		// Press a direction until Target has the focus, up to Limit presses (_walk_to).
		void WalkTo(const FString& Label, TFunction<UWidget*()> Target, const FKey& Key, int32 Limit = 30)
		{
			Do(TEXT("walk ") + Label, [this]() { Presses = 0; }, 0);
			Steps.Add({ TEXT("walk ") + Label, [this, Label, Target, Key, Limit]()
			{
				if (Frames % (PressFrames + SettleFrames + 2) != 0)
				{
					return false; // a press in flight
				}
				if (Focused(Target()))
				{
					Check(Label, true);
					return true;
				}
				if (Presses >= Limit)
				{
					Check(Label, false, FString::Printf(TEXT("%d presses"), Presses));
					return true;
				}
				// The focus never lost on the way (the focus tour).
				if (!SomethingFocused())
				{
					Check(Label + TEXT(": the focus was lost"), false);
					return true;
				}
				++Presses;
				SendKey(Key, true);
				SendKey(Key, false);
				return false;
			}, (Limit + 2) * (PressFrames + SettleFrames + 2) });
		}

		// The focus tour: every direction pressed N times on this screen, and the focus is never lost.
		void FocusTour(const FString& Screen, TFunction<bool()> StillUp, int32 Each = 4)
		{
			const FKey Directions[] = { EKeys::Gamepad_DPad_Down, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_DPad_Left };
			Do(TEXT("tour ") + Screen, [this]() { Presses = 0; }, 0);
			Steps.Add({ TEXT("tour ") + Screen, [this, Screen, StillUp, Each, Directions]()
			{
				if (Frames % (SettleFrames + 2) != 0)
				{
					return false;
				}
				if (!SomethingFocused())
				{
					Check(Screen + TEXT(": the focus survives every direction"), false, FString::Printf(TEXT("lost after %d presses"), Presses));
					return true;
				}
				if (Presses >= Each * 4)
				{
					Check(Screen + TEXT(": the focus survives every direction"), StillUp(), StillUp() ? FString() : TEXT("the screen went away"));
					return true;
				}
				const FKey Key = Directions[Presses / Each];
				++Presses;
				SendKey(Key, true);
				SendKey(Key, false);
				return false;
			}, Each * 4 * (SettleFrames + 2) + 4 });
		}

		void Expect(const FString& Label, TFunction<bool()> Cond, TFunction<FString()> Detail = nullptr)
		{
			Do(Label, [this, Label, Cond, Detail]() { const bool bOk = Cond(); Check(Label, bOk, !bOk && Detail ? Detail() : FString()); }, 0);
		}

		bool Tick()
		{
			if (bDone)
			{
				return false;
			}
			if (Hold > 0)
			{
				--Hold;
				return true;
			}
			if (Index >= Steps.Num())
			{
				Finish();
				return false;
			}
			FStep& S = Steps[Index];
			const bool bOk = S.Run();
			++Frames;
			if (bOk)
			{
				++Index;
				Frames = 0;
			}
			else if (Frames > S.Budget)
			{
				Check(S.Name, false, TEXT("timed out: the run stops here"));
				Finish();
				return false;
			}
			return true;
		}

		void Finish()
		{
			bDone = true;
			UE_LOG(LogPaw, Display, TEXT("PADPLAY RESULT %s %d checks, %d failed"), Failures.Num() == 0 ? TEXT("PASS") : TEXT("FAIL"), Checks, Failures.Num());
			for (const FString& F : Failures)
			{
				UE_LOG(LogPaw, Display, TEXT("PADPLAY FAILED %s"), *F);
			}
			FPlatformMisc::RequestExitWithStatus(false, Failures.Num() == 0 ? 0 : 1);
		}

		void Build();
	};

	TUniquePtr<FPadPlay> GPlay;
	FTSTicker::FDelegateHandle GPlayHandle;

	int64 UpgradeLevel(const char* Id)
	{
		const UPawGameSubsystem* G = Game();
		if (!G)
		{
			return -1;
		}
		const auto It = G->Progress().UpgradeLevels.find(Id);
		return It == G->Progress().UpgradeLevels.end() ? 0 : It->second;
	}

	void FPadPlay::Build()
	{
		// --- the title screen ---------------------------------------------------------------------------------------------
		Until(TEXT("the title screen loads"), []() { UPawMainMenu* M = Title(); return M && M->IsActivated(); });
		Do(TEXT("settle"), []() {}, 30);
		Press(EKeys::Gamepad_DPad_Down);
		Press(EKeys::Gamepad_DPad_Up);
		Expect(TEXT("the prompts are the pad's"), []()
		{
			const UGameInstance* G = GI();
			const UPawInputDeviceSubsystem* D = G ? G->GetSubsystem<UPawInputDeviceSubsystem>() : nullptr;
			return D && D->GetFamily() == Pc::PcFamily::Xbox;
		});
		Expect(TEXT("the title opens on New Game (no save yet)"), []() { return Title() && Focused(Title()->GetMenuButton(1)); });
		FocusTour(TEXT("the title"), []() { return Title() && Title()->IsActivated(); });
		WalkTo(TEXT("back to New Game"), []() -> UWidget* { return Title() ? Title()->GetMenuButton(1) : nullptr; }, EKeys::Gamepad_DPad_Up, 8);
		Press(EKeys::Gamepad_FaceButton_Bottom);
		Expect(TEXT("A on New Game opens the save slots"), []() { return Title() && Title()->IsLayerShown(UPawMainMenu::ELayer::Slots); });
		Press(EKeys::Gamepad_FaceButton_Bottom);
		Until(TEXT("A on a slot starts a new game in the Backyard"), []()
		{
			UPawHudScreen* H = Hud();
			const UPawRealmSubsystem* R = Realm();
			return H && H->IsActivated() && R && R->IsRunning();
		});
		Do(TEXT("settle"), []() {}, 30);

		// --- the shop ------------------------------------------------------------------------------------------------------
		Do(TEXT("SEEDED: the shop's first buys, without the hour"), []()
		{
			if (UPawGameSubsystem* G = Game())
			{
				G->Progress().Catnip = 1.0e7;
			}
			if (UPawHudScreen* H = Hud())
			{
				H->RefreshNow();
			}
		});
		Do(TEXT("level before"), [this]() { Level = UpgradeLevel("paw_damage"); }, 0);
		Press(EKeys::Gamepad_FaceButton_Bottom);
		Expect(TEXT("the first A selects a row (a shop row is focused)"), []()
		{
			UPawHudScreen* H = Hud();
			for (int32 I = 0; H && I < 20; ++I)
			{
				if (Focused(H->GetRowButton(I)) || Focused(H->GetMultiplierButton()))
				{
					return true;
				}
			}
			return false;
		});
		WalkTo(TEXT("D-pad reaches Paw Damage"), []() -> UWidget* { return Hud() ? Hud()->GetRowButton(0) : nullptr; }, EKeys::Gamepad_DPad_Down);
		// THE PRESS PROMPT AFTER THE TRAVEL (review CR1, 2026-11-10): the prompt was made and added to the viewport once, on the
		// title, and the viewport removes a game-instance widget with the world it was added in, so after the travel to the
		// realm keys and pad players never saw it again. Here, after that travel: on screen, at the focused dock row.
		Expect(TEXT("the press prompt is on screen at the focused dock row (after the travel from the title)"), []()
		{
			const UGameInstance* G = GI();
			const UPawInputDeviceSubsystem* D = G ? G->GetSubsystem<UPawInputDeviceSubsystem>() : nullptr;
			const UPawPromptIcon* P = D ? D->GetFocusPrompt() : nullptr;
			return P && P->IsInViewport() && P->GetVisibility() == ESlateVisibility::HitTestInvisible;
		}, []()
		{
			const UGameInstance* G = GI();
			const UPawInputDeviceSubsystem* D = G ? G->GetSubsystem<UPawInputDeviceSubsystem>() : nullptr;
			const UPawPromptIcon* P = D ? D->GetFocusPrompt() : nullptr;
			return P ? FString::Printf(TEXT("in the viewport %d, visibility %d"), P->IsInViewport() ? 1 : 0, static_cast<int32>(P->GetVisibility()))
				: FString(TEXT("no prompt"));
		});
		Do(TEXT("a picture of the dock and its prompt"), []()
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("PadPlay/dock_prompt.png"), true, false);
		}, 10);
		Do(TEXT("level"), [this]() { Level = UpgradeLevel("paw_damage"); }, 0);
		Press(EKeys::Gamepad_FaceButton_Bottom);
		Expect(TEXT("A buys the selected row"), [this]() { return UpgradeLevel("paw_damage") == Level + 1; },
			[this]() { return FString::Printf(TEXT("%lld -> %lld"), Level, UpgradeLevel("paw_damage")); });
		// Holding LT keeps buying (hold-to-buy: PcDockRepeat). The trigger's analog value, and its digital stand-in, as XInput.
		Do(TEXT("level"), [this]() { Level = UpgradeLevel("paw_damage"); HeldSeconds = 0.0; }, 0);
		Do(TEXT("LT down"), []() { SendAxis(EKeys::Gamepad_LeftTriggerAxis, 1.0f); SendKey(EKeys::Gamepad_LeftTrigger, true); }, 0);
		Until(TEXT("LT held 1.4 s"), [this]() { HeldSeconds += FApp::GetDeltaTime(); return HeldSeconds >= 1.4; }, 600);
		Do(TEXT("LT up"), []() { SendAxis(EKeys::Gamepad_LeftTriggerAxis, 0.0f); SendKey(EKeys::Gamepad_LeftTrigger, false); });
		Expect(TEXT("holding LT keeps buying"), [this]() { return UpgradeLevel("paw_damage") >= Level + 3; },
			[this]() { return FString::Printf(TEXT("%lld -> %lld"), Level, UpgradeLevel("paw_damage")); });
		FocusTour(TEXT("the dock"), []() { return Hud() && !Hud()->IsMenuUp(); }, 3);
		Do(TEXT("multiplier"), [this]() { Count = Hud() ? Hud()->GetBuyMultiplier() : -1; }, 0);
		Press(EKeys::Gamepad_FaceButton_Left);
		Expect(TEXT("X changes the buy amount"), [this]() { return Hud() && Hud()->GetBuyMultiplier() != Count; });

		// --- a card -----------------------------------------------------------------------------------------------------------
		Do(TEXT("SEEDED: the level-up that deals a hand"), [this]()
		{
			UPawRealmSubsystem* R = Realm();
			UPawGameSubsystem* G = Game();
			if (R && G)
			{
				Count = static_cast<int64>(G->Progress().CardLevels.size());
				Pc::PcRngCardRolls Rolls(R->GetSim().Field.Seed, ++R->GetSim().Field.Deals);
				Pc::OfferHand(G->Data(), G->Progress(), Rolls, false);
			}
		}, 10);
		Until(TEXT("a level-up deals a hand"), []() { return Hud() && Hud()->IsCardPickUp(); }, 120);
		Expect(TEXT("the hand opens on a card"), []() { return Hud() && Focused(Hud()->GetCardPick()) && SomethingFocused(); });
		Press(EKeys::Gamepad_FaceButton_Bottom);
		Expect(TEXT("A inside the guard does not take a card"), []() { return Hud() && Hud()->IsCardPickUp(); });
		Do(TEXT("wait out the guard"), []() {}, 40);
		FocusTour(TEXT("the card pick"), []() { return Hud() && Hud()->IsCardPickUp(); }, 2);
		Do(TEXT("guard"), []() {}, 40);
		Press(EKeys::Gamepad_FaceButton_Bottom);
		Until(TEXT("A takes the card"), [this]()
		{
			const UPawGameSubsystem* G = Game();
			return Hud() && !Hud()->IsCardPickUp() && G && static_cast<int64>(G->Progress().CardLevels.size()) > Count;
		}, 120);

		// --- travel -------------------------------------------------------------------------------------------------------------
		Do(TEXT("SEEDED: the Catnip that opens biomes 2-3"), [this]()
		{
			if (UPawGameSubsystem* G = Game())
			{
				G->Progress().LifetimeCatnipThisCycle = 1.0e7;
				Biome = G->Progress().CurrentBiomeId;
			}
			if (UPawHudScreen* H = Hud())
			{
				H->RefreshNow();
			}
		});
		for (int32 I = 0; I < 6; ++I)
		{
			Do(TEXT("RB to Realms"), []()
			{
				if (Hud() && Hud()->GetActiveTab() != Pc::PcDockTab::Realms)
				{
					SendKey(EKeys::Gamepad_RightShoulder, true);
					SendKey(EKeys::Gamepad_RightShoulder, false);
				}
			});
		}
		Expect(TEXT("RB reaches the Realms tab"), []() { return Hud() && Hud()->GetActiveTab() == Pc::PcDockTab::Realms; });
		WalkTo(TEXT("the D-pad reaches the third realm"), []() -> UWidget* { return Hud() ? Hud()->GetRealmButton(2) : nullptr; }, EKeys::Gamepad_DPad_Down);
		Press(EKeys::Gamepad_FaceButton_Bottom, 30);
		Until(TEXT("A travels there"), [this]() { const UPawGameSubsystem* G = Game(); return G && G->Progress().CurrentBiomeId != Biome; }, 300);

		// --- the first boss ------------------------------------------------------------------------------------------------------
		Do(TEXT("SEEDED: the kills that earn the Bait"), []()
		{
			if (UPawGameSubsystem* G = Game())
			{
				G->Progress().TierKillCount = 100000;
				G->Progress().Catnip = 1.0e7;
			}
			if (UPawHudScreen* H = Hud())
			{
				H->RefreshNow();
			}
		});
		Expect(TEXT("the Realms tab survived the travel"), []() { return Hud() && Hud()->GetActiveTab() == Pc::PcDockTab::Realms; });
		Expect(TEXT("the Bait button is live"), []()
		{
			const UPawButton* B = Hud() ? Hud()->GetBaitButton() : nullptr;
			return B && B->GetVisibility() != ESlateVisibility::Collapsed && !B->IsLocked();
		});
		WalkTo(TEXT("the D-pad reaches Bait"), []() -> UWidget* { return Hud() ? Hud()->GetBaitButton() : nullptr; }, EKeys::Gamepad_DPad_Up);
		Press(EKeys::Gamepad_FaceButton_Bottom, 20);
		Expect(TEXT("A summons the first boss"), []() { const UPawRealmSubsystem* R = Realm(); return R && R->GetSim().Field.bBossUp; });

		// --- Reawaken ---------------------------------------------------------------------------------------------------------
		Do(TEXT("SEEDED: a run deep enough to Reawaken"), [this]()
		{
			if (UPawGameSubsystem* G = Game())
			{
				G->Progress().LifetimeCatnipEver = 1.0e12;
				G->Progress().LifetimeCatnipThisCycle = 1.0e12;
				Count = G->Progress().ReawakenCount;
			}
			if (UPawHudScreen* H = Hud())
			{
				H->RefreshNow();
			}
		});
		for (int32 I = 0; I < 8; ++I)
		{
			Do(TEXT("LB to Reawaken"), []()
			{
				if (Hud() && Hud()->GetActiveTab() != Pc::PcDockTab::Reawaken)
				{
					SendKey(EKeys::Gamepad_LeftShoulder, true);
					SendKey(EKeys::Gamepad_LeftShoulder, false);
				}
			});
		}
		Expect(TEXT("LB reaches the Reawaken tab"), []() { return Hud() && Hud()->GetActiveTab() == Pc::PcDockTab::Reawaken; });
		WalkTo(TEXT("the D-pad reaches Reawaken"), []() -> UWidget* { return Hud() ? Hud()->GetReawakenButton() : nullptr; }, EKeys::Gamepad_DPad_Up);
		Press(EKeys::Gamepad_FaceButton_Bottom);
		Expect(TEXT("the first A only arms it, with the selection on Cancel"), [this]()
		{
			const UPawGameSubsystem* G = Game();
			return G && G->Progress().ReawakenCount == Count && Hud() && Focused(Hud()->GetCancelReawakenButton());
		});
		Press(EKeys::Gamepad_DPad_Up);
		Expect(TEXT("up from Cancel is Reawaken"), []() { return Hud() && Focused(Hud()->GetReawakenButton()); });
		Press(EKeys::Gamepad_FaceButton_Bottom);
		Expect(TEXT("the second A Reawakens"), [this]() { const UPawGameSubsystem* G = Game(); return G && G->Progress().ReawakenCount == Count + 1; });

		// --- the tree ---------------------------------------------------------------------------------------------------------
		Until(TEXT("the ceremony hands over to the tree"), []()
		{
			const UPawSkillTreeScreen* T = Hud() ? Hud()->GetSkillTree() : nullptr;
			return T && T->IsActivated() && T->IsSpendMode();
		}, 1200);
		Do(TEXT("settle"), []() {}, 20);
		Expect(TEXT("the tree opens on a node"), []() { return Hud() && Focused(Hud()->GetSkillTree()) && SomethingFocused(); });
		Do(TEXT("levels"), [this]()
		{
			Count = 0;
			if (const UPawGameSubsystem* G = Game())
			{
				for (const auto& B : G->Progress().BranchLevels)
				{
					Count += B.second;
				}
			}
		}, 0);
		Press(EKeys::Gamepad_FaceButton_Bottom);
		Expect(TEXT("A buys the selected node"), [this]()
		{
			int64 Now = 0;
			if (const UPawGameSubsystem* G = Game())
			{
				for (const auto& B : G->Progress().BranchLevels)
				{
					Now += B.second;
				}
			}
			return Now == Count + 1;
		});
		FocusTour(TEXT("the tree"), []() { const UPawSkillTreeScreen* T = Hud() ? Hud()->GetSkillTree() : nullptr; return T && T->IsActivated(); }, 2);
		Press(EKeys::Gamepad_FaceButton_Right);
		Expect(TEXT("the first B asks before starting the next run"), []()
		{
			const UPawSkillTreeScreen* T = Hud() ? Hud()->GetSkillTree() : nullptr;
			return T && T->IsActivated() && T->IsCloseArmed();
		});
		Press(EKeys::Gamepad_FaceButton_Right, 30);
		Until(TEXT("B leaves the tree; the new run starts"), []()
		{
			const UPawSkillTreeScreen* T = Hud() ? Hud()->GetSkillTree() : nullptr;
			return (!T || !T->IsActivated()) && Hud() && !Hud()->IsMenuUp();
		}, 600);
		Do(TEXT("settle"), []() {}, 30);

		// --- Settings, through the pause menu --------------------------------------------------------------------------------
		Press(EKeys::Gamepad_Special_Right);
		Expect(TEXT("Start pauses"), []() { const UPawPauseMenu* P = Hud() ? Hud()->GetPauseMenu() : nullptr; return P && P->IsActivated(); });
		Expect(TEXT("the pause menu opens on Continue"), []()
		{
			const UPawPauseMenu* P = Hud() ? Hud()->GetPauseMenu() : nullptr;
			return P && Focused(P->GetButton(TEXT("ContinueButton")));
		});
		FocusTour(TEXT("the pause menu"), []() { const UPawPauseMenu* P = Hud() ? Hud()->GetPauseMenu() : nullptr; return P && P->IsActivated(); }, 2);
		WalkTo(TEXT("the D-pad reaches Settings"), []() -> UWidget*
		{
			const UPawPauseMenu* P = Hud() ? Hud()->GetPauseMenu() : nullptr;
			return P ? P->GetButton(TEXT("SettingsButton")) : nullptr;
		}, EKeys::Gamepad_DPad_Down);
		Press(EKeys::Gamepad_FaceButton_Bottom, 12);
		Expect(TEXT("A opens Settings"), []() { const UPawPauseMenu* P = Hud() ? Hud()->GetPauseMenu() : nullptr; return P && P->IsSettingsUp(); });
		Press(EKeys::Gamepad_RightShoulder);
		Press(EKeys::Gamepad_RightShoulder);
		Expect(TEXT("RB turns to the Controls page"), []()
		{
			APawPlayerController* P = PC();
			UPawRootLayout* Root = P ? P->GetRootLayout() : nullptr;
			const UPawSettingsDialog* D = Root ? Cast<UPawSettingsDialog>(Root->ModalStack->GetActiveWidget()) : nullptr;
			return D && D->GetDialogState().Section == Pc::PcSettingsSection::Controls;
		});
		FocusTour(TEXT("the settings dialog"), []()
		{
			const UPawPauseMenu* P = Hud() ? Hud()->GetPauseMenu() : nullptr;
			return P && P->IsSettingsUp();
		}, 3);
		Press(EKeys::Gamepad_FaceButton_Right, 12);
		Expect(TEXT("B closes Settings and returns to the pause menu"), []()
		{
			const UPawPauseMenu* P = Hud() ? Hud()->GetPauseMenu() : nullptr;
			return P && P->IsActivated() && !P->IsSettingsUp();
		});
		Expect(TEXT("...with the selection back on Settings"), []()
		{
			const UPawPauseMenu* P = Hud() ? Hud()->GetPauseMenu() : nullptr;
			return P && Focused(P->GetButton(TEXT("SettingsButton")));
		});

		// --- save and quit ----------------------------------------------------------------------------------------------------
		WalkTo(TEXT("the D-pad reaches Quit to menu"), []() -> UWidget*
		{
			const UPawPauseMenu* P = Hud() ? Hud()->GetPauseMenu() : nullptr;
			return P ? P->GetButton(TEXT("MainMenuButton")) : nullptr;
		}, EKeys::Gamepad_DPad_Down);
		Press(EKeys::Gamepad_FaceButton_Bottom);
		Press(EKeys::Gamepad_FaceButton_Bottom, 20);
		Until(TEXT("A, A saves and quits to the title"), []() { UPawMainMenu* M = Title(); return M && M->IsActivated(); });
		Do(TEXT("settle"), []() {}, 30);
		Expect(TEXT("the save is on disk"), []()
		{
			UGameInstance* G = GI();
			UPawSaveSubsystem* S = G ? G->GetSubsystem<UPawSaveSubsystem>() : nullptr;
			return S && S->HasSlot(0);
		});
		Expect(TEXT("the title now opens on Continue"), []() { return Title() && Focused(Title()->GetMenuButton(0)); });

		// THE POINTER TAKES OVER (review CR10, 2026-11-10): with the pointer the last thing used the press prompt is hidden, and
		// the search of the window's widgets that places it is not made at all (it ran every frame a button had the focus).
		Do(TEXT("the pointer moves"), []()
		{
			FSlateApplication& S = FSlateApplication::Get();
			for (int32 I = 0; I < 3; ++I)
			{
				const FVector2D From(400.0 + I * 12.0, 300.0), To(412.0 + I * 12.0, 306.0);
				S.ProcessMouseMoveEvent(FPointerEvent(0, To, From, TSet<FKey>(), FKey(), 0.0f, FModifierKeysState()));
			}
		}, 10);
		Expect(TEXT("the pointer is the last thing used"), []()
		{
			const UGameInstance* G = GI();
			const UPawInputDeviceSubsystem* D = G ? G->GetSubsystem<UPawInputDeviceSubsystem>() : nullptr;
			return D && D->GetFocusPrompt() && D->GetFocusPrompt()->GetVisibility() == ESlateVisibility::Collapsed;
		});
		Do(TEXT("searches so far"), [this]()
		{
			const UGameInstance* G = GI();
			const UPawInputDeviceSubsystem* D = G ? G->GetSubsystem<UPawInputDeviceSubsystem>() : nullptr;
			Count = D ? D->GetPromptPathSearches() : -1;
		}, 20);
		Expect(TEXT("with the pointer last used, the prompt searches nothing"), [this]()
		{
			const UGameInstance* G = GI();
			const UPawInputDeviceSubsystem* D = G ? G->GetSubsystem<UPawInputDeviceSubsystem>() : nullptr;
			return D && D->GetPromptPathSearches() == Count;
		}, [this]()
		{
			const UGameInstance* G = GI();
			const UPawInputDeviceSubsystem* D = G ? G->GetSubsystem<UPawInputDeviceSubsystem>() : nullptr;
			return FString::Printf(TEXT("%lld searches in 20 frames"), D ? D->GetPromptPathSearches() - Count : -1);
		});
	}

	FAutoConsoleCommand PadPlayCommand(TEXT("paw.PadPlay"), TEXT("paw.PadPlay: the pad-only playthrough and the focus tour (logs PADPLAY ...; quits)."),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			if (GPlay.IsValid())
			{
				return;
			}
			GPlay = MakeUnique<FPadPlay>();
			GPlay->Build();
			UE_LOG(LogPaw, Display, TEXT("PADPLAY start: %d steps"), GPlay->Steps.Num());
			GPlayHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
			{
				return GPlay.IsValid() && GPlay->Tick();
			}));
		}));
}

#endif
