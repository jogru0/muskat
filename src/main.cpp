#include <iostream>
#include <cassert>

#include "set_of_cards.h"

int main() {
	assert(std::cout << "Wir debuggen.\n");
	
	std::cout << "Hello Sailor!\n";

	auto set = muskat::SetOfCards{};
	return static_cast<int>(set.size());
}