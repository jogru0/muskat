#pragma once

#include <stdc/literals.h>
#include <stdc/type_traits.h>

#include <bitset>

#include <climits>
#include <cassert>

namespace muskat {
	
	//We cast these to uint_fast8_t
	enum class Card {
		S7, S8, S9, SZ, SU, SO, SK, SA,
		H7, H8, H9, HZ, HU, HO, HK, HA,
		G7, G8, G9, GZ, GU, GO, GK, GA,
		E7, E8, E9, EZ, EU, EO, EK, EA
	};
	
	enum class Suit {
		S, H, G, E
	};

	enum class Rank {
		L7, L8, L9, Z, U, O, K, A
	};

	enum class GameType {
		Schell, Herz, Green, Eichel, Null, Grand
	};

	enum class TrickType {
		Schell, Herz, Green, Eichel, Trump
	};

	template<typename TypeOfType>
	[[nodiscard]] inline auto to_suit(TypeOfType type) {
		static_assert(stdc::is_any_of_v<TypeOfType, GameType, TrickType>);
		
		auto val = static_cast<size_t>(type);
		assert(val < 4);
		return static_cast<Suit>(val);
	}

	class TrickAndGameType {
	private:
		TrickType m_trick;
		GameType m_game;

	public:
		constexpr TrickAndGameType(TrickType trick, GameType game) :
			m_trick{trick},
			m_game{game}
		{
			assert(static_cast<size_t>(m_trick) != static_cast<size_t>(m_game));
		}

		[[nodiscard]] auto trick() const {return m_trick; }
		[[nodiscard]] auto game() const {return m_game; }
	};
	

	
	using SetOfCards = std::bitset<32>;

	inline constexpr auto number_of_cards_per_suit = 8;

	inline constexpr auto cards_of_suit(Suit suit) {
		return SetOfCards{std::bitset<32>{0b00000000'00000000'00000000'11111111u << (number_of_cards_per_suit * static_cast<size_t>(suit))}};
	};
	
	inline constexpr auto cards_of_rank(Rank rank) {
		return SetOfCards{std::bitset<32>{0b00000001'00000001'00000001'00000001u << static_cast<size_t>(rank)}};
	}

	inline constexpr auto buben = cards_of_rank(Rank::U);

	

	inline constexpr auto trump_cards(GameType game) {
		switch (game) {
			case GameType::Null:
				return SetOfCards{};
			case GameType::Grand:
				return buben;
			case GameType::Schell: [[fallthrough]];
			case GameType::Herz: [[fallthrough]];
			case GameType::Green: [[fallthrough]];
			case GameType::Eichel:
				return cards_of_suit(to_suit(game)) | buben;
		}
	}

	inline constexpr auto cards_following_trick_type(TrickAndGameType type) {
		auto current_trump_cards = trump_cards(type.game());
		
		return type.trick() == TrickType::Trump
			? current_trump_cards
			: cards_of_suit(to_suit(type.trick())) & ~current_trump_cards;
	}

	inline auto legal_response_cards(SetOfCards hand, TrickAndGameType type) {
		assert(hand.any());
		
		auto possible_cards_following_trick_type = hand & cards_following_trick_type(type);

		auto result = possible_cards_following_trick_type.none()
			? hand
			: possible_cards_following_trick_type;

		assert(result.any());
		return result;
	}



} // namespace muskat
