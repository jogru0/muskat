#pragma once

#include "world_simulation.h"
#include "situation_solver.h"
#include "spinlock.h"

#include <stdc/WATCH.h>

#include <thread>


#include <stdc/mathematics.h>

namespace wa {
inline constexpr auto start = size_t{0};
inline constexpr auto loop = size_t{1};
inline constexpr auto lock = size_t{2};
inline constexpr auto three = size_t{3};
inline constexpr auto four = size_t{4};
static std::array<std::array<detail::Watch, 5>, 12> watches_th;
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
	wa::watches_th[worker_id][wa::four].start();

	auto rng = stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness{0, static_cast<unsigned int>(worker_id)});

	wa::watches_th[worker_id][wa::four].stop();
	//Stop when the main thread signals that time is up via should_continue.
	//It's not enough to just wait until the main thread locks the lock, because we coudn't be sure that we
	//are always faster than the main thread by locking it, therefore blocking it forever.
	while (!stoken.stop_requested()) {
		auto situation = possible_worlds.get_one_uniformly_clever(rng);
		auto solver = muskat::SituationSolver{possible_worlds.get_game_type()};

		// solver.calculate_potential_score_2(situation);
		auto points = solver.score_for_possible_plays(situation);

		auto need_to_reallocate = result_ptr->size() == result_ptr->capacity();

		if (!need_to_reallocate) {
			if (!result_write_lock_ptr->try_lock()) {
				//TODO: Is this guaranteed to be observed in this order when the main thread does it in this order?
				assert(stoken.stop_requested());
				break;
			}
			result_ptr->push_back(points);
			result_write_lock_ptr->unlock();
			continue;
		}	

		auto result_copy = std::vector<std::array<uint8_t, 32>>{};
		result_copy.reserve(2 * (result_ptr->capacity() + 1));
		result_copy = *result_ptr;
		result_copy.push_back(points);
		
		if (!result_write_lock_ptr->try_lock()) {
			//TODO: Is this guaranteed to be observed in this order when the main thread does it in this order?
			assert(stoken.stop_requested());
			break;
		}
		*result_ptr = std::move(result_copy);
		result_write_lock_ptr->unlock();
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


[[nodiscard]] inline auto multithreaded_world_simulation(const PossibleWorlds &possible_worlds) {
	using namespace stdc::literals;
	
	auto number_of_threads = std::thread::hardware_concurrency();
	if (number_of_threads == 0) {
		throw std::runtime_error{"Could not determine best number of threads."};
	}

	std::cout << "Number of threads: " << number_of_threads << ".\n";

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

	std::this_thread::sleep_for(std::chrono::seconds(10));
	

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
		wa::watches_th[thread_id][wa::three].start();
		threads[thread_id].detach();
		wa::watches_th[thread_id][wa::three].stop();
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


} // namespace muskat
