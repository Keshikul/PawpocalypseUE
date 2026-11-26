// Pawpocalypse.Platform.*: the platform layer (doc 10 §5) in every build that is not the Steam one: no Steam at all in the
// editor and the tests (so nothing automated touches the designer's account), and the parts that decide what Steam is told:
// the presence token, the pad family, the save slots moving into a Steam account's folder.

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Input/PawInputDeviceSubsystem.h"
#include "InputCoreTypes.h"
#include "Misc/AutomationTest.h"
#include "Platform/PawPlatformSubsystem.h"
#include "Platform/PawSteam.h"
#include "Save/PawSaveStorage.h"

namespace
{
	struct FPlatformWorld
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
	};

	TArray<uint8> PlatformBytes(uint8 Seed)
	{
		return TArray<uint8>({ Seed, static_cast<uint8>(Seed + 1), static_cast<uint8>(Seed + 2) });
	}

	int32 FamilyInt(Pc::PcFamily F) { return static_cast<int32>(F); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPlatformOffTest, "Pawpocalypse.Platform.Off",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPlatformOffTest::RunTest(const FString& Parameters)
{
	// The editor build is not the Steam build, and an automated run never starts Steam even in it.
	TestFalse(TEXT("not the Steam build"), PawSteam::IsSteamBuild());
	TestFalse(TEXT("no Steam API"), PawSteam::IsActive());
	TestFalse(TEXT("no platform"), UPawPlatformSubsystem::IsActive());
	TestTrue(TEXT("the slots stay in the SaveGames root"), UPawPlatformSubsystem::SaveUserFolder().IsEmpty());
	TestTrue(TEXT("no platform language: the system's decides"), UPawPlatformSubsystem::PlatformLanguage().IsEmpty());
	TestFalse(TEXT("no translated pad"), UPawPlatformSubsystem::TranslatedPadFamily().IsSet());
	bool bRetry = true;
	TestEqual(TEXT("an achievement write goes nowhere"), PawSteam::WriteAchievements({ TEXT("boss_1") }, bRetry), 0);
	TestFalse(TEXT("and does not ask to be tried again"), bRetry);

	FPlatformWorld W;
	W.Open();
	UPawPlatformSubsystem* P = W.GI->GetSubsystem<UPawPlatformSubsystem>();
	if (TestNotNull(TEXT("the platform subsystem"), P))
	{
		P->Poll();
		TestEqual(TEXT("nothing set"), P->GetAchievementsSet(), 0);
		TestTrue(TEXT("no presence sent (no realm)"), P->GetPresence().IsEmpty());
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPlatformPresenceTest, "Pawpocalypse.Platform.Presence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPlatformPresenceTest::RunTest(const FString& Parameters)
{
	// The token names the biome; tools/steam_files.py writes "#Biome_<id>" for every tier of Data/TierData.csv.
	TestEqual(TEXT("in the realm: the biome"), UPawPlatformSubsystem::PresenceToken(true, TEXT("tier_3")), FString(TEXT("#Biome_tier_3")));
	TestTrue(TEXT("on the title: cleared"), UPawPlatformSubsystem::PresenceToken(false, TEXT("tier_3")).IsEmpty());
	TestTrue(TEXT("no biome yet: cleared"), UPawPlatformSubsystem::PresenceToken(true, FString()).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPlatformPadFamilyTest, "Pawpocalypse.Platform.PadFamily",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPlatformPadFamilyTest::RunTest(const FString& Parameters)
{
	using Fam = Pc::PcFamily;
	// Steam Input's names -> the prompt families (no Nintendo prompts: a Switch pad draws as Generic, Xbox's).
	TestEqual(TEXT("PlayStation"), FamilyInt(UPawPlatformSubsystem::FamilyFromPlatform(TEXT("PlayStation")).Get(Fam::Keyboard)), FamilyInt(Fam::PlayStation));
	TestEqual(TEXT("the Deck"), FamilyInt(UPawPlatformSubsystem::FamilyFromPlatform(TEXT("SteamDeck")).Get(Fam::Keyboard)), FamilyInt(Fam::SteamDeck));
	TestEqual(TEXT("Xbox"), FamilyInt(UPawPlatformSubsystem::FamilyFromPlatform(TEXT("Xbox")).Get(Fam::Keyboard)), FamilyInt(Fam::Xbox));
	TestEqual(TEXT("Switch: generic"), FamilyInt(UPawPlatformSubsystem::FamilyFromPlatform(TEXT("Switch")).Get(Fam::Keyboard)), FamilyInt(Fam::Generic));
	TestFalse(TEXT("none: unset"), UPawPlatformSubsystem::FamilyFromPlatform(NAME_None).IsSet());

	FPlatformWorld W;
	W.Open();
	UPawInputDeviceSubsystem* D = W.GI->GetSubsystem<UPawInputDeviceSubsystem>();
	if (!TestNotNull(TEXT("the device subsystem"), D))
	{
		W.Close();
		return false;
	}
	const auto Press = [D](int32 Device)
	{
		D->ObserveKey(FKeyEvent(EKeys::Gamepad_FaceButton_Bottom, FModifierKeysState(), FInputDeviceId::CreateFromInternalId(Device), false, 0, 0), true);
		D->ObserveKey(FKeyEvent(EKeys::Gamepad_FaceButton_Bottom, FModifierKeysState(), FInputDeviceId::CreateFromInternalId(Device), false, 0, 0), false);
	};
	// No platform: XInput's Xbox.
	Press(3);
	TestEqual(TEXT("no platform: Xbox prompts"), FamilyInt(D->GetFamily()), FamilyInt(Fam::Xbox));
	// Steam Input says the pad is a PlayStation pad: the next press draws PlayStation prompts (the pad was known as Xbox).
	D->SetPadFamilyForTest(Fam::PlayStation);
	Press(3);
	TestEqual(TEXT("Steam Input's PlayStation pad"), FamilyInt(D->GetFamily()), FamilyInt(Fam::PlayStation));
	// A new pad registers with the platform's answer at once.
	D->SetPadFamilyForTest(Fam::SteamDeck);
	Press(4);
	TestEqual(TEXT("the Deck's pad"), FamilyInt(D->GetFamily()), FamilyInt(Fam::SteamDeck));
	// The platform stops answering: a known pad keeps what it was told.
	D->SetPadFamilyForTest(TOptional<Fam>());
	Press(4);
	TestEqual(TEXT("no answer: kept"), FamilyInt(D->GetFamily()), FamilyInt(Fam::SteamDeck));
	D->SetPadFamilyForTest(TOptional<Fam>());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPlatformAdoptSlotsTest, "Pawpocalypse.Platform.AdoptSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPlatformAdoptSlotsTest::RunTest(const FString& Parameters)
{
	const TArray<FString> Slots = { TEXT("slot_0"), TEXT("slot_1"), TEXT("slot_2") };
	{
		// The root holds slot_0 (and its backup), slot_1 and slot_2's backup alone; the Steam folder already has its own slot_1.
		FPawMemorySaveStorage Root;
		FPawMemorySaveStorage User;
		Root.Put(TEXT("slot_0"), PlatformBytes(1));
		Root.Put(TEXT("slot_0_bak"), PlatformBytes(2));
		Root.Put(TEXT("slot_1"), PlatformBytes(3));
		Root.Put(TEXT("slot_2_bak"), PlatformBytes(4));
		Root.Put(TEXT("settings"), PlatformBytes(5));
		User.Put(TEXT("slot_1"), PlatformBytes(9));
		TestEqual(TEXT("slot_0 and slot_2 move"), PawAdoptSlots(Root, User, Slots), 2);
		TArray<uint8> Got;
		TestTrue(TEXT("slot_0 in the folder"), User.Read(TEXT("slot_0"), Got) && Got == PlatformBytes(1));
		TestTrue(TEXT("its backup too"), User.Read(TEXT("slot_0_bak"), Got) && Got == PlatformBytes(2));
		TestTrue(TEXT("a slot with only a backup moves"), User.Read(TEXT("slot_2_bak"), Got) && Got == PlatformBytes(4));
		TestFalse(TEXT("slot_0 gone from the root"), Root.Exists(TEXT("slot_0")) || Root.Exists(TEXT("slot_0_bak")));
		TestTrue(TEXT("the folder's own slot_1 is never overwritten"), User.Read(TEXT("slot_1"), Got) && Got == PlatformBytes(9));
		TestTrue(TEXT("the root's slot_1 stays where it was"), Root.Exists(TEXT("slot_1")));
		TestTrue(TEXT("the settings stay per PC"), Root.Exists(TEXT("settings")) && !User.Exists(TEXT("settings")));
		TestEqual(TEXT("a second start moves nothing"), PawAdoptSlots(Root, User, Slots), 0);
	}
	{
		// A write that fails: nothing deleted from the root, nothing half-copied left in the folder (the next start retries).
		FPawMemorySaveStorage Root;
		FPawMemorySaveStorage User;
		Root.Put(TEXT("slot_0"), PlatformBytes(1));
		Root.Put(TEXT("slot_0_bak"), PlatformBytes(2));
		User.FailWritesAfter(1); // the slot copies, its backup does not
		TestEqual(TEXT("nothing moved"), PawAdoptSlots(Root, User, Slots), 0);
		TestTrue(TEXT("the originals stay"), Root.Exists(TEXT("slot_0")) && Root.Exists(TEXT("slot_0_bak")));
		TestFalse(TEXT("no half slot in the folder"), User.Exists(TEXT("slot_0")) || User.Exists(TEXT("slot_0_bak")));
		User.FailWritesAfter(-1);
		TestEqual(TEXT("the next start moves it"), PawAdoptSlots(Root, User, Slots), 1);
	}
	return true;
}

#endif
