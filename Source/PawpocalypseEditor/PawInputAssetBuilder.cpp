#include "PawInputAssetBuilder.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Input/PawHalfAxis.h"
#include "Input/PawInputMap.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
	bool Save(UObject* Asset, FString& Error)
	{
		UPackage* Package = Asset->GetOutermost();
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.Error = GError;
		if (!UPackage::SavePackage(Package, Asset, *File, Args))
		{
			Error = TEXT("could not save ") + File;
			return false;
		}
		return true;
	}

	// The asset at Path (loaded), or a new one there.
	template <typename T>
	T* FindOrCreate(const FString& Path, bool& bCreated)
	{
		const FString Name = FPackageName::GetLongPackageAssetName(Path);
		if (T* Existing = LoadObject<T>(nullptr, *(Path + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			bCreated = false;
			return Existing;
		}
		UPackage* Package = CreatePackage(*Path);
		bCreated = true;
		return NewObject<T>(Package, *Name, RF_Public | RF_Standalone);
	}
}

FString UPawInputAssetBuilder::BuildInputAssets(const FString& CsvPath)
{
	TArray<FString> Report;
	TArray<FPawInputMapRow> Rows;
	FString Error;
	if (!PawInput::ReadInputMap(CsvPath, Rows, Error))
	{
		return TEXT("FAILED ") + Error;
	}
	// The actions, in the map's order.
	TArray<FString> Actions;
	for (const FPawInputMapRow& R : Rows)
	{
		Actions.AddUnique(R.Action);
	}
	TMap<FString, UInputAction*> ByName;
	for (const FString& A : Actions)
	{
		const FString Path = FString(PawInput::ActionsDir) / PawInput::ActionAssetName(A);
		bool bCreated = false;
		UInputAction* IA = FindOrCreate<UInputAction>(Path, bCreated);
		const bool bAxis = PawInput::IsAxisAction(Rows, A);
		IA->ValueType = bAxis ? EInputActionValueType::Axis1D : EInputActionValueType::Boolean;
		IA->ActionDescription = FText::AsCultureInvariant(A); // the Godot action it is (never shown to the player)
		if (!Save(IA, Error))
		{
			return TEXT("FAILED ") + Error;
		}
		ByName.Add(A, IA);
		Report.Add(FString::Printf(TEXT("%s %s (%s)"), *PawInput::ActionAssetName(A), bCreated ? TEXT("created") : TEXT("updated"),
			bAxis ? TEXT("Axis1D") : TEXT("Boolean")));
	}
	// The context: rebuilt whole from the map every time.
	bool bCreated = false;
	UInputMappingContext* IMC = FindOrCreate<UInputMappingContext>(PawInput::ContextPath, bCreated);
	IMC->UnmapAll();
	int32 Mapped = 0;
	for (const FPawInputMapRow& R : Rows)
	{
		if (R.bMerged)
		{
			continue;
		}
		const FKey Key(R.UeKey);
		if (!Key.IsValid())
		{
			return FString::Printf(TEXT("FAILED %s: %s is not a key"), *R.Id, *R.UeKey.ToString());
		}
		FEnhancedActionKeyMapping& M = IMC->MapKey(ByName[R.Action], Key);
		if (R.GodotEvent == TEXT("pad_axis"))
		{
			UPawHalfAxis* Half = NewObject<UPawHalfAxis>(IMC);
			Half->Sign = static_cast<float>(R.UeAxisSign);
			Half->DeadZone = R.DeadZone;
			M.Modifiers.Add(Half);
		}
		++Mapped;
	}
	if (!Save(IMC, Error))
	{
		return TEXT("FAILED ") + Error;
	}
	Report.Add(FString::Printf(TEXT("IMC_Pawpocalypse %s: %d mappings, %d actions"), bCreated ? TEXT("created") : TEXT("rebuilt"), Mapped, Actions.Num()));
	return FString::Join(Report, TEXT("\n"));
}
