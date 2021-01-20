#pragma once

#include "utility.h"
#include "get.h"
#include "literals.h"

#include <functional>
#include <type_traits>

#include <cstddef>

namespace stdc {
namespace srange {

enum class comparison_result {less = -1, equal = 0, greater = 1};

namespace detail {

template<typename ...StaticContainers>
using mutual_index_sequence_for = std::make_index_sequence<
	any_of_equal_values(std::tuple_size_v<StaticContainers> ...)
>;

template<typename F, size_t ...Is>
constexpr void for_impl([[maybe_unused]] F f, std::index_sequence<Is ...>) {
	(... , std::invoke(f, std::integral_constant<size_t, Is>{}));
}

template<typename F, size_t ...Is>
[[nodiscard]] constexpr auto find_if_impl([[maybe_unused]] F f, std::index_sequence<Is ...>) -> size_t {
	using namespace literals;
	auto I = 0_z;
	[[maybe_unused]] auto tmp = (... || (std::invoke(f, std::integral_constant<size_t, Is>{}) || (++I, false)));
	return I;
}


template<typename F, size_t ...Is>
[[nodiscard]] constexpr auto lexicographical_compare_impl([[maybe_unused]] F f, std::index_sequence<Is ...>) -> comparison_result {
	using namespace literals;
	auto result = comparison_result::equal;
	[[maybe_unused]] auto tmp = (... || static_cast<bool>(result = std::invoke(f, std::integral_constant<size_t, Is>{})));
	return result;
}

template<typename F, size_t ...Is>
[[nodiscard]] constexpr auto all_impl([[maybe_unused]] F f, std::index_sequence<Is ...>) -> bool {
	return (... && std::invoke(f, std::integral_constant<size_t, Is>{}));
}

template<typename F, size_t ...Is>
[[nodiscard]] constexpr auto any_impl([[maybe_unused]] F f, std::index_sequence<Is ...>) -> bool {
	return (... || std::invoke(f, std::integral_constant<size_t, Is>{}));
}

template<typename Result, typename F, size_t ...Is>
[[nodiscard]] constexpr auto construct_impl([[maybe_unused]] F f, std::index_sequence<Is ...>) -> Result {
	return Result{std::invoke(f, std::integral_constant<size_t, Is>{}) ...};
}

} //namespace detail

template<size_t I, typename F, typename ...StaticContainers>
[[nodiscard]] constexpr decltype(auto) apply_to_index(F &&f, StaticContainers &&...static_containers) {
	return std::invoke(std::forward<F>(f), get<I>(std::forward<StaticContainers>(static_containers)) ...);
}

template<typename F, typename ...StaticContainers>
constexpr void ranged_for(F f, StaticContainers &&...static_containers) {
	detail::for_impl([&](auto I) {
		apply_to_index<I>(f, std::forward<StaticContainers>(static_containers) ...);
	}, detail::mutual_index_sequence_for<std::remove_reference_t<StaticContainers> ...>{});
}

template<typename Result, typename F, typename ...StaticContainers>
[[nodiscard]] constexpr auto ranged_construct(F f, StaticContainers &&...static_containers) -> Result {
	return detail::construct_impl<Result>([&](auto I) {
		return apply_to_index<I>(f, std::forward<StaticContainers>(static_containers) ...);
	}, detail::mutual_index_sequence_for<Result, std::remove_reference_t<StaticContainers> ...>{});
}

template<typename F, typename ...StaticContainers>
[[nodiscard]] constexpr auto ranged_all(F f, StaticContainers &&...static_containers) -> bool {
	return detail::all_impl([&](auto I) {
		return apply_to_index<I>(f, std::forward<StaticContainers>(static_containers) ...);
	}, detail::mutual_index_sequence_for<std::remove_reference_t<StaticContainers> ...>{});
}

template<typename F, typename ...StaticContainers>
[[nodiscard]] constexpr auto ranged_any(F f, StaticContainers &&...static_containers) -> bool {
	return detail::any_impl([&](auto I) {
		return apply_to_index<I>(f, std::forward<StaticContainers>(static_containers) ...);
	}, detail::mutual_index_sequence_for<std::remove_reference_t<StaticContainers> ...>{});
}

template<typename F, typename ...StaticContainers>
[[nodiscard]] constexpr auto ranged_find_if(F f, StaticContainers &&...static_containers) -> size_t {
	return detail::find_if_impl([&](auto I) {
		return apply_to_index<I>(f, std::forward<StaticContainers>(static_containers) ...);
	}, detail::mutual_index_sequence_for<std::remove_reference_t<StaticContainers> ...>{});
}

template<size_t N, typename F>
constexpr void indexed_for(F &&f) {
	detail::for_impl(std::forward<F>(f), std::make_index_sequence<N>{});
}

template<typename Result, typename F>
[[nodiscard]] constexpr auto indexed_construct(F &&f) -> Result {
	return detail::construct_impl<Result>(std::forward<F>(f), detail::mutual_index_sequence_for<Result>{});
}

template<size_t N, typename F>
[[nodiscard]] constexpr auto indexed_all(F &&f) -> bool {
	return detail::all_impl(std::forward<F>(f), std::make_index_sequence<N>{});
}

template<size_t N, typename F>
[[nodiscard]] constexpr auto indexed_any(F &&f) -> bool {
	return detail::any_impl(std::forward<F>(f), std::make_index_sequence<N>{});
}

template<size_t N, typename F>
[[nodiscard]] constexpr auto indexed_find_if(F &&f) -> size_t {
	return detail::find_if_impl(std::forward<F>(f), std::make_index_sequence<N>{});
}


template<typename StaticContainerLeft, typename StaticContainerRight>
[[nodiscard]] constexpr auto lexicographically_compare(StaticContainerLeft &&l, StaticContainerRight &&r) -> comparison_result {
	return detail::lexicographical_compare_impl([&](auto I) {
		return apply_to_index<I>([](const auto &li, const auto &ri) {
			return li == ri
				? comparison_result::equal
				: li < ri
					? comparison_result::less
					: comparison_result::greater;
		}, l, r);
	}, detail::mutual_index_sequence_for<
		std::remove_reference_t<StaticContainerRight>,
		std::remove_reference_t<StaticContainerLeft>
	>{});
}

} // namespace static
} //namespace stdc