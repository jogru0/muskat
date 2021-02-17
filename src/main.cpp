#include <iostream>
#include <cassert>

#include "cards.h"
#include "situation.h"

#include "human_player.h"
#include "random_player.h"
#include "game.h"

#include <stdc/mathematics.h>

int main() {
	assert(std::cout << "Wir asserten.\n");
	
	std::cout << "Hello Sailor!\n";

	auto rng = stdc::seeded_RNG(/*stdc::DeterministicSourceOfRandomness{}*/);



	auto p1 = muskat::RandomPlayer{"1", stdc::seeded_RNG(/*stdc::DeterministicSourceOfRandomness{}*/)};
	auto p2 = muskat::RandomPlayer{"2", stdc::seeded_RNG(/*stdc::DeterministicSourceOfRandomness{}*/)};
	auto p3 = muskat::RandomPlayer{"3", stdc::seeded_RNG(/*stdc::DeterministicSourceOfRandomness{}*/)};

	auto number_of_iterations = 1'000'000;
	auto number_of_wins = 0;

	for (auto i = 0; i < number_of_iterations; ++i) {
		auto deck = muskat::get_shuffled_deck(rng);
		auto res = muskat::play_one_game(p1, p2, p3, deck);
		if (61 <= res) {
			++number_of_wins;
		}
	}

	std::cout << "won by declarer: " << (100. * number_of_wins) / number_of_iterations << " %\n";
}