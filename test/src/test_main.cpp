#include <cassert>

#include <gtest/gtest.h>

#include "test_set_of_cards.h"

int main(int argc, char **argv) {
	auto asserts_are_active = false;
	assert(asserts_are_active = true);
	if (!asserts_are_active) {
		std::cout << "DONT RUN THE TESTS WITHOUT ASSERTS!" << std::endl;
		exit(1);
	}

	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}