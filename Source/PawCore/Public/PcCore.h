#pragma once

// PawCore's shared base header. Pure C++17: std headers only, never an engine header (docs/03 §2).

// UBT defines PAWCORE_API as DLLEXPORT/DLLIMPORT in modular builds, but those two macros live in the engine's platform
// headers, which PawCore never includes. Define them here with the engine's exact tokens (WindowsPlatform.h,
// UnixPlatform.h), so an engine header included later is an identical, legal redefinition.
#ifndef DLLEXPORT
#if defined(_WIN32)
#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)
#else
#define DLLEXPORT __attribute__((visibility("default")))
#define DLLIMPORT __attribute__((visibility("default")))
#endif
#endif

// The standalone test build (tools/build_pawcore_tests.ps1) links everything into one exe and needs no decoration.
#ifndef PAWCORE_API
#define PAWCORE_API
#endif

namespace Pc
{
	// Identifies the library in logs and in the standalone test exe; proves the game module links PawCore.
	PAWCORE_API const char* LibraryName();
}
