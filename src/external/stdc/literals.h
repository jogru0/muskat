#pragma once

#include <string>

namespace stdc {
	inline namespace literals {
		constexpr size_t operator""_z(unsigned long long int id) { return id; } //NOLINT
		
		//pragma magic to work around MSVC bug https://developercommunity.visualstudio.com/content/problem/270349/warning-c4455-issued-when-using-standardized-liter.html
		#if defined(_MSC_VER)
			#pragma warning(push)
			#pragma warning(disable : 4455)
		#endif
		using std::string_literals::operator""s;
		using std::string_view_literals::operator""sv;
		#if defined(_MSC_VER)
			#pragma warning(pop) 
		#endif

	} //namespace literals
} //namespace stdc




