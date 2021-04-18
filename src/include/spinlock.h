#pragma once

#include <atomic>

namespace stdc {

class SpinLock {
private:
	std::atomic<bool> m_is_locked = false;

	bool try_lock_impl() noexcept {
		// Previously true -> still true, but we return false becase it was locked.
		// Previously false -> we put it to true and return true as this means we locked it.
		return !m_is_locked.exchange(true, std::memory_order_acquire);
	}

	bool is_locked() const noexcept {
		return m_is_locked.load(std::memory_order_relaxed);
	}
public:

	void lock() noexcept {
		for (;;) {
			// Optimistically assume the lock is free on the first try
			if (try_lock_impl()) {
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

} // namespace stdc
