#pragma once

#include <stdc/literals.h>
#include <stdc/type_traits.h>

#include <bitset>
#include <random>

#include <climits>
#include <cassert>

namespace muskat {

	template<typename Enum>
	constexpr auto to_underlying(Enum e) {
		return static_cast<std::underlying_type_t<Enum>>(e);
	}

	enum class Card : uint8_t {
		S7, S8, S9, SZ, SU, SO, SK, SA,
		H7, H8, H9, HZ, HU, HO, HK, HA,
		G7, G8, G9, GZ, GU, GO, GK, GA,
		E7, E8, E9, EZ, EU, EO, EK, EA
	};

	[[nodiscard]] inline auto to_string(Card card) {
		using namespace stdc::literals;
		
		switch (card) {
			case Card::S7: return "S7"s;
			case Card::S8: return "S8"s;
			case Card::S9: return "S9"s;
			case Card::SZ: return "SZ"s;
			case Card::SU: return "SU"s;
			case Card::SO: return "SO"s;
			case Card::SK: return "SK"s;
			case Card::SA: return "SA"s;
			case Card::H7: return "H7"s;
			case Card::H8: return "H8"s;
			case Card::H9: return "H9"s;
			case Card::HZ: return "HZ"s;
			case Card::HU: return "HU"s;
			case Card::HO: return "HO"s;
			case Card::HK: return "HK"s;
			case Card::HA: return "HA"s;
			case Card::G7: return "G7"s;
			case Card::G8: return "G8"s;
			case Card::G9: return "G9"s;
			case Card::GZ: return "GZ"s;
			case Card::GU: return "GU"s;
			case Card::GO: return "GO"s;
			case Card::GK: return "GK"s;
			case Card::GA: return "GA"s;
			case Card::E7: return "E7"s;
			case Card::E8: return "E8"s;
			case Card::E9: return "E9"s;
			case Card::EZ: return "EZ"s;
			case Card::EU: return "EU"s;
			case Card::EO: return "EO"s;
			case Card::EK: return "EK"s;
			case Card::EA: return "EA"s;
		}
	}

	inline std::istream &operator>> (std::istream &is, Card &card)
{
	char c_suit;
	char c_rank;
	is >> c_suit >> c_rank;
	
	auto code = 0;

	switch (c_suit) {
		case 'S': break;
		case 'H': code += 8; break;
		case 'G': code += 16; break;
		case 'E': code += 24; break;
		default: assert(false);
	}

	switch (c_rank) {
		case '7': break;
		case '8': code += 1; break;
		case '9': code += 2; break;
		case 'Z': code += 3; break;
		case 'U': code += 4; break;
		case 'O': code += 5; break;
		case 'K': code += 6; break;
		case 'A': code += 7; break;
		default: assert(false);
	}

	card = static_cast<Card>(code);
	return is;
}

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

[[nodiscard]] inline auto get_random_card(std::mt19937 &rng) {
	return static_cast<Card>(std::uniform_int_distribution{0, 31}(rng));
}

enum class GameType {
	Schell, Herz, Green, Eichel, Null, Grand
};

using PointsOnly = uint8_t;
	
[[nodiscard]] inline auto to_points(Card card, [[maybe_unused]] GameType game) -> PointsOnly {
	assert(game != GameType::Null);
	switch (to_rank(card)) {
	case Rank::L7: [[fallthrough]];
		case Rank::L8: [[fallthrough]];
		case Rank::L9: return 0;
		case Rank::Z: return 10;
		case Rank::U: return 2;
		case Rank::O: return 3;
		case Rank::K: return 4;
		case Rank::A: return 11;
	}
}

[[nodiscard]] inline constexpr auto to_string(GameType game) {
	switch (game) {
		case GameType::Schell: return "Schell";
		case GameType::Herz: return "Herz";
		case GameType::Green: return "Green";
		case GameType::Eichel: return "Eichel";
		case GameType::Null: return "Null";
		case GameType::Grand: return "Grand";
	}
}


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


} // namespace muskat
