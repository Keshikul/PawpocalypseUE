#include "UI/PawWidgets.h"

#include "UI/PawIcons.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Engine/Blueprint.h"
#include "Components/TextWidgetTypes.h"
#include "Engine/Font.h"
#include "Styling/SlateTypes.h"
#include "UI/PawFontSpec.h"
#include "UI/PawUiKit.h"
#include "UI/PawUiAssets.h"
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
	// The resolved classes are UPawUiAssets' (review CR12: held, not weak: a class whose last widget went away was loaded
	// again by the next one).
	void ForgetResolved()
	{
		if (UPawUiAssets* Held = UPawUiAssets::Get())
		{
			Held->Classes.Reset();
		}
	}

	bool GBaking = false;
	const UClass* GBakeRoot = nullptr;
	int32 GCodeLayout = 0;
	FAutoConsoleVariableRef CodeLayoutVar(TEXT("paw.UI.CodeLayout"), GCodeLayout,
		TEXT("1: build every screen and piece from its C++ layout, ignoring the baked Widget Blueprints (tests, comparison)."),
		FConsoleVariableDelegate::CreateLambda([](IConsoleVariable*) { ForgetResolved(); }));
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
		UPawUiAssets* Held = UPawUiAssets::Get();
		if (Held)
		{
			if (const TObjectPtr<UClass>* Hit = Held->Classes.Find(Native); Hit && *Hit)
			{
				return *Hit;
			}
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
		if (Held)
		{
			Held->Classes.Add(Native, Out);
		}
		return Out;
	}

	void ResetResolver()
	{
		ForgetResolved();
	}

	void SetCodeLayoutForced(bool bForced)
	{
		GCodeLayout = bForced ? 1 : 0;
		ForgetResolved();
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
		// A struct value the walk met: >= 0 = handled (that many written), -1 = not a leaf, walk into it. Role: what the value
		// colours (a text's colour, a brush's outline, else a surface; the outermost text wins).
		using FLeaf = TFunctionRef<int32(const UScriptStruct* Struct, void* Value, EPawColourRole Role)>;

		int32 StructsIn(const UStruct* Type, void* Data, FLeaf Leaf, EPawColourRole Role = EPawColourRole::Surface)
		{
			int32 Written = 0;
			EPawColourRole RoleHere = Role;
			auto One = [&](const UScriptStruct* Struct, void* Value)
			{
				const int32 N = Leaf(Struct, Value, RoleHere);
				Written += N >= 0 ? N : StructsIn(Struct, Value, Leaf, RoleHere);
			};
			for (TFieldIterator<FProperty> It(Type); It; ++It)
			{
				if (It->HasAnyPropertyFlags(CPF_Transient))
				{
					continue; // never saved, rebuilt on load (a rich text's DefaultTextStyle): not the asset's look
				}
				const bool bText = Role == EPawColourRole::Text || IsTextColour(Type, *It);
				RoleHere = bText ? EPawColourRole::Text
					: Role == EPawColourRole::Edge || IsEdgeColour(Type, *It) ? EPawColourRole::Edge : EPawColourRole::Surface;
				if (const FStructProperty* SP = CastField<FStructProperty>(*It))
				{
					for (int32 I = 0; I < SP->ArrayDim; ++I)
					{
						One(SP->Struct, SP->ContainerPtrToValuePtr<void>(Data, I));
					}
				}
				else if (const FArrayProperty* AP = CastField<FArrayProperty>(*It))
				{
					if (const FStructProperty* Inner = CastField<FStructProperty>(AP->Inner))
					{
						FScriptArrayHelper Array(AP, AP->ContainerPtrToValuePtr<void>(Data));
						for (int32 I = 0; I < Array.Num(); ++I)
						{
							One(Inner->Struct, Array.GetRawPtr(I));
						}
					}
				}
			}
			return Written;
		}

		// One value of a struct type: a colour (1 when written, 0 when kept), or -1 to walk into it.
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
			return -1;
		}
	}

	int32 ForEachColour(UObject* Object, TFunctionRef<bool(FColor& Srgb)> Visit)
	{
		return Object ? StructsIn(Object->GetClass(), Object, [&Visit](const UScriptStruct* S, void* V, EPawColourRole) { return ColourValue(S, V, Visit); }) : 0;
	}

	int32 ForEachColourRole(UObject* Object, TFunctionRef<bool(FColor& Srgb, EPawColourRole Role)> Visit)
	{
		if (!Object)
		{
			return 0;
		}
		return StructsIn(Object->GetClass(), Object, [&Visit](const UScriptStruct* S, void* V, EPawColourRole Role)
		{
			return ColourValue(S, V, [&Visit, Role](FColor& C) { return Visit(C, Role); });
		});
	}

	bool IsEdgeColour(const UStruct* Owner, const FProperty* Property)
	{
		return Owner == FSlateBrushOutlineSettings::StaticStruct() && Property->GetFName() == TEXT("Color");
	}

	bool IsTextColour(const UStruct* Owner, const FProperty* Property)
	{
		const FName Name = Property->GetFName();
		if (Name == TEXT("TextColour"))
		{
			return true; // the pieces' own text colour (UPawIconText, UPawPromptText)
		}
		if (Name != TEXT("ColorAndOpacity"))
		{
			return false; // a shadow, an outline, a tint: a surface
		}
		if (Owner == FTextBlockStyle::StaticStruct())
		{
			return true;
		}
		const UClass* Class = Cast<UClass>(Owner);
		return Class && Class->IsChildOf(UTextLayoutWidget::StaticClass());
	}

	int32 ForEachFont(UObject* Object, TFunctionRef<bool(FSlateFontInfo& Font)> Visit)
	{
		if (!Object)
		{
			return 0;
		}
		return StructsIn(Object->GetClass(), Object, [&Visit](const UScriptStruct* Struct, void* Value, EPawColourRole) -> int32
		{
			if (Struct != FSlateFontInfo::StaticStruct())
			{
				return -1;
			}
			return Visit(*static_cast<FSlateFontInfo*>(Value)) ? 1 : 0;
		});
	}

	bool IsEngineFont(const UObject* FontObject)
	{
		return !FontObject || FontObject->GetPathName().StartsWith(TEXT("/Engine/"));
	}

	UFont* PawFont()
	{
		// Held for the engine's life (review CR12: UPawUiAssets).
		UPawUiAssets* Held = UPawUiAssets::Get();
		if (Held && Held->Font)
		{
			return Held->Font;
		}
		UFont* Font = LoadObject<UFont>(nullptr, PawFontSpec::Font);
		if (Held)
		{
			Held->Font = Font;
		}
		return Font;
	}

	TArray<FString> UnderTextFloor(UObject* Object, bool bRaise)
	{
		TArray<FString> Out;
		if (!Object || Object->GetFName() == TEXT("KeyLabel"))
		{
			return Out;
		}
		const float MinPt = FontPt(TextFloorPx);
		ForEachFont(Object, [&](FSlateFontInfo& F)
		{
			if (F.Size + 0.01f >= MinPt)
			{
				return false;
			}
			Out.Add(FString::Printf(TEXT("%s font %.2f pt"), *Object->GetName(), F.Size));
			if (bRaise)
			{
				F.Size = MinPt;
			}
			return bRaise;
		});
		for (const TCHAR* Name : { TEXT("TextPx"), TEXT("LabelPx") })
		{
			if (const FFloatProperty* P = FindFProperty<FFloatProperty>(Object->GetClass(), Name))
			{
				float* V = P->ContainerPtrToValuePtr<float>(Object);
				if (*V + 0.01f < static_cast<float>(TextFloorPx))
				{
					Out.Add(FString::Printf(TEXT("%s %s %.1f px"), *Object->GetName(), Name, *V));
					if (bRaise)
					{
						*V = static_cast<float>(TextFloorPx);
					}
				}
			}
		}
		return Out;
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
