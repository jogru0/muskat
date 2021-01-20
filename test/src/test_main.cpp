#include <cassert>

#include <gtest/gtest.h>

#include "testsuite_config.h"

#include "test_drillings.h"

int main(int argc, char **argv) {
	assert(std::cout << "Wir asserten beim testen" << std::endl);

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}