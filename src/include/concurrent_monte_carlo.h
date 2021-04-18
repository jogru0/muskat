#pragma once

#include "world_simulation.h"
#include "situation_solver.h"
#include "spinlock.h"



#include <thread>


#include <stdc/mathematics.h>

namespace muskat {


inline void execute_worker(
	PossibleWorlds possible_worlds, //TODO: Not a reference, so not every thread has to access the same memory there?
	const std::unique_ptr<std::vector<std::array<uint8_t, 32>>> result_ptr,
	const std::unique_ptr<stdc::SpinLock> result_write_lock_ptr, //Reading is always allowed, since noone else writes.
	const std::unique_ptr<std::atomic<bool>> should_continue_ptr,
	size_t worker_id
) {
	assert(result_ptr->size() == 0);

	auto rng = stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness{0, static_cast<unsigned int>(worker_id)});

	//Stop when the main thread signals that time is up via should_continue.
	//It's not enough to just wait until the main thread locks the lock, because we coudn't be sure that we
	//are always faster than the main thread by locking it, therefore blocking it forever.
	while (should_continue_ptr->load(std::memory_order::relaxed)) {
		auto situation = possible_worlds.get_one_uniformly_clever(rng);
		auto solver = muskat::SituationSolver{possible_worlds.get_game_type()};

		solver.calculate_potential_score_2(situation);
		auto points = solver.score_for_possible_plays(situation);

		auto need_to_reallocate = result_ptr->size() == result_ptr->capacity();

		if (!need_to_reallocate) {
			if (!result_write_lock_ptr->try_lock()) {
				//TODO: Is this guaranteed to be observed in this order when the main thread does it in this order?
				assert(!should_continue_ptr->load(std::memory_order::relaxed));
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
			assert(!should_continue_ptr->load(std::memory_order::relaxed));
			break;
		}
		*result_ptr = std::move(result_copy);
		result_write_lock_ptr->unlock();
	}

	while (should_continue_ptr->load(std::memory_order::relaxed)) {
		//Nothing.
	}
}


[[nodiscard]] inline auto multithreaded_world_simulation(const PossibleWorlds &possible_worlds) {
	using namespace stdc::literals;
	
	auto number_of_threads = std::thread::hardware_concurrency();
	if (number_of_threads == 0) {
		throw std::runtime_error{"Could not determine best number of threads."};
	}

	std::cout << "Number of threads: " << number_of_threads << ".\n";


	//This way, threads are joined before the results are destructed, guranteeing that the results exists while threads exist.
	auto results_temp = std::vector<std::unique_ptr<std::vector<std::array<uint8_t, 32>>>>(number_of_threads);
	//Same for the locks
	auto locks_temp = std::vector<std::unique_ptr<stdc::SpinLock>>(number_of_threads);
	auto should_continue_temp = std::vector<std::unique_ptr<std::atomic<bool>>>(number_of_threads);

	auto results_th = std::vector<std::vector<std::array<uint8_t, 32>> *>(number_of_threads);
	auto locks_th = std::vector<stdc::SpinLock *>(number_of_threads);
	auto should_continue_th = std::vector<std::atomic<bool> *>(number_of_threads);
	
	

	for (auto i = 0_z; i < number_of_threads; ++i) {
		results_temp[i] = std::make_unique<std::vector<std::array<uint8_t, 32>>>();
		locks_temp[i] = std::make_unique<stdc::SpinLock>();
		should_continue_temp[i] = std::make_unique<std::atomic<bool>>(true);

		results_th[i] = results_temp[i].get();
		locks_th[i] = locks_temp[i].get();
		should_continue_th[i] = should_continue_temp[i].get();
	}
	
	for (auto thread_id = 0_z; thread_id < number_of_threads; ++thread_id) {
		std::jthread{
			execute_worker,
			possible_worlds,
			std::move(results_temp[thread_id]),
			std::move(locks_temp[thread_id]),
			std::move(should_continue_temp[thread_id]),
			thread_id
		}.detach();
	}

	std::this_thread::sleep_for(std::chrono::seconds(10));

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
			*should_continue_th[thread_id] = false;
			
			results_obtained_th[thread_id] = true;
			++results_obtained;
		}
	}

	return results;
}


} // namespace muskat
