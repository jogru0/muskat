#pragma once
#include <gtest/gtest.h>

#include "cards.h"
#include "trick.h"

#include <stdc/literals.h>

TEST(cards, size) {
	using namespace stdc::literals;

	auto set = muskat::Cards{};
	ASSERT_EQ(set.size(), 0_z);

	set = muskat::buben;
	ASSERT_EQ(set.size(), 4);

	set = muskat::cards_of_suit(muskat::Suit::H);
	ASSERT_EQ(set.size(), 8);

	set = muskat::get_cards_following_trick_type(muskat::TrickAndGameType{muskat::Card::SO, muskat::GameType::Herz});
	ASSERT_EQ(set.size(), 7);
}
