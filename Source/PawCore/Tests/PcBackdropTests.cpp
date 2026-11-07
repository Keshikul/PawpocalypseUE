#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcBackdrop.h"
#include "PcCsv.h"

#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// The biome backdrop against the REAL biome_backdrop.gd (fixture `backdrop`): the 12 biomes and the title's garden, at seven
// viewport sizes and four effects budgets. Every prop Godot draws (kind, texture, rectangle, sway) and every patch and puddle
// outline (centre, radius, all 22 points), in Godot's order, exactly; and Godot's own hash() of the three seeds.

namespace
{
	std::string DataDir()
	{
		const char* Env = std::getenv("PAWCORE_DATA_DIR");
		return Env ? std::string(Env) : std::string("Data");
	}

	std::vector<std::string> Split(const std::string& S, char Sep)
	{
		std::vector<std::string> Out;
		if (S.empty())
		{
			return Out;
		}
		size_t Start = 0;
		while (true)
		{
			const size_t At = S.find(Sep, Start);
			Out.push_back(S.substr(Start, At == std::string::npos ? std::string::npos : At - Start));
			if (At == std::string::npos)
			{
				return Out;
			}
			Start = At + 1;
		}
	}

	std::vector<std::map<std::string, std::string>> Read(const std::string& Path)
	{
		std::string Text;
		REQUIRE(Pc::ReadTextFile(Path, Text));
		std::istringstream In(Text);
		std::string Line;
		std::vector<std::string> Header;
		std::vector<std::map<std::string, std::string>> Rows;
		while (std::getline(In, Line))
		{
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			const std::vector<std::string> C = Split(Line, '\t');
			if (Header.empty())
			{
				Header = C;
				continue;
			}
			while (C.size() < Header.size() && !Line.empty() && Line.back() == '\t')
			{
				break;
			}
			std::map<std::string, std::string> Row;
			for (size_t I = 0; I < Header.size(); ++I)
			{
				Row[Header[I]] = I < C.size() ? C[I] : std::string();
			}
			Rows.push_back(Row);
		}
		return Rows;
	}

	double Hex(const std::string& S) { return std::strtod(S.c_str(), nullptr); }

	const char* KindLetter(Pc::PcBackdropPropKind K)
	{
		switch (K)
		{
		case Pc::PcBackdropPropKind::Scatter: return "s";
		case Pc::PcBackdropPropKind::Landmark: return "l";
		case Pc::PcBackdropPropKind::Border: return "b";
		default: return "e";
		}
	}
}

TEST_CASE("Backdrop: Godot's RandomNumberGenerator and hash()")
{
	// Godot 4.7.1 (read by running it): seed 12345 -> randi 3017463036, then randf 0.2533..., randf_range(0, 10) ...
	// The fixture's seeds are the real check; this pins the generator's shape on its own.
	Pc::PcGodotRng A(1), B(1), C(2);
	CHECK(A.Rand() == B.Rand());
	CHECK(A.Rand() != C.Rand());
	Pc::PcGodotRng R(7);
	for (int I = 0; I < 1000; ++I)
	{
		const double F = R.Randf();
		CHECK(F >= 0.0);
		CHECK(F <= 1.0);
	}
	CHECK(Pc::GodotHashString("") == 5381u);
	CHECK(Pc::GodotHashString("a") == 5381u * 33u + 97u);
}

TEST_CASE("Backdrop: every draw matches the real biome_backdrop.gd")
{
	const auto Rows = Read(DataDir() + "/../Source/PawCore/Tests/Data/backdrop_godot.tsv");
	REQUIRE(Rows.size() >= 364);
	size_t Props = 0, Blobs = 0, Puddles = 0, Landmarks = 0, Borders = 0, Mirrored = 0, Still = 0;
	std::map<std::string, bool> SeenId;
	for (const auto& R : Rows)
	{
		const std::string Id = R.at("id");
		CAPTURE(Id);
		CAPTURE(R.at("w"));
		CAPTURE(R.at("budget"));
		SeenId[Id] = true;
		// The seeds: Godot's hash() of the config Dictionary and of two strings.
		const std::string ArtId = Id == "menu" ? "tier_1" : Id;
		CHECK(Pc::GodotHashString("variation:" + Id) == std::stoul(R.at("seed_variation")));
		CHECK(Pc::GodotHashString("border:" + Id) == std::stoul(R.at("seed_border")));

		const Pc::PcBackdropLayout L = Pc::BuildBackdrop(Id, std::stod(R.at("w")), std::stod(R.at("h")), Hex(R.at("budget")));
		CHECK(L.Floor == R.at("floor"));
		CHECK(L.TonesId == ArtId);

		const std::vector<std::string> GB = Split(R.at("blobs"), ';');
		REQUIRE(L.Blobs.size() == GB.size());
		for (size_t I = 0; I < GB.size(); ++I)
		{
			const std::vector<std::string> F = Split(GB[I], ',');
			REQUIRE(F.size() == 4);
			const Pc::PcBackdropBlob& B = L.Blobs[I];
			CAPTURE(I);
			CHECK(B.AtX == Hex(F[0]));
			CHECK(B.AtY == Hex(F[1]));
			CHECK(B.Radius == Hex(F[2]));
			const std::vector<std::string> V = Split(F[3], ':');
			REQUIRE(V.size() == B.Xs.size() * 2);
			for (size_t K = 0; K < B.Xs.size(); ++K)
			{
				CHECK(B.Xs[K] == Hex(V[2 * K]));
				CHECK(B.Ys[K] == Hex(V[2 * K + 1]));
			}
			++Blobs;
			Puddles += B.bPuddle ? 1 : 0;
		}

		const std::vector<std::string> GP = Split(R.at("props"), ';');
		REQUIRE(L.Props.size() == GP.size());
		for (size_t I = 0; I < GP.size(); ++I)
		{
			const std::vector<std::string> F = Split(GP[I], ',');
			REQUIRE(F.size() == 9);
			const Pc::PcBackdropProp& P = L.Props[I];
			CAPTURE(I);
			CHECK(std::string(KindLetter(P.Kind)) == F[0]);
			CHECK(P.Stem == F[1]);
			CHECK(P.X == Hex(F[2]));
			CHECK(P.Y == Hex(F[3]));
			CHECK(P.W == Hex(F[4]));
			CHECK(P.H == Hex(F[5]));
			CHECK(P.bSway == (F[6] == "1"));
			CHECK(P.Phase == Hex(F[7]));
			CHECK(P.Hz == Hex(F[8]));
			++Props;
			Landmarks += P.Kind == Pc::PcBackdropPropKind::Landmark ? 1 : 0;
			Borders += P.Kind == Pc::PcBackdropPropKind::Border ? 1 : 0;
			Mirrored += P.W < 0.0 ? 1 : 0;
			Still += P.bSway ? 0 : 1;
		}
	}
	// Coverage: every biome and the garden, puddles, trees, rows, mirrors and still rocks all reached.
	CHECK(SeenId.size() == 13);
	CHECK(Puddles > 0);
	CHECK(Landmarks > 0);
	CHECK(Borders > 0);
	CHECK(Mirrored > 0);
	CHECK(Still > 0);
	MESSAGE("backdrop: " << Rows.size() << " layouts, " << Props << " props, " << Blobs << " outlines (" << Puddles << " puddles)");
}

TEST_CASE("Backdrop: the sway")
{
	Pc::PcBackdropProp P;
	P.bSway = true;
	P.Hz = 0.5;
	P.Phase = 0.0;
	CHECK(Pc::BackdropSway(P, 0.5) == doctest::Approx(3.0 * 3.14159265358979 / 180.0)); // sin(pi / 2): the full 3 degrees
	P.bSway = false;
	CHECK(Pc::BackdropSway(P, 0.5) == 0.0);
}

#endif
