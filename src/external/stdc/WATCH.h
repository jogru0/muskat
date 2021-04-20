#pragma once

#include <chrono>
#include <string_view>
#include <unordered_map>

namespace detail {

	class Watch {
	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> old_now{};
		std::chrono::nanoseconds elap{};

	public:
		void start() noexcept {
			old_now = std::chrono::high_resolution_clock::now();
		}

		void stop() noexcept {
			auto now = std::chrono::high_resolution_clock::now();
			elap += now - old_now;
		}

		template<typename Duration>
		[[nodiscard]] auto elapsed() const noexcept {
			return std::chrono::duration_cast<Duration>(elap).count();
		}

		void reset() noexcept {
			elap = std::chrono::nanoseconds{};
		}
 	};

	//A managed global storage that can be used for time measurement.
	//deferred as asked for by clang-tidy "[cert-err58-cpp]"
	[[nodiscard]] inline auto get_watch(std::string_view name) -> Watch & {
		static std::unordered_map<std::string_view, Watch> watches;
		return watches[name];
	}
} //namespace detail

[[nodiscard]] inline auto WATCH(std::string_view name) noexcept -> detail::Watch & {
	return detail::get_watch(name);
}