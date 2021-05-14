#pragma once

#include <json.hpp>
#include <fmt/core.h>

#include <stdc/filesystem.h>
#include <stdc/io.h>

#include "cards.h"
#include "trick.h"
#include "situation.h"
#include "world_simulation.h"

#include <exception>

namespace muskat {

	namespace parse {

		struct ParseException : std::runtime_error{
			ParseException(const std::string &str) : std::runtime_error{str} {}
		};

		inline auto card(const nlohmann::json &json) -> Card {
			auto str = static_cast<std::string>(json);

			if (str == "H7") { return Card::H7; }
			if (str == "H8") { return Card::H8; }
			if (str == "H9") { return Card::H9; }
			if (str == "HZ") { return Card::HZ; }
			if (str == "HU") { return Card::HU; }
			if (str == "HO") { return Card::HO; }
			if (str == "HK") { return Card::HK; }
			if (str == "HA") { return Card::HA; }

			if (str == "E7") { return Card::E7; }
			if (str == "E8") { return Card::E8; }
			if (str == "E9") { return Card::E9; }
			if (str == "EZ") { return Card::EZ; }
			if (str == "EU") { return Card::EU; }
			if (str == "EO") { return Card::EO; }
			if (str == "EK") { return Card::EK; }
			if (str == "EA") { return Card::EA; }

			if (str == "S7") { return Card::S7; }
			if (str == "S8") { return Card::S8; }
			if (str == "S9") { return Card::S9; }
			if (str == "SZ") { return Card::SZ; }
			if (str == "SU") { return Card::SU; }
			if (str == "SO") { return Card::SO; }
			if (str == "SK") { return Card::SK; }
			if (str == "SA") { return Card::SA; }

			if (str == "G7") { return Card::G7; }
			if (str == "G8") { return Card::G8; }
			if (str == "G9") { return Card::G9; }
			if (str == "GZ") { return Card::GZ; }
			if (str == "GU") { return Card::GU; }
			if (str == "GO") { return Card::GO; }
			if (str == "GK") { return Card::GK; }
			if (str == "GA") { return Card::GA; }
			
			throw ParseException{fmt::format("Could not parse '{}' as card.", str)};
		}

		inline auto cards(const nlohmann::json &json) -> Cards {
			auto result = Cards{};
			for (auto json_entry : json) {
				auto c = card(json_entry);
				if (result.contains(c)) {
					throw ParseException{fmt::format("Card {} appears in too often in initial situation.", to_string(c))};
				}

				result.add(c);
			}
			return result;
		}

		inline auto moves(const nlohmann::json &json) -> std::vector<Card> {
			auto result = std::vector<Card>{};
			auto cards_in_result = Cards{};
			
			if (json.empty()) {
				return result;
			}

			result.reserve(3 * (json.size() - 1) +  json.back().size());
			
			for (auto json_of_trick : json) {
				for (auto json_of_card : json_of_trick) {
					auto c = card(json_of_card);

					if (cards_in_result.contains(c)) {
						throw ParseException{fmt::format("Card {} is played more than once.", to_string(c))};
					}
					result.push_back(c);
					cards_in_result.add(c);
				}
			}
			return result;
		}

		inline auto position(const nlohmann::json &json) -> Position {
			auto str = static_cast<std::string>(json);

			if (str == "geber") { return Position::Hinterhand; }
			if (str == "hoerer") { return Position::Vorhand; }
			if (str == "sager") { return Position::Mittelhand; }
			
			throw ParseException{fmt::format("Could not parse '{}' as position", str)};
		}

		inline auto game(const nlohmann::json &json) -> GameType {
			auto str = static_cast<std::string>(json);

			if (str == "Eichel") { return GameType::Eichel; }
			if (str == "Schell") { return GameType::Schell; }
			if (str == "Green") { return GameType::Green; }
			if (str == "Herz") { return GameType::Herz; }
			if (str == "Grand") { return GameType::Grand; }
			if (str == "Null") { return GameType::Null; }
			
			throw ParseException{fmt::format("Could not parse '{}' as game", str)};
		}

		template<typename Parser>
		inline auto maybe(const nlohmann::json &json, Parser &&parser)
			-> std::optional<std::invoke_result_t<Parser, nlohmann::json>>
		{
			if (json.is_null()) {
				return std::nullopt;
			}

			return std::forward<Parser>(parser)(json);
		}
	} //namespace parse


	[[nodiscard]] inline auto parse_game_record(const nlohmann::json &json) {
		
		auto my_hand = parse::cards(json.at("hand"));
		auto my_pos = parse::position(json.at("position"));
		auto declarer_pos = parse::position(json.at("game_mode").at("declarer"));
		
		auto active_role = [&]() {
			switch (declarer_pos) {
				case Position::Vorhand: return Role::Declarer;
				case Position::Mittelhand: return Role::SecondDefender;
				case Position::Hinterhand: return Role::FirstDefender;
			}
			assert(false);
		}();

		auto maybe_skat = parse::maybe(json.at("skat"), parse::cards);
		
		auto my_role = [&]() {
			if (my_pos == declarer_pos) {
				return Role::Declarer;
			}

			if (next(my_pos) == declarer_pos) {
				return Role::SecondDefender;
			}
			assert(next(next(my_pos)) == declarer_pos);
			return Role::FirstDefender;
		}();

		if (my_role != Role::Declarer && maybe_skat) {
			throw parse::ParseException{"Skat is visible, but you are not the declarer."};
		}

		auto game = parse::game(json.at("game_mode").at("type"));

		auto worlds = PossibleWorlds{
			my_hand,
			my_role,
			maybe_skat,
			game,
			active_role
		};

		auto moves = parse::moves(json.at("played_cards"));

		auto worlds_copy = worlds;
		for (auto move : moves) {
			if (!worlds_copy.probably_could_be_played_next(move)) {
				throw parse::ParseException{fmt::format("Move {} is impossible in that situation.", to_string(move))};
			}
			worlds_copy.play_card(move);
		}

		//TODO: Check if the resulting worlds_copy has any possible situation left.

		return std::tuple{worlds, moves, my_role};
	}
} // namespace muskat
