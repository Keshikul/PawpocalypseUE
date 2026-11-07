#pragma once

// The biome's floor and the things on it (Godot biome_backdrop.gd, phase 6 slice 2): which floor texture, the patches and
// puddles laid over it, the scattered props, the title garden's trees, the border rows and the four corner pieces, in the
// order Godot draws them. Pure: the realm view and the title screen draw what this decides. Proven against the real Godot
// node in a SubViewport of seven sizes at four effects budgets (Tests/Data/backdrop_godot.tsv): every prop rectangle and
// every outline vertex, to the float.
//
// EXACTNESS. Godot lays the backdrop out with its own RandomNumberGenerator (PCG32) seeded by its own hash() of the biome's
// config and of two strings, so the port carries both (PcGodotRng, GodotHash*): the same seed, the same sequence, the same
// floor. Godot's Vector2 / Rect2 are float32 (real_t), its script numbers double: the layout rounds to float exactly where
// a Godot Vector2 is made.

#include "PcCore.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Pc
{
	// Godot's RandomNumberGenerator (core/math/random_pcg.h): PCG32 with Godot's seeding, randf() in float32.
	struct PAWCORE_API PcGodotRng
	{
		explicit PcGodotRng(uint64_t Seed);
		uint32_t Rand();                         // pcg32_random_r
		float Randf();                           // RandomPCG::randf
		float RandfRange(float From, float To);  // RandomPCG::random(float, float)
		uint32_t Randi() { return Rand(); }

	private:
		uint64_t State = 0;
		uint64_t Inc = 0;
	};

	// Godot's hash() of a String (djb2 over its code points) and of the backdrop's config Dictionary
	// {"motif": int, "count": int} (Dictionary::recursive_hash: murmur3 over each key's and value's hash, fmix32).
	PAWCORE_API uint32_t GodotHashString(const std::string& Utf8);
	PAWCORE_API uint32_t GodotHashBiomeConfig(int64_t Motif, int64_t Count);

	enum class PcBackdropPropKind : uint8_t
	{
		Scatter,  // a scattered prop (PROP_PX x its scale)
		Landmark, // the title garden's trees (LANDMARK_PX)
		Border,   // a row along a side (EDGE_PX x BORDER_SCALE)
		Edge,     // a corner piece (EDGE_PX)
	};

	// One prop drawn: its texture's stem and Godot's Rect2 (top-left, size; a NEGATIVE width is mirrored in place).
	struct PcBackdropProp
	{
		PcBackdropPropKind Kind = PcBackdropPropKind::Scatter;
		std::string Stem;
		double X = 0.0, Y = 0.0, W = 0.0, H = 0.0;
		bool bSway = false;          // sways when the backdrop dances (the title garden); a rock never does
		double Phase = 0.0, Hz = 0.0;
	};

	// A patch (darker, flat) or a puddle (lighter, with a rim): a closed outline of BlobVerts points, star-shaped about At.
	struct PcBackdropBlob
	{
		bool bPuddle = false;
		double AtX = 0.0, AtY = 0.0, Radius = 0.0;
		std::vector<double> Xs, Ys;
	};

	struct PcBackdropLayout
	{
		std::string Id;             // the biome (tier_N) or "menu"
		std::string Floor;          // the floor texture's stem (backdrop_tier_N); the menu borrows tier_1's
		std::string TonesId;        // whose palette tones the patches, puddles and rims wear
		std::string WaterTonesId;   // whose FIELD tone the puddles wear when the biome's own puddle tone is empty ("" = its own)
		double Width = 0.0, Height = 0.0;
		std::vector<PcBackdropBlob> Blobs; // patches first, then puddles (Godot's order)
		std::vector<PcBackdropProp> Props; // scatter, landmarks, border rows, corners (Godot's order)
	};

	namespace Backdrop
	{
		constexpr double TileScale = 0.5;      // BACKDROP_TILE_SCALE: one floor tile at half its texture's size
		constexpr double PatchAlpha = 0.55;
		constexpr double PuddleAlpha = 0.9;
		constexpr double PuddleRimPx = 3.0;
		constexpr double RimAlpha = 0.6;
		constexpr int BlobVerts = 22;
		constexpr double PropPx = 78.0;
		constexpr double EdgePx = 190.0;
		constexpr double LandmarkPx = 230.0;
		constexpr double BorderScale = 0.7;
		constexpr double BorderStepPx = 240.0;
		constexpr double SwayDeg = 3.0;
		constexpr double MinQuality = 0.35;    // the backdrop is the floor, not an effect: it never thins below this
	}

	// _rebuild() + _draw() of BiomeBackdrop for biome Id on a viewport W x H (logical px) at the settings' effect budget
	// (SettingsManager.effect_budget(): 0 off, 0.35 low, 0.7 medium, 1 high). An unknown id gets Godot's DEFAULT_BIOME and
	// no props.
	PAWCORE_API PcBackdropLayout BuildBackdrop(const std::string& Id, double W, double H, double EffectBudget);

	// _draw_swaying's angle (radians, clockwise on screen) for a swaying prop at time T when the backdrop dances.
	PAWCORE_API double BackdropSway(const PcBackdropProp& Prop, double T);

	// THE SKILL TREE'S SKY (skill_tree_sky.gd, phase 6 slice 5d): a dark violet sky, a few clouds drifting and breathing
	// between two violets, dim stars twinkling under them. Laid out once per viewport size by Godot's own RNG seeded with
	// hash("skill_tree_sky") (the same sky every time the tree opens), at the effects budget of that moment; then stepped
	// while the tree is open. Proven against the real node (fixture `rainbow`: sky_layout, sky_frames). Float where Godot's
	// Vector2 / Color are float32.
	namespace Sky
	{
		constexpr int CloudCount = 9;                     // CLOUD_COUNT
		constexpr float CloudRadiusMin = 220.0f, CloudRadiusMax = 520.0f; // CLOUD_RADIUS
		constexpr float CloudAlpha = 0.35f;               // CLOUD_ALPHA
		constexpr float CloudDriftMin = 3.0f, CloudDriftMax = 8.0f;       // CLOUD_DRIFT (px / s)
		constexpr float CloudHuePeriodMin = 40.0f, CloudHuePeriodMax = 75.0f; // CLOUD_HUE_PERIOD (s)
		constexpr double CloudWobblePeriod = 30.0;        // CLOUD_WOBBLE_PERIOD (s)
		constexpr int CloudVerts = 26;                    // CLOUD_VERTS
		constexpr float CloudSlowAmpMin = 0.12f, CloudSlowAmpMax = 0.30f; // the outline's two octaves
		constexpr float CloudFastAmpMin = 0.03f, CloudFastAmpMax = 0.07f;
		constexpr int StarCount = 60;                     // STAR_COUNT
		constexpr float StarRadiusMin = 1.0f, StarRadiusMax = 2.4f;       // STAR_RADIUS
		constexpr float StarAlphaMin = 0.12f, StarAlphaMax = 0.42f;       // STAR_ALPHA: "not too bright"
		constexpr float StarTwinkleMin = 8.0f, StarTwinkleMax = 20.0f;    // STAR_TWINKLE_PERIOD (s)
		constexpr double MinQuality = 0.35;               // maxf(effect_budget(), 0.35)
	}

	struct PcSkyCloud
	{
		float AtX = 0.0f, AtY = 0.0f;       // its centre (a Vector2: float)
		double Radius = 0.0;
		float DriftX = 0.0f, DriftY = 0.0f; // px / s
		double HuePeriod = 0.0, Phase = 0.0, SlowAmp = 0.0, FastAmp = 0.0;
	};
	struct PcSkyStar
	{
		float AtX = 0.0f, AtY = 0.0f;
		double Radius = 0.0, Alpha = 0.0, Period = 0.0, Phase = 0.0;
	};
	// A Godot Color (float32 channels, straight alpha).
	struct PcSkyColour
	{
		float R = 0.0f, G = 0.0f, B = 0.0f, A = 1.0f;
	};

	struct PcSky
	{
		float Width = 0.0f, Height = 0.0f;
		std::vector<PcSkyCloud> Clouds;
		std::vector<PcSkyStar> Stars;
		double Time = 0.0;

		// _process(delta) while the tree is open: the clock, each cloud drifted and wrapped round the far side once it has
		// fully left (a margin of its own radius).
		PAWCORE_API void Step(double Dt);
		// _draw(): the base colour over the whole sky; each star's colour (alpha twinkling); each cloud's colour and outline.
		PAWCORE_API static PcSkyColour BaseColour();
		PAWCORE_API PcSkyColour StarColour(const PcSkyStar& Star) const;
		PAWCORE_API PcSkyColour CloudColour(const PcSkyCloud& Cloud) const;
		// (Godot's float32 points, handed out as doubles: exact.)
		PAWCORE_API void CloudOutline(const PcSkyCloud& Cloud, std::vector<double>& Xs, std::vector<double>& Ys) const;
	};

	// _layout() for a viewport W x H at the effects budget of that moment.
	PAWCORE_API PcSky BuildSky(double W, double H, double EffectBudget);
}
