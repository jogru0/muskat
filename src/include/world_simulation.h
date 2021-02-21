#pragma once

#include "situation.h"

#include <stdc/mathematics.h>

#include <robin_hood/robin_hood.h>

namespace muskat {

[[nodiscard]] auto multithreaded_world_simulation(Situation sit) {
	auto number_of_threads = std::thread::hardware_concurrency();
	if (number_of_threads == 0) {
		throw std::runtime_error{"Could not determine best number of threads."};
	}

	std::cout << "Number of threads: " << number_of_threads << ".\n";

	auto do_stuff = [] (std::stop_token stoken, std::vector<std::array<Points, 32>> &results) {
		assert(results.empty());
		
		auto rng = stdc::seeded_RNG();
		
		while(!stoken.stop_requested()) {
			auto deck = muskat::get_shuffled_deck(rng);
			auto [hand_geber, hand_hoerer, hand_sager, skat] = deal_deck(deck);
			auto initial_situation = muskat::Situation{hand_geber, hand_hoerer, hand_sager, skat, role_vorhand};
		
			
		}
		for(int i=0; i < 10; i++) {
			std::this_thread::sleep_for(300ms);
			if() {
				std::cout << "Sleepy worker is requested to stop\n";
				return;
			}
			std::cout << "Sleepy worker goes back to sleep\n";
		}
	}

}


} // namespace muskat
