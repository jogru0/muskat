#pragma once

#include <algorithm>
#include <functional>
#include <unordered_set>
#include <vector>

#include "hasher.h"
#include "literals.h"
#include "type_traits.h"
#include "utility.h"
#include "iterator.h"

namespace stdc {
	namespace detail {
		template<typename InputIt, typename Func>
		constexpr decltype(auto) cyclic_helper(
			InputIt from, InputIt to,
			InputIt begin, InputIt end,
			Func &&func
		) {
			if (0 <= std::distance(from, to)) {
				return std::invoke(std::forward<Func>(func), from, to);
			}
			
			std::invoke(func, from, end);
			return std::invoke(std::forward<Func>(func), begin, to);
		}
	}

	template<typename InputIt, typename OutputIt>
	constexpr auto copy_cyclic(InputIt from, InputIt to, InputIt begin, InputIt end, OutputIt result) -> OutputIt {
		detail::cyclic_helper(from, to, begin, end, [&](auto a, auto b) {
			result = std::copy(a, b, result);
		});
		return result;
	}
	
	template<typename RAI>
	constexpr auto unsigned_distance_cyclic(RAI from, RAI to, RAI begin, RAI end) -> size_t {
		auto result = 0_z;
		detail::cyclic_helper(from, to, begin, end, [&](auto a, auto b) {
			result += unsigned_distance(a, b);
		});
		return result;
	}
} //namespace stdc
