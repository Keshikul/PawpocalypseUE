#include "Input/PawInputMap.h"

#include "Input/PawHalfAxis.h"
#include "Input/PawInputDefaults.h"
#include "Input/PawKeyCodes.h"
#include "InputCoreTypes.h"
#include "PcSettings.h"
#include "UObject/Package.h"

#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace PawInput
{
	const TCHAR* const ActionsDir = TEXT("/Game/Pawpocalypse/Input/Actions");
	const TCHAR* const ContextPath = TEXT("/Game/Pawpocalypse/Input/IMC_Pawpocalypse");

	FString InputMapCsv()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Data/InputMap.csv"));
	}

	bool ReadInputMap(const FString& Path, TArray<FPawInputMapRow>& Out, FString& Error)
	{
		Out.Reset();
		TArray<FString> Lines;
		if (!FFileHelper::LoadFileToStringArray(Lines, *Path) || Lines.Num() < 2)
		{
			Error = TEXT("cannot read ") + Path;
			return false;
		}
		TArray<FString> Header;
		Lines[0].ParseIntoArray(Header, TEXT(","), false);
		auto Col = [&Header](const TCHAR* Name) { return Header.IndexOfByKey(FString(Name)); };
		const int32 CId = Col(TEXT("id")), CAction = Col(TEXT("action")), CDead = Col(TEXT("deadzone")), CDevice = Col(TEXT("device")),
			CEvent = Col(TEXT("godot_event")), CCode = Col(TEXT("godot_code")), CGSign = Col(TEXT("godot_axis_sign")),
			CKey = Col(TEXT("ue_key")), CSign = Col(TEXT("ue_axis_sign")), CMerged = Col(TEXT("merged"));
		for (const int32 C : { CId, CAction, CDead, CDevice, CEvent, CCode, CGSign, CKey, CSign, CMerged })
		{
			if (C == INDEX_NONE)
			{
				Error = TEXT("InputMap.csv: a column is missing (re-export: py tools/export_godot_data.py)");
				return false;
			}
		}
		for (int32 L = 1; L < Lines.Num(); ++L)
		{
			if (Lines[L].IsEmpty())
			{
				continue;
			}
			TArray<FString> C;
			Lines[L].ParseIntoArray(C, TEXT(","), false);
			if (C.Num() != Header.Num())
			{
				Error = FString::Printf(TEXT("InputMap.csv line %d: %d cells, the header has %d"), L + 1, C.Num(), Header.Num());
				return false;
			}
			FPawInputMapRow R;
			R.Id = C[CId];
			R.Action = C[CAction];
			R.DeadZone = FCString::Atof(*C[CDead]);
			R.Device = C[CDevice];
			R.GodotEvent = C[CEvent];
			R.GodotCode = FCString::Atoi64(*C[CCode]);
			R.GodotAxisSign = FCString::Atoi(*C[CGSign]);
			R.UeKey = FName(*C[CKey]);
			R.UeAxisSign = FCString::Atoi(*C[CSign]);
			R.bMerged = C[CMerged] == TEXT("1");
			Out.Add(MoveTemp(R));
		}
		return true;
	}

	FString ActionAssetName(const FString& GodotAction)
	{
		FString Out = TEXT("IA_");
		bool bUpper = true;
		for (const TCHAR Ch : GodotAction)
		{
			if (Ch == TEXT('_'))
			{
				bUpper = true;
				continue;
			}
			Out.AppendChar(bUpper ? FChar::ToUpper(Ch) : Ch);
			bUpper = false;
		}
		return Out;
	}

	bool IsAxisAction(const TArray<FPawInputMapRow>& Rows, const FString& GodotAction)
	{
		return Rows.ContainsByPredicate([&GodotAction](const FPawInputMapRow& R) { return R.Action == GodotAction && R.GodotEvent == TEXT("pad_axis"); });
	}

	UInputMappingContext* LoadContext()
	{
		const FString Name = FPaths::GetBaseFilename(ContextPath);
		return LoadObject<UInputMappingContext>(nullptr, *(FString(ContextPath) + TEXT(".") + Name));
	}

	UInputAction* LoadAction(const FString& GodotAction)
	{
		const FString Name = ActionAssetName(GodotAction);
		return LoadObject<UInputAction>(nullptr, *(FString(ActionsDir) / Name + TEXT(".") + Name));
	}

	// --- the live bindings -------------------------------------------------------------------------------------------

	namespace
	{
		struct FBindings
		{
			bool bBuilt = false;
			TMap<FKey, TArray<FString>> ByKey;
			struct FEntry { FKey Key; FString Action; bool bStandIn = false; };
			TArray<FEntry> InOrder; // in the context's order, a stand-in after its axis
			struct FHalf { FString Action; FKey Key; float Sign = 1.0f; float DeadZone = 0.5f; };
			TArray<FHalf> Halves;   // every axis half: which action, which key, which way, its dead zone
		};

		FBindings& Bindings()
		{
			static FBindings B;
			if (!B.bBuilt)
			{
				B.bBuilt = true;
				const UInputMappingContext* IMC = LiveContext();
				if (!IMC)
				{
					return B;
				}
				auto Add = [](const FKey& Key, const FString& Action, bool bStandIn)
				{
					TArray<FString>& List = B.ByKey.FindOrAdd(Key);
					if (!List.Contains(Action))
					{
						List.Add(Action);
						B.InOrder.Add({ Key, Action, bStandIn });
					}
				};
				for (const FEnhancedActionKeyMapping& M : IMC->GetMappings())
				{
					if (!M.Action)
					{
						continue;
					}
					// The builder writes the Godot action into the description (UPawInputAssetBuilder).
					const FString Action = M.Action->ActionDescription.ToString();
					Add(M.Key, Action, false);
					const UPawHalfAxis* Half = M.Modifiers.Num() == 1 ? Cast<UPawHalfAxis>(M.Modifiers[0]) : nullptr;
					if (Half)
					{
						B.Halves.Add({ Action, M.Key, Half->Sign, Half->DeadZone });
					}
					FKey Digital;
					if (Half && DigitalOfHalf(M.Key, Half->Sign > 0.0f ? 1 : -1, Digital))
					{
						Add(Digital, Action, true);
					}
				}
			}
			return B;
		}
	}

	bool DigitalOfHalf(const FKey& Axis, int32 Sign, FKey& OutDigital)
	{
		struct FHalf { FKey Axis; int32 Sign; FKey Digital; };
		static const FHalf Halves[] = {
			{ EKeys::Gamepad_LeftX, 1, EKeys::Gamepad_LeftStick_Right }, { EKeys::Gamepad_LeftX, -1, EKeys::Gamepad_LeftStick_Left },
			{ EKeys::Gamepad_LeftY, 1, EKeys::Gamepad_LeftStick_Up }, { EKeys::Gamepad_LeftY, -1, EKeys::Gamepad_LeftStick_Down },
			{ EKeys::Gamepad_RightX, 1, EKeys::Gamepad_RightStick_Right }, { EKeys::Gamepad_RightX, -1, EKeys::Gamepad_RightStick_Left },
			{ EKeys::Gamepad_RightY, 1, EKeys::Gamepad_RightStick_Up }, { EKeys::Gamepad_RightY, -1, EKeys::Gamepad_RightStick_Down },
			{ EKeys::Gamepad_LeftTriggerAxis, 1, EKeys::Gamepad_LeftTrigger }, { EKeys::Gamepad_RightTriggerAxis, 1, EKeys::Gamepad_RightTrigger },
		};
		for (const FHalf& H : Halves)
		{
			if (H.Axis == Axis && H.Sign == Sign)
			{
				OutDigital = H.Digital;
				return true;
			}
		}
		return false;
	}

	const TArray<FString>& ActionsOfKey(const FKey& Key)
	{
		static const TArray<FString> None;
		const TArray<FString>* Found = Bindings().ByKey.Find(Key);
		return Found ? *Found : None;
	}

	bool KeyIs(const FKey& Key, const TCHAR* GodotAction)
	{
		return ActionsOfKey(Key).Contains(GodotAction);
	}

	TArray<FKey> KeysOf(const TCHAR* GodotAction, bool bStandIns)
	{
		TArray<FKey> Out;
		for (const FBindings::FEntry& E : Bindings().InOrder)
		{
			if (E.Action == GodotAction && (bStandIns || !E.bStandIn))
			{
				Out.AddUnique(E.Key);
			}
		}
		return Out;
	}

	bool HasAction(const FString& GodotAction)
	{
		for (const FBindings::FEntry& E : Bindings().InOrder)
		{
			if (E.Action == GodotAction)
			{
				return true;
			}
		}
		return false;
	}

	void ResetBindings()
	{
		Bindings() = FBindings();
	}

	// --- remapping ---------------------------------------------------------------------------------------------------------

	namespace
	{
		Pc::PcInputState GLiveState;
		bool bLiveStateSet = false;
		UInputMappingContext* GLiveContext = nullptr;

		// Copies IMC_Pawpocalypse's mappings into the live context (what the game reads before apply() runs).
		void CopyAsset(UInputMappingContext* Live)
		{
			Live->UnmapAll();
			if (const UInputMappingContext* Asset = LoadContext())
			{
				for (const FEnhancedActionKeyMapping& M : Asset->GetMappings())
				{
					FEnhancedActionKeyMapping& Copy = Live->MapKey(M.Action, M.Key);
					for (const TObjectPtr<UInputModifier>& Mod : M.Modifiers)
					{
						Copy.Modifiers.Add(DuplicateObject<UInputModifier>(Mod, Live));
					}
				}
			}
		}
	}

	const Pc::PcInputState& DefaultState()
	{
		static const Pc::PcInputState State = []()
		{
			Pc::PcInputState S;
			for (const FPawInputDefault& D : PawInputDefaults)
			{
				Pc::BindingAt(S, D.Action).push_back(Pc::BindingEventOf(D.GodotEvent, D.GodotCode, D.GodotAxisSign));
			}
			return Pc::BindingGodotOrder(S);
		}();
		return State;
	}

	float DefaultDeadZone(const FString& GodotAction)
	{
		const std::string Action(TCHAR_TO_UTF8(*GodotAction));
		for (const FPawInputDefault& D : PawInputDefaults)
		{
			if (Action == D.Action)
			{
				return D.DeadZone;
			}
		}
		return 0.5f;
	}

	const Pc::PcInputState& LiveState()
	{
		return bLiveStateSet ? GLiveState : DefaultState();
	}

	UInputMappingContext* LiveContext()
	{
		if (!GLiveContext)
		{
			// Rooted for the run: the local players' Enhanced Input holds it from the first BeginPlay to the last.
			GLiveContext = NewObject<UInputMappingContext>(GetTransientPackage(), TEXT("IMC_PawpocalypseLive"), RF_Transient);
			GLiveContext->AddToRoot();
			CopyAsset(GLiveContext);
		}
		return GLiveContext;
	}

	FSimpleMulticastDelegate& OnBindingsChanged()
	{
		static FSimpleMulticastDelegate D;
		return D;
	}

	void ApplyBindings(const Pc::PcSettings& Settings)
	{
		GLiveState = Pc::BindingApply(Settings.Bindings, Settings.bSwapSticks, Settings.bSwapConfirmBack, DefaultState());
		bLiveStateSet = true;
		const float SteerDeadZone = static_cast<float>(Pc::BindingSteerDeadZone(Settings.StickDeadzone));
		UInputMappingContext* Live = LiveContext();
		Live->UnmapAll();
		for (const Pc::PcActionEvents& A : GLiveState)
		{
			const FString Action = UTF8_TO_TCHAR(A.Action.c_str());
			const UInputAction* IA = LoadAction(Action);
			if (!IA)
			{
				continue; // one of Godot's own actions the game has no asset for
			}
			const float DeadZone = Action.StartsWith(TEXT("steer_")) ? SteerDeadZone : DefaultDeadZone(Action);
			TArray<TPair<FKey, int32>> Mapped; // keypad Enter is Enter: one mapping
			for (const Pc::PcInputEvent& E : A.Events)
			{
				int32 Sign = 0;
				for (const FKey& Key : KeysOfEvent(E, Sign))
				{
					if (Mapped.Contains(TPair<FKey, int32>(Key, Sign)))
					{
						continue;
					}
					Mapped.Add(TPair<FKey, int32>(Key, Sign));
					FEnhancedActionKeyMapping& M = Live->MapKey(IA, Key);
					if (E.Kind == Pc::PcInputKind::JoyAxis)
					{
						UPawHalfAxis* Half = NewObject<UPawHalfAxis>(Live);
						Half->Sign = static_cast<float>(Sign);
						Half->DeadZone = DeadZone;
						M.Modifiers.Add(Half);
					}
				}
			}
		}
		SetSteerDeadZone(SteerDeadZone);
		ResetBindings();
		OnBindingsChanged().Broadcast();
	}

	float RawStrength(const TCHAR* GodotAction, const FKey& AxisKey, float Value)
	{
		for (const FBindings::FHalf& H : Bindings().Halves)
		{
			if (H.Key == AxisKey && H.Action == GodotAction)
			{
				const bool bThisWay = H.Sign > 0.0f ? Value > 0.0f : Value < 0.0f;
				return bThisWay ? FMath::Abs(Value) : 0.0f;
			}
		}
		return 0.0f;
	}

	float Strength(const TCHAR* GodotAction, const FKey& AxisKey, float Value)
	{
		for (const FBindings::FHalf& H : Bindings().Halves)
		{
			if (H.Key == AxisKey && H.Action == GodotAction)
			{
				return UPawHalfAxis::Strength(Value, H.Sign, H.DeadZone);
			}
		}
		return 0.0f;
	}

	bool IsStandIn(const FKey& Key)
	{
		for (const FBindings::FEntry& E : Bindings().InOrder)
		{
			if (E.Key == Key && E.bStandIn)
			{
				return true;
			}
		}
		return false;
	}

	namespace
	{
		float GSteerDeadZone = -1.0f;
	}

	void SetSteerDeadZone(float DeadZone)
	{
		GSteerDeadZone = DeadZone < 0.0f ? -1.0f : FMath::Clamp(DeadZone, 0.05f, 0.9f);
	}

	bool IsPressed(const TCHAR* GodotAction, const FKey& AxisKey, float Value)
	{
		for (const FBindings::FHalf& H : Bindings().Halves)
		{
			if (H.Key == AxisKey && H.Action == GodotAction)
			{
				const float Dead = (GSteerDeadZone >= 0.0f && H.Action.StartsWith(TEXT("steer_"))) ? GSteerDeadZone : H.DeadZone;
				return UPawHalfAxis::Pressed(Value, H.Sign, Dead);
			}
		}
		return false;
	}

	FVector2f StickVelocity(const FVector2f& Input, float FieldWidth)
	{
		// minf(input.length(), 1.0); input.normalized() * pow(m, STICK_CURVE) * field_width / STICK_CROSSING_SEC, in Vector2's
		// float arithmetic and Godot's order.
		const float Length = FMath::Sqrt(Input.X * Input.X + Input.Y * Input.Y);
		const double M = FMath::Min(static_cast<double>(Length), 1.0);
		if (M <= 0.0)
		{
			return FVector2f::ZeroVector;
		}
		FVector2f N = Input;
		const float L2 = N.X * N.X + N.Y * N.Y;
		if (L2 != 0.0f)
		{
			const float L = FMath::Sqrt(L2);
			N = FVector2f(N.X / L, N.Y / L);
		}
		const float Curve = static_cast<float>(FMath::Pow(M, StickCurve));
		FVector2f V(N.X * Curve, N.Y * Curve);
		V = FVector2f(V.X * FieldWidth, V.Y * FieldWidth);
		return FVector2f(V.X / StickCrossingSec, V.Y / StickCrossingSec);
	}

	float DeadZoneOf(const TCHAR* GodotAction)
	{
		if (GSteerDeadZone >= 0.0f && FString(GodotAction).StartsWith(TEXT("steer_")))
		{
			return GSteerDeadZone;
		}
		for (const FBindings::FHalf& H : Bindings().Halves)
		{
			if (H.Action == GodotAction)
			{
				return H.DeadZone;
			}
		}
		return 0.5f;
	}

	FVector2f GodotVector(float NegX, float PosX, float NegY, float PosY, float DeadZone)
	{
		// Input::get_vector: Vector2 (float) arithmetic, in Godot's order.
		const FVector2f V(PosX - NegX, PosY - NegY);
		const float Length = FMath::Sqrt(V.X * V.X + V.Y * V.Y);
		if (Length <= DeadZone)
		{
			return FVector2f::ZeroVector;
		}
		if (Length > 1.0f)
		{
			return FVector2f(V.X / Length, V.Y / Length);
		}
		const float Scale = ((Length - DeadZone) / (1.0f - DeadZone)) / Length;
		return FVector2f(V.X * Scale, V.Y * Scale);
	}

	bool IsSteerAxis(const FKey& Key)
	{
		for (const FBindings::FHalf& H : Bindings().Halves)
		{
			if (H.Key == Key && H.Action.StartsWith(TEXT("steer_")))
			{
				return true;
			}
		}
		return false;
	}

	FVector2f SteerVector(const TMap<FKey, float>& AxisValues)
	{
		float L = 0.0f, R = 0.0f, U = 0.0f, D = 0.0f;
		for (const TPair<FKey, float>& A : AxisValues)
		{
			L = FMath::Max(L, RawStrength(TEXT("steer_left"), A.Key, A.Value));
			R = FMath::Max(R, RawStrength(TEXT("steer_right"), A.Key, A.Value));
			U = FMath::Max(U, RawStrength(TEXT("steer_up"), A.Key, A.Value));
			D = FMath::Max(D, RawStrength(TEXT("steer_down"), A.Key, A.Value));
		}
		const float DeadZone = 0.25f * (DeadZoneOf(TEXT("steer_left")) + DeadZoneOf(TEXT("steer_right")) + DeadZoneOf(TEXT("steer_up"))
			+ DeadZoneOf(TEXT("steer_down")));
		return GodotVector(L, R, U, D, DeadZone);
	}
}
