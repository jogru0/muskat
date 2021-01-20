#pragma once

#include "set_of_cards.h"

#include <stdc/literals.h>

TEST(set_of_cards, size) {
	using namespace stdc::literals;

	auto set = muskat::SetOfCards{};
	ASSERT_EQ(set.count(), 0_z);

	set = muskat::buben;
	ASSERT_EQ(set.count(), 4);

	set = muskat::cards_of_suit(muskat::Suit::H);
	ASSERT_EQ(set.count(), 8);

	set = muskat::cards_following_trick_type(muskat::TrickAndGameType{muskat::TrickType::Schell, muskat::GameType::Herz});
	ASSERT_EQ(set.count(), 7);
}
