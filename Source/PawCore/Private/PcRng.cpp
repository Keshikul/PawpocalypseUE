#include "PcRng.h"

namespace Pc
{
	namespace
	{
		uint64_t Mix(uint64_t X)
		{
			X += 0x9E3779B97F4A7C15ull;
			X = (X ^ (X >> 30)) * 0xBF58476D1CE4E5B9ull;
			X = (X ^ (X >> 27)) * 0x94D049BB133111EBull;
			return X ^ (X >> 31);
		}
	}

	uint64_t RngHash(uint64_t Seed, uint64_t Tick, uint64_t Entity, uint64_t Salt)
	{
		// Chained rather than XOR-combined, so (a, b) and (b, a) give different streams.
		return Mix(Mix(Mix(Mix(Seed) ^ Tick) ^ Entity) ^ Salt);
	}

	double RngUnit(uint64_t Seed, uint64_t Tick, uint64_t Entity, uint64_t Salt)
	{
		return static_cast<double>(RngHash(Seed, Tick, Entity, Salt) >> 11) * (1.0 / 9007199254740992.0);
	}

	uint64_t RngBelow(uint64_t Count, uint64_t Seed, uint64_t Tick, uint64_t Entity, uint64_t Salt)
	{
		// High 64 bits of the 128-bit product, built from 32-bit halves (portable; no compiler intrinsics).
		const uint64_t X = RngHash(Seed, Tick, Entity, Salt);
		const uint64_t XHi = X >> 32, XLo = X & 0xFFFFFFFFull;
		const uint64_t CHi = Count >> 32, CLo = Count & 0xFFFFFFFFull;
		const uint64_t LoLo = XLo * CLo;
		const uint64_t HiLo = XHi * CLo;
		const uint64_t LoHi = XLo * CHi;
		const uint64_t HiHi = XHi * CHi;
		const uint64_t Cross = (LoLo >> 32) + (HiLo & 0xFFFFFFFFull) + LoHi;
		return HiHi + (HiLo >> 32) + (Cross >> 32);
	}
}
