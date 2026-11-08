#include "PawFontBuilder.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "FileHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UI/PawFontSpec.h"
#include "UObject/Package.h"

namespace
{
	const UFontFace* Face(const TCHAR* Path, FString& Failed)
	{
		const UFontFace* F = LoadObject<UFontFace>(nullptr, Path);
		if (!F)
		{
			Failed += FString::Printf(TEXT(" no face %s;"), Path);
		}
		return F;
	}

	FTypefaceEntry Entry(const TCHAR* Name, const TCHAR* FacePath, int32 SubFace, FString& Failed)
	{
		FTypefaceEntry E{ FName(Name) };
		E.Font = FFontData(Face(FacePath, Failed), SubFace);
		return E;
	}
}

FString UPawFontBuilder::BuildFaces()
{
	TArray<FString> Lines;
	for (const PawFontSpec::FFace& F : PawFontSpec::Faces)
	{
		const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("SourceArt/fonts") / F.File);
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *File) || Bytes.Num() == 0)
		{
			Lines.Add(FString::Printf(TEXT("%s FAILED: cannot read %s"), F.Name, *File));
			continue;
		}
		const FString PackageName = FString(PawFontSpec::Folder) / F.Name;
		const EFontLoadingPolicy Policy = FCString::Strcmp(F.LoadingPolicy, TEXT("Inline")) == 0 ? EFontLoadingPolicy::Inline
			: FCString::Strcmp(F.LoadingPolicy, TEXT("Stream")) == 0 ? EFontLoadingPolicy::Stream : EFontLoadingPolicy::LazyLoad;

		UFontFace* Old = LoadObject<UFontFace>(nullptr, *(PackageName + TEXT(".") + F.Name));
		if (Old && Old->SourceFilename == File && Old->LoadingPolicy == Policy && Old->GetFontFaceData()->GetData() == Bytes)
		{
			Lines.Add(FString::Printf(TEXT("%s kept"), F.Name));
			continue;
		}
		UPackage* Package = CreatePackage(*PackageName);
		Package->FullyLoad();
		if (Old)
		{
			// InitializeFromBulkData takes only an empty face: the old one leaves the package, a new one takes its name.
			Old->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
			Old->ClearFlags(RF_Public | RF_Standalone);
		}
		UFontFace* Face = NewObject<UFontFace>(Package, FName(F.Name), RF_Public | RF_Standalone | RF_Transactional);
		Face->InitializeFromBulkData(File, EFontHinting::Default, Bytes.GetData(), Bytes.Num());
		Face->LoadingPolicy = Policy;
		if (!Old)
		{
			FAssetRegistryModule::AssetCreated(Face);
		}
		Package->MarkPackageDirty();
		Lines.Add(UEditorLoadingAndSavingUtils::SavePackages({ Package }, false)
			? FString::Printf(TEXT("%s made from %s (%d bytes, %s)"), F.Name, F.File, Bytes.Num(), F.LoadingPolicy)
			: FString::Printf(TEXT("%s FAILED: save"), F.Name));
	}
	return FString::Join(Lines, TEXT("\n"));
}

FString UPawFontBuilder::BuildFont()
{
	const FString ObjectPath = PawFontSpec::Font;
	const FString PackageName = FPackageName::ObjectPathToPackageName(ObjectPath);
	const FString AssetName = FPackageName::ObjectPathToObjectName(ObjectPath);

	UFont* Font = LoadObject<UFont>(nullptr, *ObjectPath);
	bool bCreated = false;
	if (!Font)
	{
		UPackage* Package = CreatePackage(*PackageName);
		Font = NewObject<UFont>(Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Font);
		bCreated = true;
	}

	FString Failed;
	Font->Modify();
	Font->FontCacheType = EFontCacheType::Runtime;
	Font->LegacyFontName = FName(PawFontSpec::Default[0].Name);

	FCompositeFont& C = Font->GetMutableInternalCompositeFont();
	C.DefaultTypeface.Fonts.Reset();
	for (const PawFontSpec::FTypeface& T : PawFontSpec::Default)
	{
		C.DefaultTypeface.Fonts.Add(Entry(T.Name, T.Face, T.SubFace, Failed));
	}

	C.SubTypefaces.Reset();
	for (const PawFontSpec::FSubFont& S : PawFontSpec::SubFonts)
	{
		FCompositeSubFont& Sub = C.SubTypefaces.AddDefaulted_GetRef();
		Sub.EditorName = FName(S.Name);
		Sub.Cultures = S.Cultures;
		Sub.ScalingFactor = 1.0f;
		// The sub-font's one face under every name the default typeface has, so a "Black" title in Korean asks for a
		// name the sub-font knows (its only weight).
		for (const PawFontSpec::FTypeface& T : PawFontSpec::Default)
		{
			const bool bDefault = *S.Face == 0; // the default typeface's faces, name by name (Japanese: M PLUS's two weights)
			Sub.Typeface.Fonts.Add(Entry(T.Name, bDefault ? T.Face : S.Face, bDefault ? T.SubFace : S.SubFace, Failed));
		}
		for (const PawFontSpec::FRange& R : S.Ranges)
		{
			Sub.CharacterRanges.Add(FInt32Range(FInt32Range::BoundsType::Inclusive(R.First), FInt32Range::BoundsType::Inclusive(R.Last)));
		}
	}

	C.FallbackTypeface.Typeface.Fonts.Reset();
	C.FallbackTypeface.ScalingFactor = 1.0f;
	for (const PawFontSpec::FTypeface& T : PawFontSpec::Default)
	{
		C.FallbackTypeface.Typeface.Fonts.Add(Entry(T.Name, PawFontSpec::Fallback.Face, PawFontSpec::Fallback.SubFace, Failed));
	}

	if (!Failed.IsEmpty())
	{
		return TEXT("FAILED") + Failed;
	}

	C.MakeDirty();
	Font->PostEditChange();
	Font->MarkPackageDirty();
	if (!UEditorLoadingAndSavingUtils::SavePackages({ Font->GetPackage() }, false))
	{
		return TEXT("FAILED save ") + ObjectPath;
	}
	return FString::Printf(TEXT("built %s (%s): %d typefaces, %d sub-fonts, fallback"), *ObjectPath, bCreated ? TEXT("new") : TEXT("rewritten"),
		PawFontSpec::Default.Num(), PawFontSpec::SubFonts.Num());
}
