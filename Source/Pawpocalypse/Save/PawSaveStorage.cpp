#include "Save/PawSaveStorage.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "PlatformFeatures.h"
#include "SaveGameSystem.h"

namespace
{
	ISaveGameSystem* SaveSystem()
	{
		return IPlatformFeaturesModule::Get().GetSaveGameSystem();
	}

	// Single-user game: user 0 (the platform's primary user).
	constexpr int32 UserIndex = 0;
}

// --- the platform --------------------------------------------------------------------------------------------------------

bool FPawPlatformSaveStorage::Exists(const FString& Name)
{
	ISaveGameSystem* S = SaveSystem();
	return S && S->DoesSaveGameExist(*Name, UserIndex);
}

bool FPawPlatformSaveStorage::Read(const FString& Name, TArray<uint8>& Out)
{
	ISaveGameSystem* S = SaveSystem();
	return S && S->LoadGame(false, *Name, UserIndex, Out);
}

bool FPawPlatformSaveStorage::Write(const FString& Name, const TArray<uint8>& Data)
{
	ISaveGameSystem* S = SaveSystem();
	return S && S->SaveGame(false, *Name, UserIndex, Data);
}

bool FPawPlatformSaveStorage::Delete(const FString& Name)
{
	ISaveGameSystem* S = SaveSystem();
	return S && S->DeleteGame(false, *Name, UserIndex);
}

// --- a folder ------------------------------------------------------------------------------------------------------------

FPawFolderSaveStorage::FPawFolderSaveStorage(const FString& InFolder)
	: Folder(FPaths::ConvertRelativePathToFull(InFolder))
{
	IFileManager::Get().MakeDirectory(*Folder, true);
}

FString FPawFolderSaveStorage::PathOf(const FString& Name) const
{
	return FPaths::Combine(Folder, Name + TEXT(".sav"));
}

bool FPawFolderSaveStorage::Exists(const FString& Name)
{
	return IFileManager::Get().FileExists(*PathOf(Name));
}

bool FPawFolderSaveStorage::Read(const FString& Name, TArray<uint8>& Out)
{
	return FFileHelper::LoadFileToArray(Out, *PathOf(Name), FILEREAD_Silent);
}

bool FPawFolderSaveStorage::Write(const FString& Name, const TArray<uint8>& Data)
{
	return FFileHelper::SaveArrayToFile(Data, *PathOf(Name));
}

bool FPawFolderSaveStorage::Delete(const FString& Name)
{
	return IFileManager::Get().Delete(*PathOf(Name), false, false, true);
}

// --- memory --------------------------------------------------------------------------------------------------------------

bool FPawMemorySaveStorage::Exists(const FString& Name)
{
	FScopeLock L(&Lock);
	return Entries.Contains(Name);
}

bool FPawMemorySaveStorage::Read(const FString& Name, TArray<uint8>& Out)
{
	FScopeLock L(&Lock);
	if (const TArray<uint8>* Found = Entries.Find(Name))
	{
		Out = *Found;
		return true;
	}
	return false;
}

bool FPawMemorySaveStorage::Write(const FString& Name, const TArray<uint8>& Data)
{
	FScopeLock L(&Lock);
	++Writes;
	if (WritesLeft == 0)
	{
		return false;
	}
	if (WritesLeft > 0)
	{
		--WritesLeft;
	}
	if (!TruncateSuffix.IsEmpty() && Name.EndsWith(TruncateSuffix))
	{
		TArray<uint8> Half(Data.GetData(), Data.Num() / 2);
		Entries.Add(Name, MoveTemp(Half));
		return true;
	}
	Entries.Add(Name, Data);
	return true;
}

bool FPawMemorySaveStorage::Delete(const FString& Name)
{
	FScopeLock L(&Lock);
	return Entries.Remove(Name) > 0;
}

void FPawMemorySaveStorage::FailWritesAfter(int32 N)
{
	FScopeLock L(&Lock);
	WritesLeft = N;
}

void FPawMemorySaveStorage::TruncateWritesTo(const FString& Suffix)
{
	FScopeLock L(&Lock);
	TruncateSuffix = Suffix;
}

void FPawMemorySaveStorage::Put(const FString& Name, const TArray<uint8>& Data)
{
	FScopeLock L(&Lock);
	Entries.Add(Name, Data);
}

TArray<FString> FPawMemorySaveStorage::Names()
{
	FScopeLock L(&Lock);
	TArray<FString> Out;
	Entries.GetKeys(Out);
	Out.Sort();
	return Out;
}

int32 FPawMemorySaveStorage::WriteCount()
{
	FScopeLock L(&Lock);
	return Writes;
}
