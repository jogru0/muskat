#pragma once

#include <algorithm>
#include <cassert>
#include <optional>
#include <utility>
#include <type_traits>

#include "literals.h"

namespace stdc {
	
	// ==== Utility. ====
	//-1_z triggers warnings
	inline constexpr auto nullid = static_cast<size_t>(-1);

	//Like std::equal_to, but as unary predicate.
	//For now, takes a copy of the element with which it compares,
	//because we use it for small types.
	template<typename T>
	class equal_to {
	private: 
		const T value_construction;
	public:
		explicit constexpr equal_to(T _value_construction) noexcept : value_construction{std::move(_value_construction)} {}

		[[nodiscard]] constexpr auto operator()(const T &value_call) const noexcept -> bool {
			return value_call == value_construction;
		}
	};

	//Like std::not_equal_to, but as unary predicate.
	//For now, takes a copy of the element with which it compares,
	//because we use it for small types.
	template<typename T>
	class not_equal_to {
	private: 
		const T value_construction;
	public:
		explicit constexpr not_equal_to(T _value_construction) noexcept : value_construction{std::move(_value_construction)} {}

		[[nodiscard]] constexpr auto operator()(const T &value_call) const noexcept -> bool {
			return value_call != value_construction;
		}
	};

	//Will be part of C++20.
	struct identity {
		using is_transparent = void;
		template<typename T>
		[[nodiscard]] constexpr auto operator()(T &&t) const noexcept -> T && {
			return std::forward<T>(t);
		}
	};

	template<typename To>
	struct static_cast_to{
		using is_transparent = void;
		template<typename From>
		[[nodiscard]] constexpr auto operator()(From &&val) -> To {
			return static_cast<To>(std::forward<From>(val));
		}
	};

	template<typename T>
	constexpr auto as_mutable(const T &value) noexcept -> T &{
		return const_cast<T &>(value); //NOLINT
	}
	template<typename T>
	constexpr auto as_mutable(const T *value) noexcept -> T *{
		return &as_mutable(*value);
	}
	template<typename T>
	constexpr auto as_mutable(T *value) noexcept -> T *{
		return value;
	}
	template<typename T>
	void as_mutable(const T &&) = delete;

	//fixed bug with VS 16.4, see https://developercommunity.visualstudio.com/content/problem/649879/maybe-unused-for-variadic-template-is-ignored-and.html
	template<typename Head, typename ...Tail>
	[[nodiscard]] constexpr auto any_of_same_sizes(Head &&head, [[maybe_unused]] Tail &&...tail)
		-> decltype(head.size())
	{
		assert(((std::forward<Head>(head).size() == std::forward<Tail>(tail).size()) && ...));

		return std::forward<Head>(head).size();
	}

	#define PROJECT_TO(FIELD_NAME)												\
	[](const auto &to_project) {												\
		return to_project.FIELD_NAME;											\
	}																			\
	
	#define PROJECT_TO_WITH_CAPTURE(FIELD_NAME)									\
	[&](const auto &to_project) {												\
		return to_project.FIELD_NAME;											\
	}																			\


	#define PROJECT_CONTAINER_TO(FIELD_NAME)																						\
	[](const auto &container){																										\
		auto _result = std::vector<decltype(std::declval<typename std::decay_t<decltype(container)>::value_type>().FIELD_NAME)>{};	\
		_result.reserve(container.size());																							\
		std::transform(container.begin(), container.end(), std::back_inserter(_result), PROJECT_TO(FIELD_NAME));					\
		return _result;																												\
	}																																\

	#define CMP_PROJECTION_TO(FIELD_NAME)	\
	[](const auto &i, const auto &j){		\
		auto proj = PROJECT_TO(FIELD_NAME);	\
		return proj(i) < proj(j);			\
	}										\

	//Necessary for overloaded ORIGINAL. Otherwise, no macro required, just use
	//constexpr auto ALIAS = ORIGINAL;
	#define FUNCTION_ALIAS(ALIAS, ORIGINAL)						\
		template <typename ...Args>								\
		auto ALIAS(Args &&...args)								\
			-> decltype(ORIGINAL(std::forward<Args>(args)...))	\
		{														\
			return ORIGINAL(std::forward<Args>(args)...);		\
		}														\

	#define IS_VALID(EXPR, ARG_0)					 			\
		[]() constexpr -> bool {								\
			auto validity_check_lambda =						\
				[](auto _0) constexpr -> decltype(EXPR) {};		\
			return std::is_invocable_v<							\
				decltype(validity_check_lambda),				\
				decltype(ARG_0)									\
			>;													\
		}()														\

	#define USING(SUPER, SUB) using SUB = typename SUPER::SUB


	#define EXPOSE_CONST_ITERATORS(CONTAINER)														\
		using const_iterator = typename decltype(CONTAINER):: const_iterator;								\
		[[nodiscard]] constexpr auto begin() const -> const_iterator { return CONTAINER.begin(); }	\
		[[nodiscard]] constexpr auto end() const -> const_iterator { return CONTAINER.end(); }		\
	
	#define EXPOSE_ITERATORS(CONTAINER)													\
		using iterator = typename decltype(CONTAINER)::iterator;						\
		[[nodiscard]] constexpr auto begin() -> iterator { return CONTAINER.begin(); }	\
		[[nodiscard]] constexpr auto end() -> iterator { return CONTAINER.end(); }		\
		EXPOSE_CONST_ITERATORS(CONTAINER)												\

} //namespace stdc