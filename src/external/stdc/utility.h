#pragma once

#include <utility>

#include "type_traits.h"

#include <charconv>
#include <iostream>
#include <iomanip>
#include <optional>
#include <string_view>
#include <sstream>
#include <string>

//Needs to be a macro in order to allow short circuiting.
#define IMPLIES(antecedent, consequent) (!(antecedent) || (consequent))

//Convenience until we can pass containers in C++20 range algorithms.
#define RANGE(container) container.begin(), container.end()
#define REVERSE_RANGE(container) container.rbegin(), container.rend()
#define CONST_RANGE(container) container.cbegin(), container.cend()
#define CONST_REVERSE_RANGE(container) container.crbegin(), container.crend()

namespace stdc {
	template<typename It, typename ParallelIt>
	[[nodiscard]] constexpr auto get_parallel_it(It first, It it, ParallelIt parallel_first) -> ParallelIt {
		auto distance = std::distance(first, it);
		assert(0 <= distance);
		return std::next(parallel_first, distance);
	}
	
	template<typename RAI>
	[[nodiscard]] constexpr auto unsigned_distance(RAI first, RAI last) -> size_t {
		static_assert(std::is_convertible_v<typename std::iterator_traits<RAI>::iterator_category, std::random_access_iterator_tag>);
		assert(first <= last);
		auto dist = last - first;
		assert(0 <= dist);
		return static_cast<size_t>(dist);
	}

	template<typename RAC>
	[[nodiscard]] constexpr const auto &bidir_access(const RAC &container, typename RAC::size_type id, bool reversed) {
		assert(id < container.size());
		return container[reversed ? container.size() - id - 1 : id];
	}

	template<typename RAC>
	[[nodiscard]] constexpr auto to_index(const RAC &container, typename RAC::const_iterator it) {
		assert(container.begin() <= it && it <= container.end());
		return unsigned_distance(container.begin(), it);
	}

	template<typename RAC>
	[[nodiscard]] constexpr auto to_it(const RAC &container, typename RAC::size_type index) {
		assert(index <= container.size());
		return container.begin() + static_cast<ptrdiff_t>(index);
	}

	template<typename RAC>
	[[nodiscard]] constexpr auto to_it(RAC &container, typename RAC::size_type index) {
		assert(index <= container.size());
		return container.begin() + static_cast<ptrdiff_t>(index);
	}

	template<typename T, typename ...Options>
	[[nodiscard]] constexpr auto equals_any_of(const T &value, const Options &...options) -> bool {
		//Saftey measure.
		static_assert(are_same_v<T, Options ...>);
		return (... || (value == options));
	}

	[[nodiscard]] constexpr auto are_all_equal() -> bool {
		return true;
	}

	template<typename Head, typename ...Tail>
	[[nodiscard]] constexpr auto are_all_equal(const Head &head, const Tail &...tail) -> bool {
		//Saftey measure.
		static_assert(are_same_v<Head, Tail ...>);
		return (... && (head == tail));
	}
	
	template<typename Head, typename ...Tail>
	[[nodiscard]] constexpr auto any_of_equal_values(Head head, [[maybe_unused]] const Tail &...tail) {
		//Saftey measure.
		static_assert(are_same_v<Head, Tail ...>);
		assert(are_all_equal(head, tail ...));
		return head;
	}

	//TODO: Negative precision. --jgr/21/01
	[[nodiscard]] inline auto to_string(double value, int precision) -> std::string {
		assert(0 <= precision);
		auto sstream = std::stringstream{};
		sstream << std::fixed << std::setprecision(precision) << value;
		return sstream.str();
	}
	
	template<typename Maybe>
	[[nodiscard]] constexpr decltype(auto) surely(Maybe &&maybe) {
		assert(maybe);
		return *std::forward<Maybe>(maybe);
	}

	template<typename TargetType>
	[[nodiscard]] constexpr auto maybe_parse_chars(std::string_view sv) -> std::optional<TargetType> {
		const auto* data_begin = sv.data();
		auto data_end = data_begin + sv.size(); //NOLINT
		
		TargetType result;
		auto from_chars_result = std::from_chars(data_begin, data_end, result);
		if (from_chars_result.ec != std::errc{} || from_chars_result.ptr != data_end) {
			return std::nullopt;	
		}
		return result;
	}

} //namespace stdc

#ifdef	NDEBUG
	#define IN_DEBUG(expr) (static_cast<void>(0))
#else
	#define IN_DEBUG(expr) (expr)
#endif /* NDEBUG.  */



// ==== adanced type detection ====

//Has to be in global scope!
//by https://stackoverflow.com/a/56766138/8038465
template <typename T>
[[nodiscard]] constexpr auto stdc_detail_type_name() {
	using namespace std::string_view_literals;
#ifdef __clang__
	constexpr auto prefix = "auto stdc_detail_type_name() [T = "sv;
	constexpr auto suffix = "]"sv;
	std::string_view name = __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
	constexpr auto prefix = "constexpr auto stdc_detail_type_name() [with T = "sv;
	constexpr auto suffix = "]"sv;
	std::string_view name = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
	constexpr auto prefix = "auto __cdecl stdc_detail_type_name<"sv;
	constexpr auto suffix = ">(void)"sv;
	std::string_view name = __FUNCSIG__;
#endif
	name.remove_prefix(prefix.size());
	name.remove_suffix(suffix.size());
	return name;
}

namespace stdc {
	template<typename Type>
	[[nodiscard]] constexpr auto type_name() -> std::string_view {
		return stdc_detail_type_name<Type>();
	}
} //namespace stdc