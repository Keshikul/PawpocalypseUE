// Copyright Epic Games, Inc. All Rights Reserved.

#include "Pawpocalypse.h"
#include "HAL/PlatformMisc.h"
#include "Modules/ModuleManager.h"
#include "Platform/PawSteam.h"
#include "UI/PawText.h"

// The game module: registers the string tables before anything can ask for a line (docs/08 §4), then their translations
// (phase 7; the language shown is UPawSettingsSubsystem's to apply). The Steam build starts the Steam API here, before any
// subsystem asks it for the save folder or the language (doc 10 §5); a Shipping Steam build opened outside Steam asks Steam
// to start it and exits.
class FPawpocalypseModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		PawText::RegisterTables();
		PawText::RegisterTranslations();
		// The engine goes on starting until its first frame sees the exit: the tables are there for whatever asks meanwhile.
		if (PawSteam::RestartThroughSteamIfNeeded())
		{
			FPlatformMisc::RequestExit(false, TEXT("Steam relaunch"));
			return;
		}
		PawSteam::Startup();
	}

	virtual void ShutdownModule() override
	{
		PawSteam::Shutdown();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FPawpocalypseModule, Pawpocalypse, "Pawpocalypse");
