#pragma once

#include "world_simulation.h"
#include "situation_solver.h"
#include "spinlock.h"

#include <stdc/swatch.h>
#include <stdc/WATCH.h>

#include <thread>


#include <stdc/mathematics.h>

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


inline void execute_worker(
	std::stop_token stoken,
	PossibleWorlds possible_worlds, //TODO: Not a reference, so not every thread has to access the same memory there?
	const std::shared_ptr<std::vector<std::array<uint8_t, 32>>> result_ptr,
	const std::shared_ptr<stdc::SpinLock> result_write_lock_ptr, //Reading is always allowed, since noone else writes.
	size_t worker_id
) try {
	wa::watches_th[worker_id][wa::start].stop();
	wa::watches_th[worker_id][wa::rng].start();

	auto rng = stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness{0, static_cast<unsigned int>(worker_id)});

	wa::watches_th[worker_id][wa::rng].stop();
	//Stop when the main thread signals that time is up via should_continue.
	//It's not enough to just wait until the main thread locks the lock, because we coudn't be sure that we
	//are always faster than the main thread by locking it, therefore blocking it forever.
	wa::watches_th[worker_id][wa::loop_pre].start();
	
	while (!stoken.stop_requested()) {

		
		auto situation = possible_worlds.get_one_uniformly_clever(rng);
		auto solver = muskat::SituationSolver{possible_worlds.get_game_type()};

		wa::watches_th[worker_id][wa::loop_pre].stop();
		++wa::iterations_pre[worker_id];
		wa::watches_th[worker_id][wa::loop_main].start();
		auto points = solver.score_for_possible_plays(situation);
		
		wa::watches_th[worker_id][wa::loop_main].stop();
		++wa::iterations_main[worker_id];
		wa::watches_th[worker_id][wa::loop_post].start();
		

		auto need_to_reallocate = result_ptr->size() == result_ptr->capacity();

		if (!need_to_reallocate) {
			if (!result_write_lock_ptr->try_lock()) {
				//TODO: Is this guaranteed to be observed in this order when the main thread does it in this order?
				assert(stoken.stop_requested());
				break;
			}
			{ //LOCKED 
				result_ptr->push_back(points);
			}
			result_write_lock_ptr->unlock();
		} else {
			auto result_copy = std::vector<std::array<uint8_t, 32>>{};
			result_copy.reserve(2 * (result_ptr->capacity() + 1));
			result_copy = *result_ptr;
			result_copy.push_back(points);
			
			if (!result_write_lock_ptr->try_lock()) {
				//TODO: Is this guaranteed to be observed in this order when the main thread does it in this order?
				assert(stoken.stop_requested());
				break;
			}
			
			{ //LOCKED 
				*result_ptr = std::move(result_copy);
			}
			result_write_lock_ptr->unlock();
		}

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

	//Not okay while measuring.
	throw;
}


[[nodiscard]] inline auto multithreaded_world_simulation(const PossibleWorlds &possible_worlds, std::chrono::milliseconds time) {
	using namespace stdc::literals;
	
	auto number_of_threads = std::thread::hardware_concurrency();
	if (number_of_threads == 0) {
		throw std::runtime_error{"Could not determine best number of threads."};
	}

	std::cout << "Simulating with " << number_of_threads << " threads.\n";

	assert(number_of_threads == 12);
	for (auto &watches : wa::watches_th) {
		watches = std::array{
			::detail::Watch{}, ::detail::Watch{}, ::detail::Watch{}, ::detail::Watch{}, ::detail::Watch{}
		};
	}


	//This way, threads are joined before the results are destructed, guranteeing that the results exists while threads exist.
	auto results_th = std::vector<std::shared_ptr<std::vector<std::array<uint8_t, 32>>>>(number_of_threads);
	//Same for the locks
	auto locks_th = std::vector<std::shared_ptr<stdc::SpinLock>>(number_of_threads);

	for (auto i = 0_z; i < number_of_threads; ++i) {
		results_th[i] = std::make_shared<std::vector<std::array<uint8_t, 32>>>();
		locks_th[i] = std::make_shared<stdc::SpinLock>();
	}
	
	auto threads = std::vector<std::jthread>{};
	threads.reserve(number_of_threads);

	for (auto thread_id = 0_z; thread_id < number_of_threads; ++thread_id) {
		wa::watches_th[thread_id][wa::start].start();
	}

	for (auto thread_id = 0_z; thread_id < number_of_threads; ++thread_id) {
		threads.push_back(std::jthread{
			execute_worker,
			possible_worlds,
			results_th[thread_id],
			locks_th[thread_id],
			thread_id
		});
	}

	std::this_thread::sleep_for(time);
	

	WATCH("aftermath").reset();
	WATCH("aftermath").start();

	WATCH("aftermath1").reset();
	WATCH("aftermath1").start();

	for (auto thread_id = 0_z; thread_id < number_of_threads; ++thread_id) {
		threads[thread_id].request_stop();
	}

	WATCH("aftermath1").stop();
	WATCH("aftermath2").reset();
	WATCH("aftermath2").start();

	for (auto thread_id = 0_z; thread_id < number_of_threads; ++thread_id) {
		// wa::watches_th[thread_id][wa::detach].start();
		threads[thread_id].detach();
		// wa::watches_th[thread_id][wa::detach].stop();
	}

	WATCH("aftermath2").stop();
	WATCH("aftermath3").reset();
	WATCH("aftermath3").start();

	auto results_obtained = 0_z;
	auto results_obtained_th = std::vector(number_of_threads, false);

	auto results = std::vector<std::array<uint8_t, 32>>{};

	while (results_obtained != number_of_threads) {
		for (auto thread_id = 0_z; thread_id < number_of_threads; ++thread_id) {
			if (results_obtained_th[thread_id]) {
				continue;
			}

			//Lock forever.
			if (!locks_th[thread_id]->try_lock()) {
				continue;
			}

			std::move((results_th[thread_id])->begin(), (results_th[thread_id])->end(), std::back_inserter(results));
			
			results_obtained_th[thread_id] = true;
			++results_obtained;
		}
	}

	WATCH("aftermath3").stop();

	WATCH("aftermath").stop();
	return results;
}


[[nodiscard]] inline auto pick_best_card(
	const PossibleWorlds &worlds,
	//TODO: Track in worlds!!!!!!
	uint8_t current_score,
	std::chrono::milliseconds time
) -> Card {
	auto swatch = stdc::SWatch{};
	swatch.start();

	WATCH("simulation").reset();
	WATCH("simulation").start();
	auto results = muskat::multithreaded_world_simulation(worlds, time);
	
	//TODO: Empty -> Assert triggert.
	auto sample = muskat::to_sample(std::move(results));
	WATCH("simulation").stop();
	std::cout << "Samples calculated: " << sample.points_for_situations().size() << '\n';
	std::cout << "Time spent to run the simulations: " << WATCH("simulation").elapsed<std::chrono::milliseconds>() << "ms.\n";

	std::cout << "\nUsing these samples to make an informed choice now.\n";
	std::cout << "Legal moves to choose from: " << to_string(sample.playable_cards()) << '\n';

	WATCH("ana").reset();
	WATCH("ana").start();
	auto picks = analyze(sample, current_score, worlds.active_role);
	WATCH("ana").stop();
	assert(!picks.empty());
	auto result = picks.remove_next();
	std::cout << "Arbitrary final choice: " << to_string(result) << '\n';
	std::cout << "Time spent to do these choices: " << WATCH("ana").elapsed<std::chrono::milliseconds>() << "ms.\n";

	swatch.stop();

	std::cout << "\nTotal time allowed to use: " << time.count() << "ms.\n";
	std::cout << "Total time used: " << swatch.elapsed<std::chrono::milliseconds>() << "ms.\n\n";

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


} // namespace muskat
