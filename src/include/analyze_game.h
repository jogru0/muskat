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
		size_t iterations,
		Contract contract,
		int bidding_value
	) {

		auto score_without_skat = Score{0, 0};

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
					iterations,
					contract,
					bidding_value
				);
				std::cout << "\nRecommendation: " << to_string(card) << ".\n";
			}

			if (next_to_play_it == moves.end()) {
				break;
			}
			
			auto played_card = *next_to_play_it;
			++next_to_play_it;

			auto additional_score = worlds.play_card(played_card);
			score_without_skat.add(additional_score);
			std::cout << "Played: " << to_string(played_card) << ".\n\n";
		}
	}
} // namespace muskat
