#pragma once

#include <cstddef>
#include <tuple> //Just for std::tuple_size...
#include <type_traits>

#include "pack.h"

// ==== Macros to quickly define new type traits.

#define DEFINE_TYPE_TRAIT_V(NAME)							\
	template<typename ...Args>								\
	inline constexpr auto NAME##_v = NAME<Args ...>::value;	\

#define DEFINE_TYPE_TRAIT_T(NAME)							\
	template<typename ...Args>								\
	using NAME##_t = typename NAME<Args ...>::type;			\

#define DEFINE_IS_PRETTYTYPE_TRAIT(PRETTYTYPE, TYPE)		\
	template<typename T>									\
	struct is_##PRETTYTYPE									\
		: std::is_base_of<TYPE, std::remove_cv_t<T>> {};	\
	DEFINE_TYPE_TRAIT_V(is_##PRETTYTYPE)					\

namespace stdc {

	template<typename>								
	struct dependent_false
		: std::false_type {};
	DEFINE_TYPE_TRAIT_V(dependent_false)

	template<typename>								
	struct dependent_true
		: std::true_type {};
	DEFINE_TYPE_TRAIT_V(dependent_true)		

	template<typename ...Args>
	struct is_any_of : std::false_type {};

	template<typename T, typename FirstOption, typename ...OtherOptions>
	struct is_any_of<T, FirstOption, OtherOptions ...> : std::disjunction<
		std::is_same<T, FirstOption>,
		stdc::is_any_of<T, OtherOptions ...>
	> {};
	DEFINE_TYPE_TRAIT_V(is_any_of)

	template<typename ...Args>
	struct is_argument_for_implicit_copy_or_move_constructor : std::false_type {};
	
	template<typename Type, typename Single_Argument>
	struct is_argument_for_implicit_copy_or_move_constructor<Type, Single_Argument> : std::is_base_of<Type, std::decay_t<Single_Argument>> {};
	DEFINE_TYPE_TRAIT_V(is_argument_for_implicit_copy_or_move_constructor)

	template<typename ...Args>
	struct is_argument_for_implicit_constructor : is_argument_for_implicit_copy_or_move_constructor<Args ...> {};
	
	template<typename Type>
	struct is_argument_for_implicit_constructor<Type> : std::true_type {};
	DEFINE_TYPE_TRAIT_V(is_argument_for_implicit_constructor)

	template<typename T, typename = void>
	struct is_class_enum : std::false_type {};

	template<typename T>
	struct is_class_enum<T, std::enable_if_t<std::is_enum_v<T>>> : std::negation<std::is_convertible<T, std::underlying_type_t<T>>> {};
	DEFINE_TYPE_TRAIT_V(is_class_enum)

	template<typename ...Args> 
	struct are_same : std::false_type {};

	template<typename Head, typename ...Tail> 
	struct are_same<Head, Head, Tail ...> : stdc::are_same<Head, Tail ...> {};
	
	template<typename Single_Type> 
	struct are_same<Single_Type> : std::true_type {};

	template<> 
	struct are_same<> : std::true_type {};
	DEFINE_TYPE_TRAIT_V(are_same)

	template<typename ...Args> 
	struct are_unique : std::false_type {};

	template<typename Head, typename ...Tail> 
	struct are_unique<Head, Tail ...> :
		std::conjunction<
			std::negation<stdc::is_any_of<Head, Tail ...>>,
			stdc::are_unique<Tail ...>
		>
	{};
	
	template<> 
	struct are_unique<> : std::true_type {};
	DEFINE_TYPE_TRAIT_V(are_unique)

	template<typename Arg, typename ...Args>
	struct homogenous_type : std::enable_if<stdc::are_same_v<Arg, Args ...>, Arg> {};
	DEFINE_TYPE_TRAIT_T(homogenous_type)

	template<typename T, typename = void>
	struct is_iterator : std::false_type {};

	template<typename T>
	struct is_iterator<T, typename std::void_t<typename std::iterator_traits<T>::value_type>> : std::true_type {};
	DEFINE_TYPE_TRAIT_V(is_iterator)

	template<typename T, typename = void>
	struct is_container : std::false_type {};
		
	template<typename T>
	struct is_container<T, std::void_t<typename T::value_type>> : std::bool_constant<!is_iterator_v<T>> {};
	DEFINE_TYPE_TRAIT_V(is_container)

	template<typename T, typename = void>
	struct is_static_container : std::false_type {};

	template<typename T>
	struct is_static_container<T, std::void_t<decltype(std::tuple_size<T>::value)>> : std::true_type{};
	DEFINE_TYPE_TRAIT_V(is_static_container)

	template<typename T>
	struct is_stringy : std::disjunction<
		std::is_same<T, std::string>,
		std::is_same<T, const char *>
	> {};
	DEFINE_TYPE_TRAIT_V(is_stringy)

	#define CREATE_HAS_METHOD_TYPE_TRAIT(METHOD) 									\
		CREATE_IS_VALID_EXPRESSION_FOR_T_TYPE_TRAIT(&T::METHOD, has_method_##METHOD)\

	#define CREATE_IS_VALID_EXPRESSION_FOR_T_TYPE_TRAIT(EXPR, TRAIT_NAME)		 	\
		template<typename T, typename = void>										\
		struct TRAIT_NAME : std::false_type {};										\
																					\
		template<typename T>														\
		struct TRAIT_NAME<T, std::void_t<decltype(EXPR)>> : std::true_type {};		\
		DEFINE_TYPE_TRAIT_V(TRAIT_NAME)												\

	
	template<typename Impl, template<typename> typename Interface>
	struct complies_to_interface : std::is_base_of<Interface<Impl>, Impl> {};
	template<typename Impl, template<typename> typename Interface>
	inline constexpr bool complies_to_interface_v = complies_to_interface<Impl, Interface>::value;

	namespace detail {
		template<typename ...Args>
		struct Pack {};

		template<typename Type, typename Pack, typename = void>
		struct is_embraceable_impl : std::false_type {};

		template<typename Type, typename ...Args>
		struct is_embraceable_impl<Type, Pack<Args ...>,
			std::void_t<decltype(Type{std::declval<Args>() ...})>
		> : std::true_type {};
	} //namespace detail
	
	template<typename Type, typename ...Args>
	struct is_embraceable : detail::is_embraceable_impl<Type, detail::Pack<Args ...>> {};
	DEFINE_TYPE_TRAIT_V(is_embraceable)

} //namespace stdc