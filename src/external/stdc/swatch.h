#pragma once

#include <chrono>

#include <fmt/core.h>

#include<cassert>

namespace stdc {

class SWatch {
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> old_now{};
	std::chrono::nanoseconds elap{};
	bool running_DEBUG;

public:
	void start() noexcept {
		assert(!running_DEBUG);
		assert((running_DEBUG = true, true));
		old_now = std::chrono::high_resolution_clock::now();
	}

	void stop() noexcept {
		auto now = std::chrono::high_resolution_clock::now();
		elap += now - old_now;
		assert(running_DEBUG);
		assert((running_DEBUG = false, true));
	}
	
	template<typename Duration>
	[[nodiscard]] auto peek() const noexcept {
		auto now = std::chrono::high_resolution_clock::now();
		assert(running_DEBUG);
		return elap + (now - old_now);
	}

	template<typename Duration>
	[[nodiscard]] auto elapsed() const noexcept {
		assert(!running_DEBUG);
		return std::chrono::duration_cast<Duration>(elap).count();
	}

	[[nodiscard]] auto elapsed() const noexcept {
		assert(!running_DEBUG);
		return elap;
	}

	void reset() noexcept {
		assert(!running_DEBUG);
		elap = std::chrono::nanoseconds{};
	}
};

namespace detail {

template<int factor_to_ns>
[[nodiscard]] auto to_string(std::chrono::nanoseconds ns, int prec, std::string abbreviation) {
	return fmt::format(
		"{:.{}f}{}",
		ns.count() / static_cast<double>(factor_to_ns),
		prec,
		abbreviation
	);
}

} //namespace detail

[[nodiscard]] auto to_string_ms(std::chrono::nanoseconds ns, int prec = 0) {
	return detail::to_string<1'000'000>(ns, prec, "ms");
}

[[nodiscard]] auto to_string_us(std::chrono::nanoseconds ns, int prec = 0) {
	return detail::to_string<1'000>(ns, prec, "us");
}

[[nodiscard]] auto to_string_s(std::chrono::nanoseconds ns, int prec = 0) {
	return detail::to_string<1'000'000'000>(ns, prec, "s");
}


} //namespace stdc