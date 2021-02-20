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

enum class Role : uint8_t {Declarer, FirstDefender, SecondDefender};

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

		hand_left.add(deck[0]);
		hand_left.add(deck[1]);
		hand_left.add(deck[2]);
		
		hand_right.add(deck[3]);
		hand_right.add(deck[4]);
		hand_right.add(deck[5]);
		
		hand_self.add(deck[6]);
		hand_self.add(deck[7]);
		hand_self.add(deck[8]);

		skat.add(deck[9]);
		skat.add(deck[10]);

		hand_left.add(deck[11]);
		hand_left.add(deck[12]);
		hand_left.add(deck[13]);
		hand_left.add(deck[14]);
		
		hand_right.add(deck[15]);
		hand_right.add(deck[16]);
		hand_right.add(deck[17]);
		hand_right.add(deck[18]);
		
		hand_self.add(deck[19]);
		hand_self.add(deck[20]);
		hand_self.add(deck[21]);
		hand_self.add(deck[22]);
		
		hand_left.add(deck[23]);
		hand_left.add(deck[24]);
		hand_left.add(deck[25]);
		
		hand_right.add(deck[26]);
		hand_right.add(deck[27]);
		hand_right.add(deck[28]);
		
		hand_self.add(deck[29]);
		hand_self.add(deck[30]);
		hand_self.add(deck[31]);

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
[[nodiscard]] inline auto next_possible_plays(Situation &, GameType) -> Cards;


class MaybeCard {
private:
	//This can be nocard_internal!
	Card m_card_or_nocard;
	static constexpr auto nocard_internal = static_cast<Card>(32);
public:
	/*explicit*/ constexpr MaybeCard(Card card = nocard_internal) : m_card_or_nocard{card} {}

	explicit operator bool() const { return m_card_or_nocard != nocard_internal; }
	constexpr auto operator*() const {
		assert(m_card_or_nocard != nocard_internal);
		return m_card_or_nocard;
	}

	[[nodiscard]] auto operator==(MaybeCard other) const {
		return m_card_or_nocard == other.m_card_or_nocard;
	}

	friend struct std::hash<MaybeCard>;
	friend auto hash_8(MaybeCard) -> uint8_t;
};

[[nodiscard]] inline auto hash_8(MaybeCard maybe_card) -> uint8_t {
	return static_cast<uint8_t>(maybe_card.m_card_or_nocard);
}

[[nodiscard]] inline auto hash_8(Role role) -> uint8_t {
	return static_cast<uint8_t>(role);
}

inline constexpr auto nocard = MaybeCard{};

class Situation {
private:
	Cards m_hand_declarer;
	Cards m_hand_first_defender;
	Cards m_hand_second_defender;
	MaybeCard m_maybe_first_trick_card;
	MaybeCard m_maybe_second_trick_card;
	Role m_active_role;

public:

	[[nodiscard]] auto remaining_cards_in_hands() const {
		return m_hand_declarer | m_hand_first_defender | m_hand_second_defender;
	}

	[[nodiscard]] auto cellar() const {
		auto result = ~remaining_cards_in_hands();
		if(m_maybe_first_trick_card) {
			result.remove(*m_maybe_first_trick_card);
		}
		if(m_maybe_second_trick_card) {
			result.remove(*m_maybe_second_trick_card);
		}
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
		[[maybe_unused]] Cards gedrueckt,
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

		[[maybe_unused]] auto number_cards_belonging_to_p = hand(m_active_role).size();
		[[maybe_unused]] auto number_cards_belonging_to_np = hand(next(m_active_role)).size() + (m_maybe_second_trick_card ? 1 : 0);
		[[maybe_unused]] auto number_cards_belonging_to_nnp = hand(next(next(m_active_role))).size() + (m_maybe_first_trick_card ? 1 : 0);
		assert(stdc::are_all_equal(number_cards_belonging_to_p, number_cards_belonging_to_np, number_cards_belonging_to_nnp));
	}
public:
	//Returns what points the declarer makes  with this move.
	[[nodiscard]] auto play_card(Card card, GameType game) -> Points {
		assert(next_possible_plays(*this, game).contains(card));
		auto &hand = mutable_hand(m_active_role);
		hand.remove(card);

		m_active_role = next(m_active_role);

		auto result = Points{};

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
				result += to_points(trick, game);
			}
			
			m_maybe_first_trick_card = nocard;
			m_maybe_second_trick_card = nocard;
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
	struct hash<muskat::MaybeCard> {
		[[nodiscard]] auto operator()(muskat::MaybeCard maybe_card) const
			-> size_t
		{
			return std::hash<muskat::Card>{}(maybe_card.m_card_or_nocard);
		}
	};
	
	template<>
	struct hash<muskat::Situation> {
		[[nodiscard]] auto operator()(muskat::Situation s) const
			-> size_t
		{
			static_assert(sizeof(size_t) == 8);
			auto hash_32_cards_in_hands = muskat::hash_32(s.remaining_cards_in_hands());
			auto hash_8_maybe_first_trick_card = muskat::hash_8(s.get_maybe_first_trick_card());
			auto hash_8_maybe_second_trick_card = muskat::hash_8(s.get_maybe_second_trick_card());
			auto hash_8_active_role = muskat::hash_8(s.active_role());
			
			auto result = size_t{};
			result |= hash_8_active_role;
			
			result <<= 8;
			result |= hash_8_maybe_second_trick_card;
			
			result <<= 8;
			result |= hash_8_maybe_first_trick_card;
			
			result <<= 32;
			result |= hash_32_cards_in_hands;

			return result;
			
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
