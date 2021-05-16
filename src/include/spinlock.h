#pragma once

#include <atomic>
#include <shared_mutex>

namespace stdc {

static_assert(std::atomic<bool>::is_always_lock_free);
static_assert(std::atomic<size_t>::is_always_lock_free);

class SpinLock {
private:
	std::atomic<bool> m_is_locked = false;

	bool try_lock_impl() noexcept {
		// Previously true -> still true, but we return false becase it was locked.
		// Previously false -> we put it to true and return true as this means we locked it.
		return !m_is_locked.exchange(true, std::memory_order_acquire);
	}

public:
	bool is_locked() const noexcept {
		return m_is_locked.load(std::memory_order_relaxed);
	}

	void lock() noexcept {
		for (;;) {
			// Optimistically assume the lock is free on the first try (and in general)
			if (try_lock_impl()) /*[[likely]]*/ {
				return;
			}
			// Wait for lock to be released without generating cache misses
			while (is_locked()) {
				// Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
				// hyper-threads
				__builtin_ia32_pause();
			}
		}
	}

	bool try_lock() noexcept {
		// First do a relaxed load to check if lock is free in order to prevent
		// unnecessary cache misses if someone does while(!try_lock())
		return !is_locked() && try_lock_impl();
	}

	void unlock() noexcept {
		m_is_locked.store(false, std::memory_order_release);
	}
};

template<typename Data>
class Ransom {
private:
	std::atomic<bool> m_is_delivered;
	Data m_data;

public:
	Ransom() : m_is_delivered{false} {}

	template<typename DataToo>
	[[nodiscard]] auto deliver(DataToo &&data) {
		static_assert(std::is_same_v<Data, std::remove_cvref_t<DataToo>>);
		m_data = std::forward<DataToo>(data);
		[[maybe_unused]] auto was_already_there = m_is_delivered.exchange(true, std::memory_order_release);
		assert(!was_already_there);
	}

	[[nodiscard]] auto maybe_collect() const -> std::optional<Data> {
		if (m_is_delivered.load(std::memory_order_acquire)) {
			return m_data;
		}

		return std::nullopt;
	}
};

template<typename Data>
class ConcurrentResultVector {
private:
	size_t m_size;
	std::atomic<size_t> m_count_requested;
	std::vector<Ransom<Data>> m_partial_result_vec;
	
public:
	ConcurrentResultVector(size_t size) :
		m_size(size),
		m_count_requested{0},
		m_partial_result_vec(size)
	{}

	[[nodiscard]] auto maybe_pull_a_number() -> std::optional<size_t> {
		auto number = m_count_requested.fetch_add(1, std::memory_order_relaxed);
		if (number < m_size) {
			return number;
		}

		return std::nullopt;
	}

	//Includes entries that are currently calculated.
	[[nodiscard]] auto get_current_progress() const {
		return m_count_requested.load(std::memory_order_relaxed);
	}

	

	template<typename DataToo>
	[[nodiscard]] auto report_a_result(size_t number, DataToo &&data) {
		static_assert(std::is_same_v<Data, std::remove_cvref_t<DataToo>>);
		assert(number < m_count_requested.load(std::memory_order_relaxed));
		assert(number < m_size);
		m_partial_result_vec[number].deliver(std::forward<DataToo>(data));
	}

	[[nodiscard]] auto collect_all_results_so_far() -> std::vector<Data> {
		auto end_id = std::min(
			m_size,
			m_count_requested.load(std::memory_order_relaxed)
		);
		auto begin_it = m_partial_result_vec.begin();
		auto end_it = stdc::to_it(m_partial_result_vec, end_id);

		auto result = std::vector<Data>{};
		result.reserve(end_id);
		for (auto ransom_it = begin_it; ransom_it != end_it; ++ransom_it) {
			if (auto maybe_res = ransom_it->maybe_collect()) {
				result.push_back(std::move(*maybe_res));
			}
		}
		return result;
	}
};


} // namespace stdc
