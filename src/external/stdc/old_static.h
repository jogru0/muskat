#pragma once

#include <functional>
#include <optional>
#include <utility>

#include "literals.h"
#include "get.h"

namespace stdc {
namespace legacy {

namespace detail {
template<size_t I, size_t J, typename Static_Container, typename Compare>
[[nodiscard]] constexpr auto swap_if_necessary(Static_Container &c, Compare comp) noexcept -> bool {
	static_assert(I < std::tuple_size<Static_Container>::value);
	static_assert(J < std::tuple_size<Static_Container>::value);
	static_assert(I < J);
	
	if (comp(get<J>(c), get<I>(c))) {
		std::swap(get<I>(c), get<J>(c));
		return true;
	}
	return false;
}
} //namespace detail


//The [[maybe_unused]] is for Size == 1.
template<typename Static_Container, typename Compare = std::less<>>
[[nodiscard]] constexpr auto static_sort_and_get_oriented([[maybe_unused]] Static_Container &c, [[maybe_unused]] Compare comp = Compare{}) noexcept -> bool {
	
	constexpr auto Size = std::tuple_size<Static_Container>::value;
	static_assert(Size < 4, "Not implemented for size bigger than 3.");
	static_assert(Size != 0, "Not sure if orientation is meaningful for size 0.");
	if constexpr (Size == 1) {
		return true;
	} else { //NOLINT
		auto oriented = !detail::swap_if_necessary<0, 1>(c, comp);
		if constexpr (Size == 3) {
			if (detail::swap_if_necessary<1, 2>(c, comp)) {
				return oriented == detail::swap_if_necessary<0, 1>(c, comp);
			}
		}
		return oriented;
	}
}

} //namespace legacy
} //namespace stdc