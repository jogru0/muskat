#pragma once

#include "concurrent_monte_carlo.h"
#include "parse_game_record.h"

#include <stdc/filesystem.h>
#include <stdc/io.h>

namespace muskat {

	inline void analyze_game(
		PossibleWorlds &worlds,
		std::vector<Card> &moves,
		Role my_role,
		size_t iterations
	) {

		auto score_without_skat = uint8_t{};

		auto next_to_play_it = moves.begin();

		for (;;) {

			if (worlds.active_role == my_role) {
				auto remaining_cards = worlds.known_cards_dec_fdef_sdef_skat[static_cast<size_t>(my_role)];
				if (remaining_cards.empty()) {
					break;
				}
				
				auto card = pick_best_card(
					worlds,
					score_without_skat,
					iterations
				);
				std::cout << "\nRecommendation: " << to_string(card) << ".\n";
			}

			if (next_to_play_it == moves.end()) {
				break;
			}
			
			auto played_card = *next_to_play_it;
			++next_to_play_it;

			score_without_skat += worlds.play_card(played_card);
			std::cout << "Played: " << to_string(played_card) << ".\n\n";
		}
	}
} // namespace muskat
