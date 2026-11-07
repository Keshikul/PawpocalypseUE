#include "PcBackdrop.h"

#include "PcBackdropTones.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace Pc
{
	namespace
	{
		constexpr double Tau = 6.28318530717958647692; // Math_TAU
		constexpr uint64_t PcgMult = 6364136223846793005ull;
		constexpr uint64_t PcgDefaultInc = 1442695040888963407ull; // PCG_DEFAULT_INC_64, RandomPCG::DEFAULT_INC

		int CountLeadingZeros(uint32_t V)
		{
			int N = 0;
			for (uint32_t Bit = 0x80000000u; Bit && !(V & Bit); Bit >>= 1)
			{
				++N;
			}
			return N;
		}

		// hashfuncs.h
		uint32_t Murmur3One32(uint32_t In, uint32_t Seed = 0x7F07C65u)
		{
			In *= 0xcc9e2d51u;
			In = (In << 15) | (In >> 17);
			In *= 0x1b873593u;
			Seed ^= In;
			Seed = (Seed << 13) | (Seed >> 19);
			return Seed * 5u + 0xe6546b64u;
		}
		uint32_t Fmix32(uint32_t H)
		{
			H ^= H >> 16;
			H *= 0x85ebca6bu;
			H ^= H >> 13;
			H *= 0xc2b2ae35u;
			H ^= H >> 16;
			return H;
		}
		uint32_t HashOneUint64(uint64_t V)
		{
			V = (~V) + (V << 18);
			V = V ^ (V >> 31);
			V = V * 21u;
			V = V ^ (V >> 11);
			V = V + (V << 6);
			V = V ^ (V >> 22);
			return static_cast<uint32_t>(V);
		}
		constexpr uint32_t VariantDictionary = 27; // Variant::DICTIONARY in Godot 4

		// UTF-8 -> code points (the ids are ASCII; this keeps the hash honest for anything else).
		std::vector<uint32_t> CodePoints(const std::string& S)
		{
			std::vector<uint32_t> Out;
			for (size_t I = 0; I < S.size();)
			{
				const unsigned char C = static_cast<unsigned char>(S[I]);
				int N = C < 0x80 ? 1 : (C >> 5) == 0x6 ? 2 : (C >> 4) == 0xE ? 3 : 4;
				uint32_t Cp = N == 1 ? C : N == 2 ? (C & 0x1Fu) : N == 3 ? (C & 0x0Fu) : (C & 0x07u);
				for (int K = 1; K < N && I + K < S.size(); ++K)
				{
					Cp = (Cp << 6) | (static_cast<unsigned char>(S[I + K]) & 0x3Fu);
				}
				Out.push_back(Cp);
				I += static_cast<size_t>(N);
			}
			return Out;
		}

		// BiomeBackdrop's tables (presentation; Data/CodeConstantsMap.csv: biome_backdrop.gd -> presentation).
		enum Motif : int64_t { TUFT, BRICK, TREE, VENT, PIPE, PAW, RING, STAR };
		struct FBiome
		{
			const char* Id;
			int64_t Motif;
			int64_t Count;
		};
		constexpr FBiome Biomes[] = {
			{ "tier_1", TUFT, 34 }, { "tier_2", BRICK, 30 }, { "tier_3", TREE, 26 }, { "tier_4", VENT, 24 },
			{ "tier_5", PIPE, 22 }, { "tier_6", PAW, 28 }, { "tier_7", RING, 20 }, { "tier_8", STAR, 30 },
			{ "tier_9", BRICK, 28 }, { "tier_10", TUFT, 24 }, { "tier_11", BRICK, 32 }, { "tier_12", TUFT, 30 },
			{ "menu", TUFT, 40 },
		};
		constexpr FBiome DefaultBiome = { "", STAR, 24 };

		struct FProps
		{
			const char* Id;
			std::vector<const char*> Scatter;
			const char* Edge;
			const char* Landmark;
			std::vector<const char*> Borders; // BIOME_BORDERS
		};
		const std::vector<FProps>& PropTable()
		{
			static const std::vector<FProps> T = {
				{ "tier_1", { "prop_tier1_grass", "prop_tier1_rock" }, "prop_tier1_bush", nullptr, { "top", "bottom" } },
				{ "tier_2", { "prop_tier2_brick", "prop_tier2_bag" }, "prop_tier2_bin", nullptr, { "bottom" } },
				{ "tier_3", { "prop_tier3_leaves", "prop_tier3_flowers" }, "prop_tier3_tree", nullptr, { "top", "bottom", "left" } },
				{ "tier_4", { "prop_tier4_vent", "prop_tier4_tiles" }, "prop_tier4_chimney", nullptr, { "bottom" } },
				{ "tier_5", { "prop_tier5_pipe", "prop_tier5_puddle" }, "prop_tier5_grate", nullptr, { "top", "bottom" } },
				{ "tier_6", { "prop_tier6_bowl", "prop_tier6_post" }, "prop_tier6_fence", nullptr, { "top", "bottom", "left" } },
				{ "tier_7", { "prop_tier7_shard", "prop_tier7_ring" }, "prop_tier7_arch", nullptr, {} },
				{ "tier_8", { "prop_tier8_star", "prop_tier8_crystal" }, "prop_tier8_rock", nullptr, {} },
				{ "tier_9", { "prop_tier9_crumbs", "prop_tier9_crumbs", "prop_tier9_crumbs", "prop_tier9_crumbs", "prop_tier9_crumbs",
					"prop_tier9_toaster" }, "prop_tier9_shelf", nullptr, { "top" } },
				{ "tier_10", { "prop_tier10_fur", "prop_tier10_fur", "prop_tier10_fur", "prop_tier10_carrier" }, "prop_tier10_chair",
					nullptr, { "bottom" } },
				{ "tier_11", { "prop_tier11_kibble", "prop_tier11_kibble", "prop_tier11_kibble", "prop_tier11_hopper" },
					"prop_tier11_conveyor", nullptr, { "top", "bottom" } },
				{ "tier_12", { "prop_tier12_clump", "prop_tier12_puddle" }, "prop_tier12_scoop", nullptr, {} },
				{ "menu", { "prop_tier1_grass", "prop_tier1_grass", "prop_tier1_rock", "prop_tier3_flowers" }, "prop_tier1_bush",
					"prop_tier3_tree", { "top", "bottom", "left", "right" } },
			};
			return T;
		}

		const PcBackdropTones* TonesOf(const std::string& Id)
		{
			for (const PcBackdropTones& T : BackdropTones)
			{
				if (Id == T.Id)
				{
					return &T;
				}
			}
			return nullptr;
		}

		// Every prop is a square drawing (sweet_biomes.py writes each at S.SIZE): its height over its width is 1.
		constexpr double PropAspect = 1.0;

		// Rect2(At - Vector2(W, H) * 0.5, Vector2(W, H)) in float, then mirrored in place when asked (_mirrored).
		PcBackdropProp CentredRect(float AtX, float AtY, double W, double H)
		{
			const float Wf = static_cast<float>(W), Hf = static_cast<float>(H);
			PcBackdropProp P;
			P.X = AtX - Wf * 0.5f;
			P.Y = AtY - Hf * 0.5f;
			P.W = Wf;
			P.H = Hf;
			return P;
		}

		void Blob(PcGodotRng& Rng, float AtX, float AtY, double Radius, std::vector<double>& Xs, std::vector<double>& Ys)
		{
			const double SlowPhase = static_cast<double>(Rng.Randf()) * Tau;
			const double SlowAmp = Rng.RandfRange(0.10f, 0.32f);
			const double FastAmp = Rng.RandfRange(0.03f, 0.08f);
			for (int I = 0; I < Backdrop::BlobVerts; ++I)
			{
				const double T = static_cast<double>(I) / static_cast<double>(Backdrop::BlobVerts);
				const double Angle = T * Tau;
				const double R = Radius * (1.0 + SlowAmp * std::sin(Angle * 2.0 + SlowPhase) + FastAmp * std::sin(Angle * 7.0 + SlowPhase * 3.0));
				const float Rf = static_cast<float>(R);
				const float Cx = static_cast<float>(std::cos(Angle)), Sy = static_cast<float>(std::sin(Angle));
				Xs.push_back(AtX + Cx * Rf);
				Ys.push_back(AtY + Sy * Rf);
			}
		}
	}

	PcGodotRng::PcGodotRng(uint64_t Seed)
	{
		// pcg32_srandom_r(&pcg, seed, DEFAULT_INC)
		State = 0;
		Inc = (PcgDefaultInc << 1u) | 1u;
		Rand();
		State += Seed;
		Rand();
	}

	uint32_t PcGodotRng::Rand()
	{
		const uint64_t Old = State;
		State = Old * PcgMult + (Inc | 1u);
		const uint32_t Xorshifted = static_cast<uint32_t>(((Old >> 18u) ^ Old) >> 27u);
		const uint32_t Rot = static_cast<uint32_t>(Old >> 59u);
		return (Xorshifted >> Rot) | (Xorshifted << ((0u - Rot) & 31u));
	}

	float PcGodotRng::Randf()
	{
		const uint32_t ProtoExpOffset = Rand();
		if (ProtoExpOffset == 0)
		{
			return 0.0f;
		}
		return std::ldexp(static_cast<float>(Rand() | 0x80000001u), -32 - CountLeadingZeros(ProtoExpOffset));
	}

	float PcGodotRng::RandfRange(float From, float To)
	{
		return Randf() * (To - From) + From;
	}

	uint32_t GodotHashString(const std::string& Utf8)
	{
		uint32_t H = 5381;
		for (const uint32_t C : CodePoints(Utf8))
		{
			H = ((H << 5) + H) + C;
		}
		return H;
	}

	uint32_t GodotHashBiomeConfig(int64_t Motif, int64_t Count)
	{
		uint32_t H = Murmur3One32(VariantDictionary);
		H = Murmur3One32(GodotHashString("motif"), H);
		H = Murmur3One32(HashOneUint64(static_cast<uint64_t>(Motif)), H);
		H = Murmur3One32(GodotHashString("count"), H);
		H = Murmur3One32(HashOneUint64(static_cast<uint64_t>(Count)), H);
		return Fmix32(H);
	}

	PcBackdropLayout BuildBackdrop(const std::string& Id, double W, double H, double EffectBudget)
	{
		PcBackdropLayout L;
		L.Id = Id;
		// The viewport rect is a Vector2: float.
		const float Vw = static_cast<float>(W), Vh = static_cast<float>(H);
		L.Width = Vw;
		L.Height = Vh;
		const std::string ArtId = Id == "menu" ? "tier_1" : Id; // BIOME_ART_ALIAS
		const PcBackdropTones* Tones = TonesOf(ArtId);
		if (!Tones)
		{
			return L; // no floor texture: Godot's flat wash and motifs (unreachable in play), not ported
		}
		L.Floor = "backdrop_" + ArtId;
		L.TonesId = ArtId;
		FBiome Config = DefaultBiome;
		for (const FBiome& B : Biomes)
		{
			if (Id == B.Id)
			{
				Config = B;
			}
		}
		const FProps* Props = nullptr;
		for (const FProps& P : PropTable())
		{
			if (Id == P.Id)
			{
				Props = &P;
			}
		}
		const double Quality = std::max(EffectBudget, Backdrop::MinQuality);

		// _rebuild(): the scatter and the landmarks, from hash(_config).
		struct FShape
		{
			float AtX, AtY;
			double Scale;
			uint32_t Pick;
			bool bFlip;
			double Phase, Hz;
		};
		std::vector<FShape> Shapes;
		{
			PcGodotRng Rng(GodotHashBiomeConfig(Config.Motif, Config.Count));
			const int Count = static_cast<int>(std::round(static_cast<double>(Config.Count) * Quality));
			for (int I = 0; I < Count; ++I)
			{
				FShape S;
				S.AtX = static_cast<float>(static_cast<double>(Rng.Randf()) * Vw);
				S.AtY = static_cast<float>(static_cast<double>(Rng.Randf()) * Vh);
				S.Scale = Rng.RandfRange(0.7f, 1.6f);
				(void)Rng.Randf(); // "turn": the flat motifs' rotation (props are never rotated)
				S.Pick = Rng.Randi();
				S.bFlip = Rng.Randf() < 0.5f;
				S.Phase = static_cast<double>(Rng.Randf()) * Tau;
				S.Hz = Rng.RandfRange(0.35f, 0.7f);
				Shapes.push_back(S);
			}
			if (Props && Props->Landmark)
			{
				struct FMark
				{
					float AtX, AtY;
					double Scale;
					bool bFlip;
					double Phase, Hz;
				};
				std::vector<FMark> Marks;
				constexpr int LandmarkCount = 5;
				for (int I = 0; I < LandmarkCount; ++I)
				{
					float AtX = 0.0f, AtY = 0.0f;
					for (int Attempt = 0; Attempt < 12; ++Attempt)
					{
						const double X = I % 2 == 0 ? Rng.RandfRange(0.05f, 0.24f) : Rng.RandfRange(0.76f, 0.95f);
						AtX = static_cast<float>(X * Vw);
						AtY = static_cast<float>(static_cast<double>(Rng.RandfRange(0.42f, 0.94f)) * Vh);
						bool bClear = true;
						for (const FMark& O : Marks)
						{
							const float Dx = O.AtX - AtX, Dy = O.AtY - AtY;
							if (static_cast<double>(std::sqrt(Dx * Dx + Dy * Dy)) < Backdrop::LandmarkPx * 0.9)
							{
								bClear = false;
								break;
							}
						}
						if (bClear)
						{
							break;
						}
					}
					FMark M;
					M.AtX = AtX;
					M.AtY = AtY;
					M.Scale = Rng.RandfRange(0.8f, 1.15f);
					M.bFlip = Rng.Randf() < 0.5f;
					M.Phase = static_cast<double>(Rng.Randf()) * Tau;
					M.Hz = Rng.RandfRange(0.35f, 0.7f);
					Marks.push_back(M);
				}
				// Kept for after the scatter: _draw() draws the landmarks second.
				for (const FMark& M : Marks)
				{
					const double Wd = Backdrop::LandmarkPx * M.Scale;
					const double Hd = Wd * PropAspect;
					PcBackdropProp P;
					P.Kind = PcBackdropPropKind::Landmark;
					P.Stem = Props->Landmark;
					P.X = M.AtX - static_cast<float>(Wd * 0.5);
					P.Y = M.AtY - static_cast<float>(Hd);
					P.W = static_cast<float>(Wd);
					P.H = static_cast<float>(Hd);
					if (M.bFlip)
					{
						P.W = -P.W;
					}
					P.bSway = true;
					P.Phase = M.Phase;
					P.Hz = M.Hz;
					L.Props.push_back(P); // reordered below
				}
			}
		}

		// _draw_variation(): patches, then puddles.
		{
			PcGodotRng Rng(GodotHashString("variation:" + Id));
			const int Patches = static_cast<int>(std::round(7.0 * Quality));
			for (int I = 0; I < Patches; ++I)
			{
				PcBackdropBlob B;
				const float AtX = static_cast<float>(static_cast<double>(Rng.Randf()) * Vw);
				const float AtY = static_cast<float>(static_cast<double>(Rng.Randf()) * Vh);
				B.AtX = AtX;
				B.AtY = AtY;
				B.Radius = Rng.RandfRange(170.0f, 420.0f);
				Blob(Rng, AtX, AtY, B.Radius, B.Xs, B.Ys);
				L.Blobs.push_back(B);
			}
			bool bWet = Tones->Puddle.A > 0;
			if (!bWet && Id == "menu") // BIOME_WATER_FROM: the menu's puddles reflect the Rooftops' sky
			{
				L.WaterTonesId = "tier_4";
				bWet = true;
			}
			if (bWet)
			{
				const int Puddles = static_cast<int>(std::round(3.0 * Quality));
				for (int I = 0; I < Puddles; ++I)
				{
					PcBackdropBlob B;
					B.bPuddle = true;
					const float AtX = static_cast<float>(static_cast<double>(Rng.Randf()) * Vw);
					const float AtY = static_cast<float>(static_cast<double>(Rng.Randf()) * Vh);
					B.AtX = AtX;
					B.AtY = AtY;
					B.Radius = Rng.RandfRange(110.0f, 230.0f);
					Blob(Rng, AtX, AtY, B.Radius, B.Xs, B.Ys);
					L.Blobs.push_back(B);
				}
			}
		}
		if (!Props)
		{
			return L;
		}

		// The scatter (_draw_prop), drawn first; the landmarks built above follow it.
		std::vector<PcBackdropProp> Out;
		for (const FShape& S : Shapes)
		{
			const size_t Slot = S.Pick % Props->Scatter.size();
			const char* Stem = Props->Scatter[Slot];
			const double Wd = Backdrop::PropPx * S.Scale;
			PcBackdropProp P = CentredRect(S.AtX, S.AtY, Wd, Wd * PropAspect);
			P.Kind = PcBackdropPropKind::Scatter;
			P.Stem = Stem;
			if (S.bFlip)
			{
				P.W = -P.W;
			}
			P.bSway = std::strstr(Stem, "rock") == nullptr; // plants sway; a rock that leans is a rock that is wrong
			P.Phase = P.bSway ? S.Phase : 0.0;
			P.Hz = P.bSway ? S.Hz : 0.0;
			Out.push_back(P);
		}
		Out.insert(Out.end(), L.Props.begin(), L.Props.end());

		// _draw_border_props(): rows of the edge piece along the biome's sides.
		const double EdgeW = Backdrop::EdgePx, EdgeH = EdgeW * PropAspect;
		if (!Props->Borders.empty())
		{
			PcGodotRng Rng(GodotHashString("border:" + Id));
			const double Wd = Backdrop::EdgePx * Backdrop::BorderScale;
			const double Hd = Wd * PropAspect;
			const float OutX = static_cast<float>(Wd) * 0.34f, OutY = static_cast<float>(Hd) * 0.34f;
			const double Step = Backdrop::BorderStepPx / Quality;
			bool bFlip = false;
			for (const char* Side : Props->Borders)
			{
				const std::string S = Side;
				const bool bAcross = S == "top" || S == "bottom";
				double Along = Backdrop::EdgePx * 0.9;
				const double Limit = static_cast<double>(bAcross ? Vw : Vh) - Backdrop::EdgePx * 0.9;
				while (Along < Limit)
				{
					const double Jitter = static_cast<double>(Rng.RandfRange(-0.18f, 0.18f)) * Step;
					float X = 0.0f, Y = 0.0f;
					if (S == "top")
					{
						X = static_cast<float>(Along + Jitter);
						Y = -OutY;
					}
					else if (S == "bottom")
					{
						X = static_cast<float>(Along + Jitter);
						Y = static_cast<float>(static_cast<double>(Vh) - Hd + OutY);
					}
					else if (S == "left")
					{
						X = -OutX;
						Y = static_cast<float>(Along + Jitter);
					}
					else
					{
						X = static_cast<float>(static_cast<double>(Vw) - Wd + OutX);
						Y = static_cast<float>(Along + Jitter);
					}
					PcBackdropProp P;
					P.Kind = PcBackdropPropKind::Border;
					P.Stem = Props->Edge;
					P.X = X;
					P.Y = Y;
					P.W = bFlip ? -static_cast<float>(Wd) : static_cast<float>(Wd);
					P.H = static_cast<float>(Hd);
					P.bSway = true;
					P.Phase = static_cast<double>(Rng.Randf()) * Tau;
					P.Hz = Rng.RandfRange(0.35f, 0.7f);
					Out.push_back(P);
					bFlip = !bFlip;
					Along += Step;
				}
			}
		}

		// _draw_edge_props(): a corner piece in each corner, a third of it off-screen.
		{
			const float OutX = static_cast<float>(EdgeW) * 0.34f, OutY = static_cast<float>(EdgeH) * 0.34f;
			const float Xs[4] = { -OutX, static_cast<float>(static_cast<double>(Vw) - EdgeW + OutX), -OutX,
				static_cast<float>(static_cast<double>(Vw) - EdgeW + OutX) };
			const float Ys[4] = { -OutY, -OutY, static_cast<float>(static_cast<double>(Vh) - EdgeH + OutY),
				static_cast<float>(static_cast<double>(Vh) - EdgeH + OutY) };
			const bool Flips[4] = { false, true, false, true };
			for (int I = 0; I < 4; ++I)
			{
				PcBackdropProp P;
				P.Kind = PcBackdropPropKind::Edge;
				P.Stem = Props->Edge;
				P.X = Xs[I];
				P.Y = Ys[I];
				P.W = Flips[I] ? -static_cast<float>(EdgeW) : static_cast<float>(EdgeW);
				P.H = static_cast<float>(EdgeH);
				P.bSway = true;
				P.Phase = static_cast<double>(I) * 1.7;
				P.Hz = 0.4;
				Out.push_back(P);
			}
		}
		L.Props = std::move(Out);
		return L;
	}

	double BackdropSway(const PcBackdropProp& Prop, double T)
	{
		if (!Prop.bSway)
		{
			return 0.0;
		}
		return std::sin(T * Tau * Prop.Hz + Prop.Phase) * (Backdrop::SwayDeg * Tau / 360.0);
	}

	// --- the skill tree's sky (skill_tree_sky.gd) --------------------------------------------------------------------------

	namespace
	{
		// Color("#RRGGBB"): each channel a byte over 255, in float.
		PcSkyColour Colour(const PcTone& T)
		{
			return { T.R / 255.0f, T.G / 255.0f, T.B / 255.0f, T.A / 255.0f };
		}
	}

	PcSky BuildSky(double W, double H, double EffectBudget)
	{
		PcSky S;
		// get_viewport_rect().size: a Vector2.
		S.Width = static_cast<float>(W);
		S.Height = static_cast<float>(H);
		PcGodotRng Rng(GodotHashString("skill_tree_sky"));
		const double Quality = std::max(EffectBudget, Sky::MinQuality);
		const int Clouds = static_cast<int>(std::round(Sky::CloudCount * Quality));
		for (int I = 0; I < Clouds; ++I)
		{
			PcSkyCloud C;
			const double Angle = static_cast<double>(Rng.Randf()) * Tau;
			C.AtX = static_cast<float>(static_cast<double>(Rng.Randf()) * S.Width);
			C.AtY = static_cast<float>(static_cast<double>(Rng.Randf()) * S.Height);
			C.Radius = Rng.RandfRange(Sky::CloudRadiusMin, Sky::CloudRadiusMax);
			// Vector2(cos, sin) * speed: a float vector times the speed as a float.
			const float Speed = Rng.RandfRange(Sky::CloudDriftMin, Sky::CloudDriftMax);
			C.DriftX = static_cast<float>(std::cos(Angle)) * Speed;
			C.DriftY = static_cast<float>(std::sin(Angle)) * Speed;
			C.HuePeriod = Rng.RandfRange(Sky::CloudHuePeriodMin, Sky::CloudHuePeriodMax);
			C.Phase = static_cast<double>(Rng.Randf()) * Tau;
			C.SlowAmp = Rng.RandfRange(Sky::CloudSlowAmpMin, Sky::CloudSlowAmpMax);
			C.FastAmp = Rng.RandfRange(Sky::CloudFastAmpMin, Sky::CloudFastAmpMax);
			S.Clouds.push_back(C);
		}
		const int Stars = static_cast<int>(std::round(Sky::StarCount * Quality));
		for (int I = 0; I < Stars; ++I)
		{
			PcSkyStar T;
			T.AtX = static_cast<float>(static_cast<double>(Rng.Randf()) * S.Width);
			T.AtY = static_cast<float>(static_cast<double>(Rng.Randf()) * S.Height);
			T.Radius = Rng.RandfRange(Sky::StarRadiusMin, Sky::StarRadiusMax);
			T.Alpha = Rng.RandfRange(Sky::StarAlphaMin, Sky::StarAlphaMax);
			T.Period = Rng.RandfRange(Sky::StarTwinkleMin, Sky::StarTwinkleMax);
			T.Phase = static_cast<double>(Rng.Randf()) * Tau;
			S.Stars.push_back(T);
		}
		return S;
	}

	void PcSky::Step(double Dt)
	{
		Time += Dt;
		const float D = static_cast<float>(Dt);
		for (PcSkyCloud& C : Clouds)
		{
			// cloud["at"] + cloud["drift"] * delta: Vector2 arithmetic, float.
			float X = C.AtX + C.DriftX * D;
			float Y = C.AtY + C.DriftY * D;
			const double R = C.Radius;
			if (X < -R)
			{
				X = static_cast<float>(X + (Width + 2.0 * R));
			}
			else if (X > Width + R)
			{
				X = static_cast<float>(X - (Width + 2.0 * R));
			}
			if (Y < -R)
			{
				Y = static_cast<float>(Y + (Height + 2.0 * R));
			}
			else if (Y > Height + R)
			{
				Y = static_cast<float>(Y - (Height + 2.0 * R));
			}
			C.AtX = X;
			C.AtY = Y;
		}
	}

	PcSkyColour PcSky::BaseColour()
	{
		return Colour(SkyBase);
	}

	PcSkyColour PcSky::StarColour(const PcSkyStar& Star) const
	{
		// Under half the paper cream at its brightest, twinkling by a third of that.
		const double Twinkle = 0.7 + 0.3 * std::sin(Time * Tau / Star.Period + Star.Phase);
		PcSkyColour C = Colour(SkyStar);
		C.A = static_cast<float>(Star.Alpha * Twinkle);
		return C;
	}

	PcSkyColour PcSky::CloudColour(const PcSkyCloud& Cloud) const
	{
		// Breathing between the two violets: Color.lerp, float.
		const double T = 0.5 + 0.5 * std::sin(Time * Tau / Cloud.HuePeriod + Cloud.Phase);
		const float Wt = static_cast<float>(T);
		const PcSkyColour A = Colour(SkyCloud), B = Colour(SkyCloudWarm);
		PcSkyColour C;
		C.R = A.R + Wt * (B.R - A.R);
		C.G = A.G + Wt * (B.G - A.G);
		C.B = A.B + Wt * (B.B - A.B);
		C.A = Sky::CloudAlpha;
		return C;
	}

	void PcSky::CloudOutline(const PcSkyCloud& Cloud, std::vector<double>& Xs, std::vector<double>& Ys) const
	{
		// _blob(): a circle whose radius wobbles per vertex in two octaves, the wobble's phase advancing so it breathes.
		Xs.clear();
		Ys.clear();
		const double Wobble = Time * Tau / Sky::CloudWobblePeriod + Cloud.Phase;
		for (int I = 0; I < Sky::CloudVerts; ++I)
		{
			const double Angle = static_cast<double>(I) / static_cast<double>(Sky::CloudVerts) * Tau;
			const double R = Cloud.Radius * (1.0 + Cloud.SlowAmp * std::sin(Angle * 2.0 + Wobble)
				+ Cloud.FastAmp * std::sin(Angle * 7.0 - Wobble * 0.7));
			const float Rf = static_cast<float>(R);
			const float X = Cloud.AtX + static_cast<float>(std::cos(Angle)) * Rf;
			const float Y = Cloud.AtY + static_cast<float>(std::sin(Angle)) * Rf;
			Xs.push_back(static_cast<double>(X));
			Ys.push_back(static_cast<double>(Y));
		}
	}
}
