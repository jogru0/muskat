#pragma once

#include <mutex>

// basic file operations
#include <fstream>

#include <fmt/core.h>

namespace stdc {

// enum class LogLevel {
// 	DEBUG,
// 	INFO,
// 	ERROR
// };

namespace detail {


class Logger {
private:
	std::mutex mut{};
	std::ofstream out{"log.log", std::ios::app | std::ios::out};

public:
	template<typename ...Args>
	void operator()(std::string_view msg, Args &&...args) {
		auto str = fmt::format(msg, std::forward<Args>(args) ...);
		auto lock = std::scoped_lock{mut};
		out << '\n' << str;
	}

	void flush() {
		auto lock = std::scoped_lock{mut};
		out << std::flush;
	}


};


inline auto log = detail::Logger{};

} //namespace detail

template<typename ...Args>
void log(Args &&...args) {
	detail::log(std::forward<Args>(args) ...);
}


template<typename ...Args>
void log_debug(Args &&.../*args*/) {
	// detail::log(std::forward<Args>(args) ...);
}

} // namespace stdc
