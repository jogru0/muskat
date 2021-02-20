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
#include <stdc/utility.h>

namespace perf {
	[[nodiscard]] static auto decide_winner_farbspiel() -> std::array<std::vector<double>, 2> {
		using namespace stdc::literals;

		auto rng = stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness<0, 77'777'777>{});
		// auto rng = stdc::seeded_RNG();

		auto game = muskat::GameType::Herz;
		auto role_vorhand = muskat::Role::Declarer;

		auto iters = 1000_z;

		auto times_in_ms = std::vector<double>{};
		times_in_ms.reserve(iters);
		auto numbers_of_nodes = std::vector<double>{};
		times_in_ms.reserve(iters);

		auto wins = 0;
		
		for (auto i = 0_z; i < iters; ++i) {
			auto deck = muskat::get_shuffled_deck(rng);
			auto [hand_geber, hand_hoerer, hand_sager, skat] = deal_deck(deck);
			auto initial_situation = muskat::Situation{hand_geber, hand_hoerer, hand_sager, skat, role_vorhand};


			auto solver = muskat::SituationSolver{game};

			auto watch_solve = detail::Watch{};
			watch_solve.start();
			auto won = solver.still_makes_at_least(initial_situation, 61);
			watch_solve.stop();
			
			times_in_ms.push_back(static_cast<double>(watch_solve.elapsed<std::chrono::nanoseconds>() / 1'000'000.l));
			numbers_of_nodes.push_back(static_cast<double>(solver.number_of_nodes()));

			if (won) {
				++wins;
			}
		}

		std::cout << "won by declarer: " << (100. * wins) / static_cast<double>(iters) << " %\n";
		return {numbers_of_nodes, times_in_ms};
	}
} //namespace perf

static void display_statistics(std::vector<double> data) {
	auto size = data.size();
	assert(2 <= size);
	
	auto sum = std::accumulate(RANGE(data), 0.);
	auto sample_mean = sum / static_cast<double>(size);

	auto sum_of_squared_distances = stdc::transform_accumulate(RANGE(data), [&](auto x) {
		auto dist = std::abs(x - sample_mean);
		return dist * dist;
	});
	auto unbiased_sample_variance = sum_of_squared_distances / static_cast<double>(size - 1);
	auto corrected_sample_standard_deviation = std::sqrt(unbiased_sample_variance);

	auto index_median_l = (size - 1) / 2;
	auto index_median_r = size / 2;

	std::nth_element(data.begin(), stdc::to_it(data, index_median_l), data.end());
	auto median_l = data[index_median_l];
	
	std::nth_element(data.begin(), stdc::to_it(data, index_median_r), data.end());
	auto median_r = data[index_median_l];

	auto sample_median = std::midpoint(median_l, median_r);

	std::cout << "\tmean:   " << sample_mean << '\n';
	std::cout << "\tstddev: " << corrected_sample_standard_deviation << '\n';
	std::cout << "\tmedian: " << sample_median << '\n';
}

static void performance() {
	// assert(false); //ONLY RUN IN RELEASE MODE

	auto [numbers_of_nodes, times_in_ms] = perf::decide_winner_farbspiel();

	std::cout << "Number of nodes:\n";
	display_statistics(numbers_of_nodes);
	
	std::cout << "Run time in ms:\n";
	display_statistics(times_in_ms);
	

}


int main() {
	assert(std::cout << "Wir asserten.\n");

	std::cout << "size: " << sizeof(muskat::Card) << "\n";
	std::cout << "size: " << sizeof(std::optional<muskat::Card>) << "\n";
	std::cout << "size: " << sizeof(muskat::Cards) << "\n";
	std::cout << "size: " << sizeof(muskat::Role) << "\n";
	std::cout << "size: " << sizeof(muskat::Situation) << "\n";
	
	
	std::cout << "Hello Sailor!\n";
	WATCH("perf").reset();
	WATCH("perf").start();
	performance();
	WATCH("perf").stop();
	std::cout << "\nMeasuring this took " << WATCH("perf").elapsed<std::chrono::seconds>() << "s.\n";
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