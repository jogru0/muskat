#pragma once

#include <utility>

#include "iterator.h"
#include "metaprogramming.h"

namespace stdc {
	template<typename Begin_It, typename End_It>
	class Range final {
	private:
		Begin_It begin_it;
		End_It end_it;
	
	public:
		constexpr Range(Begin_It _begin_it, End_It _end_it) : begin_it{std::move(_begin_it)}, end_it{std::move(_end_it)} {}

		[[nodiscard]] constexpr auto begin() const noexcept -> Begin_It {
			return begin_it;
		}

		[[nodiscard]] constexpr auto end() const noexcept -> End_It {
			return end_it;
		}
	};



} //namespace stdc

