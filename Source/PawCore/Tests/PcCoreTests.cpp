#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCore.h"

#include <string>

TEST_CASE("PawCore links into the standalone test exe")
{
	CHECK(std::string(Pc::LibraryName()) == "PawCore");
}

#endif
