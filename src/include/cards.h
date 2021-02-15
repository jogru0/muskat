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

	[[nodiscard]] inline auto to_suit(Card card) {
		return static_cast<Suit>(static_cast<size_t>(card) / 8);
	}

	[[nodiscard]] inline auto to_rank(Card card) {
		return static_cast<Rank>(static_cast<size_t>(card) % 8);
	}

	enum class GameType {
		Schell, Herz, Green, Eichel, Null, Grand
	};

	enum class TrickType {
		Schell, Herz, Green, Eichel, Trump
	};

	template<typename To, typename From>
	[[nodiscard]] inline auto convert_between_suit_types(From from) {
		static_assert(stdc::is_any_of_v<From, GameType, TrickType, Suit>);
		static_assert(stdc::is_any_of_v<To, GameType, TrickType, Suit>);
		static_assert(!std::is_same_v<From, To>);

		auto val = static_cast<size_t>(from);
		assert(val < 4);
		return static_cast<To>(val);
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
	

	
	class Cards {
	private:
		std::bitset<32> m_bitset;
	public:
		explicit constexpr Cards(uint32_t bits) : m_bitset{bits} {}
		explicit constexpr Cards() = default;

		[[nodiscard]] constexpr auto operator[](Card card) const {
			return m_bitset[static_cast<size_t>(card)];
		}

		[[nodiscard]] auto operator[](Card card) {
			return m_bitset[static_cast<size_t>(card)];
		}

		[[nodiscard]] auto size() const {
			return m_bitset.count();
		}

		[[nodiscard]] auto empty() const {
			return m_bitset.none();
		}

		auto &operator&=(const Cards &other) noexcept {
			m_bitset &= other.m_bitset;
			return *this;
		}
		auto &operator|=(const Cards &other) noexcept {
			m_bitset |= other.m_bitset;
			return *this;
		}
		auto &operator^=(const Cards &other) noexcept {
			m_bitset ^= other.m_bitset;
			return *this;
		}

		[[nodiscard]] auto operator~() noexcept {
			auto result = *this;
			result.m_bitset.flip();
			return result;
		}

		[[nodiscard]] auto operator==(const Cards &other) const noexcept {
			return m_bitset == other.m_bitset;
		}
	};

	[[nodiscard]] inline auto operator&(Cards lhs, const Cards &rhs) noexcept {
		return lhs &= rhs;
	}
	[[nodiscard]] inline auto operator|(Cards lhs, const Cards &rhs) noexcept {
		return lhs |= rhs;
	}
	[[nodiscard]] inline auto operator^(Cards lhs, const Cards &rhs) noexcept {
		return lhs ^= rhs;
	}

	inline constexpr auto number_of_cards_per_suit = 8;

	inline constexpr auto cards_of_suit(Suit suit) {
		return Cards{0b00000000'00000000'00000000'11111111u << (number_of_cards_per_suit * static_cast<size_t>(suit))};
	};
	
	inline constexpr auto cards_of_rank(Rank rank) {
		return Cards{0b00000001'00000001'00000001'00000001u << static_cast<size_t>(rank)};
	}

	inline constexpr auto buben = cards_of_rank(Rank::U);

	

	inline constexpr auto trump_cards(GameType game) {
		switch (game) {
			case GameType::Null:
				return Cards{};
			case GameType::Grand:
				return buben;
			case GameType::Schell: [[fallthrough]];
			case GameType::Herz: [[fallthrough]];
			case GameType::Green: [[fallthrough]];
			case GameType::Eichel:
				return cards_of_suit(convert_between_suit_types<Suit>(game)) | buben;
		}
	}

	inline constexpr auto cards_following_trick_type(TrickAndGameType type) {
		auto current_trump_cards = trump_cards(type.game());
		
		return type.trick() == TrickType::Trump
			? current_trump_cards
			: cards_of_suit(convert_between_suit_types<Suit>(type.trick())) & ~current_trump_cards;
	}

	inline auto legal_response_cards(Cards hand, TrickAndGameType type) {
		assert(!hand.empty());
		
		auto possible_cards_following_trick_type = hand & cards_following_trick_type(type);

		auto result = possible_cards_following_trick_type.empty()
			? hand
			: possible_cards_following_trick_type;

		assert(!result.empty());
		return result;
	}



} // namespace muskat
