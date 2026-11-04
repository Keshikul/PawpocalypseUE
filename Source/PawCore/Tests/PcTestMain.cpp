// L1 unit tests (docs/09 §2): built by tools/build_pawcore_tests.ps1 with a plain compiler, never by UBT. UBT compiles
// every .cpp under a module folder, so each test file is empty unless the standalone build defines the macro.
#if defined(PAWCORE_STANDALONE_TESTS)

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "ThirdParty/doctest/doctest.h"

#endif
