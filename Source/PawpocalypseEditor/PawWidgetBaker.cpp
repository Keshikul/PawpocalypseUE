#include "PawWidgetBaker.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ContentWidget.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/PanelSlot.h"
#include "Components/Widget.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Components/PanelWidget.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "UI/PawWidgets.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"

namespace
{
	bool SaveAsset(UObject* Asset, UPackage* Package, FString& Error)
	{
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

	// The properties a class binds to widgets by name (BindWidget / BindWidgetOptional).
	TSet<FName> BoundNames(const UClass* Class)
	{
		TSet<FName> Out;
		for (TFieldIterator<FObjectPropertyBase> It(Class); It; ++It)
		{
			if (It->HasMetaData(TEXT("BindWidget")) || It->HasMetaData(TEXT("BindWidgetOptional")))
			{
				Out.Add(It->GetFName());
			}
		}
		return Out;
	}

	// A user widget nested in the tree keeps only its properties: references into its OWN (transient) tree would be copied
	// into the owner's asset as stray objects, so they are cleared first (its own Widget Blueprint rebuilds them at runtime).
	void ClearInnerReferences(UUserWidget* Nested)
	{
		for (TFieldIterator<FObjectPropertyBase> It(Nested->GetClass()); It; ++It)
		{
			UObject* Value = It->GetObjectPropertyValue_InContainer(Nested);
			if (Value && Value->IsIn(Nested))
			{
				It->SetObjectPropertyValue_InContainer(Nested, nullptr);
			}
		}
	}

	void MakePersistent(UObject* Root)
	{
		Root->ClearFlags(RF_Transient);
		Root->SetFlags(RF_Transactional);
		TArray<UObject*> Inner;
		GetObjectsWithOuter(Root, Inner, true);
		for (UObject* O : Inner)
		{
			O->ClearFlags(RF_Transient);
			O->SetFlags(RF_Transactional);
		}
	}

	FString BakeStyle(UClass* Native, UPackage* Package, const FString& Name)
	{
		UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(Native, Package, FName(*Name), BPTYPE_Normal,
			UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), TEXT("PawWidgetBaker"));
		if (!BP)
		{
			return TEXT("FAILED: no Blueprint");
		}
		FKismetEditorUtilities::CompileBlueprint(BP);
		FString Error;
		return SaveAsset(BP, Package, Error) ? FString(TEXT("baked")) : TEXT("FAILED: ") + Error;
	}

	FString BakeWidget(UClass* Native, UPackage* Package, const FString& Name)
	{
		// The code layout, built as the native class builds it (only this class builds: PawUi::SetBaking's root).
		PawUi::SetBaking(true, Native);
		UUserWidget* Source = NewObject<UUserWidget>(GetTransientPackage(), Native, NAME_None, RF_Transient);
		Source->Initialize();
		PawUi::SetBaking(false);
		// A widget that is all paint (the tree's links) has no layout: its Blueprint keeps an empty tree, and its Class
		// Defaults are what the designer edits.
		const bool bPaintOnly = !Source->WidgetTree || !Source->WidgetTree->RootWidget;
		UWidgetBlueprint* BP = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(Native, Package, FName(*Name), BPTYPE_Normal,
			UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass(), TEXT("PawWidgetBaker")));
		if (!BP)
		{
			return TEXT("FAILED: no Widget Blueprint");
		}
		if (bPaintOnly)
		{
			FKismetEditorUtilities::CompileBlueprint(BP);
			FString Error;
			return SaveAsset(BP, Package, Error) ? FString(TEXT("baked (paint only)")) : TEXT("FAILED: ") + Error;
		}
		// A Common button wraps its content in an internal button as it initializes (CommonButtonBase.cpp): that wrapper is
		// made again by every instance, so the asset holds the content alone (saved, it would wrap twice and the clicks would
		// land on the inner, unconnected one).
		if (UContentWidget* Wrap = Cast<UContentWidget>(Source->WidgetTree->RootWidget);
			Wrap && Wrap->GetClass()->GetFName() == TEXT("CommonButtonInternalBase"))
		{
			UWidget* Content = Wrap->GetContent();
			if (Content)
			{
				Content->RemoveFromParent();
			}
			Source->WidgetTree->RootWidget = Content;
			Wrap->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
		}
		// Nested pieces: properties only. Their own trees (made as they initialized) are cleared from their properties and
		// moved out, so the copy below takes none of it into this asset.
		TArray<UUserWidget*> Nested;
		Source->WidgetTree->ForEachWidget([&Nested](UWidget* W)
		{
			if (UUserWidget* U = Cast<UUserWidget>(W))
			{
				Nested.Add(U);
			}
		});
		for (UUserWidget* U : Nested)
		{
			ClearInnerReferences(U);
			TArray<UObject*> Inner;
			GetObjectsWithOuter(U, Inner, false);
			for (UObject* O : Inner)
			{
				O->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
			}
		}
		if (!Source->WidgetTree->RootWidget)
		{
			return TEXT("FAILED: the class built no layout");
		}
		// The tree, copied into the asset under the name the editor expects.
		if (BP->WidgetTree)
		{
			BP->WidgetTree->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
		}
		BP->WidgetTree = DuplicateObject<UWidgetTree>(Source->WidgetTree, BP, TEXT("WidgetTree"));
		MakePersistent(BP->WidgetTree);
		const TSet<FName> Bound = BoundNames(Native);
		BP->WidgetTree->ForEachWidget([BP, &Bound](UWidget* W)
		{
			// A bound part is a variable (and shows as one in the Designer); every widget has its id.
			W->bIsVariable = Bound.Contains(W->GetFName()) || W->bIsVariable;
			if (!BP->WidgetVariableNameToGuidMap.Contains(W->GetFName()))
			{
				BP->WidgetVariableNameToGuidMap.Add(W->GetFName(), FGuid::NewGuid());
			}
		});
		FKismetEditorUtilities::CompileBlueprint(BP);
		FString Error;
		return SaveAsset(BP, Package, Error) ? FString(TEXT("baked")) : TEXT("FAILED: ") + Error;
	}
}

namespace
{
	// A leaf of the code layout, copied into the asset's tree (a nested piece keeps only its properties).
	UWidget* CopyPart(UWidget* Part, UWidgetBlueprint* BP, const UClass* Native)
	{
		if (UUserWidget* U = Cast<UUserWidget>(Part))
		{
			ClearInnerReferences(U);
			TArray<UObject*> Inner;
			GetObjectsWithOuter(U, Inner, false);
			for (UObject* O : Inner)
			{
				O->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
			}
		}
		UPanelSlot* SourceSlot = Part->Slot;
		Part->Slot = nullptr;
		UWidget* New = DuplicateObject<UWidget>(Part, BP->WidgetTree, Part->GetFName());
		Part->Slot = SourceSlot;
		MakePersistent(New);
		New->bIsVariable = BoundNames(Native).Contains(New->GetFName()) || New->bIsVariable;
		if (!BP->WidgetVariableNameToGuidMap.Contains(New->GetFName()))
		{
			BP->WidgetVariableNameToGuidMap.Add(New->GetFName(), FGuid::NewGuid());
		}
		return New;
	}

	// A panel of the code layout made EMPTY in the asset: the same class and name, its own settings (not its children).
	UWidget* CopyEmptyPanel(UPanelWidget* Part, UWidgetBlueprint* BP, const UClass* Native)
	{
		UPanelWidget* New = BP->WidgetTree->ConstructWidget<UPanelWidget>(Part->GetClass(), Part->GetFName());
		for (TFieldIterator<FProperty> It(Part->GetClass()); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Edit) && It->GetOwnerClass() != UPanelWidget::StaticClass()
				&& !It->HasAnyPropertyFlags(CPF_InstancedReference))
			{
				It->CopyCompleteValue_InContainer(New, Part);
			}
		}
		New->bIsVariable = BoundNames(Native).Contains(New->GetFName()) || New->bIsVariable;
		if (!BP->WidgetVariableNameToGuidMap.Contains(New->GetFName()))
		{
			BP->WidgetVariableNameToGuidMap.Add(New->GetFName(), FGuid::NewGuid());
		}
		return New;
	}

	FString AddPart(UUserWidget* Source, UWidgetBlueprint* BP, const UClass* Native, const FName Name);

	// The code slot's settings onto the asset's new slot (the same slot class: the same kind of panel).
	void CopySlot(const UPanelSlot* From, UPanelSlot* To)
	{
		if (!From || !To || From->GetClass() != To->GetClass())
		{
			return;
		}
		for (TFieldIterator<FProperty> It(From->GetClass()); It; ++It)
		{
			if (It->GetOwnerClass() != UPanelSlot::StaticClass() && It->HasAnyPropertyFlags(CPF_Edit))
			{
				It->CopyCompleteValue_InContainer(To, From);
			}
		}
		To->SynchronizeProperties();
	}

	FString AddPart(UUserWidget* Source, UWidgetBlueprint* BP, const UClass* Native, const FName Name)
	{
		UWidget* Part = Source->WidgetTree->FindWidget(Name);
		if (!Part)
		{
			return TEXT("FAILED: the code layout has no ") + Name.ToString();
		}
		if (BP->WidgetTree->FindWidget(Name))
		{
			return TEXT("kept (the asset has it)");
		}
		UPanelWidget* AsPanel = Cast<UPanelWidget>(Part);
		const bool bTree = AsPanel && AsPanel->GetChildrenCount() > 0;
		if (bTree)
		{
			// A part with children (phase 6 slice 5b: the dock's quick bar): allowed when the asset has none of them.
			TArray<UWidget*> Below;
			UWidgetTree::GetChildWidgets(AsPanel, Below);
			for (UWidget* W : Below)
			{
				if (BP->WidgetTree->FindWidget(W->GetFName()))
				{
					return TEXT("FAILED: the asset already has ") + W->GetName() + TEXT(" under it");
				}
			}
		}
		UPanelWidget* Parent = Part->GetParent();
		if (!Parent)
		{
			return TEXT("FAILED: the part is the root");
		}
		// Where: after the nearest earlier sibling the asset has, else before the nearest later one, else into the parent.
		const int32 At = Parent->GetChildIndex(Part);
		UPanelWidget* Into = nullptr;
		int32 Index = 0;
		for (int32 I = At - 1; I >= 0 && !Into; --I)
		{
			if (UWidget* S = BP->WidgetTree->FindWidget(Parent->GetChildAt(I)->GetFName()); S && S->GetParent())
			{
				Into = S->GetParent();
				Index = Into->GetChildIndex(S) + 1;
			}
		}
		for (int32 I = At + 1; I < Parent->GetChildrenCount() && !Into; ++I)
		{
			if (UWidget* S = BP->WidgetTree->FindWidget(Parent->GetChildAt(I)->GetFName()); S && S->GetParent())
			{
				Into = S->GetParent();
				Index = Into->GetChildIndex(S);
			}
		}
		if (!Into)
		{
			Into = Cast<UPanelWidget>(BP->WidgetTree->FindWidget(Parent->GetFName()));
			Index = Into ? FMath::Min(At, Into->GetChildrenCount()) : 0;
		}
		if (!Into)
		{
			return TEXT("FAILED: the asset has none of its siblings and not its parent ") + Parent->GetName();
		}
		UWidget* New = bTree ? CopyEmptyPanel(AsPanel, BP, Native) : CopyPart(Part, BP, Native);
		UPanelSlot* NewSlot = Into->InsertChildAt(Index, New);
		CopySlot(Part->Slot, NewSlot);
		FString Children;
		if (bTree)
		{
			// Each child by name, in order, into the new panel (it is the asset's now): leaves and panels alike.
			for (int32 I = 0; I < AsPanel->GetChildrenCount(); ++I)
			{
				const FString R = AddPart(Source, BP, Native, AsPanel->GetChildAt(I)->GetFName());
				Children += TEXT("; ") + AsPanel->GetChildAt(I)->GetName() + TEXT(": ") + R;
			}
		}
		return FString::Printf(TEXT("added into %s at %d%s"), *Into->GetName(), Index, *Children);
	}

	FString SwapPart(UUserWidget* Source, UWidgetBlueprint* BP, const UClass* Native, const FName OldName, const FName NewName)
	{
		UWidget* Part = Source->WidgetTree->FindWidget(NewName);
		if (!Part)
		{
			return TEXT("FAILED: the code layout has no ") + NewName.ToString();
		}
		if (BP->WidgetTree->FindWidget(NewName))
		{
			return TEXT("kept (the asset has it)");
		}
		UWidget* Old = BP->WidgetTree->FindWidget(OldName);
		if (!Old)
		{
			return TEXT("FAILED: the asset has no ") + OldName.ToString();
		}
		UPanelWidget* Into = Old->GetParent();
		if (!Into)
		{
			return TEXT("FAILED: the old part is the root");
		}
		const int32 Index = Into->GetChildIndex(Old);
		UWidget* New = CopyPart(Part, BP, Native);
		Into->ReplaceChildAt(Index, New);
		Old->Slot = nullptr;
		BP->WidgetVariableNameToGuidMap.Remove(OldName);
		Old->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
		return FString::Printf(TEXT("swapped into %s at %d (the old slot kept)"), *Into->GetName(), Index);
	}
}

namespace
{
	// restyle: the part's LOOK copied from the code layout into the asset's part of the same name: a border's brush and
	// padding (the padding is the stylebox's content margins), an image's brush, a bar's style, a size box's overrides.
	// Nothing else of it (its slot, its children, its visibility) changes.
	FString RestylePart(UUserWidget* Source, UWidgetBlueprint* BP, const FName Name)
	{
		UWidget* Part = Source->WidgetTree->FindWidget(Name);
		if (!Part)
		{
			return TEXT("FAILED: the code layout has no ") + Name.ToString();
		}
		UWidget* Mine = BP->WidgetTree->FindWidget(Name);
		if (!Mine)
		{
			return TEXT("FAILED: the asset has no ") + Name.ToString();
		}
		Mine->Modify();
		if (UBorder* From = Cast<UBorder>(Part))
		{
			UBorder* To = Cast<UBorder>(Mine);
			if (!To)
			{
				return TEXT("FAILED: the asset's part is not a border");
			}
			To->SetBrush(From->Background);
			To->SetPadding(From->GetPadding());
			return TEXT("restyled (brush, padding)");
		}
		if (UImage* From = Cast<UImage>(Part))
		{
			UImage* To = Cast<UImage>(Mine);
			if (!To)
			{
				return TEXT("FAILED: the asset's part is not an image");
			}
			To->SetBrush(From->GetBrush());
			return TEXT("restyled (brush)");
		}
		if (UProgressBar* From = Cast<UProgressBar>(Part))
		{
			UProgressBar* To = Cast<UProgressBar>(Mine);
			if (!To)
			{
				return TEXT("FAILED: the asset's part is not a progress bar");
			}
			To->SetWidgetStyle(From->GetWidgetStyle());
			To->SetBarFillStyle(From->GetBarFillStyle()); // phase 6 slice 5d: Scale, the fill's caps kept as in Godot
			return TEXT("restyled (style)");
		}
		if (USizeBox* From = Cast<USizeBox>(Part))
		{
			USizeBox* To = Cast<USizeBox>(Mine);
			if (!To)
			{
				return TEXT("FAILED: the asset's part is not a size box");
			}
			if (From->IsWidthOverride()) { To->SetWidthOverride(From->GetWidthOverride()); } else { To->ClearWidthOverride(); }
			if (From->IsHeightOverride()) { To->SetHeightOverride(From->GetHeightOverride()); } else { To->ClearHeightOverride(); }
			return TEXT("restyled (size)");
		}
		return TEXT("FAILED: no look to copy from a ") + Part->GetClass()->GetName();
	}

	// props: a piece's own settings (the UPROPERTY(EditAnywhere) its native class declares: a button's label, its size...)
	// copied from the code layout's part to the asset's part of the same name: what a code change to a piece instance's
	// settings needs to reach an asset baked before it. Nothing else of the part changes.
	FString CopyPieceProps(UUserWidget* Source, UWidgetBlueprint* BP, const FName Name)
	{
		UWidget* Part = Source->WidgetTree->FindWidget(Name);
		UWidget* Mine = BP->WidgetTree->FindWidget(Name);
		if (!Part || !Mine)
		{
			return FString::Printf(TEXT("FAILED: %s has no %s"), !Part ? TEXT("the code layout") : TEXT("the asset"), *Name.ToString());
		}
		const UClass* Native = Part->GetClass();
		while (Native && Cast<UWidgetBlueprintGeneratedClass>(Native))
		{
			Native = Native->GetSuperClass();
		}
		if (!Native || !Mine->IsA(Native))
		{
			return TEXT("FAILED: not the same piece");
		}
		Mine->Modify();
		TArray<FString> Copied;
		for (TFieldIterator<FProperty> It(Native, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Edit) && !It->HasAnyPropertyFlags(CPF_EditConst))
			{
				It->CopyCompleteValue_InContainer(Mine, Part);
				Copied.Add(It->GetName());
			}
		}
		Mine->SynchronizeProperties();
		return TEXT("restyled (props: ") + FString::Join(Copied, TEXT(", ")) + TEXT(")");
	}

	// names: the asset's widgets, "Name(Class)" (to find a part baked without a name of its own).
	FString AssetNames(UWidgetBlueprint* BP)
	{
		TArray<FString> Out;
		BP->WidgetTree->ForEachWidget([&Out](UWidget* W) { Out.Add(FString::Printf(TEXT("%s(%s)"), *W->GetName(), *W->GetClass()->GetName())); });
		return FString::Join(Out, TEXT(" "));
	}
}

FString UPawWidgetBaker::GraftParts(const FString& ClassName, const FString& Ops)
{
	UClass* Native = nullptr;
	for (UClass* C : PawUi::BakedClasses())
	{
		if (C->GetName() == ClassName)
		{
			Native = C;
		}
	}
	if (!Native || !Native->IsChildOf(UUserWidget::StaticClass()))
	{
		return TEXT("FAILED: no baked widget class ") + ClassName;
	}
	const FString Path = PawUi::AssetPathFor(Native);
	UWidgetBlueprint* BP = LoadObject<UWidgetBlueprint>(nullptr, *(Path + TEXT(".") + PawUi::AssetNameFor(Native)));
	if (!BP || !BP->WidgetTree)
	{
		return TEXT("FAILED: no Widget Blueprint at ") + Path;
	}
	PawUi::SetBaking(true, Native);
	UUserWidget* Source = NewObject<UUserWidget>(GetTransientPackage(), Native, NAME_None, RF_Transient);
	Source->Initialize();
	PawUi::SetBaking(false);
	if (!Source->WidgetTree)
	{
		return TEXT("FAILED: the class built no layout");
	}
	BP->Modify();
	TArray<FString> Lines;
	bool bChanged = false;
	TArray<FString> List;
	Ops.ParseIntoArray(List, TEXT(";"));
	for (const FString& Op : List)
	{
		FString Kind, Arg, Result;
		Op.TrimStartAndEnd().Split(TEXT(":"), &Kind, &Arg);
		FString Old, New;
		if (Kind == TEXT("add"))
		{
			Result = AddPart(Source, BP, Native, FName(*Arg));
		}
		else if (Kind == TEXT("swap") && Arg.Split(TEXT(">"), &Old, &New))
		{
			Result = SwapPart(Source, BP, Native, FName(*Old), FName(*New));
		}
		else if (Kind == TEXT("restyle"))
		{
			Result = RestylePart(Source, BP, FName(*Arg));
		}
		else if (Kind == TEXT("props"))
		{
			Result = CopyPieceProps(Source, BP, FName(*Arg));
		}
		else if (Kind == TEXT("names"))
		{
			Result = AssetNames(BP);
		}
		else
		{
			Result = TEXT("FAILED: not an op");
		}
		bChanged |= Result.StartsWith(TEXT("added")) || Result.StartsWith(TEXT("swapped")) || Result.StartsWith(TEXT("restyled"));
		Lines.Add(FString::Printf(TEXT("%s %s: %s"), *PawUi::AssetNameFor(Native), *Op, *Result));
	}
	if (bChanged)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		FKismetEditorUtilities::CompileBlueprint(BP);
		FString Error;
		if (!SaveAsset(BP, BP->GetOutermost(), Error))
		{
			Lines.Add(TEXT("FAILED: ") + Error);
		}
	}
	PawUi::ResetResolver();
	return FString::Join(Lines, TEXT("\n"));
}

FString UPawWidgetBaker::Recolour(const FString& Pairs)
{
	TMap<uint32, FColor> Map;
	TArray<FString> List;
	Pairs.ParseIntoArray(List, TEXT(";"));
	for (const FString& Pair : List)
	{
		FString Old, New;
		if (!Pair.Split(TEXT(">"), &Old, &New) || Old.Len() != 6 || New.Len() != 6)
		{
			return TEXT("FAILED: not a pair: ") + Pair;
		}
		const FColor From = FColor::FromHex(Old), To = FColor::FromHex(New);
		Map.Add((From.R << 16) | (From.G << 8) | From.B, To);
	}
	TArray<FString> Lines;
	for (UClass* Native : PawUi::BakedClasses())
	{
		const FString Name = PawUi::AssetNameFor(Native);
		UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *(PawUi::AssetPathFor(Native) + TEXT(".") + Name));
		if (!BP)
		{
			Lines.Add(Name + TEXT(" FAILED: no asset"));
			continue;
		}
		// The editable tree (what the designer edits and the compile copies) and the default object.
		TArray<UObject*> Objects;
		if (UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(BP))
		{
			if (WBP->WidgetTree)
			{
				WBP->WidgetTree->ForEachWidget([&Objects](UWidget* W) { Objects.Add(W); });
			}
		}
		if (BP->GeneratedClass)
		{
			Objects.Add(BP->GeneratedClass->GetDefaultObject());
		}
		int32 Written = 0;
		for (UObject* O : Objects)
		{
			O->Modify();
			Written += PawUi::ForEachColour(O, [&Map](FColor& C)
			{
				const FColor* To = Map.Find((C.R << 16) | (C.G << 8) | C.B);
				if (!To)
				{
					return false;
				}
				C = FColor(To->R, To->G, To->B, C.A);
				return true;
			});
		}
		if (Written == 0)
		{
			Lines.Add(Name + TEXT(" unchanged"));
			continue;
		}
		FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
		FKismetEditorUtilities::CompileBlueprint(BP);
		FString Error;
		Lines.Add(SaveAsset(BP, BP->GetOutermost(), Error) ? FString::Printf(TEXT("%s %d recoloured"), *Name, Written)
			: Name + TEXT(" FAILED: ") + Error);
	}
	PawUi::ResetResolver();
	return FString::Join(Lines, TEXT("\n"));
}

FString UPawWidgetBaker::BakeWidgets()
{
	TArray<FString> Lines;
	for (UClass* Native : PawUi::BakedClasses())
	{
		const FString Path = PawUi::AssetPathFor(Native);
		const FString Name = PawUi::AssetNameFor(Native);
		FString Result;
		if (FPackageName::DoesPackageExist(Path))
		{
			Result = TEXT("kept");
		}
		else
		{
			UPackage* Package = CreatePackage(*Path);
			Package->FullyLoad();
			Result = Native->IsChildOf(UUserWidget::StaticClass()) ? BakeWidget(Native, Package, Name) : BakeStyle(Native, Package, Name);
		}
		// The next class must hold the Blueprints baked so far.
		PawUi::ResetResolver();
		Lines.Add(FString::Printf(TEXT("%s %s"), *Name, *Result));
	}
	return FString::Join(Lines, TEXT("\n"));
}
