#pragma once

#include <array>
#include <cassert>
#include <functional>
#include <limits>
#include <iostream>
#include <random>
#include <type_traits>

#include "algorithm.h"
#include "metaprogramming.h"
#include "type_traits.h"
#include "iterator.h"

namespace stdc {

	#define std_to_stdc(NAME) 																	\
	namespace detail {																			\
		template<typename T>																	\
		struct NAME##_er {																		\
			[[nodiscard]] constexpr auto operator()(T value) const {							\
				return std::NAME(value);														\
			}																					\
		};																						\
	}																							\
																								\
	template<typename Field>																	\
	[[nodiscard]] constexpr auto NAME(Field &&value) {											\
		return detail::NAME##_er<std::remove_cv_t<std::remove_reference_t<Field>>>{}(			\
			std::forward<Field>(value)															\
		);																						\
	}																							\
	
	std_to_stdc(abs)
	std_to_stdc(sqrt)
	std_to_stdc(isnan)
	std_to_stdc(isinf)

	#undef std_to_stdc

	[[nodiscard]] inline auto atan2(double y, double x) {
		return std::atan2(y, x);
	}

	template<typename Field, typename = std::enable_if_t<std::numeric_limits<Field>::has_infinity>>
	inline constexpr auto inf = std::numeric_limits<Field>::infinity();

	inline constexpr auto truth_values = std::array{true, false};

	template<typename Field>
	[[nodiscard]] inline constexpr auto reciprocal(Field denominator) noexcept -> Field {
		static_assert(!std::is_integral_v<Field>);
		assert(denominator);
		auto result = static_cast<Field>(1) / denominator;
		assert(result);
		return result;
	}

	template<typename T>
	[[nodiscard]] constexpr decltype(auto) cot(T &&x) noexcept {
		return stdc::reciprocal(tan(std::forward<T>(x)));
	}
	
	template<typename T>
	[[nodiscard]] constexpr decltype(auto) sec(T &&x) noexcept {
		return stdc::reciprocal(cos(std::forward<T>(x)));
	}
	
	template<typename T>
	[[nodiscard]] constexpr decltype(auto) csc(T &&x) noexcept {
		return stdc::reciprocal(sin(std::forward<T>(x)));
	}

	#define COOL_PI 3.1415926535897932384626433832795028841972L

	template<typename T>
	inline constexpr auto pi = static_cast<T>(COOL_PI);

	template<typename T>
	inline constexpr auto tau = static_cast<T>(2 * COOL_PI);

	#undef COOL_PI

	template<typename Integer>
	[[nodiscard]] constexpr auto factorial(Integer n) {
		static_assert(std::is_integral_v<Integer>);
		if constexpr (std::is_signed_v<Integer>) {
			assert(0 <= n);
		}

		auto result = static_cast<Integer>(1);
		while (n > 0) {
			result *= n;
			--n;
		}
		return result;
	}

	template<typename T>
	[[nodiscard]] constexpr auto sgn(T t) {
		return (T{0} < t) - (t < T{0});
	}


	template<typename Field>
	[[nodiscard]] constexpr auto changing_negativity(Field a, Field b) noexcept -> size_t {
		return (a < 0) != (b < 0);
	}

	template<typename Integer>
	[[nodiscard]] inline constexpr auto swapped_bit_halves(Integer i) noexcept -> Integer {
		constexpr size_t Half_Bit_Size = 4 * sizeof(Integer);
		return i << Half_Bit_Size | i >> Half_Bit_Size;
	}

	//Works for negative numbers.
	template<typename Integer>
	[[nodiscard]] constexpr auto is_odd(Integer i) noexcept -> bool {
		return static_cast<bool>(i % 2);
	}

	template<typename Integer>
	[[nodiscard]] constexpr auto is_even(Integer i) noexcept -> bool {
		return !is_odd(i);
	}

	template<typename UInteger>
	[[nodiscard]] constexpr auto non_neg_mod(
		intmax_t i,
		UInteger input_n
	) noexcept
		-> uintmax_t
	{
		static_assert(std::is_unsigned_v<UInteger>);
		//this might be necessary due weird interactions like https://stackoverflow.com/a/39337256
		const auto n = static_cast<intmax_t>(input_n);
		auto result = (i % n + n) % n;
		assert(0 <= result);
		return static_cast<uintmax_t>(result);
	}

	template<typename T, typename ...Ts>
	constexpr void minimize(
		T &to_minimize,
		const Ts &... others
	) noexcept {
		static_assert(stdc::are_same_v<T, std::decay_t<Ts> ...>);
		((to_minimize = std::min(to_minimize, others)), ...);
	}

	template<typename T, typename ...Ts>
	void maximize(
		T &to_maximize,
		const Ts &...others
	) noexcept {
		static_assert(stdc::are_same_v<T, std::decay_t<Ts> ...>);
		((to_maximize = std::max(to_maximize, others)), ...);
	}

	template<typename It>
	[[nodiscard]] constexpr auto supremum(It begin, It end) {
		using Field = typename std::iterator_traits<It>::reference;
		auto sup = -stdc::inf<Field>;
		while(begin != end) {
			stdc::maximize(sup, *begin);
			++begin;
		}
		return sup;
	}

	template<typename It, typename Proj>
	[[nodiscard]] constexpr auto supremum(It begin, It end, Proj proj) {
		return supremum(transform_iterator(begin, proj), transform_iterator(end, proj));
	}

	template<typename It>
	[[nodiscard]] constexpr auto infimum(It begin, It end) {
		using Field = typename std::iterator_traits<It>::reference;
		auto inf = stdc::inf<Field>;
		while(begin != end) {
			stdc::minimize(inf, *begin);
			++begin;
		}
		return inf;
	}

	template<typename It, typename Proj>
	[[nodiscard]] constexpr auto infimum(It begin, It end, Proj proj) {
		return infimum(transform_iterator(begin, proj), transform_iterator(end, proj));
	}

	template<typename ...Ts>
	[[nodiscard]] constexpr auto average(
		const Ts &...values
	) noexcept
	{
		static_assert(stdc::are_same_v<Ts ...>);
		constexpr auto factor = stdc::reciprocal(static_cast<double>(sizeof...(Ts)));
		return factor * (... + values);
	}

	struct DeterministicSourceOfRandomness {
		unsigned int value = 0;
		[[nodiscard]] auto operator()() {
			value += 77'777'777;
			return value;
		}
	};

	template<typename RNG = std::mt19937, typename SourceOfRandomness = std::random_device>
	[[nodiscard]] auto seeded_RNG(SourceOfRandomness source = SourceOfRandomness{})
		-> RNG
	{
		constexpr auto N = RNG::state_size;

		auto random_data = std::array<typename RNG::result_type, N>{};
		std::generate_n(random_data.begin(), N, std::ref(source));
		
		auto seeds = std::seed_seq(random_data.begin(), random_data.end());
		return RNG{seeds};
	}

	template<typename RNG = std::mt19937>
	[[nodiscard]] auto warn_for_true_random_seeded_RNG()
		-> RNG
	{
		std::cout << "WARNING: program not deterministic!\n";
		return seeded_RNG<RNG>();
	}

	template<typename RNG = std::mt19937>
	[[nodiscard]] auto warn_for_true_random_seeded_RNG(typename RNG::result_type seed)
		-> RNG
	{
		return RNG{seed};
	}

	template<typename T>
	[[nodiscard]] constexpr auto is_strict_monotone(const T &l, const T &m, const T &r) -> bool {
		return (l < m && m < r) || (r < m && m < l);
	}
	
	template<typename T>
	[[nodiscard]] constexpr auto is_monotone(const T &l, const T &m, const T &r) -> bool {
		return (l <= m && m <= r) || (r <= m && m <= l);
	}

	namespace detail {
		template<typename Field>
		[[nodiscard]] constexpr auto get_epsilon(size_t prec) {
			auto result = Field{1};
			while (prec --> 0) {
				result /= 10;
			}
			return result;
		}
	} //namespace detail

	template<typename Field, size_t Prec,
		typename = std::enable_if_t<std::is_floating_point_v<Field>>
	>
	inline constexpr auto Epsilon = detail::get_epsilon<Field>(Prec);

	//TODO!!!: Die beiden Epsilons sind aber sehr grob. --jgr/19/08
	template<typename Field>
	inline constexpr auto Epsilon_Parallel = Epsilon<Field, 3>;

	template<typename Field>
	inline constexpr auto Epsilon_Orthogonal = Epsilon<Field, 3>;

	//TODO: Which Epsilon makes sense? --jgr/19/05
	template<typename Field>
	inline constexpr auto Epsilon_Normalizable = Epsilon<Field, 14>;
} //namespace stdc