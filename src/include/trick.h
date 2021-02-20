#pragma once

#include "cards.h"

#include <stdc/container_wrapper.h>
#include <stdc/utility.h>


namespace muskat {


enum class Position {
	Vorhand, Mittelhand, Hinterhand
};

[[nodiscard]] inline constexpr auto to_string(Position position) {
	switch (position) {
		case Position::Vorhand: return "Vorhand";
		case Position::Mittelhand: return "Mittelhand";
		case Position::Hinterhand: return "Hinterhand";
	}
}


struct Trick : public stdc::ExposedArray<std::array<Card, 3>> {
	using Parent = stdc::ExposedArray<std::array<Card, 3>>;

	Trick(Card first, Card second, Card third) :
		Parent{std::array{first, second, third}}
	{}
};

[[nodiscard]] inline constexpr auto get_trick_type(Card card, GameType game) {
	if (trump_cards(game).contains(card)) {
		return TrickType::Trump;
	}
	return convert_between_suit_types<TrickType>(to_suit(card));
}

class TrickAndGameType {
private:
	GameType m_game;
	TrickType m_trick;

public:
	constexpr TrickAndGameType(Card card, GameType game) :
		m_game{game},
		m_trick{get_trick_type(card, m_game)}
	{
		assert(static_cast<size_t>(m_trick) != static_cast<size_t>(m_game));
	}

	[[nodiscard]] auto trick() const {return m_trick; }
	[[nodiscard]] auto game() const {return m_game; }
};

[[nodiscard]] inline auto to_points(const Trick &trick, GameType game) {
	return to_points(trick[0], game) +
		to_points(trick[1], game) +
		to_points(trick[2], game);
}

inline constexpr auto cards_following_trick_type(TrickAndGameType type) {
auto current_trump_cards = trump_cards(type.game());

return type.trick() == TrickType::Trump
	? current_trump_cards
	: cards_of_suit(convert_between_suit_types<Suit>(type.trick())) & ~current_trump_cards;
}

using Power = int8_t;

[[nodiscard]] inline auto to_power(Card card, TrickAndGameType type) -> Power {
	auto card_is_trump = trump_cards(type.game()).contains(card);
	auto card_follows_trick_type = cards_following_trick_type(type).contains(card);
	
	if (!card_is_trump && !card_follows_trick_type) {
		return -1;
	}

	auto rank = to_rank(card);

	if (type.game() == GameType::Null) {
		return static_cast<Power>(rank);
	}

	auto result = [&]() -> Power {
		switch (rank) {
			case Rank::Z: return 7;
			case Rank::A: return 8;
			case Rank::U: return 9 + static_cast<Power>(to_suit(card));
			default: return static_cast<Power>(rank);
		}
	}();

	//Trumpf?
	if (card_is_trump) {
		constexpr auto bonus_trump = 10;
		result += bonus_trump;
	}

	return result;
}

[[nodiscard]] inline auto trick_winner_position(const Trick &trick, TrickAndGameType type) -> Position {
	auto power_vorhand = to_power(trick[0], type);
	auto power_mittelhand = to_power(trick[1], type);
	auto power_hinterhand = to_power(trick[2], type);
	
	assert(0 <= power_vorhand);
	assert(power_vorhand != power_mittelhand);
	assert(power_vorhand != power_hinterhand);
	assert(IMPLIES(power_mittelhand == power_hinterhand, power_mittelhand == -1));

	return power_vorhand <= power_mittelhand
		? power_mittelhand <= power_hinterhand
			? Position::Hinterhand
			: Position::Mittelhand
		: power_vorhand <= power_hinterhand
			? Position::Hinterhand
			: Position::Vorhand;
}

[[nodiscard]] inline auto legal_response_cards(Cards hand, TrickAndGameType type) {
	assert(!hand.empty());
	
	auto possible_cards_following_trick_type = hand & cards_following_trick_type(type);

	auto result = possible_cards_following_trick_type.empty()
		? hand
		: possible_cards_following_trick_type;

	assert(!result.empty());
	return result;
}

[[nodiscard]] inline auto get_legal_cards(
	Cards &hand,
	std::optional<TrickAndGameType> maybe_trick_game_type
) {
	return maybe_trick_game_type
		? legal_response_cards(hand, *maybe_trick_game_type)
		: hand;
}

} // namespace muskat
