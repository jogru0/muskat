#pragma once

#include "cards.h"
#include "trick.h"

#include <stdc/utility.h>
#include <stdc/algorithm.h>

#include <tuple>
#include <cassert>
#include <optional>

namespace muskat {

enum class Role {Declarer, FirstDefender, SecondDefender};

[[nodiscard]] constexpr auto next(Role role) {
	switch (role) {
		case Role::Declarer: return Role::FirstDefender;
		case Role::FirstDefender: return Role::SecondDefender;
		case Role::SecondDefender: return Role::Declarer;
	}
}

using Deck = std::array<Card, 32>;

[[nodiscard]] inline auto deal_deck(const Deck &deck) {
		assert(!stdc::contains_duplicates(RANGE(deck)));
		
		auto hand_left = Cards{};
		auto hand_right = Cards{};
		auto hand_self = Cards{};
		auto skat = Cards{};

		hand_left[deck[0]] = true;
		hand_left[deck[1]] = true;
		hand_left[deck[2]] = true;
		
		hand_right[deck[3]] = true;
		hand_right[deck[4]] = true;
		hand_right[deck[5]] = true;
		
		hand_self[deck[6]] = true;
		hand_self[deck[7]] = true;
		hand_self[deck[8]] = true;

		skat[deck[9]] = true;
		skat[deck[10]] = true;

		hand_left[deck[11]] = true;
		hand_left[deck[12]] = true;
		hand_left[deck[13]] = true;
		hand_left[deck[14]] = true;
		
		hand_right[deck[15]] = true;
		hand_right[deck[16]] = true;
		hand_right[deck[17]] = true;
		hand_right[deck[18]] = true;
		
		hand_self[deck[19]] = true;
		hand_self[deck[20]] = true;
		hand_self[deck[21]] = true;
		hand_self[deck[22]] = true;
		
		hand_left[deck[23]] = true;
		hand_left[deck[24]] = true;
		hand_left[deck[25]] = true;
		
		hand_right[deck[26]] = true;
		hand_right[deck[27]] = true;
		hand_right[deck[28]] = true;
		
		hand_self[deck[29]] = true;
		hand_self[deck[30]] = true;
		hand_self[deck[31]] = true;

		return std::tuple{
			hand_self, hand_left, hand_right, skat
		};
}


[[nodiscard]] constexpr auto get_unshuffled_deck() {
	Deck result;
	stdc::generate_dependent(RANGE(result), [](auto i) {
		return static_cast<Card>(i);
	});
	return result;
}

template<typename RNG>
[[nodiscard]] constexpr auto get_shuffled_deck(RNG &rng) {
	auto result = get_unshuffled_deck();
	std::shuffle(RANGE(result), rng);
	return result;
}

class Situation;

[[nodiscard]] inline auto is_at_game_start(const Situation &) -> bool;

class Situation {
private:
	Cards m_hand_declarer;
	Cards m_hand_first_defender;
	Cards m_hand_second_defender;
	std::optional<Card> m_maybe_first_trick_card;
	std::optional<Card> m_maybe_second_trick_card;
	Role m_active_role;

public:

	[[nodiscard]] auto cellar() const {
		auto result = ~(m_hand_declarer | m_hand_first_defender | m_hand_second_defender);
		IMPLIES(m_maybe_first_trick_card, result[*m_maybe_first_trick_card] = false);
		IMPLIES(m_maybe_second_trick_card, result[*m_maybe_first_trick_card] = false);
		return result;
	}

	[[nodiscard]] auto active_role() const {
		return m_active_role;
	}

	[[nodiscard]] auto get_maybe_first_trick_card() const {
		return m_maybe_first_trick_card;
	}

	[[nodiscard]] auto get_maybe_second_trick_card() const {
		return m_maybe_second_trick_card;
	}

	[[nodiscard]] auto hand(Role role) const {
		switch (role) {
			case Role::Declarer: return m_hand_declarer;
			case Role::FirstDefender: return m_hand_first_defender;
			case Role::SecondDefender: return m_hand_second_defender;
		}
	}
	
	explicit Situation(const Deck &deck) :
		m_hand_declarer{},
		m_hand_first_defender{},
		m_hand_second_defender{},
		m_maybe_first_trick_card{},
		m_maybe_second_trick_card{},
		m_active_role{Role::Declarer}
	{
		auto skat = Cards{};
		std::tie(m_hand_declarer, m_hand_first_defender, m_hand_second_defender, skat) = deal_deck(deck);
		
		assert_invariants();
		assert(skat == cellar());
		assert(is_at_game_start(*this));
	}

	private:
	void assert_invariants() const {
		auto number_of_appearing_cards = m_hand_declarer.size() + m_hand_first_defender.size() + m_hand_second_defender.size();
		if (m_maybe_first_trick_card) {
			++number_of_appearing_cards;
		}
		if (m_maybe_second_trick_card) {
			++number_of_appearing_cards;
		}
		
		//All cards appear at most once.
		assert(number_of_appearing_cards + cellar().size() == 32);

		//Trick played in order.
		assert(IMPLIES(m_maybe_second_trick_card, m_maybe_first_trick_card));

		auto number_cards_belonging_to_p = hand(m_active_role).size();
		auto number_cards_belonging_to_np = hand(next(m_active_role)).size() + (m_maybe_second_trick_card ? 1 : 0);
		auto number_cards_belonging_to_nnp = hand(next(next(m_active_role))).size() + (m_maybe_first_trick_card ? 1 : 0);
		assert(stdc::are_all_equal(number_cards_belonging_to_p, number_cards_belonging_to_np, number_cards_belonging_to_nnp));
	}

};

[[nodiscard]] inline auto get_trick_and_game_type(Card &first_trick_card, GameType game) {
	return TrickAndGameType{
		get_trick_type(first_trick_card, game),
		game
	};
}

[[nodiscard]] inline auto next_possible_plays(Situation &sit, GameType game) {
	auto player_hand = sit.hand(sit.active_role());
	
	if (auto maybe_first_trick_card = sit.get_maybe_first_trick_card()) {
		return legal_response_cards(player_hand, get_trick_and_game_type(*maybe_first_trick_card, game));
	}

	return player_hand;
	
}

[[nodiscard]] inline auto is_at_game_start(const Situation &sit) -> bool {
	return sit.cellar().size() == 2 && sit.active_role() == Role::Declarer;
}


} // namespace muskat
