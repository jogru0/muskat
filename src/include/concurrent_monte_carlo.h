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

} //namespace wa

static auto done_threads = std::atomic<uint8_t>{0};

namespace muskat {

inline void execute_worker_sampling(
	std::stop_token stoken,
	const std::vector<std::tuple<Situation, Card, Card, GameType>> &inputs,
	stdc::ConcurrentResultVector<std::array<uint8_t, 32>> &results,
	size_t worker_id,
	std::vector<double> &times_in_ms,
	std::vector<std::vector<double>> &numbers_of_nodes
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
		
		auto solver = muskat::SituationSolver{situation, game, skat_0, skat_1};
		auto points = solver.score_for_possible_plays(situation);
		auto nodes = std::vector{static_cast<double>(solver.number_of_nodes()) / 1000.};
		
		// auto [points, nodes] = score_for_possible_plays_separate(situation, game);
		
		//MEASURE END
		
		watch_solve.stop();
		
		wa::watches_th[worker_id][wa::loop_main].stop();
		++wa::iterations_main[worker_id];
		wa::watches_th[worker_id][wa::loop_post].start();


		times_in_ms[result_id] = (static_cast<double>(watch_solve.elapsed<std::chrono::nanoseconds>()) + .5) / 1'000'000.;
		numbers_of_nodes[result_id] = nodes;

		results.report_a_result(result_id, points);
		
		wa::watches_th[worker_id][wa::loop_post].stop();
		++wa::iterations_post[worker_id];
		wa::watches_th[worker_id][wa::loop_pre].start();
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

template<typename SituationDistribution>
[[nodiscard]] inline auto multithreaded_sampling(
	const SituationDistribution &situation_dist,
	size_t number_samples
)
	-> std::vector<PerfectInformationResult>
{
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

	//Uses shared_ptr because threads might be finished before or after this function returns.
	auto results = stdc::ConcurrentResultVector<std::array<uint8_t, 32>>{number_samples};

	auto times_in_ms = std::vector<double>(number_samples);
	auto numbers_of_nodes = std::vector<std::vector<double>>(number_samples);
		
	// Seed with a real random value, if available
	// auto seed_source = pcg_extras::seed_seq_from<std::random_device>{};

	// Make a random number engine
	auto rng = pcg32(/*seed_source*/);

	auto inputs = std::vector<std::tuple<Situation, Card, Card, GameType>>{};
	inputs.reserve(number_samples);

	auto watch_sampling = stdc::SWatch{};
	watch_sampling.start();
	std::generate_n(std::back_inserter(inputs), number_samples, [&](){
		return situation_dist(rng);
	});
	watch_sampling.stop();
	stdc::log_debug(
		"Sampling of {} situations took {}.",
		number_samples,
		stdc::to_string_us(watch_sampling.elapsed())
	);
	
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
			std::ref(numbers_of_nodes)
		});
	}

	for (auto thread_id = 0_z; thread_id < number_of_threads; ++thread_id) {
		threads[thread_id].join();
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

[[nodiscard]] inline auto pick_best_card(
	const PossibleWorlds &worlds,
	//TODO: Track in worlds!!!!!!
	uint8_t current_score,
	size_t number_samples_to_do
) -> Card {
	using namespace stdc::literals;
	
	stdc::log("Start pick_best_card.");
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


	// stdc::log(fmt::format("Start simulation of {} worlds.", number_samples_to_do));
	auto watch_simulation = stdc::SWatch{};
	watch_simulation.start();
	
	// auto results = muskat::multithreaded_world_simulation(worlds, std::chrono::seconds(100));
	auto results = muskat::multithreaded_sampling(dist, number_samples_to_do);
	
	//TODO: Empty -> Assert triggert.
	auto sample = muskat::to_sample(std::move(results));
	
	watch_simulation.stop();
	assert(sample.points_for_situations().size() == number_samples_to_do);

	stdc::log(
		"Simulations took {} -> perceived time per sample: {}.",
		stdc::to_string_s(watch_simulation.elapsed(), 1),
		stdc::to_string_ms((watch_simulation.elapsed() * 12) / number_samples_to_do)
	);


	std::cout << "\nUsing these samples to make an informed choice now.\n";

	WATCH("ana").reset();
	WATCH("ana").start();
	auto picks = analyze(sample, current_score, worlds.active_role);
	WATCH("ana").stop();
	assert(!picks.empty());
	auto result = picks.remove_next();
	std::cout << "Time spent to do these choices: " << WATCH("ana").elapsed<std::chrono::milliseconds>() << "ms.\n";

	watch_whole.stop();

	std::cout << "\nTotal time used: " << watch_whole.elapsed<std::chrono::milliseconds>() << "ms.\n\n";
	stdc::detail::log.flush();

	show_statistics(sample, current_score, worlds.active_role, result);


	return result;

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
	
	auto results = muskat::multithreaded_sampling(
		UniformInitialSitDistribution{game, initial_role},
		number_samples_to_do
	);
	
	watch_simulation.stop();
	
	// assert(sample.points_for_situations().size() == number_samples_to_do);

	stdc::log(
		"Simulations took {} -> perceived time per sample: {}.",
		stdc::to_string_s(watch_simulation.elapsed(), 1),
		stdc::to_string_ms((watch_simulation.elapsed() * 12) / number_samples_to_do)
	);


	stdc::log("Hash: {}", stdc::GeneralHasher{}(results));
	stdc::detail::log.flush();
}


} // namespace muskat
