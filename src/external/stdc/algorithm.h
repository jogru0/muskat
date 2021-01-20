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
template<typename Container>
constexpr void append(const Container &) {}

template<typename Container, typename ItHead, typename ...ItTail>
constexpr void append(Container &container, ItHead begin, ItHead end, ItTail ...it_tail) {
	std::copy(begin, end, std::back_inserter(container));
	append(container, it_tail ...);
}

template<class OutputIt, typename Size, typename T>
constexpr void iota_n(OutputIt first, Size count, T start_value) {
	for (Size i = 0; i < count; ++i) {
		*first = start_value;
		++first;
		++start_value;
	}
}

template<typename InputIt, typename T, typename BinaryOp, typename UnaryOp>
[[nodiscard]] constexpr auto transform_accumulate(InputIt first, InputIt last, T init, BinaryOp binary_op, UnaryOp unary_op) -> T {
	while (first != last) {
		init = std::invoke(binary_op, std::move(init), std::invoke(unary_op, *first));
		++first;
	}
	return init;
}

template<typename InputIt, typename T, typename UnaryOp>
[[nodiscard]] constexpr auto transform_accumulate(InputIt first, InputIt last, T &&init, UnaryOp &&unary_op) -> T {
	return transform_accumulate(first, last, std::forward<T>(init), std::plus{}, std::forward<UnaryOp>(unary_op));
}

template<typename InputIt, typename UnaryOp>
[[nodiscard]] constexpr auto transform_accumulate(InputIt first, InputIt last, UnaryOp &&unary_op) {
	using T = std::remove_reference_t<std::invoke_result_t<UnaryOp, typename std::iterator_traits<InputIt>::reference>>;
	return transform_accumulate(first, last, T{}, std::forward<UnaryOp>(unary_op));
}

template<typename OutputIt, typename Generator>
constexpr void generate_dependent(OutputIt first, OutputIt last, Generator g) {
	auto i = 0_z;
	while (first != last) {
		*first = std::invoke(g, i);
		++first;
		++i;
	}
}

template<typename OutputIt, typename Size, typename Generator>
constexpr auto generate_n_dependent(OutputIt first, Size count, Generator g) -> OutputIt {
	for (auto i = Size{}; i < count; ++i) {
		*first = std::invoke(g, i);
		++first;
	}
	return first;
}

template<typename It>
[[nodiscard]] constexpr auto find_first_duplicate(It first, It last) -> It {
	auto size = std::distance(first, last);
	assert(0 <= size);
	auto set = std::unordered_set<typename std::iterator_traits<It>::value_type, GeneralHasher>{};
	set.reserve(static_cast<size_t>(size));
	for (; first != last; ++first) {
		if (
			auto [_, was_inserted] = set.insert(*first);
			!was_inserted
		) {
			break;
		}
	}
	return first;
}

template<typename It>
[[nodiscard]] constexpr auto contains_duplicates(It first, It last) -> bool {
	return find_first_duplicate(first, last) != last;
}

namespace detail {
template<typename It, typename ParallelIt, typename UnaryPredicate>
[[nodiscard]] constexpr auto remove_if_in_parallel_range(It first, It last, ParallelIt parallel_first, UnaryPredicate pred) -> It {
	auto parallel_last = get_parallel_it(first, last, parallel_first);

	auto parallel_it = std::find_if(parallel_first, parallel_last, pred);
	auto it = get_parallel_it(parallel_first, parallel_it, first);
	
	auto new_end = it;
	while (it != last) {
		if (!std::invoke(pred, *parallel_it)) {
			*new_end = std::move(*it);
			++new_end;
		}
		++it;
		++parallel_it;
	}
	return new_end;
}
} //namespace detail

template<typename T, typename Alloc, typename InputIt2, typename UnaryPredicate>
auto erase_if_in_parallel_range(std::vector<T, Alloc>& c, InputIt2 first2, UnaryPredicate &&pred) {
	auto it = stdc::detail::remove_if_in_parallel_range(c.begin(), c.end(), first2, std::forward<UnaryPredicate>(pred));
	auto r = std::distance(it, c.end());
	c.erase(it, c.end());
	return r;
}

template<typename It, typename ParallelIt, typename OutputIt, typename UnaryPredicate>
constexpr auto copy_if_in_parallel_range(It first, It last, ParallelIt parallel_first, OutputIt d_first, UnaryPredicate pred) -> OutputIt {
	while (first != last) {
		if (std::invoke(pred, *parallel_first)) {
			*d_first = *first;
			++d_first;
		}
		++first;
		++parallel_first;
	}
	return d_first;
}

//unnecessary with C++20
template<typename T, typename Alloc, typename U>
auto erase(std::vector<T, Alloc> &c, const U &value) {
	auto it = std::remove(c.begin(), c.end(), value);
	auto r = std::distance(it, c.end());
	c.erase(it, c.end());
	return r;
}

//unnecessary with C++20
template<typename T, typename Alloc, typename Pred>
auto erase_if(std::vector<T, Alloc> &c, Pred &&pred) {
	auto it = std::remove_if(c.begin(), c.end(), std::forward<Pred>(pred));
	auto r = std::distance(it, c.end());
	c.erase(it, c.end());
	return r;
}

//unnecessary with C++20
template<typename Key, typename Hash, typename KeyEqual, typename Alloc, typename Pred>
auto erase_if(std::unordered_set<Key,Hash,KeyEqual,Alloc> &c, Pred pred) {
	auto old_size = c.size();
	
	auto it = c.begin();
	auto last = c.end(); //not invalidated by erase
	while (it != last) {
		if (std::invoke(pred, *it)) {
			it = c.erase(it);
		} else {
			++it;
		}
	}
	
	return old_size - c.size();
}

//still necessary with C++20 and faster, see 680c91ef1a91a0495608115d414088817adab9d8
//pos needs to be valid and dereferencable (not c.end())
//no const_iterator due to swap
//no return since the positions after the erase are implementation defined anyway
template<typename T, typename Alloc>
constexpr void unstable_erase(std::vector<T, Alloc>& c, typename std::vector<T, Alloc>::iterator pos) {
	using std::swap;
	swap(*pos, c.back());
	c.erase(c.end() - 1);
}

//not implemented since way more sophisticated, unnecessary slow, see 96a4155a85896811ed06cb1e7270f9a611520ef6
//template<class T, class Alloc, class Pred>
//void unstable_erase_if(std::vector<T, Alloc> &c, Pred pred);

template<typename InputIt, typename T>
[[nodiscard]] constexpr auto contains(InputIt first, InputIt last, const T &value) -> bool {
	return std::find(first, last, value) != last;
};

template<typename It>
[[nodiscard]] constexpr auto has_adjacent_duplicates(It first, It last) -> bool {
	return std::adjacent_find(first, last) != last;
}

template<typename It, typename Pred>
[[nodiscard]] constexpr auto adjacent_all_of(It first, It last, Pred &&pred) -> bool {
	return std::adjacent_find(first, last, std::not_fn(std::forward<Pred>(pred))) == last;
}

template<typename It>
[[nodiscard]] constexpr auto is_sorted_and_unique(It first, It last) -> bool {
	return adjacent_all_of(first, last, std::less{});
}	

template<typename Container>
constexpr auto erase_adjacent_duplicates(Container &c) {
	auto it = std::unique(RANGE(c));
	auto r = std::distance(it, c.end());
	c.erase(it, c.end());
	return r;
}

template<typename Container>
constexpr auto sort_and_make_unique(Container &c) {
	std::sort(RANGE(c));
	return erase_adjacent_duplicates(c);
}	

template<typename InputIt, typename Size, typename OutputIt >
constexpr auto move_n(InputIt first, Size count, OutputIt result) -> OutputIt {
	return std::copy_n(std::make_move_iterator(first), count, result);
}

template<typename InputIt, typename OutputIt, typename UnaryPredicate>
constexpr auto move_if(InputIt first, InputIt last, OutputIt d_first, UnaryPredicate &&pred) -> OutputIt {
	return std::copy_if(
		std::make_move_iterator(first),
		std::make_move_iterator(last),
		d_first,
		std::forward<UnaryPredicate>(pred)
	);
}

template<typename InputIt1, typename InputIt2, typename OutputIt, typename UnaryPredicate>
constexpr auto move_if_in_parallel_range(InputIt1 first1, InputIt1 last1, InputIt2 first2, OutputIt d_first, UnaryPredicate pred) -> OutputIt {
	return stdc::copy_if_in_parallel_range(
		std::make_move_iterator(first1),
		std::make_move_iterator(last1),
		first2,
		d_first,
		std::forward<UnaryPredicate>(pred));
}

//No don't return what std::transform returns, since that would be last anyway.
template<typename It, typename Func>
constexpr void transform(It first, It last, Func &&func) {
	std::transform(first, last, first, std::forward<Func>(func));
}

namespace detail {
template<typename It, typename Func>
constexpr auto transformed_vector(It first, It last, Func &&func, size_t size_to_reserve) {
	using value_type = std::remove_reference_t<std::invoke_result_t<
		Func,
		typename std::iterator_traits<It>::reference
	>>;
	
	auto result = std::vector<value_type>{};
	result.reserve(size_to_reserve);
	std::transform(first, last, std::back_inserter(result), std::forward<Func>(func));
	
	assert(result.size() == size_to_reserve);
	return result;
}

template<typename It, typename ParallelIt, typename Func>
constexpr auto transformed_vector(It first, It last, ParallelIt parallel_first, Func &&func, size_t size_to_reserve) {
	using value_type = std::remove_reference_t<std::invoke_result_t<
		Func,
		typename std::iterator_traits<It>::reference,
		typename std::iterator_traits<ParallelIt>::reference
	>>;
	
	auto result = std::vector<value_type>{};
	result.reserve(size_to_reserve);
	std::transform(first, last, parallel_first, std::back_inserter(result), std::forward<Func>(func));
	
	assert(result.size() == size_to_reserve);
	return result;
}

template<typename It>
[[nodiscard]] constexpr auto get_size_of_random_access_range(It first, It last) -> size_t {
	static_assert(std::is_base_of_v<
		std::random_access_iterator_tag,
		typename std::iterator_traits<It>::iterator_category
	>);
	auto dist = last - first;
	assert(0 <= dist);
	return static_cast<size_t>(dist);
}
} //namespace detail

template<typename It, typename Func>
constexpr auto transformed_vector(It first, It last, Func &&func) {
	auto size = detail::get_size_of_random_access_range(first, last);
	return detail::transformed_vector(first, last, std::forward<Func>(func), size);
}

template<typename Container, typename Func>
constexpr auto transformed_vector(const Container &container, Func &&func) {
	return detail::transformed_vector(RANGE(container), std::forward<Func>(func), container.size());
}

template<typename It, typename ParallelIt, typename Func>
constexpr auto transformed_vector(It first, It last, ParallelIt parallel_first, Func &&func) {
	auto size = detail::get_size_of_random_access_range(first, last);
	return detail::transformed_vector(first, last, parallel_first, std::forward<Func>(func), size);
}

template<typename Container, typename ParallelIt, typename Func>
constexpr auto transformed_vector(const Container &container, ParallelIt parallel_first, Func &&func) {
	return detail::transformed_vector(RANGE(container), parallel_first, std::forward<Func>(func), container.size());
}
} //namespace stdc
