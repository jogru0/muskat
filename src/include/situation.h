#pragma once

#include "cards.h"
#include "trick.h"

#include <stdc/utility.h>
#include <stdc/algorithm.h>
#include <stdc/hasher.h>

#include <tuple>
#include <cassert>
#include <optional>

namespace muskat {

enum class Role {Declarer, FirstDefender, SecondDefender};

[[nodiscard]] inline constexpr auto to_string(Role role) {
	switch (role) {
		case Role::Declarer: return "Declarer";
		case Role::FirstDefender: return "First Defender";
		case Role::SecondDefender: return "Second Defender";
	}
}

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

		hand_left.get_ref(deck[0]) = true;
		hand_left.get_ref(deck[1]) = true;
		hand_left.get_ref(deck[2]) = true;
		
		hand_right.get_ref(deck[3]) = true;
		hand_right.get_ref(deck[4]) = true;
		hand_right.get_ref(deck[5]) = true;
		
		hand_self.get_ref(deck[6]) = true;
		hand_self.get_ref(deck[7]) = true;
		hand_self.get_ref(deck[8]) = true;

		skat.get_ref(deck[9]) = true;
		skat.get_ref(deck[10]) = true;

		hand_left.get_ref(deck[11]) = true;
		hand_left.get_ref(deck[12]) = true;
		hand_left.get_ref(deck[13]) = true;
		hand_left.get_ref(deck[14]) = true;
		
		hand_right.get_ref(deck[15]) = true;
		hand_right.get_ref(deck[16]) = true;
		hand_right.get_ref(deck[17]) = true;
		hand_right.get_ref(deck[18]) = true;
		
		hand_self.get_ref(deck[19]) = true;
		hand_self.get_ref(deck[20]) = true;
		hand_self.get_ref(deck[21]) = true;
		hand_self.get_ref(deck[22]) = true;
		
		hand_left.get_ref(deck[23]) = true;
		hand_left.get_ref(deck[24]) = true;
		hand_left.get_ref(deck[25]) = true;
		
		hand_right.get_ref(deck[26]) = true;
		hand_right.get_ref(deck[27]) = true;
		hand_right.get_ref(deck[28]) = true;
		
		hand_self.get_ref(deck[29]) = true;
		hand_self.get_ref(deck[30]) = true;
		hand_self.get_ref(deck[31]) = true;

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
[[nodiscard]] inline auto is_at_game_end(const Situation &) -> bool;
[[nodiscard]] inline auto next_possible_plays(Situation &, GameType ) -> Cards;

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
		IMPLIES(m_maybe_first_trick_card, result.get_ref(*m_maybe_first_trick_card) = false);
		IMPLIES(m_maybe_second_trick_card, result.get_ref(*m_maybe_second_trick_card) = false);
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

private:
	[[nodiscard]] auto &mutable_hand(Role role) {
		switch (role) {
			case Role::Declarer: return m_hand_declarer;
			case Role::FirstDefender: return m_hand_first_defender;
			case Role::SecondDefender: return m_hand_second_defender;
		}
	}

public:
	explicit Situation(
		Cards hand_declarer,
		Cards hand_first_defender,
		Cards hand_second_defender,
		Cards gedrueckt,
		Role first_active_role
	) :
		m_hand_declarer{hand_declarer},
		m_hand_first_defender{hand_first_defender},
		m_hand_second_defender{hand_second_defender},
		m_maybe_first_trick_card{},
		m_maybe_second_trick_card{},
		m_active_role{first_active_role}
	{
		assert_invariants();
		assert(gedrueckt == cellar());
		assert(is_at_game_start(*this));
	}

	//Final situations.
	explicit Situation(Role winner_of_last_trick = Role::Declarer) :
		m_hand_declarer{},
		m_hand_first_defender{},
		m_hand_second_defender{},
		m_maybe_first_trick_card{},
		m_maybe_second_trick_card{},
		m_active_role{winner_of_last_trick}
	{
		assert_invariants();
		assert(cellar().size() == 32);
		assert(is_at_game_end(*this));
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
public:
	//Returns what points the declarer makes  with this move.
	[[nodiscard]] auto play_card(Card card, GameType game) -> uint_fast16_t {
		assert(next_possible_plays(*this, game)[card]);
		auto &hand = mutable_hand(m_active_role);
		assert(hand[card]);
		hand.get_ref(card) = false;

		m_active_role = next(m_active_role);

		auto result = uint_fast16_t{};

		if (!m_maybe_first_trick_card) {
			m_maybe_first_trick_card = card;
		} else if (!m_maybe_second_trick_card) {
			m_maybe_second_trick_card = card;
		} else {
			auto trick = Trick{
				*m_maybe_first_trick_card,
				*m_maybe_second_trick_card,
				card
			};
			auto pos = trick_winner_position(trick, TrickAndGameType{
				*m_maybe_first_trick_card,
				game
			});

			//Active role currently is Vorhand.
			switch (pos) {
				case Position::Vorhand: break;
				case Position::Mittelhand: m_active_role = next(m_active_role); break;
				case Position::Hinterhand: m_active_role = next(next(m_active_role));
			}

			if (m_active_role == Role::Declarer) {
				result += to_points(trick);
			}
			
			m_maybe_first_trick_card = std::nullopt;
			m_maybe_second_trick_card = std::nullopt;
		}

		assert_invariants();
		return result;
	}
};

[[nodiscard]] inline auto operator==(const Situation &l, const Situation &r) {
	return l.active_role() == r.active_role() &&
		l.get_maybe_first_trick_card() == r.get_maybe_first_trick_card() &&
		l.get_maybe_second_trick_card() == r.get_maybe_second_trick_card() &&
		l.hand(Role::Declarer) == r.hand(Role::Declarer) &&
		l.hand(Role::FirstDefender) == r.hand(Role::FirstDefender) &&
		l.hand(Role::SecondDefender) == r.hand(Role::SecondDefender);
}

} //namespace muskat

namespace std {
	template<>
	struct hash<muskat::Situation> {
		[[nodiscard]] constexpr auto operator()(const muskat::Situation &s) const
			-> size_t
		{
			return stdc::GeneralHasher{}(
				s.active_role(),
				s.get_maybe_first_trick_card(),
				s.get_maybe_second_trick_card(),
				s.hand(muskat::Role::Declarer),
				s.hand(muskat::Role::FirstDefender),
				s.hand(muskat::Role::SecondDefender)
			);
		}
	};
} //namespace std

namespace muskat {

[[nodiscard]] inline auto maybe_trick_game_type(Situation &sit, GameType game)
	-> std::optional<TrickAndGameType>
{
	if (auto maybe_first_trick_card = sit.get_maybe_first_trick_card()) {
		return TrickAndGameType{*maybe_first_trick_card, game};
	}

	return std::nullopt;
	
}

[[nodiscard]] inline auto next_possible_plays(Situation &sit, GameType game) -> Cards {
	auto player_hand = sit.hand(sit.active_role());
	
	return get_legal_cards(player_hand, maybe_trick_game_type(sit, game));
}

[[nodiscard]] inline auto is_at_game_start(const Situation &sit) -> bool {
	return sit.cellar().size() == 2 && !sit.get_maybe_first_trick_card();
}

[[nodiscard]] inline auto is_at_game_end(const Situation &sit) -> bool {
	return sit.cellar().size() == 32 && !sit.get_maybe_first_trick_card();
}

} // namespace muskat
