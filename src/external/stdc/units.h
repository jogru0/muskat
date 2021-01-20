#pragma once

#include <climits>

#include "hasher.h"
#include "mathematics.h"

namespace stdc {

	template<int _s, int _kg, int _m, int _domain_specific>
	class Unit {
	private:
		using This = Unit<_s, _kg, _m, _domain_specific>;
		
		constexpr Unit(double a_value, int) : value{a_value} {}
	
		double value;
	public:
		static constexpr auto s = _s;
		static constexpr auto kg = _kg;
		static constexpr auto m = _m;
		static constexpr auto domain_specific = _domain_specific;

		constexpr Unit() : Unit{0., 0} {} //NOLINT

		explicit constexpr Unit(double a_value) //NOLINT
			: Unit{a_value, 0}
		{
			static_assert(s == 0);
			static_assert(kg == 0);
			static_assert(m == 0);
			static_assert(domain_specific == 0);
		}

		[[nodiscard]] static constexpr auto detail_make_with_value_in_base_unit(double a_value) {
			return This{a_value, 0};
		}

		[[nodiscard]] constexpr auto detail_get_value_in_base_unit() const {
			return value;
		}

		[[nodiscard]] constexpr auto get_value_in_unit(This unit) const -> double {
			return value / unit.value;
		}

		constexpr auto &operator+=(This other) {
			value += other.value;
			return *this;
		}
		
		constexpr auto &operator-=(This other) {
			value -= other.value;
			return *this;
		}
		
		constexpr auto &operator*=(double scalar) {
			value *= scalar;
			return *this;
		}

		[[nodiscard]] constexpr explicit operator bool() const noexcept {
			return static_cast<bool>(value);
		}
	};

	template<int s, int kg, int m, int domain_specific>
	[[nodiscard]] constexpr auto is_positive(Unit<s, kg, m, domain_specific> unit) -> bool {
		return unit.detail_get_value_in_base_unit() > 0.;
	}

	template<int s, int kg, int m, int domain_specific>
	[[nodiscard]] constexpr auto is_exactly_zero(Unit<s, kg, m, domain_specific> unit) -> bool {
		return unit.detail_get_value_in_base_unit() == 0.;
	}

	template<int s, int kg, int m, int domain_specific>
	[[nodiscard]] constexpr auto is_non_negative(Unit<s, kg, m, domain_specific> unit) -> bool {
		return is_positive(unit) || is_exactly_zero(unit);
	}

	[[nodiscard]] inline constexpr auto as_factor(Unit<0, 0, 0, 0> unit) -> double {
		return unit.detail_get_value_in_base_unit();
	}
	
	
	template<int s, int kg, int m, int domain_specific>
	[[nodiscard]] constexpr auto operator*(
		double factor,
		Unit<s, kg, m, domain_specific> unit
	) {
		return Unit<s, kg, m, domain_specific>::detail_make_with_value_in_base_unit(factor * unit.detail_get_value_in_base_unit());
	}

	template<
		int s_l, int kg_l, int m_l, int domain_specific_l,
		int s_r, int kg_r, int m_r, int domain_specific_r	
	>
	[[nodiscard]] constexpr auto operator*(
		Unit<s_l, kg_l, m_l, domain_specific_l> l,
		Unit<s_r, kg_r, m_r, domain_specific_r> r
	) {
		return Unit<s_l + s_r, kg_l + kg_r, m_l + m_r, domain_specific_l + domain_specific_r>::detail_make_with_value_in_base_unit(
			l.detail_get_value_in_base_unit() * r.detail_get_value_in_base_unit()
		);
	}
	
	template<
		int s_l, int kg_l, int m_l, int domain_specific_l,
		int s_r, int kg_r, int m_r, int domain_specific_r	
	>
	[[nodiscard]] constexpr auto operator/(
		Unit<s_l, kg_l, m_l, domain_specific_l> l,
		Unit<s_r, kg_r, m_r, domain_specific_r> r
	) {
		return Unit<s_l - s_r, kg_l - kg_r, m_l - m_r, domain_specific_l - domain_specific_r>::detail_make_with_value_in_base_unit(
			l.detail_get_value_in_base_unit() / r.detail_get_value_in_base_unit()
		);
	}

	template<int s, int kg, int m, int domain_specific>
	[[nodiscard]] constexpr auto operator+(
		Unit<s, kg, m, domain_specific> l,
		Unit<s, kg, m, domain_specific> r
	) {
		return Unit<s, kg, m, domain_specific>::detail_make_with_value_in_base_unit(
			l.detail_get_value_in_base_unit() + r.detail_get_value_in_base_unit()
		);
	}

	template<int s, int kg, int m, int domain_specific>
	[[nodiscard]] constexpr auto operator-(
		Unit<s, kg, m, domain_specific> l,
		Unit<s, kg, m, domain_specific> r
	) {
		return Unit<s, kg, m, domain_specific>::detail_make_with_value_in_base_unit(
			l.detail_get_value_in_base_unit() - r.detail_get_value_in_base_unit()
		);
	}

	template<int s, int kg, int m, int domain_specific>
	[[nodiscard]] constexpr auto operator==(
		Unit<s, kg, m, domain_specific> l,
		Unit<s, kg, m, domain_specific> r
	) -> bool {
		return l.detail_get_value_in_base_unit() == r.detail_get_value_in_base_unit();
	}

	template<int s, int kg, int m, int domain_specific>
	[[nodiscard]] constexpr auto operator!=(
		Unit<s, kg, m, domain_specific> l,
		Unit<s, kg, m, domain_specific> r
	) -> bool {
		return !(l == r);
	}

	template<int s, int kg, int m, int domain_specific>
	[[nodiscard]] constexpr auto operator<(
		Unit<s, kg, m, domain_specific> l,
		Unit<s, kg, m, domain_specific> r
	) -> bool {
		return l.detail_get_value_in_base_unit() < r.detail_get_value_in_base_unit();
	}


	template<int s, int kg, int m, int domain_specific>
	[[nodiscard]] constexpr auto operator>(
		Unit<s, kg, m, domain_specific> l,
		Unit<s, kg, m, domain_specific> r
	) -> bool {
		return !(l < r) && l != r;
	}

	template<int s, int kg, int m, int domain_specific>
	[[nodiscard]] constexpr auto operator<=(
		Unit<s, kg, m, domain_specific> l,
		Unit<s, kg, m, domain_specific> r
	) -> bool {
		return !(l > r);
	}
	
	template<int s, int kg, int m, int domain_specific>
	[[nodiscard]] constexpr auto operator>=(
		Unit<s, kg, m, domain_specific> l,
		Unit<s, kg, m, domain_specific> r
	) -> bool {
		return !(l < r);
	}

	template<int s, int kg, int m, int domain_specific>
	[[nodiscard]] constexpr auto operator-(Unit<s, kg, m, domain_specific> arg)
		-> Unit<s, kg, m, domain_specific>
	{
		return Unit<s, kg, m, domain_specific>::detail_make_with_value_in_base_unit(-arg.detail_get_value_in_base_unit());
	}
	
	namespace units {
		using Time = Unit<1, 0, 0, 0>;
		using Mass = Unit<0, 1, 0, 0>;
		using Length = Unit<0, 0, 1, 0>;
		using DomainSpecific = Unit<0, 0, 0, 1>;
		using Dimensionless = Unit<0, 0, 0, 0>;

		template<typename ...Units>
		using ProductUnit = Unit<(... + Units::s), (... + Units::kg), (... + Units::m), (... + Units::domain_specific)>;

		template<typename BaseUnit, size_t Power>
		using PowerUnit = Unit<Power * BaseUnit::s, Power * BaseUnit::kg, Power * BaseUnit::m, Power * BaseUnit::domain_specific>;

		template<typename Numerator, typename Denominator>
		using QuotientUnit = Unit<Numerator::s - Denominator::s, Numerator::kg - Denominator::kg, Numerator::m - Denominator::m, Numerator::domain_specific - Denominator::domain_specific>;
		
		template<typename Unit>
		using InverseUnit = QuotientUnit<Dimensionless, Unit>;


		inline constexpr auto s = Time::detail_make_with_value_in_base_unit(1);
		inline constexpr auto kg = Mass::detail_make_with_value_in_base_unit(1);
		inline constexpr auto m = Length::detail_make_with_value_in_base_unit(1);
		inline constexpr auto domain_specific_base_unit = DomainSpecific::detail_make_with_value_in_base_unit(1);
		inline constexpr auto one =  Dimensionless::detail_make_with_value_in_base_unit(1);

		inline constexpr auto Kilo = 1000.;
		inline constexpr auto Milli = stdc::reciprocal(Kilo);
		inline constexpr auto Mega = Kilo * Kilo;
		inline constexpr auto Micro = Milli * Milli;
		

		inline constexpr auto min = 60 * s;
		inline constexpr auto mm = Milli * m;
		inline constexpr auto um = Micro * m;
		inline constexpr auto g = Milli * kg;
		static_assert(kg == Kilo * g);

		inline constexpr auto N = kg * m / (s * s);
		inline constexpr auto Pa = N / (m * m);

		using Area = PowerUnit<Length, 2>;
		using Volume = PowerUnit<Length, 3>;
		
		static_assert(std::is_same_v<Area, ProductUnit<Length, Length>>);
		static_assert(std::is_same_v<Volume, ProductUnit<Length, Area>>);

		inline constexpr auto mm2 = mm * mm;
		inline constexpr auto mm3 = mm * mm2;

		static_assert(std::is_same_v<Area, std::remove_const_t<decltype(mm2)>>);
		static_assert(std::is_same_v<Volume, std::remove_const_t<decltype(mm3)>>);

		using Frequency = InverseUnit<Time>;
		using Speed = QuotientUnit<Length, Time>;
		using Acceleration = QuotientUnit<Speed, Time>;
		using Force = ProductUnit<Mass, Acceleration>;
		using Pressure = QuotientUnit<Force, Area>;
		using Energy = ProductUnit<Force, Length>;
		using Torque = Energy;
		using Power = QuotientUnit<Energy, Time>;


		inline constexpr auto J = N * m;
		inline constexpr auto W = J / s;

		inline constexpr auto MPa = Mega * Pa;
		static_assert(std::is_same_v<std::remove_const_t<decltype(m)>, Length>);
		static_assert(std::is_same_v<std::remove_const_t<decltype(mm)>, Length>);
		static_assert(std::is_same_v<std::remove_const_t<decltype(N)>, Force>);
		static_assert(std::is_same_v<std::remove_const_t<decltype(Pa)>, Pressure>);
		static_assert(std::is_same_v<std::remove_const_t<decltype(MPa)>, Pressure>);
	} //namespace units

	template<int s, int kg, int m, int domain_specific>
	[[nodiscard]] constexpr auto atan2(Unit<s, kg, m, domain_specific> y, Unit<s, kg, m, domain_specific> x) {
		return stdc::atan2(y.detail_get_value_in_base_unit(), x.detail_get_value_in_base_unit());
	}

	namespace detail {
		template<int s, int kg, int m, int domain_specific>
		struct sqrt_er<Unit<s, kg, m, domain_specific>> {
			[[nodiscard]] constexpr auto operator()(Unit<s, kg, m, domain_specific> unit) const {
				static_assert(s % 2 == 0);
				static_assert(kg % 2 == 0);
				static_assert(m % 2 == 0);
				static_assert(domain_specific % 2 == 0);
				return Unit<s / 2, kg / 2, m / 2, domain_specific / 2>::detail_make_with_value_in_base_unit(stdc::sqrt(unit.detail_get_value_in_base_unit()));
			}	
		};
		
		template<int s, int kg, int m, int domain_specific>
		struct abs_er<Unit<s, kg, m, domain_specific>> {
			[[nodiscard]] constexpr auto operator()(Unit<s, kg, m, domain_specific> unit) const {
				return Unit<s, kg, m, domain_specific>::detail_make_with_value_in_base_unit(stdc::abs(unit.detail_get_value_in_base_unit()));
			}	
		};

		template<int s, int kg, int m, int domain_specific>
		struct isnan_er<Unit<s, kg, m, domain_specific>> {
			[[nodiscard]] constexpr auto operator()(Unit<s, kg, m, domain_specific> unit) const {
				return stdc::isnan(unit.detail_get_value_in_base_unit());
			}
		};
		
		template<int s, int kg, int m, int domain_specific>
		struct isinf_er<Unit<s, kg, m, domain_specific>> {
			[[nodiscard]] constexpr auto operator()(Unit<s, kg, m, domain_specific> unit) const {
				return stdc::isinf(unit.detail_get_value_in_base_unit());
			}
		};
	} //namespace detail

	//Output mainly for debug/unit tests
	template<int s, int kg, int m, int domain_specific>
	auto operator<<(std::ostream &output, Unit<s, kg, m, domain_specific> unit) -> std::ostream & {
		output << unit.detail_get_value_in_base_unit();
		if constexpr (s != 0 || kg != 0 || m != 0 || domain_specific != 0) {
			output << ' ';
			if constexpr (s != 0) {
				output << 's';
				if constexpr (s != 1) {
					output << s;
				}
			}
			if constexpr (kg != 0) {
				output << "kg";
				if constexpr (kg != 1) {
					output << kg;
				}
			}
			if constexpr (m != 0) {
				output << 'm';
				if constexpr (m != 1) {
					output << m;
				}
			}
			if constexpr (domain_specific != 0) {
				output << 'x';
				if constexpr (domain_specific != 1) {
					output << domain_specific;
				}
			}
		}

		return output;
	}

} //namespace stdc

namespace std {
	template<int s, int kg, int m, int domain_specific>
	struct hash<stdc::Unit<s, kg, m, domain_specific>> {
		auto operator()(const stdc::Unit<s, kg, m, domain_specific> &unit) const -> size_t {
			return stdc::GeneralHasher{}(unit.detail_get_value_in_base_unit());
		}
	};

	template<int s, int kg, int m, int domain_specific>
	struct numeric_limits<stdc::Unit<s, kg, m, domain_specific>>{
		static constexpr bool is_specialized = numeric_limits<double>::is_specialized;
		static constexpr bool is_signed = numeric_limits<double>::is_signed;
		static constexpr bool is_integer = numeric_limits<double>::is_integer;
		static constexpr bool is_exact = numeric_limits<double>::is_exact;
		static constexpr bool has_infinity = numeric_limits<double>::has_infinity;
		static constexpr bool has_quiet_NaN = numeric_limits<double>::has_quiet_NaN;
		static constexpr bool has_signaling_NaN = numeric_limits<double>::has_signaling_NaN;
		static constexpr auto has_denorm = numeric_limits<double>::has_denorm;
		static constexpr bool has_denorm_loss = numeric_limits<double>::has_denorm_loss;
		static constexpr auto round_style = numeric_limits<double>::round_style;
		static constexpr bool is_iec559 = numeric_limits<double>::is_iec559;
		static constexpr bool is_bounded = numeric_limits<double>::is_bounded;
		static constexpr bool is_modulo = numeric_limits<double>::is_modulo;
		static constexpr int digits = numeric_limits<double>::digits;
		static constexpr int digits10 = numeric_limits<double>::digits10;
		static constexpr int max_digits10 = numeric_limits<double>::max_digits10;
		static constexpr int radix = numeric_limits<double>::radix;
		static constexpr int min_exponent = numeric_limits<double>::min_exponent;
		static constexpr int min_exponent10 = numeric_limits<double>::min_exponent10;
		static constexpr int max_exponent = numeric_limits<double>::max_exponent;
		static constexpr int max_exponent10 = numeric_limits<double>::max_exponent10;
		static constexpr bool traps = numeric_limits<double>::traps;
		static constexpr bool tinyness_before = numeric_limits<double>::tinyness_before;
		
		[[nodiscard]] static constexpr auto min() noexcept -> stdc::Unit<s, kg, m, domain_specific> {
			return stdc::Unit<s, kg, m, domain_specific>::detail_make_with_value_in_base_unit(numeric_limits<double>::min());
		}
		[[nodiscard]] static constexpr auto lowest() noexcept -> stdc::Unit<s, kg, m, domain_specific> {
			return stdc::Unit<s, kg, m, domain_specific>::detail_make_with_value_in_base_unit(numeric_limits<double>::lowest());
		}
		[[nodiscard]] static constexpr auto max() noexcept -> stdc::Unit<s, kg, m, domain_specific> {
			return stdc::Unit<s, kg, m, domain_specific>::detail_make_with_value_in_base_unit(numeric_limits<double>::max());
		}
		[[nodiscard]] static constexpr auto epsilon() noexcept -> stdc::Unit<s, kg, m, domain_specific> {
			return stdc::Unit<s, kg, m, domain_specific>::detail_make_with_value_in_base_unit(numeric_limits<double>::epsilon());
		}
		[[nodiscard]] static constexpr auto round_error() noexcept -> stdc::Unit<s, kg, m, domain_specific> {
			return stdc::Unit<s, kg, m, domain_specific>::detail_make_with_value_in_base_unit(numeric_limits<double>::round_error());
		}
		[[nodiscard]] static constexpr auto infinity() noexcept -> stdc::Unit<s, kg, m, domain_specific> {
			return stdc::Unit<s, kg, m, domain_specific>::detail_make_with_value_in_base_unit(numeric_limits<double>::infinity());
		}
		[[nodiscard]] static constexpr auto quiet_NaN() noexcept -> stdc::Unit<s, kg, m, domain_specific> {
			return stdc::Unit<s, kg, m, domain_specific>::detail_make_with_value_in_base_unit(numeric_limits<double>::quiet_NaN());
		}
		[[nodiscard]] static constexpr auto signaling_NaN() noexcept -> stdc::Unit<s, kg, m, domain_specific> {
			return stdc::Unit<s, kg, m, domain_specific>::detail_make_with_value_in_base_unit(numeric_limits<double>::signaling_NaN());
		}
		[[nodiscard]] static constexpr auto denorm_min() noexcept -> stdc::Unit<s, kg, m, domain_specific> {
			return stdc::Unit<s, kg, m, domain_specific>::detail_make_with_value_in_base_unit(numeric_limits<double>::denorm_min());
		}
	};

	template<int s, int kg, int m, int domain_specific>
	struct numeric_limits<const stdc::Unit<s, kg, m, domain_specific>>
		: numeric_limits<stdc::Unit<s, kg, m, domain_specific>>
	{};
	template<int s, int kg, int m, int domain_specific>
	struct numeric_limits<volatile stdc::Unit<s, kg, m, domain_specific>>
		: numeric_limits<stdc::Unit<s, kg, m, domain_specific>>
	{};
	template<int s, int kg, int m, int domain_specific>
	struct numeric_limits<const volatile stdc::Unit<s, kg, m, domain_specific>>
		: numeric_limits<stdc::Unit<s, kg, m, domain_specific>>
	{};
} //namespace std