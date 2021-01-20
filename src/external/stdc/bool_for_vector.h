#pragma once

namespace stdc {
	class bool_for_vector {
	private:
		bool value{};

	public:
		//Explicitely not explicit.
		//Makes it behave like a bool.
		constexpr bool_for_vector(bool _value = bool{}) noexcept : value{_value} {} //NOLINT
		[[nodiscard]] constexpr operator bool &() noexcept { return value; } //NOLINT
		[[nodiscard]] constexpr operator const bool &() const noexcept { return value; } //NOLINT
	};
} //namespace stdc