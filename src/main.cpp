#include <iostream>
#include <cassert>

#include "cards.h"
#include "situation.h"

#include "human_player.h"
#include "game.h"

#include <stdc/mathematics.h>

int main() {
	assert(std::cout << "Wir asserten.\n");
	
	std::cout << "Hello Sailor!\n";

	auto rng = stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness{});

	auto deck = muskat::get_shuffled_deck(rng);

	auto situation = muskat::Situation{deck};
	std::cout << "Im Keller: " << situation.cellar().size() << '\n';

	auto p1 = muskat::HumanPlayer{"1"};
	auto p2 = muskat::HumanPlayer{"2"};
	auto p3 = muskat::HumanPlayer{"3"};

	auto res = muskat::play_one_game(p1, p2, p3, deck);
	std::cout << "Result: " << res << '\n';
}