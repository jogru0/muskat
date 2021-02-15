#pragma once

#include "cards.h"

#include <stdc/container_wrapper.h>

namespace muskat {

struct Trick : public stdc::ExposedArray<std::array<Card, 3>> {
	using Parent = stdc::ExposedArray<std::array<Card, 3>>;
	
	Trick(Card first, Card second, Card third) :
		Parent{std::array{first, second, third}}
	{}
};

[[nodiscard]] inline auto get_trick_type(Card card, GameType game) {
	if (trump_cards(game)[card]) {
		return TrickType::Trump;
	}
	return convert_between_suit_types<TrickType>(to_suit(card));
}

} // namespace muskat
