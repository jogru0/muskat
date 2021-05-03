#pragma once

#include "concurrent_monte_carlo.h"

#include <json.hpp>

#include <stdc/filesystem.h>
#include <stdc/io.h>


namespace muskat {

	namespace parse {

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

			assert(false);
		}

		inline auto cards(const nlohmann::json &json) -> Cards {
			auto result = Cards{};
			for (auto json_entry : json) {
				result.add(card(json_entry));
			}
			return result;
		}

		inline auto moves(const nlohmann::json &json) -> std::vector<Card> {
			auto result = std::vector<Card>{};
			
			if (json.empty()) {
				return result;
			}

			result.reserve(3 * (json.size() - 1) +  json.back().size());
			
			for (auto json_of_trick : json) {
				for (auto json_of_card : json_of_trick) {
					result.push_back(card(json_of_card));
				}
			}
			return result;
		}

		inline auto position(const nlohmann::json &json) -> Position {
			auto str = static_cast<std::string>(json);

			if (str == "geber") { return Position::Hinterhand; }
			if (str == "hoerer") { return Position::Vorhand; }
			if (str == "sager") { return Position::Mittelhand; }
			
			assert(false);
		}

		inline auto game(const nlohmann::json &json) -> GameType {
			auto str = static_cast<std::string>(json);

			if (str == "Eichel") { return GameType::Eichel; }
			if (str == "Schell") { return GameType::Schell; }
			if (str == "Green") { return GameType::Green; }
			if (str == "Herz") { return GameType::Herz; }
			if (str == "Grand") { return GameType::Grand; }
			if (str == "Null") { return GameType::Null; }
			
			assert(false);
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

	inline void analyze_game(const stdc::fs::path &path_to_json) {
		
		auto fs = stdc::IO::open_file_with_checks_for_reading(path_to_json.string());
		
		auto json = nlohmann::json::parse(fs);
		
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

		auto game = parse::game(json.at("game_mode").at("type"));

		auto worlds = PossibleWorlds{
			my_hand,
			my_role,
			maybe_skat,
			game,
			active_role
		};

		auto moves = parse::moves(json.at("played_cards"));

		auto score = uint8_t{};

		for (auto played_card : moves) {
			
			if (worlds.active_role == my_role) {
				auto remaining_cards = worlds.known_cards_dec_fdef_sdef_skat[static_cast<size_t>(my_role)];
				assert(!remaining_cards.empty());
				auto card = pick_best_card(worlds, score, std::chrono::seconds{10 * remaining_cards.size()});
				std::cout << "\nRecommendation: " << to_string(card) << ".\n";
			}

			score += worlds.play_card(played_card);
			std::cout << "Played: " << to_string(played_card) << ".\n\n";
		}

		if (worlds.active_role == my_role) {
			auto remaining_cards = worlds.known_cards_dec_fdef_sdef_skat[static_cast<size_t>(my_role)];
			if (!remaining_cards.empty()) {
				auto card = pick_best_card(worlds, score, std::chrono::seconds{10 * remaining_cards.size()});
				std::cout << "\nRecommendation: " << to_string(card) << ".\n\n";
			}
		}
	}


} // namespace muskat
