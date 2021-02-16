#include <iostream>
#include <cassert>

#include "cards.h"
#include "situation.h"

#include <stdc/mathematics.h>

int main() {
	assert(std::cout << "Wir asserten.\n");
	
	std::cout << "Hello Sailor!\n";

	auto rng = stdc::seeded_RNG();

	auto deck = muskat::get_shuffled_deck(rng);

	auto situation = muskat::Situation{deck};
	std::cout << "Im Keller: " << situation.cellar().size() << '\n';
}