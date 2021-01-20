#pragma once

#include <iomanip>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>

namespace stdc::IO {

	namespace detail {
		[[nodiscard]] inline auto open_file_with_checks(
			const std::filesystem::path& file_path, const std::ios_base::openmode mode
		) /*except*/
			-> std::fstream
		{
			if (std::filesystem::is_directory(file_path)) {
				throw std::runtime_error("'" + file_path.string() + "' is a directory, not a file" + "\n");
			}

			auto result = std::fstream{file_path, mode};
			if (not result.is_open()) {
				throw std::runtime_error("Could not open file '" + file_path.string() + "'\n");
			}
			return result;
		}
	} //namespace detail

	template<typename Source>
	[[nodiscard]] auto open_file_with_checks_for_reading(const Source& file_path) /*except*/
		-> std::fstream
	{
		return detail::open_file_with_checks(std::filesystem::path{file_path}, std::ios_base::in);
	}

	template<typename Source>
	[[nodiscard]] auto open_file_with_checks_for_writing(
		const Source& file_path, const std::ios_base::openmode mode = std::ios_base::out
	) /*except*/
		-> std::fstream
	{
		return detail::open_file_with_checks(std::filesystem::path{file_path}, mode);
	}

	template<typename Source>
	[[nodiscard]] auto read_file_to_string(const Source& file_path) /*except*/
		-> std::string
	{
		auto fs = open_file_with_checks_for_reading(file_path);
		//https://stackoverflow.com/a/2602258
		auto ss = std::stringstream{};
		ss << fs.rdbuf();
		return ss.str();
	}

	template<typename Integer>
	[[nodiscard]] auto int_to_string_with_leading_zeroes(Integer i, int string_length/* = 6*/)
		-> std::string
	{
		std::stringstream ss;
		ss << std::setw(string_length) << std::setfill('0') << i;
		return ss.str();
	}

	template<typename Field>
	[[nodiscard]] auto to_string_no_trailing_zeros(Field f) noexcept
		-> std::string
	{
		auto str = std::to_string(f);
		auto pos_of_dot = str.find('.');
		if (pos_of_dot != std::string::npos) {
			str.erase(str.find_last_not_of('0') + 1, std::string::npos);
			assert(pos_of_dot < str.size());
			if (pos_of_dot + 1 == str.size()) {
				str.pop_back();
			}
		}
		return str;
	}

	template<typename Field>
	[[nodiscard]] auto to_string_with_fixed_precision(Field f, int precision)
		-> std::string
	{
		//apparently this is way to go: https://stackoverflow.com/a/29200671/8038465
		std::stringstream ss;
		ss << std::fixed << std::setprecision(precision) << f;
		return ss.str();
	}
} //namespace stdc::IO