#include <cassert>

#include <gtest/gtest.h>

#include "test_cards.h"
#include "test_solver.h"

int main(int argc, char **argv) {
	std::cout << "Custom test main runs." << std::endl;
	
	auto asserts_are_active = false;
	assert(asserts_are_active = true);
	if (!asserts_are_active) {
		std::cout << "DONT RUN THE TESTS WITHOUT ASSERTS!" << std::endl;
		exit(1);
	}

	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}