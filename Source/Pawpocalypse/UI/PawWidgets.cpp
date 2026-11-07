#include "UI/PawWidgets.h"

#include "UI/PawIcons.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Engine/Blueprint.h"
#include "Core/PawLog.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"
#include "UI/PawButton.h"
#include "UI/PawCardPickScreen.h"
#include "UI/PawControlsPanel.h"
#include "UI/PawGarden.h"
#include "UI/PawRainbow.h"
#include "UI/PawTreeSky.h"
#include "UI/PawUnicornCeremony.h"
#include "UI/PawHudScreen.h"
#include "UI/PawJobPicker.h"
#include "UI/PawPounceClock.h"
#include "UI/PawReawakenCeremony.h"
#include "UI/PawSkillTreeScreen.h"
#include "UI/PawVictoryScreen.h"
#include "UI/PawMainMenu.h"
#include "UI/PawPauseMenu.h"
#include "UI/PawSettingsDialog.h"
#include "UI/PawPrompts.h"
#include "UI/PawToast.h"
#include "UI/PawRootLayout.h"

namespace
{
	TMap<TWeakObjectPtr<UClass>, TWeakObjectPtr<UClass>> GResolved;
	bool GBaking = false;
	const UClass* GBakeRoot = nullptr;
	int32 GCodeLayout = 0;
	FAutoConsoleVariableRef CodeLayoutVar(TEXT("paw.UI.CodeLayout"), GCodeLayout,
		TEXT("1: build every screen and piece from its C++ layout, ignoring the baked Widget Blueprints (tests, comparison)."),
		FConsoleVariableDelegate::CreateLambda([](IConsoleVariable*) { GResolved.Reset(); }));
}

namespace PawUi
{
	const TCHAR* WidgetFolder()
	{
		return TEXT("/Game/Pawpocalypse/UI/Widgets");
	}

	FString AssetNameFor(const UClass* Native)
	{
		// UPawMainMenu's GetName() is "PawMainMenu": the "Paw" goes, a style gets BP_, a widget WBP_.
		FString Name = Native ? Native->GetName() : FString();
		Name.RemoveFromStart(TEXT("Paw"));
		const bool bStyle = Native && Native->IsChildOf(UObject::StaticClass()) && !Native->IsChildOf(UUserWidget::StaticClass());
		return (bStyle ? TEXT("BP_") : TEXT("WBP_")) + Name;
	}

	FString AssetPathFor(const UClass* Native)
	{
		return FString(WidgetFolder()) / AssetNameFor(Native);
	}

	UClass* Resolve(UClass* Native)
	{
		if (!Native || GCodeLayout != 0)
		{
			return Native;
		}
		if (const TWeakObjectPtr<UClass>* Hit = GResolved.Find(Native); Hit && Hit->IsValid())
		{
			return Hit->Get();
		}
		UClass* Out = Native;
		const FString Package = AssetPathFor(Native);
		if (FPackageName::DoesPackageExist(Package))
		{
			const FString Name = AssetNameFor(Native);
			if (UClass* Baked = LoadClass<UObject>(nullptr, *FString::Printf(TEXT("%s.%s_C"), *Package, *Name)); Baked && Baked->IsChildOf(Native))
			{
				Out = Baked;
			}
			else
			{
				UE_LOG(LogPaw, Warning, TEXT("UI: %s exists but holds no child of %s; using the code layout"), *Package, *Native->GetName());
			}
		}
		GResolved.Add(Native, Out);
		return Out;
	}

	void ResetResolver()
	{
		GResolved.Reset();
	}

	void SetCodeLayoutForced(bool bForced)
	{
		GCodeLayout = bForced ? 1 : 0;
		GResolved.Reset();
	}

	bool IsCodeLayoutForced()
	{
		return GCodeLayout != 0;
	}

	bool IsBaking()
	{
		return GBaking;
	}

	void SetBaking(bool bBaking, const UClass* Root)
	{
		GBaking = bBaking;
		GBakeRoot = bBaking ? Root : nullptr;
	}

	bool BuildsInCode(const UUserWidget* Widget)
	{
		if (!Widget || Cast<UWidgetBlueprintGeneratedClass>(Widget->GetClass()))
		{
			return false;
		}
		return !GBaking || Widget->GetClass() == GBakeRoot;
	}

	bool ShouldWire(const UUserWidget* Widget)
	{
		return Widget && !Widget->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) && !GBaking && !Widget->IsDesignTime();
	}

	TArray<UClass*> BakedClasses()
	{
		return {
			// the button styles
			UPawButtonStyle::StaticClass(), UPawLockedButtonStyle::StaticClass(), UPawFlatButtonStyle::StaticClass(), UPawBareButtonStyle::StaticClass(),
			UPawPlateButtonStyle::StaticClass(), UPawCardFaceStyle::StaticClass(), UPawTreeNodeStyle::StaticClass(),
			// the pieces (a piece used inside another before it: the button first)
			UPawButton::StaticClass(), UPawCardFace::StaticClass(), UPawTreeNode::StaticClass(), UPawTreeLinks::StaticClass(),
			UPawPounceClock::StaticClass(), UPawReawakenCeremony::StaticClass(), UPawPromptIcon::StaticClass(), UPawPromptText::StaticClass(), UPawIconText::StaticClass(),
			UPawControlsPanel::StaticClass(), UPawGarden::StaticClass(), UPawRainbowBar::StaticClass(), UPawRainbowFrame::StaticClass(),
			UPawTreeSky::StaticClass(), UPawUnicornCeremony::StaticClass(),
			// the screens
			UPawRootLayout::StaticClass(), UPawMainMenu::StaticClass(), UPawHudScreen::StaticClass(), UPawCardPickScreen::StaticClass(),
			UPawSkillTreeScreen::StaticClass(), UPawJobPicker::StaticClass(), UPawVictoryScreen::StaticClass(),
			UPawPauseMenu::StaticClass(), UPawSettingsDialog::StaticClass(), UPawToast::StaticClass(),
		};
	}

	FName Numbered(const TCHAR* Prefix, int32 Index)
	{
		return FName(*FString::Printf(TEXT("%s%d"), Prefix, Index));
	}

	namespace
	{
		int32 ColoursIn(const UStruct* Type, void* Data, TFunctionRef<bool(FColor&)> Visit);

		// One value of a struct type: a colour itself, or a struct to walk into.
		int32 ColourValue(const UScriptStruct* Struct, void* Value, TFunctionRef<bool(FColor&)> Visit)
		{
			if (Struct == TBaseStructure<FLinearColor>::Get())
			{
				FLinearColor& C = *static_cast<FLinearColor*>(Value);
				FColor S = C.ToFColorSRGB();
				S.A = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(C.A * 255.0f), 0, 255));
				if (!Visit(S))
				{
					return 0;
				}
				const float Alpha = C.A;
				FColor Rgb = S;
				Rgb.A = 255;
				C = FLinearColor(Rgb); // FLinearColor(FColor) decodes sRGB, as FromSRGBColor
				C.A = Alpha;
				return 1;
			}
			if (Struct == TBaseStructure<FColor>::Get())
			{
				FColor& C = *static_cast<FColor*>(Value);
				FColor S = C;
				if (!Visit(S))
				{
					return 0;
				}
				C = FColor(S.R, S.G, S.B, C.A);
				return 1;
			}
			return ColoursIn(Struct, Value, Visit);
		}

		int32 ColoursIn(const UStruct* Type, void* Data, TFunctionRef<bool(FColor&)> Visit)
		{
			int32 Written = 0;
			for (TFieldIterator<FProperty> It(Type); It; ++It)
			{
				if (const FStructProperty* SP = CastField<FStructProperty>(*It))
				{
					for (int32 I = 0; I < SP->ArrayDim; ++I)
					{
						Written += ColourValue(SP->Struct, SP->ContainerPtrToValuePtr<void>(Data, I), Visit);
					}
				}
				else if (const FArrayProperty* AP = CastField<FArrayProperty>(*It))
				{
					if (const FStructProperty* Inner = CastField<FStructProperty>(AP->Inner))
					{
						FScriptArrayHelper Array(AP, AP->ContainerPtrToValuePtr<void>(Data));
						for (int32 I = 0; I < Array.Num(); ++I)
						{
							Written += ColourValue(Inner->Struct, Array.GetRawPtr(I), Visit);
						}
					}
				}
			}
			return Written;
		}
	}

	int32 ForEachColour(UObject* Object, TFunctionRef<bool(FColor& Srgb)> Visit)
	{
		return Object ? ColoursIn(Object->GetClass(), Object, Visit) : 0;
	}

	TArray<UObject*> LookObjects(UBlueprint* Asset)
	{
		TArray<UObject*> Out;
		if (!Asset)
		{
			return Out;
		}
		if (const UWidgetBlueprintGeneratedClass* Generated = Cast<UWidgetBlueprintGeneratedClass>(Asset->GeneratedClass))
		{
			if (UWidgetTree* Tree = Generated->GetWidgetTreeArchetype())
			{
				Tree->ForEachWidget([&Out](UWidget* W) { Out.Add(W); });
			}
		}
		if (Asset->GeneratedClass)
		{
			Out.Add(Asset->GeneratedClass->GetDefaultObject());
		}
		return Out;
	}
}
