// Pawpocalypse.UI.Widgets.*: every UI class is a Widget Blueprint the designer can edit (designer 2026-11-04; PawWidgets.h),
// and each baked asset still has what its C++ class needs (KB U/09 §2.1: the parity test that caught a widget held by two
// properties, buttons registered only in code, a piece nested in an older asset).

// Editor only: a cooked game has no Blueprint generator links (UClass::ClassGeneratedBy) nor property metadata (BindWidget).
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UI/PawButton.h"
#include "UI/PawTokens.h"
#include "UI/PawWidgets.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FWidgetsWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;

		void Open()
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			World = GI->GetWorld();
		}

		void Close()
		{
			PawUi::SetCodeLayoutForced(false);
			if (GI)
			{
				GI->Shutdown();
			}
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}

		UUserWidget* Make(UClass* Class)
		{
			return CreateWidget<UUserWidget>(GI, Class);
		}
	};

	// What a tree holds, comparable across the two modes: each widget's class, and by name the words it shows.
	struct FTreeFacts
	{
		TMap<FString, int32> Classes;
		TMap<FName, FString> Words;
	};

	FString ClassKey(const UWidget* W)
	{
		// A piece's Widget Blueprint and its native class are the same piece.
		const UClass* C = W->GetClass();
		while (C && Cast<UWidgetBlueprintGeneratedClass>(C))
		{
			C = C->GetSuperClass();
		}
		return C ? C->GetName() : FString();
	}

	FTreeFacts Facts(const UUserWidget* Root)
	{
		FTreeFacts Out;
		if (!Root->WidgetTree)
		{
			return Out;
		}
		Root->WidgetTree->ForEachWidget([&Out](UWidget* W)
		{
			// The Common button's internal wrapper is made by every instance; it is not part of the layout.
			if (W->GetClass()->GetFName() == TEXT("CommonButtonInternalBase"))
			{
				return;
			}
			Out.Classes.FindOrAdd(ClassKey(W))++;
			if (const UTextBlock* T = Cast<UTextBlock>(W))
			{
				Out.Words.Add(W->GetFName(), T->GetText().ToString());
			}
			else if (const UPawButton* B = Cast<UPawButton>(W))
			{
				Out.Words.Add(W->GetFName(), FString::Printf(TEXT("%s|%.1f|%.1f|%d|%d"), *B->LabelText.ToString(), B->LabelPx, B->WrapAt,
					B->bAlignLeft ? 1 : 0, B->bFlatLook ? 1 : 0));
			}
		});
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawWidgetsAreBlueprintsTest, "Pawpocalypse.UI.Widgets.AreBlueprints", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Every class in the bake list resolves to its baked Blueprint (none left native), a style to a Blueprint child of it.
bool FPawWidgetsAreBlueprintsTest::RunTest(const FString& Parameters)
{
	PawUi::ResetResolver();
	int32 Count = 0;
	for (UClass* Native : PawUi::BakedClasses())
	{
		UClass* Resolved = PawUi::Resolve(Native);
		const bool bBlueprint = Resolved && Resolved != Native && Resolved->IsChildOf(Native) && Resolved->ClassGeneratedBy != nullptr;
		if (!TestTrue(FString::Printf(TEXT("%s is a Blueprint (%s)"), *Native->GetName(), *PawUi::AssetPathFor(Native)), bBlueprint))
		{
			continue;
		}
		if (Native->IsChildOf(UUserWidget::StaticClass()))
		{
			TestNotNull(FString::Printf(TEXT("%s is a Widget Blueprint"), *Native->GetName()), Cast<UWidgetBlueprintGeneratedClass>(Resolved));
		}
		++Count;
	}
	TestTrue(TEXT("the list is not empty"), Count > 0);
	// The switch for comparisons: forced, every class is its native self.
	PawUi::SetCodeLayoutForced(true);
	for (UClass* Native : PawUi::BakedClasses())
	{
		TestTrue(FString::Printf(TEXT("%s: the code layout when forced"), *Native->GetName()), PawUi::Resolve(Native) == Native);
	}
	PawUi::SetCodeLayoutForced(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawWidgetsPaletteTest, "Pawpocalypse.UI.Widgets.Palette", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// The restyle's palette migration (docs/WORKLIST-RESTYLE-2026-11-07.md): a baked asset saved its colours as values, so after a
// palette change none may still hold a colour the palette retired (PawTokens::PALETTE_RETIRED); `py tools/bake_widgets.py
// --run --recolour` migrates them. Also proves the walker sees colours at all (a test that finds nothing proves nothing).
bool FPawWidgetsPaletteTest::RunTest(const FString& Parameters)
{
	int32 Seen = 0;
	int32 Assets = 0;
	for (UClass* Native : PawUi::BakedClasses())
	{
		UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *(PawUi::AssetPathFor(Native) + TEXT(".") + PawUi::AssetNameFor(Native)));
		if (!TestNotNull(FString::Printf(TEXT("%s has its asset"), *Native->GetName()), BP))
		{
			continue;
		}
		++Assets;
		for (UObject* O : PawUi::LookObjects(BP))
		{
			PawUi::ForEachColour(O, [&](FColor& C)
			{
				++Seen;
				const uint32 Rgb = (static_cast<uint32>(C.R) << 16) | (static_cast<uint32>(C.G) << 8) | C.B;
				TestFalse(FString::Printf(TEXT("%s: %s holds the retired colour #%06X"), *PawUi::AssetNameFor(Native), *O->GetName(), Rgb),
					PawTokens::PALETTE_RETIRED.Contains(Rgb));
				return false;
			});
		}
	}
	TestTrue(TEXT("every baked class has an asset"), Assets == PawUi::BakedClasses().Num());
	TestTrue(FString::Printf(TEXT("the walker saw the assets' colours (%d)"), Seen), Seen > 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawWidgetsParityTest, "Pawpocalypse.UI.Widgets.Parity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Each Widget Blueprint against its class's code layout: every bound part present in both, the same widgets, the same words.
// A designer's edit can make this differ ON PURPOSE (the asset owns the layout): the failure then names what to look at.
bool FPawWidgetsParityTest::RunTest(const FString& Parameters)
{
	FWidgetsWorld W;
	W.Open();
	for (UClass* Native : PawUi::BakedClasses())
	{
		if (!Native->IsChildOf(UUserWidget::StaticClass()))
		{
			continue;
		}
		PawUi::SetCodeLayoutForced(false);
		UClass* Baked = PawUi::Resolve(Native);
		if (Baked == Native)
		{
			AddError(FString::Printf(TEXT("%s has no Widget Blueprint"), *Native->GetName()));
			continue;
		}
		UUserWidget* FromAsset = W.Make(Baked);
		PawUi::SetCodeLayoutForced(true);
		UUserWidget* FromCode = W.Make(Native);
		PawUi::SetCodeLayoutForced(false);
		if (!TestNotNull(TEXT("made from the asset"), FromAsset) || !TestNotNull(TEXT("made from code"), FromCode))
		{
			continue;
		}
		const FString Name = Native->GetName();
		for (TFieldIterator<FObjectPropertyBase> It(Native); It; ++It)
		{
			// Our parts only (the engine's own optional ones, e.g. a Common button's InputActionWidget, are not the layout's).
			if ((It->HasMetaData(TEXT("BindWidget")) || It->HasMetaData(TEXT("BindWidgetOptional")))
				&& It->GetOwnerClass()->GetOutermost() == Native->GetOutermost())
			{
				TestNotNull(FString::Printf(TEXT("%s.%s bound from the asset"), *Name, *It->GetName()), It->GetObjectPropertyValue_InContainer(FromAsset));
				TestNotNull(FString::Printf(TEXT("%s.%s built in code"), *Name, *It->GetName()), It->GetObjectPropertyValue_InContainer(FromCode));
			}
		}
		const FTreeFacts A = Facts(FromAsset);
		const FTreeFacts C = Facts(FromCode);
		for (const TPair<FString, int32>& P : C.Classes)
		{
			TestEqual(FString::Printf(TEXT("%s: how many %s"), *Name, *P.Key), A.Classes.FindRef(P.Key), P.Value);
		}
		for (const TPair<FString, int32>& P : A.Classes)
		{
			TestTrue(FString::Printf(TEXT("%s: the asset's %s are the code's"), *Name, *P.Key), C.Classes.Contains(P.Key));
		}
		for (const TPair<FName, FString>& P : C.Words)
		{
			if (const FString* Got = A.Words.Find(P.Key))
			{
				TestEqual(FString::Printf(TEXT("%s.%s says the same"), *Name, *P.Key.ToString()), *Got, P.Value);
			}
		}
	}
	W.Close();
	return true;
}

#endif
