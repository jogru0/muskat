#pragma once

#include "world_simulation.h"
#include "situation_solver.h"
#include "spinlock.h"



#include <thread>


#include <stdc/mathematics.h>

namespace muskat {


inline void execute_worker(
	const PossibleWorlds &possible_worlds, //TODO: Not a reference, so not every thread has to access the same memory there?
	std::vector<std::array<uint8_t, 32>> &result, //We need this to be not destructed until the thread is joined.
	stdc::SpinLock &result_write_lock, //Reading is always allowed, since noone else writes.
	const std::atomic<bool> &should_continue,
	size_t worker_id
) {
	assert(result.size() == 0);

	auto rng = stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness{0, static_cast<unsigned int>(worker_id)});

	//Stop when the main thread signals that time is up via should_continue.
	//It's not enough to just wait until the main thread locks the lock, because we coudn't be sure that we
	//are always faster than the main thread by locking it, therefore blocking it forever.
	while (should_continue.load(std::memory_order::relaxed)) {
		auto situation = possible_worlds.get_one_uniformly_clever(rng);
		auto solver = muskat::SituationSolver{possible_worlds.get_game_type()};

		solver.calculate_potential_score_2(situation);
		auto points = solver.score_for_possible_plays(situation);

		auto need_to_reallocate = result.size() == result.capacity();

		if (!need_to_reallocate) {
			if (!result_write_lock.try_lock()) {
				//TODO: Is this guaranteed to be observed in this order when the main thread does it in this order?
				assert(!should_continue.load(std::memory_order::relaxed));
				return;
			}
			result.push_back(points);
			result_write_lock.unlock();
			continue;
		}	

		auto result_copy = std::vector<std::array<uint8_t, 32>>{};
		result_copy.reserve(2 * (result.capacity() + 1));
		result_copy = result;
		result_copy.push_back(points);
		
		if (!result_write_lock.try_lock()) {
			//TODO: Is this guaranteed to be observed in this order when the main thread does it in this order?
			assert(!should_continue.load(std::memory_order::relaxed));
			return;
		}
		result = std::move(result_copy);
		result_write_lock.unlock();
	}
}


[[nodiscard]] inline auto multithreaded_world_simulation(const PossibleWorlds &possible_worlds) {
	using namespace stdc::literals;
	
	auto number_of_threads = std::thread::hardware_concurrency();
	if (number_of_threads == 0) {
		throw std::runtime_error{"Could not determine best number of threads."};
	}

	std::cout << "Number of threads: " << number_of_threads << ".\n";

	auto threads_should_continue = std::atomic{true};

	//This way, threads are joined before the results are destructed, guranteeing that the results exists while threads exist.
	auto results_th = std::vector<std::vector<std::array<uint8_t, 32>>>(number_of_threads);
	//Same for the locks
	auto locks_th = std::vector<stdc::SpinLock>(number_of_threads);
	auto threads = std::vector<std::jthread>{};
	threads.reserve(number_of_threads);

	for (auto thread_id = 0_z; thread_id < number_of_threads; ++thread_id) {
		threads.push_back(std::jthread{
			execute_worker,
			std::cref(possible_worlds),
			std::ref(results_th[thread_id]),
			std::ref(locks_th[thread_id]),
			std::cref(threads_should_continue),
			thread_id
		});
	}

	std::this_thread::sleep_for(std::chrono::seconds(10));

	auto results_obtained = 0_z;
	auto results_obtained_th = std::vector(number_of_threads, false);

	auto results = std::vector<std::array<uint8_t, 32>>{};

	threads_should_continue = false;
	while (results_obtained != number_of_threads) {
		for (auto thread_id = 0_z; thread_id < number_of_threads; ++thread_id) {
			if (results_obtained_th[thread_id]) {
				continue;
			}

			//Lock forever.
			if (!locks_th[thread_id].try_lock()) {
				continue;
			}

			std::move(RANGE(results_th[thread_id]), std::back_inserter(results));
			results_obtained_th[thread_id] = true;
			++results_obtained;
		}
	}

	return results;
}


} // namespace muskat
