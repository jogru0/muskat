#include <iostream>
#include <cassert>

#include "cards.h"
#include "situation.h"

#include "human_player.h"
#include "random_player.h"
#include "cheater.h"
#include "game.h"

#include <stdc/WATCH.h>

#include <stdc/mathematics.h>

namespace perf {
	static void decide_winner_farbspiel() {
			
		auto rng = stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness<0, 77'777'777>{});

		auto game = muskat::GameType::Herz;
		auto role_vorhand = muskat::Role::Declarer;

		auto iters = 1000;

		auto wins = 0;
		WATCH("solver").reset();
		for (auto i = 0; i < iters; ++i) {
			auto deck = muskat::get_shuffled_deck(rng);
			auto [hand_geber, hand_hoerer, hand_sager, skat] = deal_deck(deck);
			auto initial_situation = muskat::Situation{hand_geber, hand_hoerer, hand_sager, skat, role_vorhand};
			auto solver = muskat::SituationSolver{game};

			WATCH("solver").start();
			auto won = solver.still_makes_at_least(initial_situation, 61);
			WATCH("solver").stop();

			if (won) {
				++wins;
			}

		}
		std::cout << "won by declarer: " << (100. * wins) / iters << " %\n";
		std::cout << "One iteration took " << WATCH("solver").elapsed<std::chrono::milliseconds>() / iters << "ms.\n";
	}
} //namespace perf

static void performance() {
	assert(false); //ONLY RUN IN RELEASE MODE

	perf::decide_winner_farbspiel();

}


int main() {
	assert(std::cout << "Wir asserten.\n");

	
	std::cout << "Hello Sailor!\n";
	WATCH("perf").reset();
	WATCH("perf").start();
	performance();
	WATCH("perf").stop();
	std::cout << "Measuring this took " << WATCH("perf").elapsed<std::chrono::seconds>() << "s.\n";
	return 0;

	// using SOR = stdc::DeterministicSourceOfRandomness;
	// using SOR = std::random_device;
	// auto rng = stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness{});

	// auto p1 = muskat::Cheater{"1", stdc::seeded_RNG(SOR{})};
	// auto p2 = muskat::Cheater{"2", stdc::seeded_RNG(SOR{})};
	// auto p3 = muskat::Cheater{"3", stdc::seeded_RNG(SOR{})};

	// auto number_of_iterations = 1;
	// auto number_of_wins = 0;

	// for (auto i = 0; i < number_of_iterations; ++i) {
	// 	auto res = muskat::play_one_game(p1, p2, p3, muskat::get_shuffled_deck(rng));
	// 	if (61 <= res) {
	// 		++number_of_wins;
	// 	}
	// }

	// std::cout << "won by declarer: " << (100. * number_of_wins) / number_of_iterations << " %\n";
}