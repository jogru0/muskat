#pragma once

#include "world_simulation.h"
#include "uniform_sit_distribution.h"

#include <pcg/pcg_random.hpp>

#include "situation_solver.h"
#include "spinlock.h"

#include "logger.h"

#include "statistics.h"

#include <stdc/swatch.h>
#include <stdc/WATCH.h>

#include <thread>

#include <fmt/core.h>
#include <fmt/chrono.h>

#include <stdc/mathematics.h>
#include <stdc/bool_for_vector.h>

namespace wa {
inline constexpr auto start = size_t{0};
inline constexpr auto loop_pre = size_t{1};
inline constexpr auto loop_main = size_t{2};
inline constexpr auto loop_post = size_t{3};
inline constexpr auto rng = size_t{4};
static std::array<std::array<detail::Watch, 5>, 12> watches_th{};
static std::array<size_t, 12> iterations_pre{};
static std::array<size_t, 12> iterations_main{};
static std::array<size_t, 12> iterations_post{};

static std::array<double, 12> last_iter_ms{};



} //namespace wa

static auto done_threads = std::atomic<uint8_t>{0};

namespace muskat {

inline void execute_worker_sampling(
	std::stop_token stoken,
	const std::vector<std::tuple<Situation, Card, Card, GameType>> &inputs,
	stdc::ConcurrentResultVector<std::array<Score, 32>> &results,
	size_t worker_id,
	std::vector<double> &times_in_ms,
	std::vector<std::vector<double>> &numbers_of_nodes,
	Score current_score_without_skat
) try {
	wa::watches_th[worker_id][wa::start].stop();

	wa::watches_th[worker_id][wa::loop_pre].reset();
	wa::watches_th[worker_id][wa::loop_pre].start();
	
	//Stop when the main thread signals that time is up via should_continue.
	while (!stoken.stop_requested()) {

	
		auto maybe_result_id = results.maybe_pull_a_number();
		if (!maybe_result_id) {
			break;
		}
		auto result_id = *maybe_result_id;

		auto [situation, skat_0, skat_1, game] = inputs[result_id];

		wa::watches_th[worker_id][wa::loop_pre].stop();
		++wa::iterations_pre[worker_id];
		wa::watches_th[worker_id][wa::loop_main].start();
		
		auto watch_solve = stdc::SWatch{};
		watch_solve.start();
		
		//MEASURE BEGIN
		
		auto points_from_gedrueckt = Score{static_cast<uint8_t>(to_points(skat_0) + to_points(skat_1)), 0};
		auto score_so_far = points_from_gedrueckt;
		score_so_far.add(current_score_without_skat);
		
		auto solver = muskat::SituationSolver{situation, game, skat_0, skat_1};
		auto points_arr = solver.score_for_possible_plays(situation, score_so_far);
		
		for (auto &points : points_arr) {
			points.add(points_from_gedrueckt);
		}
		
		//We want to return how many points the declarer makes not already observed with the tricks so far.
		//So the points gedrückt have to be added as well.
		//TODO: In the future, it might be nicer to also add the points already made here.

		auto nodes = std::vector{static_cast<double>(solver.number_of_nodes()) / 1000.};
		
		// auto [points, nodes] = score_for_possible_plays_separate(situation, game);
		
		//MEASURE END
		
		watch_solve.stop();
		
		wa::watches_th[worker_id][wa::loop_main].stop();
		++wa::iterations_main[worker_id];

		auto time_ms = (static_cast<double>(watch_solve.elapsed<std::chrono::nanoseconds>()) + .5) / 1'000'000.;

		times_in_ms[result_id] = time_ms;
		numbers_of_nodes[result_id] = nodes;
		wa::last_iter_ms[worker_id] = time_ms;

		wa::watches_th[worker_id][wa::loop_post].start();

		results.report_a_result(result_id, points_arr);
		
		wa::watches_th[worker_id][wa::loop_post].stop();
		++wa::iterations_post[worker_id];
		wa::watches_th[worker_id][wa::loop_pre].start();


		//TODO: Auslagern, könnte in Zukunft hilfreich sein.
		// auto dbd = points_arr[static_cast<size_t>(Card::GO)];
		// if (result_id == 0 && dbd.tricks() == 0 && dbd.points() <= 120) {
		// 	auto play_next = Card::GO;
		// 	for (;;) {
		// 		stdc::log("Plaing {}.", to_string(play_next));
		// 		auto add = situation.play_card(play_next, game);
		// 		stdc::log("Added {}, {}", add.points(), add.tricks());
		// 		current_score_without_skat.add(add);
		// 		if (situation.cellar().size() == 32) {

		// 			stdc::log("Done: I forced {} tricks.", current_score_without_skat.tricks());
		// 			break;
		// 		}
		// 		auto maybe_play_next = solver.maybe_card_for_threshold(situation, Score{0, 1});
		// 		if (!maybe_play_next) {
		// 			stdc::log("I think I can't win anymore.");
		// 			if (situation.active_role() != Role::Declarer) {
		// 				break;
		// 			}
		// 			play_next = next_possible_plays(situation, game).remove_next();
					
		// 		} else {
		// 			play_next = *maybe_play_next;
		// 		}
		// 	}
		// 	stdc::log("Loop done.");
		// }
	
	}

	++done_threads;

} catch (const std::exception &e) {
	//Something happened. Probably out of memory?
	//Anyway, the result calculated so far has to be valid, so lets just log it and stop this thread.
	//TODO: Needs a lock …
	std::cerr << "There was a problem in thread " << worker_id << ": " << e.what() << '\n';

	++done_threads;

	//Not okay while measuring.
	throw;
}

[[nodiscard]] inline auto multithreaded_sampling(
	const std::vector<std::tuple<Situation, Card, Card, GameType>> &inputs,
	Score current_score_without_skat
) -> std::vector<PerfectInformationResult> {
	using namespace stdc::literals;
	
	auto number_of_threads = std::thread::hardware_concurrency();
	if (number_of_threads == 0) {
		throw std::runtime_error{"Could not determine best number of threads."};
	}

	stdc::log_debug("Simulating with {} threads.", number_of_threads);

	assert(number_of_threads == 12);
	for (auto &watches : wa::watches_th) {
		watches = std::array{
			::detail::Watch{}, ::detail::Watch{}, ::detail::Watch{}, ::detail::Watch{}, ::detail::Watch{}
		};
	}

	auto number_samples = inputs.size();

	//Uses shared_ptr because threads might be finished before or after this function returns.
	auto results = stdc::ConcurrentResultVector<std::array<Score, 32>>{number_samples};

	auto times_in_ms = std::vector<double>(number_samples);
	auto numbers_of_nodes = std::vector<std::vector<double>>(number_samples);
	
	auto threads = std::vector<std::jthread>{};
	threads.reserve(number_of_threads);

	for (auto thread_id = 0_z; thread_id < number_of_threads; ++thread_id) {
		wa::watches_th[thread_id][wa::start].start();
	}

	for (auto thread_id = 0_z; thread_id < number_of_threads; ++thread_id) {
		threads.push_back(std::jthread{
			execute_worker_sampling,
			std::cref(inputs),
			std::ref(results),
			thread_id,
			std::ref(times_in_ms),
			std::ref(numbers_of_nodes),
			current_score_without_skat
		});
	}

	auto join_watch = stdc::SWatch{};
	join_watch.start();

	for (auto thread_id = 0_z; thread_id < number_of_threads; ++thread_id) {
		threads[thread_id].join();
		join_watch.stop();
		stdc::log_debug(
			"join {}: {}\t(last iteration: {:.0f}ms)",
			thread_id,
			stdc::to_string_ms(join_watch.elapsed()),
			wa::last_iter_ms[thread_id]
		);
		join_watch.reset();
		join_watch.start();
	}


	// auto sleep_time = std::chrono::milliseconds{10};
	// auto max_sleep_time = std::chrono::milliseconds{10'000};
	// while (done_threads != number_of_threads) {
	// 	std::cout << "\rCurrent progress: " << results.get_current_progress() << "/" << number_samples << std::flush;
	// 	std::this_thread::sleep_for(sleep_time);
	// 	sleep_time *= 2;
	// 	stdc::minimize(sleep_time, max_sleep_time);
	// }
	// std::cout << "\nDone.\n";

	auto result = results.collect_all_results_so_far();
	assert(result.size() == number_samples);

	//Since all results are there, all measurements are also there.

	stdc::log("Number of 1000 nodes:");
	auto actual = std::vector<double>();
	for (const auto &part : numbers_of_nodes) {
		std::copy(RANGE(part), std::back_inserter(actual));
	}
	stdc::display_statistics(actual);
	
	stdc::log("Run time in ms:");
	stdc::display_statistics(times_in_ms);

	return result;
}

inline void log_multithreaded_performance(
	std::chrono::nanoseconds total_time,
	size_t number_of_threads,
	size_t number_of_tasks
) {
	stdc::log(
		"Time to finished {} tasks with {} threads: {}\n\t-> effective time per task: {}",
		number_of_tasks,
		number_of_threads,
		stdc::to_string_s(total_time, 1),
		stdc::to_string_ms((total_time * 12) / number_of_tasks)
	);
}

template<typename Dist>
[[nodiscard]] auto sample_situations_and_spitzen(
	const Dist &situation_dist,
	size_t number_samples
) {
	// Seed with a real random value, if available
	// auto seed_source = pcg_extras::seed_seq_from<std::random_device>{};

	// Make a random number engine
	auto rng = pcg32(/*seed_source*/);

	auto inputs = std::vector<std::tuple<Situation, Card, Card, GameType>>{};
	inputs.reserve(number_samples);

	auto spitzen = std::vector<int>{};
	spitzen.reserve(number_samples);

	auto watch_sampling = stdc::SWatch{};
	watch_sampling.start();
	std::generate_n(std::back_inserter(inputs), number_samples, [&](){
		auto [in, sp] = situation_dist(rng);
		spitzen.push_back(sp);
		return in;
	});
	watch_sampling.stop();
	stdc::log(
		"Sampling of {} situations took {}.",
		number_samples,
		stdc::to_string_us(watch_sampling.elapsed())
	);

	return std::pair{inputs, spitzen};
}

[[nodiscard]] inline auto pick_best_card(
	const PossibleWorlds &worlds,
	//TODO: Track in worlds!!!!!!
	Score current_score_without_skat,
	size_t number_samples_to_do,
	Contract contract,
	int bidding_value
) -> Card {
	using namespace stdc::literals;
	
	stdc::log(
		"\nStart pick_best_card with {} iterations.",
		number_samples_to_do
	);
	auto watch_whole = stdc::SWatch{};
	watch_whole.start();

	auto watch_dist_generation = stdc::SWatch{};
	watch_dist_generation.start();
	auto dist = UniformSitDistribution{worlds};
	watch_dist_generation.stop();
	stdc::log(
		"Creation of the uniform sit distribution took {}.",
		stdc::to_string_us(watch_dist_generation.elapsed())
	);

	stdc::log(
		"Have {} possible situations, spread over {} different color distributions.",
		dist.get_number_of_possibilities(),
		dist.get_number_of_color_distributions()
	);

	// stdc::log(fmt::format("Start simulation of {} worlds.", number_samples_to_do));
	auto watch_simulation = stdc::SWatch{};
	watch_simulation.start();



	auto [situations, spitzen] = [&]() {
		if (10000 < dist.get_number_of_possibilities()) {
			return sample_situations_and_spitzen(
				dist,
				number_samples_to_do
			);
		}

		
		number_samples_to_do = dist.get_number_of_possibilities();
		stdc::log("We do all {} situations possible.", number_samples_to_do);
		std::cout << "Calculating all " << number_samples_to_do << " situations possible.\n";

		auto watch_sampling = stdc::SWatch{};
		watch_sampling.start();
		auto result = dist.get_all_possibilities();
		watch_sampling.stop();
		stdc::log(
			"Sampling of {} situations took {}.",
			number_samples_to_do,
			stdc::to_string_us(watch_sampling.elapsed())
		);
		return std::pair{
			stdc::transformed_vector(RANGE(result), [](auto p) {return p.first; }),
			stdc::transformed_vector(RANGE(result), [](auto p) {return p.second; })
		};
	}();



	
	auto results = multithreaded_sampling(situations, current_score_without_skat);
	auto playable_cards = worlds.surely_get_playable_cards();
	auto sample = muskat::PerfectInformationSample{std::move(playable_cards), std::move(results)};
	
	watch_simulation.stop();
	assert(sample.points_for_situations().size() == number_samples_to_do);

	log_multithreaded_performance(watch_simulation.elapsed(), 12, number_samples_to_do);

	// std::cout << "\nUsing these samples to make an informed choice now.\n";

	WATCH("ana").reset();
	WATCH("ana").start();
	auto picks = analyze_new(
		sample,
		spitzen,
		contract,
		bidding_value,
		current_score_without_skat,
		worlds.active_role
	);

	WATCH("ana").stop();
	assert(!picks.empty());
	auto time_for_analysis_ms = WATCH("ana").elapsed<std::chrono::milliseconds>();
	if (1 <= time_for_analysis_ms) {
		std::cout << "WARNING! Time spent to analyze samples: " << time_for_analysis_ms << "ms.\n";
	}

	watch_whole.stop();

	// std::cout << "\nTotal time used: " << watch_whole.elapsed<std::chrono::milliseconds>() << "ms.\n\n";
	stdc::detail::log.flush();

	show_statistics(
		sample,
		current_score_without_skat,
		worlds.active_role,
		picks,
		spitzen,
		contract,
		bidding_value
	);

	//For now, just do an arbitrary choice here.
	return picks.remove_next();

	//Below is code to look at the performance in the main loop of the threads.

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
}

inline void calculate_initial_games(size_t number_samples_to_do, GameType game, Role initial_role) {
	using namespace stdc::literals;
	
	stdc::log(
		"\nStart calculate_initial_games({}, {}, {}).",
		number_samples_to_do,
		to_string(game),
		to_string(initial_role)
	);

	// stdc::log(fmt::format("Start simulation of {} worlds.", number_samples_to_do));
	auto watch_simulation = stdc::SWatch{};
	watch_simulation.start();

	auto [situations, spitzen] = sample_situations_and_spitzen(
		UniformInitialSitDistribution{game, initial_role},
		number_samples_to_do
	);
	
	auto results = muskat::multithreaded_sampling(
		situations,
		Score{0, 0}
	);
	
	watch_simulation.stop();
	
	assert(results.size() == number_samples_to_do);

	log_multithreaded_performance(watch_simulation.elapsed(), 12, number_samples_to_do);

	auto lb = 0;
	auto ls = 0;
	auto l = 0;
	auto w = 0;
	auto ws = 0;
	auto wb = 0;

	auto points = std::array<uint8_t, 121>{};

	for (auto i = 0_z; i < 32; ++i) {
		for (auto res : results) {
			auto r = res[i];
			if (121 <= r.points()) {
				continue;
			}

			++points[r.points()];

			if (r.tricks() == 0) {
				assert(r.points() <= 22);
				++lb;
				continue;
			}
			if (r.points() <= 30) {
				++ls;
				continue;
			}
			if (r.points() <= 60) {
				++l;
				continue;
			}
			if (r.points() <= 89) {
				++w;
				continue;
			}
			if (r.tricks() <= 9) {
				++ws;
				continue;
			}
			assert(r.points() == 120);
			++wb;
		}
	}

	stdc::log(
		"{} {} {} {} {} {}",
		lb, ls, l, w, ws, wb
	);
	stdc::log(
		"{} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {}",
		points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7], points[8], points[9],
		points[10], points[11], points[12], points[13], points[14], points[15], points[16], points[17], points[18], points[19],
		points[20], points[21], points[22], points[23], points[24], points[25], points[26], points[27], points[28], points[29],
		points[30], points[31], points[32], points[33], points[34], points[35], points[36], points[37], points[38], points[39],
		points[40], points[41], points[42], points[43], points[44], points[45], points[46], points[47], points[48], points[49],
		points[50], points[51], points[52], points[53], points[54], points[55], points[56], points[57], points[58], points[59],
		points[60], points[61], points[62], points[63], points[64], points[65], points[66], points[67], points[68], points[69],
		points[70], points[71], points[72], points[73], points[74], points[75], points[76], points[77], points[78], points[79],
		points[80], points[81], points[82], points[83], points[84], points[85], points[86], points[87], points[88], points[89],
		points[90], points[91], points[92], points[93], points[94], points[95], points[96], points[97], points[98], points[99],
		points[100], points[101], points[102], points[103], points[104], points[105], points[106], points[107], points[108], points[109],
		points[110], points[111], points[112], points[113], points[114], points[115], points[116], points[117], points[118], points[119],
		points[120]
	);

	//So we are very sure we see when something changes in the result of multithreaded_sampling.
	stdc::log("Hash Result: {}", stdc::GeneralHasher{}(results, spitzen));
	
	//So we are very sure we see when the generated situations changed.
	stdc::log("Hash Spitzen: {}", stdc::GeneralHasher{}(spitzen));
	stdc::detail::log.flush();
}


} // namespace muskat
