#pragma once

#include "set_of_cards.h"

#include <stdc/literals.h>

TEST(set_of_cards, size) {
	using namespace stdc::literals;

	auto set = muskat::SetOfCards{};
	ASSERT_EQ(set.size(), 0_z);
}
