#include "UI/PawText.h"

#include "Core/PawLog.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"
#include "Misc/Paths.h"
#include "PcFormat.h"
#include "PcHud.h"
#include "PcTreeText.h"

namespace PawText
{
	const FName TableUi(TEXT("ST_UI"));
	const FName TableData(TEXT("ST_Data"));
	const FName TableTree(TEXT("ST_Tree"));

	namespace
	{
		const TCHAR* Folder = TEXT("Pawpocalypse/Localization/Strings/");

		void RegisterOne(const FName Id)
		{
			if (FStringTableRegistry::Get().FindStringTable(Id).IsValid())
			{
				return;
			}
			const FString File = FString(Folder) + Id.ToString() + TEXT(".csv");
			// LOCTABLE_FROMFILE_GAME: the path is relative to the project's Content folder (staged loose: DefaultGame.ini).
			FStringTableRegistry::Get().Internal_LocTableFromFile(Id, Id.ToString(), File, FPaths::ProjectContentDir());
			if (!FStringTableRegistry::Get().FindStringTable(Id).IsValid())
			{
				UE_LOG(LogPaw, Error, TEXT("String table %s did not load from %s%s"), *Id.ToString(), *FPaths::ProjectContentDir(), *File);
			}
		}
	}

	void RegisterTables()
	{
		RegisterOne(TableUi);
		RegisterOne(TableData);
		RegisterOne(TableTree);
	}

	FText Ui(const TCHAR* Key)
	{
		return FText::FromStringTable(TableUi, Key);
	}

	FText Ui(const TCHAR* Key, const FFormatNamedArguments& Args)
	{
		return FText::Format(Ui(Key), Args);
	}

	FText Data(const TCHAR* Prefix, const std::string& Id)
	{
		return FText::FromStringTable(TableData, FString::Printf(TEXT("%s_%s_name"), Prefix, UTF8_TO_TCHAR(Id.c_str())));
	}

	FText DataField(const TCHAR* Prefix, const std::string& Id, const TCHAR* Field)
	{
		return FText::FromStringTable(TableData, FString::Printf(TEXT("%s_%s_%s"), Prefix, UTF8_TO_TCHAR(Id.c_str()), Field));
	}

	FFormatNamedArguments PromptArgs()
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("press"), Ui(TEXT("PROMPT_press")));
		Args.Add(TEXT("buy_amount"), Ui(TEXT("PROMPT_buy_amount")));
		Args.Add(TEXT("zoom_in"), Ui(TEXT("PROMPT_zoom_in")));
		Args.Add(TEXT("zoom_out"), Ui(TEXT("PROMPT_zoom_out")));
		Args.Add(TEXT("back"), Ui(TEXT("PROMPT_back")));
		Args.Add(TEXT("pounce"), Ui(TEXT("PROMPT_pounce")));
		return Args;
	}

	FText TreeLabel(const std::string& Id)
	{
		return FText::FromStringTable(TableTree, FString::Printf(TEXT("TREE_%s_label"), UTF8_TO_TCHAR(Id.c_str())));
	}

	FText TreeBlurb(const std::string& Id)
	{
		return FText::Format(FText::FromStringTable(TableTree, FString::Printf(TEXT("TREE_%s_blurb"), UTF8_TO_TCHAR(Id.c_str()))), PromptArgs());
	}

	FText TreeEffect(const std::vector<Pc::PcTreePhrase>& Parts)
	{
		TArray<FText> Texts;
		for (const Pc::PcTreePhrase& P : Parts)
		{
			const FText Phrase = FText::FromStringTable(TableTree, UTF8_TO_TCHAR(P.Key.c_str()));
			if (P.Arg.empty())
			{
				Texts.Add(Phrase);
				continue;
			}
			// The number as GDScript printed it: culture-invariant, as Godot's (phase 7 decides the separators).
			FFormatNamedArguments Args;
			Args.Add(UTF8_TO_TCHAR(P.Arg.c_str()), FText::AsCultureInvariant(FString(UTF8_TO_TCHAR(P.Number.c_str()))));
			Texts.Add(FText::Format(Phrase, Args));
		}
		return FText::Join(Ui(TEXT("TREE_list_sep")), Texts);
	}

	FText Amount(double Value)
	{
		return FText::AsCultureInvariant(FString(UTF8_TO_TCHAR(Pc::ShortAmount(Value).c_str())));
	}

	FText CardNumber(double Value)
	{
		return FText::AsCultureInvariant(FString(UTF8_TO_TCHAR(Pc::CardNumber(Value).c_str())));
	}

	FText Count(int64 Value)
	{
		return FText::AsCultureInvariant(FString::Printf(TEXT("%lld"), static_cast<long long>(Value)));
	}

	FText TwoDigits(int64 Value)
	{
		return FText::AsCultureInvariant(FString::Printf(TEXT("%02lld"), static_cast<long long>(Value)));
	}

	FText Fixed(double Value, int32 Decimals)
	{
		return FText::AsCultureInvariant(FString(UTF8_TO_TCHAR(Pc::FixedLikeGodot(Value, FMath::Max(0, Decimals)).c_str())));
	}

	FText Duration(double Seconds)
	{
		const Pc::PcDuration T = Pc::DurationParts(Seconds);
		FFormatNamedArguments Args;
		Args.Add(TEXT("s"), TwoDigits(T.S));
		if (T.bHours)
		{
			Args.Add(TEXT("h"), Count(T.H));
			Args.Add(TEXT("m"), TwoDigits(T.M));
			return Ui(TEXT("DURATION_hms"), Args);
		}
		Args.Add(TEXT("m"), Count(T.M));
		return Ui(TEXT("DURATION_ms"), Args);
	}

	bool HasUi(const TCHAR* Key)
	{
		const FStringTableConstPtr Table = FStringTableRegistry::Get().FindStringTable(TableUi);
		FString Out;
		return Table.IsValid() && Table->GetSourceString(FTextKey(Key), Out);
	}
}
