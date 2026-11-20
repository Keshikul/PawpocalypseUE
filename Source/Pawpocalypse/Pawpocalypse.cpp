// Copyright Epic Games, Inc. All Rights Reserved.

#include "Pawpocalypse.h"
#include "Modules/ModuleManager.h"
#include "UI/PawText.h"

// The game module: registers the string tables before anything can ask for a line (docs/08 §4), then their translations
// (phase 7; the language shown is UPawSettingsSubsystem's to apply).
class FPawpocalypseModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		PawText::RegisterTables();
		PawText::RegisterTranslations();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FPawpocalypseModule, Pawpocalypse, "Pawpocalypse");
