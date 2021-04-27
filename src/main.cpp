#include <iostream>
#include <cassert>

#include "cards.h"
#include "situation.h"

#include "perfect_information_sample_analysis.h"

#include "human_player.h"
#include "random_player.h"
#include "cheater.h"
#include "game.h"

#include "world_simulation.h"

#include "concurrent_monte_carlo.h"

#include <stdc/WATCH.h>

#include <stdc/mathematics.h>
#include <stdc/utility.h>

static_assert(std::atomic<bool>::is_always_lock_free);

namespace perf {

[[nodiscard]] static auto find_threshold_2() -> std::array<std::vector<double>, 2> {
	using namespace stdc::literals;

	auto rng = stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness{33, 123'346});

	auto game = muskat::GameType::Herz;
	auto role_vorhand = muskat::Role::Declarer;

	auto iters = 100_z;

	auto times_in_ms = std::vector<double>{};
	times_in_ms.reserve(iters);
	auto numbers_of_nodes = std::vector<double>{};
	times_in_ms.reserve(iters);

	auto points = 0_z;
	
	for (auto i = 0_z; i < iters; ++i) {
		auto deck = muskat::get_shuffled_deck(rng);
		auto [hand_geber, hand_hoerer, hand_sager, skat] = deal_deck(deck);
		auto initial_situation = muskat::Situation{hand_geber, hand_hoerer, hand_sager, skat, role_vorhand};


		auto solver = muskat::SituationSolver{game};

		auto watch_solve = detail::Watch{};
		watch_solve.start();
		points += solver.calculate_potential_score_2(initial_situation);
		watch_solve.stop();
		
		times_in_ms.push_back(static_cast<double>(watch_solve.elapsed<std::chrono::nanoseconds>() / 1'000'000.l));
		numbers_of_nodes.push_back(static_cast<double>(solver.number_of_nodes()));
	}

	std::cout << "average possible_points: " << static_cast<double>(points) / static_cast<double>(iters) << '\n';
	return {numbers_of_nodes, times_in_ms};
}
[[nodiscard]] static auto find_threshold_3() -> std::array<std::vector<double>, 2> {
	using namespace stdc::literals;

	auto rng = stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness{33, 123'346});

	auto game = muskat::GameType::Herz;
	auto role_vorhand = muskat::Role::Declarer;

	auto iters = 100_z;

	auto times_in_ms = std::vector<double>{};
	times_in_ms.reserve(iters);
	auto numbers_of_nodes = std::vector<double>{};
	times_in_ms.reserve(iters);

	auto points = 0_z;
	
	for (auto i = 0_z; i < iters; ++i) {
		auto deck = muskat::get_shuffled_deck(rng);
		auto [hand_geber, hand_hoerer, hand_sager, skat] = deal_deck(deck);
		auto initial_situation = muskat::Situation{hand_geber, hand_hoerer, hand_sager, skat, role_vorhand};


		auto solver = muskat::SituationSolver{game};

		auto watch_solve = detail::Watch{};
		watch_solve.start();
		points += solver.calculate_potential_score_3(initial_situation);
		watch_solve.stop();
		
		times_in_ms.push_back(static_cast<double>(watch_solve.elapsed<std::chrono::nanoseconds>() / 1'000'000.l));
		numbers_of_nodes.push_back(static_cast<double>(solver.number_of_nodes()));
	}

	std::cout << "average possible_points: " << static_cast<double>(points) / static_cast<double>(iters) << '\n';
	return {numbers_of_nodes, times_in_ms};
}

[[nodiscard]] static auto decide_winner_farbspiel() -> std::array<std::vector<double>, 2> {
	using namespace stdc::literals;

	auto rng = stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness{0, 77'777'777});
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
	{
	auto [numbers_of_nodes, times_in_ms] = perf::decide_winner_farbspiel();

	std::cout << "Number of nodes:\n";
	display_statistics(numbers_of_nodes);
	
	std::cout << "Run time in ms:\n";
	display_statistics(times_in_ms);
	}
	
	std::cout << "\n\n\n";
	
	{
	auto [numbers_of_nodes, times_in_ms] = perf::find_threshold_2();

	std::cout << "Number of nodes:\n";
	display_statistics(numbers_of_nodes);
	
	std::cout << "Run time in ms:\n";
	display_statistics(times_in_ms);
	}
	
	std::cout << "\n\n\n";
	
	{
	auto [numbers_of_nodes, times_in_ms] = perf::find_threshold_3();

	std::cout << "Number of nodes:\n";
	display_statistics(numbers_of_nodes);
	
	std::cout << "Run time in ms:\n";
	display_statistics(times_in_ms);
	}

}

int main() {
	assert(std::cout << "Wir asserten.\n");

	//Hand: S7Z H789OZ E789
	//Hinterhand plays -> we ware sdef.
	//E is trump.
	//We are Mittelhand in round 1.
	//HA -> HZ -> HU
	//S8 -> SA -> ???

	auto known_about_unknown_dec_fdef_sdef_skat = std::array{
		muskat::KnownUnknownInSet{
			8, {true, false, true, true, true}
		},
		muskat::KnownUnknownInSet{
			8, {true, true, true, true, true}
		},
		muskat::KnownUnknownInSet{
			0, {true, true, true, true, true}
		},
		muskat::KnownUnknownInSet{
			2, {true, true, true, true, true}
		}
	};

	auto known_cards_dec_fdef_sdef_skat = std::array{
		muskat::Cards{},
		muskat::Cards{},
		//                                    Z already played, so not here.
		//                  E987...........H0--987    SZ--7
		muskat::Cards{0b00000111'00000000'00100111'00001001u},
		muskat::Cards{}
	};

	auto game = muskat::GameType::Eichel;
	auto active_role = muskat::Role::SecondDefender;
	auto maybe_first_trick_card = muskat::MaybeCard{muskat::Card::S8};
	auto maybe_second_trick_card = muskat::MaybeCard{muskat::Card::SA};

	auto known_cards = muskat::Cards{};
	known_cards |= known_cards_dec_fdef_sdef_skat[2];
	known_cards.add(muskat::Card::HA);
	known_cards.add(muskat::Card::HZ);
	known_cards.add(muskat::Card::HU);
	known_cards.add(*maybe_first_trick_card);
	known_cards.add(*maybe_second_trick_card);

	auto unknown_cards = ~known_cards;

	auto possible_worlds = muskat::PossibleWorlds{
		known_about_unknown_dec_fdef_sdef_skat,
		known_cards_dec_fdef_sdef_skat,
		unknown_cards,
		game,
		active_role,
		maybe_first_trick_card,
		maybe_second_trick_card
	};

	WATCH("dfs").reset();
	WATCH("dfs").start();
	auto results = muskat::multithreaded_world_simulation(possible_worlds);
	//TODO: Empty -> Assert triggert.
	auto sample = muskat::to_sample(std::move(results));
	WATCH("dfs").stop();
	std::cout << "\nTime spent to run the simulation: " << WATCH("dfs").elapsed<std::chrono::milliseconds>() << "ms.\n";
	
	std::cout << "\nSamples calculated: " << sample.points_for_situations().size() << '\n';

	WATCH("ana").reset();
	WATCH("ana").start();
	std::cout << "\nTime spent to run the simulation: " << WATCH("dfs").elapsed<std::chrono::milliseconds>() << "ms.\n";
	std::cout << "Highest expected winrate declarer: " << to_string(muskat::analyze_for_declarer(sample)) << "\n";
	std::cout << "Highest expected winrate defender: " << to_string(muskat::analyze_for_defender(sample)) << "\n";
	WATCH("ana").stop();

	std::cout << "\nTime spent to analyze the sample: " << WATCH("ana").elapsed<std::chrono::milliseconds>() << "ms.\n";


	std::cout << "Done threads:\n";
	auto old_done = int{-1};
	do {
		auto done = static_cast<int>(done_threads);
		if (old_done == done) {
			continue;
		}

		old_done = done;
		std::cout << '\t' << done << std::flush;
	} while (old_done != 12);
	std::cout << '\n';

	using namespace stdc::literals;

	std::cout << "Performance:\n";
	for (auto th_id = 0_z; th_id < 12; ++th_id){ 
		const auto &watches = wa::watches_th[th_id];
		const auto &iterations = wa::iterations[th_id];
		auto total_ms = static_cast<double>(watches[wa::loop].elapsed<std::chrono::nanoseconds>()) / 1'000'000.;
		auto loop_ms = total_ms / static_cast<double>(iterations);
		std::cout << '\t' << iterations << '\t' << loop_ms << "ms\t" << total_ms << "ms\n";
	}


	// std::cout << "Startup:\n";
	// for (const auto &watches : wa::watches_th) {
	// 	std::cout << '\t' << watches[wa::start].elapsed<std::chrono::microseconds>() << "us.\n";
	// }

	// std::cout << "Detatch:\n";
	// for (const auto &watches : wa::watches_th) {
	// 	std::cout << '\t' << watches[wa::detach].elapsed<std::chrono::nanoseconds>() << "ns.\n";
	// }

	// std::cout << "rng:\n";
	// for (const auto &watches : wa::watches_th) {
	// 	std::cout << '\t' << watches[wa::rng].elapsed<std::chrono::microseconds>() << "us.\n";
	// }

	// //Below is our perforamnce test code for the DDS.

	// std::cout << "size: " << sizeof(muskat::Card) << "\n";
	// std::cout << "size: " << sizeof(std::optional<muskat::Card>) << "\n";
	// std::cout << "size: " << sizeof(muskat::Cards) << "\n";
	// std::cout << "size: " << sizeof(muskat::Role) << "\n";
	// std::cout << "size: " << sizeof(muskat::Situation) << "\n";
	
	// std::cout << "Hello Sailor!\n";
	// WATCH("perf").reset();
	// WATCH("perf").start();
	// performance();
	// WATCH("perf").stop();
	// std::cout << "\nMeasuring this took " << WATCH("perf").elapsed<std::chrono::seconds>() << "s.\n";
}