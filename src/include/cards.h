#pragma once

#include <stdc/literals.h>
#include <stdc/type_traits.h>

#include <bitset>
#include <random>

#include <climits>
#include <cassert>

#include "card.h"

namespace muskat {

namespace detail {
	[[nodiscard]] constexpr inline auto to_bit(Card card) -> uint32_t {
		return uint32_t{1} << to_underlying(card);
	}
}

class Cards {
private:
	uint32_t m_bits;

public:
	explicit constexpr Cards(uint32_t bits = 0) : m_bits{bits} {}

	[[nodiscard]] constexpr auto contains(Card card) const -> bool {
		return (m_bits & detail::to_bit(card)) != 0;
	}

	void add(Card card) {
		assert(!contains(card));
		m_bits |= detail::to_bit(card);
	}

	void remove(Card card) {
		assert(contains(card));
		m_bits &= ~detail::to_bit(card);
	}

	//ddd dsddsd
	[[nodiscard]] auto empty() const {
		return m_bits == 0;
	}

	[[nodiscard]] constexpr auto size() const {
		static_assert(std::is_same_v<uint32_t, unsigned int>);
		return static_cast<size_t>(__builtin_popcount(m_bits));
	}
	
	[[nodiscard]] auto remove_next() {
		assert(!empty());
		auto next_bit = m_bits & -m_bits;
		auto id_card = __builtin_ctz(next_bit);
		m_bits ^= next_bit;
		return static_cast<Card>(id_card);
	}


	auto &operator&=(const Cards &other) noexcept {
		m_bits &= other.m_bits;
		return *this;
	}
	auto &operator|=(const Cards &other) noexcept {
		m_bits |= other.m_bits;
		return *this;
	}
	auto &operator^=(const Cards &other) noexcept {
		m_bits ^= other.m_bits;
		return *this;
	}
	auto &flip() noexcept {
		m_bits = ~m_bits;
		return *this;
	}


	[[nodiscard]] auto operator==(const Cards &other) const noexcept {
		return m_bits == other.m_bits;
	}

	friend std::hash<Cards>;
	friend auto hash_32(Cards) -> uint32_t;
};

[[nodiscard]] auto hash_32(Cards cards) -> uint32_t {
	return cards.m_bits;
}

} //namespace muskat

namespace std {
	template<>
	struct hash<muskat::Cards> {
		[[nodiscard]] auto operator()(const muskat::Cards &cards) const
			-> size_t
		{
			return hash<uint32_t>{}(cards.m_bits);
		}
	};
} //namespace std

namespace muskat {

[[nodiscard]] inline constexpr auto operator&(Cards lhs, const Cards &rhs) noexcept {
	return lhs &= rhs;
}
[[nodiscard]] inline auto operator|(Cards lhs, const Cards &rhs) noexcept {
	return lhs |= rhs;
}
[[nodiscard]] inline auto operator^(Cards lhs, const Cards &rhs) noexcept {
	return lhs ^= rhs;
}
[[nodiscard]] inline auto operator~(Cards cards) noexcept {
	return cards.flip();
}

[[nodiscard]] inline auto random_card_from(const Cards &cards, std::mt19937 &rng) noexcept {
	assert(!cards.empty());
	for (;;) {
		if (
			auto card = get_random_card(rng);
			cards.contains(card)
		) {
			return card;
		}
	}
}


[[nodiscard]] inline auto to_string(Cards cards) {
	using namespace stdc::literals;
	auto result = "{ "s;
	for (auto i = 0; i < 32; ++i) {
		auto card = static_cast<Card>(i);
		if (cards.contains(card)) {
			result += to_string(card) + " ";
		}
	}
	result += "}";
	return result;
}

[[nodiscard]] inline auto to_points(Cards cards) -> GamePlayPoints {
	auto result = uint8_t{};
	for (auto i = 0; i < 32; ++i) {
		auto card = static_cast<Card>(i);
		if (cards.contains(card)) {
			result += to_points(card);
		}
	}
	return result;
}

// [[nodiscard]] inline auto to_points(Cards cards) -> GamePlayPoints {
// 	auto result = GamePlayPoints{};
// 	while (!cards.empty()) {
// 		auto card = cards.remove_next();
// 		result += to_points(card);
// 	}
// 	return result;
// }

inline constexpr auto number_of_cards_per_suit = 8;

inline constexpr auto cards_of_suit(Suit suit) {
	return Cards{0b00000000'00000000'00000000'11111111u << (number_of_cards_per_suit * static_cast<size_t>(suit))};
};

inline constexpr auto cards_of_rank(Rank rank) {
	return Cards{0b00000001'00000001'00000001'00000001u << static_cast<size_t>(rank)};
}

inline constexpr auto buben = cards_of_rank(Rank::U);

[[nodiscard]] inline constexpr auto trump_cards(GameType game) {
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

} // namespace muskat
