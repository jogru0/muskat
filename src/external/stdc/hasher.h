#pragma once

#include <cstddef>
#include <numeric>

#include "get.h"
#include "literals.h"
#include "srange.h"
#include "type_traits.h"

namespace stdc {

	//Testing for expression std::hash<T>{} is not enough for the MSVC compiler.
	CREATE_IS_VALID_EXPRESSION_FOR_T_TYPE_TRAIT(std::hash<T>{}(std::declval<T>()), has_specialized_std_hash)

	namespace detail {
		inline constexpr void hash_combine(size_t &seed, size_t new_hash) noexcept {
			constexpr auto max_size_t_on_64_bit_machines = 18446744073709551615U;
			static_assert(
				std::numeric_limits<std::size_t>::max() == max_size_t_on_64_bit_machines,
				"Only implemented for 64 bit hash values."
			);

			constexpr auto factor = 0xc6a4a7935bd1e995_z;
			constexpr auto right_shift = 47U;
			constexpr auto arbitrary_number = 0xe6546b64;

			new_hash *= factor;
			new_hash ^= new_hash >> right_shift;
			new_hash *= factor;

			seed ^= new_hash;
			seed *= factor;
			seed += arbitrary_number;
		}
	} // namespace detail

	//Unpacks packs and containers to arrive at primitive types that are hopefully hashable.
	struct GeneralHasher {
		
		[[nodiscard]] inline constexpr auto operator()() const noexcept -> size_t {
			return 0_z;
		}

		template<typename Head, typename ...Tail>
		[[nodiscard]] constexpr auto operator()(const Head &head, const Tail &...tail) const -> size_t {
			static_assert(sizeof...(Tail));
			auto seed = operator()(head);
			(..., detail::hash_combine(seed, operator()(tail)));
			return seed;
		}

		template<typename T>
		[[nodiscard]] constexpr auto operator()(const T &t) const -> size_t {
			
			//Already std hashable.
			if constexpr (has_specialized_std_hash_v<T>) { //NOLINT
				return std::hash<T>{}(t);
			
			//Static container.
			} else if constexpr (stdc::is_static_container_v<T>) { //NOLINT
				constexpr auto size = std::tuple_size_v<T>;
				if constexpr (!size) {
					return 0_z;
				} else { //NOLINT
					using stdc::get;
					auto seed = operator()(get<0>(t));
					stdc::srange::indexed_for<std::tuple_size_v<T> - 1>([&](auto I) {
						//'this->' to prevent an internal compiler error. -jgr/19/11
						detail::hash_combine(seed, this->operator()(get<I + 1>(t)));
					});
					return seed;
				}

			//Dynamic container.
			} else if constexpr (stdc::is_container_v<T>) {
				auto seed = 0_z; //Want to prevent hash collisions between size 0 and size 1.
				for (const auto &elem : t) {
					detail::hash_combine(seed, operator()(elem));
				}
				return seed;
			
			//Not hashable.
			} else {
				static_assert(stdc::dependent_false_v<T>, "Didn't find a way to hash something of type T.");
			}
		}
	};
} //namespace stdc


