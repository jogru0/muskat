#pragma once

#include <iterator>
#include <optional>
#include <type_traits>

#include "type_traits.h"
#include "utility.h"

namespace stdc {
	namespace detail {
		// ==== STREAM MANIPULATORS ====
		//function types of all stream manipulators
		//according to https://stackoverflow.com/a/1134501

		//e.g. std::endl
		using Stream_Manip_Func_Type_1 = std::add_pointer_t<std::ostream& (std::ostream&)>;

		//using ios_type = std::basic_ios<std::ostream::char_type, std::ostream::traits_type>;
		//using Stream_Manip_Func_Type_2 = std::add_pointer_t<ios_type& (ios_type&)>;

		//using Stream_Manip_Func_Type_3 = std::add_pointer_t<std::ios_base& (std::ios_base&)>;


		// ==== VALUE WRAPPER ====
		//Class to store a temporary named copy of some temporary unnamed value,
		//so we can implement operator-> for it. 
		template<typename T>
		class Value_Wrapper final {
		private:
			const T t;
		public:
			//Only the move constructor is provided, to prevent wrong usage of this class.
			explicit constexpr Value_Wrapper(const T &&_t) noexcept : t(std::move(_t)) {}
			
			auto operator->() const -> const T * {
				return &t;
			}
		};
	} //namespace detail

	class size_t_iterator final {
	private:
		size_t pos;
	public:
		using cheat = void;

		using value_type = size_t;
		using difference_type = ptrdiff_t;
		using iterator_category = std::bidirectional_iterator_tag;
		using pointer = const value_type *;
		using reference = const value_type &;

		//Not constexpr by virtue of the uninitialized pos.
		size_t_iterator() noexcept = default;
		explicit constexpr size_t_iterator(size_t _pos) noexcept : pos{_pos} {}

		constexpr auto operator++() noexcept -> size_t_iterator & {
			++pos;
			return *this;
		}

		[[nodiscard]] constexpr auto operator++(int) noexcept -> size_t_iterator {
			size_t_iterator temp(*this);
			operator++();
			return temp;
		}

		constexpr auto operator--() noexcept -> size_t_iterator & {
			--pos;
			return *this;
		}

		[[nodiscard]] constexpr auto operator--(int) noexcept -> size_t_iterator {
			size_t_iterator temp(*this);
			operator--();
			return temp;
		}

		[[nodiscard]] constexpr auto operator*() const noexcept -> reference { return pos; }
		[[nodiscard]] constexpr auto operator->() const noexcept -> pointer { return &pos; }

		[[nodiscard]] constexpr auto operator==(const size_t_iterator &other) const noexcept -> bool {
			return pos == other.pos;
		}
		[[nodiscard]] constexpr auto operator!=(const size_t_iterator &other) const noexcept -> bool {
			return !(*this == other);
		}
	};

	//I haven't yet implemented all Stream_Manip_Func_Types to see which ones we actually use
	template<typename Stream>
	class optional_stream final {
	private:
		//if Stream does not have operator<<, you'll notice when using this class ...
		std::optional<Stream> it;
	public:
		constexpr optional_stream() noexcept = default;
		explicit constexpr optional_stream(Stream &&_it) noexcept :
			it{std::move(_it)} {}

		template<typename T>
		auto operator<<(T &&value) noexcept -> optional_stream<Stream> & {
			if (it) {
				//TODO: this exists for LOG. Create one more layer such that try-catch is not in optional stream? --mka/19/07
				//ignore streaming if it throws
				try { (*it) << std::forward<T>(value); }
				catch (...) {}
			}
			return *this;
		}

		auto operator<<(detail::Stream_Manip_Func_Type_1 fp) noexcept -> optional_stream<Stream> & {
			if (it) { (*it) << fp; }
			return *this;
		}
	};

	//TODO! transform iterator <= --jgr/20/06
	template<typename It, typename Func>
	class transform_iterator final {
	private:
		using This = transform_iterator<It, Func>;
		using wrapped_iterator_tag = typename std::iterator_traits<It>::iterator_category;
		static_assert(std::is_base_of_v<std::input_iterator_tag, wrapped_iterator_tag>);
		It it;
		Func func;
	public:
		using reference = std::invoke_result_t<const Func, typename std::iterator_traits<It>::reference>;
		using value_type =  std::remove_cv_t<std::remove_reference_t<reference>>;
	private:
		static constexpr auto need_value_wrapper = !std::is_lvalue_reference_v<reference>;
		
		//If we don't need a value wrapper, then reference meets the requirements of a forward iterator.
		static_assert(IMPLIES(!need_value_wrapper, (stdc::is_any_of_v<reference, value_type &, const value_type &>)));
		static constexpr auto can_be_a_forward_iterator = !need_value_wrapper && std::is_base_of_v<std::forward_iterator_tag, wrapped_iterator_tag>;
	public:	
		using difference_type = typename std::iterator_traits<It>::difference_type;
		using iterator_category = std::conditional_t<
			can_be_a_forward_iterator,
			std::forward_iterator_tag,
			std::input_iterator_tag
		>;

		using pointer = std::conditional_t<
			need_value_wrapper,
 			detail::Value_Wrapper<value_type>,
			value_type *
		>;
	
		constexpr transform_iterator() noexcept = default;
		constexpr transform_iterator(It _it, Func _func) noexcept
			: it{_it}, func{_func} {}

		constexpr auto operator++() -> transform_iterator & {
			++it;
			return *this;
		}

		[[nodiscard]] constexpr auto operator++(int) -> transform_iterator {
			transform_iterator temp(*this);
			operator++();
			return temp;
		}

		[[nodiscard]] constexpr auto operator*() const -> reference { return func(*it); }
		[[nodiscard]] constexpr auto operator->() const {
			if constexpr (need_value_wrapper) {
				static_assert(std::is_same_v<pointer, decltype(detail::Value_Wrapper{**this})>);
				return detail::Value_Wrapper{**this};
			} else {
				static_assert(std::is_same_v<pointer, &**this>);
				return &**this;
			}
		}

		[[nodiscard]] constexpr auto operator==(const transform_iterator &other) const -> bool {
			return it == other.it;
		}
		[[nodiscard]] constexpr auto operator!=(const transform_iterator &other) const -> bool {
			return !(*this == other);
		}
	};

	template<
		typename T,
		typename charT = char,
		typename traits = std::char_traits<charT>
	>
	class infix_ostream_iterator final {
	public:
		using value_type = void;
		using difference_type = void;
		using iterator_category = std::output_iterator_tag;
		using pointer = void;
		using reference = void;

	private:
		std::basic_ostream<charT, traits> *os;
		charT const* delimiter;
		bool first_elem;
	public:
		using char_type = charT;
		using traits_type = traits;
		using ostream_type = std::basic_ostream<charT, traits>;
		explicit infix_ostream_iterator(ostream_type& s)
			: os(&s), delimiter(0), first_elem(true) {
		}
		infix_ostream_iterator(ostream_type& s, charT const *d)
			: os(&s), delimiter(d), first_elem(true) {
		}
		auto operator=(T const &item) -> infix_ostream_iterator<T, charT, traits>& {
			if (!first_elem && delimiter != nullptr) {
				*os << delimiter;
			}
			*os << item;
			first_elem = false;
			return *this;
		}
		auto operator*() -> infix_ostream_iterator<T, charT, traits> & {
			return *this;
		}
		auto operator++() -> infix_ostream_iterator<T, charT, traits> & {
			return *this;
		}

		auto operator++(int) -> infix_ostream_iterator<T, charT, traits> {
			return *this;
		}
	};
} //namespace stdc