#pragma once

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <utility>

namespace stdc {
	// recursive case
	template<size_t I, typename Head, typename ...Tail>
	struct pack_element : pack_element<I - 1, Tail ...> {};
	 
	// base case
	template<typename Head, typename ...Tail>
	struct pack_element<0, Head, Tail ...> {
		using type = Head;
	};

	template <size_t I, typename ...Args>
	using pack_element_t = typename pack_element<I, Args ...>::type;

	template<size_t I, typename Head, typename ...Tail>
	[[nodiscard]] constexpr auto pick(Head &&head, Tail &&...tail) noexcept
		-> pack_element_t<I, Head, Tail ...> {

		if constexpr (I == 0) {
			return std::forward<Head>(head);
		} else { //NOLINT
			return pick<I - 1>(std::forward<Tail &&>(tail) ...);
		}
	}

	template<typename F, typename ...Args> 
	constexpr void static_for(F &&f, Args &&...args) {
		(f(std::forward<Args>(args)), ...);
	}

	namespace detail {
		template<intmax_t, typename F> 
		constexpr void static_for_with_index_impl(F &&) {}

		template<intmax_t I, typename F, typename Head, typename ...Tail> 
		constexpr void static_for_with_index_impl(F &&f, Head &&head, Tail &&...tail) {
			f(std::forward<Head>(head), std::integral_constant<uintmax_t, static_cast<uintmax_t>(I)>{});
			static_for_with_index_impl<I + 1>(std::forward<F>(f), std::forward<Tail>(tail) ...);
		}			
	} //namespace detail

	template<typename F, typename ...Args> 
	constexpr void static_for_with_index(F &&f, Args &&...args) {
		detail::static_for_with_index_impl<0>(std::forward<F>(f), std::forward<Args>(args) ...);
	}	

	template<typename Tuple, typename Func>
	[[nodiscard]] constexpr decltype(auto) tuple_transform(Tuple &&tuple, Func &&func) {
		auto helper = [&](auto &&...args) {
			return std::tuple{func(std::forward<decltype(args)>(args)) ...};
		};
		
		return std::apply(helper, std::forward<Tuple>(tuple));
	}

	// Convert array into a tuple
	template<typename Array, std::size_t... I>
	auto a2t_impl(const Array& a, std::index_sequence<I...>)
	{
		return std::make_tuple(a[I]...);
	}
	
	template<typename T, std::size_t N, typename Indices = std::make_index_sequence<N>>
	auto a2t(const std::array<T, N>& a)
	{
		return a2t_impl(a, Indices{});
	}
} //namespace stdc