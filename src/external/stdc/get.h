#pragma once

#include "type_traits.h"

namespace stdc {
	template<class T, typename = std::enable_if_t<dependent_false_v<T>>>
	void get(int) = delete;
} //namespace stdc