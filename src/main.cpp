#include <iostream>
#include <cassert>

#include "cards.h"
#include "situation.h"

#include "perfect_information_sample_analysis.h"

#include "player/human_player.h"
#include "player/random_player.h"
#include "player/cheater.h"
#include "player/uniform_sampler.h"
#include "game.h"

#include "analyze_game.h"

#include "logger.h"

#include "world_simulation.h"

#include "concurrent_monte_carlo.h"

#include <stdc/WATCH.h>

#include <stdc/arguments.h>
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


		assert(initial_situation.cellar() == skat);
		assert(skat.size() == 2);
		auto skat_0 = skat.remove_next();
		auto skat_1 = skat.remove_next();
		
		auto solver = muskat::SituationSolver{initial_situation, game, skat_0, skat_1};

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

		assert(initial_situation.cellar() == skat);
		assert(skat.size() == 2);
		auto skat_0 = skat.remove_next();
		auto skat_1 = skat.remove_next();

		auto solver = muskat::SituationSolver{initial_situation, game, skat_0, skat_1};

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

		assert(initial_situation.cellar() == skat);
		assert(skat.size() == 2);
		auto skat_0 = skat.remove_next();
		auto skat_1 = skat.remove_next();

		auto solver = muskat::SituationSolver{initial_situation, game, skat_0, skat_1};

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

inline void performance() {
	// assert(false); //ONLY RUN IN RELEASE MODE
	{
	auto [numbers_of_nodes, times_in_ms] = perf::decide_winner_farbspiel();

	std::cout << "Number of nodes:\n";
	stdc::display_statistics(numbers_of_nodes);
	
	std::cout << "Run time in ms:\n";
	stdc::display_statistics(times_in_ms);
	}
	
	std::cout << "\n\n\n";
	
	{
	auto [numbers_of_nodes, times_in_ms] = perf::find_threshold_2();

	std::cout << "Number of nodes:\n";
	stdc::display_statistics(numbers_of_nodes);
	
	std::cout << "Run time in ms:\n";
	stdc::display_statistics(times_in_ms);
	}
	
	std::cout << "\n\n\n";
	
	{
	auto [numbers_of_nodes, times_in_ms] = perf::find_threshold_3();

	std::cout << "Number of nodes:\n";
	stdc::display_statistics(numbers_of_nodes);
	
	std::cout << "Run time in ms:\n";
	stdc::display_statistics(times_in_ms);
	}

}


namespace detail {

[[noreturn]] inline void non_valid_input_exit() {
	std::cout << "\n\n"
		<< "USAGE: -a [valid_json] [number of iterations]\n"
		<< "Exiting, please try again." << std::endl;
	exit(1);
}

inline void warn_about_ignored_flag(const char option, const std::string_view& flag) {
	std::cout << "ATTENTION: Option '" << option << "' does not apply flag '" << flag << "'!" << "\n";
}

[[noreturn]] inline void apply_flag(const char /*option*/, const std::string_view& flag)
	{
		// //plotting ignores this flag, but no reason to issue a warning
		// if (flag == "-plot") {
		// 	turn::config::do_plot = true;
		// 	return;
		// }

		// if (flag == "-write_expected" or flag == "-write_new") {
		// 	if (option != 't') {
		// 		detail::warn_about_ignored_flag(option, flag);
		// 		return;
		// 	}

		// 	turn::config::write_diff_for_validation_files = true;

		// 	if (flag == "-write_expected") {
		// 		turn::config::write_expected_file_for_CI = true;
		// 	}
		// 	else {
		// 		turn::config::write_new_expected_file_for_comparison = true;
		// 	}		
		// 	return;
		// }

		std::cout << "Unknown flag '" << flag << "'" << std::endl;
		non_valid_input_exit();
	}

inline void check_number_of_inputs_and_apply_flags(
	const stdc::arguments& args, const char /*option*/, size_t arguments_needed_by_option
) {
	size_t argument_without_flags_size = 2 + arguments_needed_by_option;
	if (args.size() < argument_without_flags_size) {
		std::cout << "Wrong number of arguments" << "\n";
		non_valid_input_exit();
	}

	// for (size_t i = argument_without_flags_size; i < args.size(); ++i) {
	// 	detail::apply_flag(option, args[i]);
	// }
}

inline auto is_equal(const char* a, const char* b)
{
	return strncmp(a, b, std::strlen(a)) == 0;
}

inline auto parse_iterations_or_exit(std::string_view sv) {
	auto maybe_number_simulation = stdc::maybe_parse_chars<size_t>(sv);
	if (!maybe_number_simulation) {
		std::cout << "Unable to read number of iterations from input." << std::endl;
		non_valid_input_exit();
	}
	return *maybe_number_simulation;
}

inline void test_calculating_initial_games(size_t iterations) {
	muskat::calculate_initial_games(iterations, muskat::GameType::Eichel, muskat::Role::Declarer);
	muskat::calculate_initial_games(iterations, muskat::GameType::Herz, muskat::Role::FirstDefender);
	muskat::calculate_initial_games(iterations, muskat::GameType::Schell, muskat::Role::SecondDefender);

	muskat::calculate_initial_games(iterations, muskat::GameType::Grand, muskat::Role::Declarer);
	muskat::calculate_initial_games(iterations, muskat::GameType::Grand, muskat::Role::FirstDefender);
	muskat::calculate_initial_games(iterations, muskat::GameType::Grand, muskat::Role::SecondDefender);
}

} //namespace detail

//At the moment, noreturn
auto main(int argc, char **argv) -> int try {
	assert(std::cout << "Asserts active.\n");

	// std::cout << "Card:      " << sizeof(muskat::Card) << "\n";
	// std::cout << "op<Card>:  " << sizeof(std::optional<muskat::Card>) << "\n";
	// std::cout << "OpCard:    " << sizeof(muskat::MaybeCard) << "\n";
	// std::cout << "Role:      " << sizeof(muskat::Role) << "\n";
	// std::cout << "Sit:       " << sizeof(muskat::Situation) << "\n";
	// std::cout << "B:         " << sizeof(muskat::Bounds) << "\n";
	// std::cout << "(B, OpC):  " << sizeof(std::pair<muskat::Bounds, muskat::MaybeCard>) << "\n";
	// std::cout << "(OpC, B):  " << sizeof(std::pair<muskat::MaybeCard, muskat::Bounds>) << "\n";
	
	stdc::log("=====================================");

	auto in_release_mode = true;
	assert((in_release_mode = false) || true);

	auto args = stdc::arguments{argc, argv};
	
	if (args.size() < 2) { detail::non_valid_input_exit(); }

	auto option = args[1].size() == 2 && args[1][0] == '-' ? args[1][1] : '\0';
	
	switch (option) {
		case 'a':
			{
				detail::check_number_of_inputs_and_apply_flags(args, option, 2);
				auto path_to_json = std::filesystem::path{args[2]};
				auto fs = stdc::IO::open_file_with_checks_for_reading(path_to_json);
				auto json = nlohmann::json::parse(fs);
				auto [worlds, moves, my_role, contract, bidding_value] = muskat::parse_game_record(json);
				auto iterations = detail::parse_iterations_or_exit(args[3]);
				
				muskat::analyze_game(
					worlds, moves, my_role, iterations, contract, bidding_value
				);
			}
			break;
		case 't':
			if (!in_release_mode) {
				stdc::log("THIS WAS NOT MEASURED IN RELEASE MODE!");
				stdc::detail::log.flush();
			}
			
			detail::check_number_of_inputs_and_apply_flags(args, option, 1);
			detail::test_calculating_initial_games(
				detail::parse_iterations_or_exit(args[2])
			);
			break;
		//plot initial state if simulator constructor does not throw
		default:
			std::cout << "Unknown argument '" << args[1] << "'" << std::endl;
			detail::non_valid_input_exit();
	}

	std::cout << "\nThank you for using muskat.\n";
	exit(0);
} catch(const std::exception &e) {
	std::cout << "Could not recover from exception '" << e.what() << "'. Exiting.\n";
	exit(1);
} catch (...) {
	std::cout << "oh no";
	exit(111);

	assert(std::cout << "Wir asserten.\n");

	// //Hand: S7Z H789OZ E789
	// //We are Mittelhand in round 1.
	// //Hinterhand plays -> we ware sdef.
	// //E is trump.
	// //HA -> HZ -> HU
	// //S8 -> SA -> ???

	// auto known_about_unknown_dec_fdef_sdef_skat = std::array{
	// 	muskat::KnownUnknownInSet{
	// 		8, {true, false, true, true, true}
	// 	},
	// 	muskat::KnownUnknownInSet{
	// 		8, {true, true, true, true, true}
	// 	},
	// 	muskat::KnownUnknownInSet{
	// 		0, {true, true, true, true, true}
	// 	},
	// 	muskat::KnownUnknownInSet{
	// 		2, {true, true, true, true, true}
	// 	}
	// };

	// //                                 E987...........H0-Z987    SZ--7
	// auto my_hand = muskat::Cards{0b00000111'00000000'00101111'00001001u};
	// auto my_hand_after_turn_one = my_hand;
	// my_hand_after_turn_one.remove(muskat::Card::HZ);


	// auto known_cards_dec_fdef_sdef_skat = std::array{
	// 	muskat::Cards{},
	// 	muskat::Cards{},
	// 	my_hand_after_turn_one,
	// 	muskat::Cards{}
	// };

	// auto gone_cards = muskat::Cards{};
	// gone_cards.add(muskat::Card::HA);
	// gone_cards.add(muskat::Card::HZ);
	// gone_cards.add(muskat::Card::HU);

	// auto game = muskat::GameType::Eichel;
	// auto active_role = muskat::Role::SecondDefender;
	// auto maybe_first_trick_card = muskat::MaybeCard{muskat::Card::S8};
	// auto maybe_second_trick_card = muskat::MaybeCard{muskat::Card::SA};

	// auto known_cards = muskat::Cards{};
	// known_cards |= known_cards_dec_fdef_sdef_skat[2];
	// known_cards.add(muskat::Card::HA);
	// known_cards.add(muskat::Card::HZ);
	// known_cards.add(muskat::Card::HU);
	// known_cards.add(*maybe_first_trick_card);
	// known_cards.add(*maybe_second_trick_card);

	// auto unknown_cards = ~known_cards;

	// auto possible_worlds = muskat::PossibleWorlds{
	// 	known_about_unknown_dec_fdef_sdef_skat,
	// 	known_cards_dec_fdef_sdef_skat,
	// 	unknown_cards,
	// 	game,
	// 	active_role,
	// 	maybe_first_trick_card,
	// 	maybe_second_trick_card,
	// 	gone_cards
	// };

	// auto initial_possible_worlds = muskat::PossibleWorlds(
	// 	my_hand,
	// 	muskat::Role::SecondDefender,
	// 	std::nullopt,
	// 	muskat::GameType::Eichel,
	// 	muskat::Role::FirstDefender
	// );
	// auto score = initial_possible_worlds.play_card(muskat::Card::HA);
	// score += initial_possible_worlds.play_card(muskat::Card::HZ);
	// score += initial_possible_worlds.play_card(muskat::Card::HU);
	// score += initial_possible_worlds.play_card(muskat::Card::S8);
	// score += initial_possible_worlds.play_card(muskat::Card::SA);

	// assert(possible_worlds == initial_possible_worlds);
	// assert(score == 23);

	// WATCH("dfs").reset();
	// WATCH("dfs").start();
	// auto results = muskat::multithreaded_world_simulation(possible_worlds);
	// //TODO: Empty -> Assert triggert.
	// auto sample = muskat::to_sample(std::move(results));
	// WATCH("dfs").stop();
	// std::cout << "\nTime spent to run the simulation: " << WATCH("dfs").elapsed<std::chrono::milliseconds>() << "ms.\n";
	
	// std::cout << "\nSamples calculated: " << sample.points_for_situations().size() << '\n';

	// WATCH("ana").reset();
	// WATCH("ana").start();
	// std::cout << "\nTime spent to run the simulation: " << WATCH("dfs").elapsed<std::chrono::milliseconds>() << "ms.\n";
	// std::cout << "Highest expected winrate declarer: " << to_string(muskat::analyze_for_declarer(sample)) << "\n";
	// std::cout << "Highest expected winrate defender: " << to_string(muskat::analyze_for_defender(sample)) << "\n";
	// WATCH("ana").stop();

	// std::cout << "\nTime spent to analyze the sample: " << WATCH("ana").elapsed<std::chrono::milliseconds>() << "ms.\n";


	// std::cout << "Done threads:\n";
	// auto old_done = int{-1};
	// do {
	// 	auto done = static_cast<int>(done_threads);
	// 	if (old_done == done) {
	// 		continue;
	// 	}

	// 	old_done = done;
	// 	std::cout << '\t' << done << std::flush;
	// } while (old_done != 12);
	// std::cout << '\n';

	// using namespace stdc::literals;

	// std::cout << "Performance:\n";
	// for (auto th_id = 0_z; th_id < 12; ++th_id){ 
	// 	const auto &watches = wa::watches_th[th_id];
	// 	const auto &iterations_pre = wa::iterations_pre[th_id];
	// 	const auto &iterations_main = wa::iterations_main[th_id];
	// 	const auto &iterations_post = wa::iterations_post[th_id];
	// 	auto total_pre_ms = static_cast<double>(watches[wa::loop_pre].elapsed<std::chrono::nanoseconds>()) / 1'000'000.;
	// 	auto total_main_ms = static_cast<double>(watches[wa::loop_main].elapsed<std::chrono::nanoseconds>()) / 1'000'000.;
	// 	auto total_post_ms = static_cast<double>(watches[wa::loop_post].elapsed<std::chrono::nanoseconds>()) / 1'000'000.;
	// 	auto pre_ms = total_pre_ms / static_cast<double>(iterations_pre);
	// 	auto main_ms = total_main_ms / static_cast<double>(iterations_main);
	// 	auto post_ms = total_post_ms / static_cast<double>(iterations_post);
	// 	std::cout << '\t' << pre_ms << "ms\t" << main_ms << "ms\t" << post_ms << "ms\n";
	// }


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

	//Below is an example game with perfect information.

	auto geber = muskat::Cheater{
		"Celine",
		stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness{7, 77'777'777})
	};
	auto hoerer = muskat::UniformSampler{
		"Anon",
		stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness{22, 123'457})
	};
	auto sager = muskat::Cheater{
		"Bernd",
		stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness{1, 444})
	};

	auto rng = stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness{1, 33});

	auto deck = muskat::get_shuffled_deck(rng);

	auto result = muskat::play_one_game(
		geber,
		hoerer,
		sager,
		deck
	);

	std::cout << "Result: " << static_cast<size_t>(result) << '\n';

}