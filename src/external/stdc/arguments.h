#pragma once

#include <array>
#include <cassert> 
#include <cstring>
#include <iterator>
#include <memory>
#include <string_view>
#include <vector>

#include "stdc/iterator.h"

namespace stdc {

	namespace detail {
		struct StringViewMaker {
			[[nodiscard]] auto operator()(const char *p) const {
				assert(p != nullptr);
				return std::string_view{p};
			}

			[[nodiscard]] auto operator()(std::string_view sv) const {
				return sv;
			}

			[[nodiscard]] auto operator()(const std::string &str) const {
				return std::string_view{str};
			}
		};
		
	} //namespace detail

	class arguments {
	private:
		//INVARIANTS
		//raw_char_pointers.size() + 1 == managed_char_arrays.size()
		//For i < managed_char_arrays.size(): managed_char_arrays[i].get() == raw_char_pointers[i];
		//raw_char_pointers.back() == nullptr
		//const access doesn't allow changing the content of the char[] managed by the object

		//C style args are arrays of char *, so we can't use std::vector<const char *>.
		std::vector<char *> raw_char_pointers;

		//We can't let std::string manage the individual strings, since that would only give us const char *.
		std::vector<std::unique_ptr<char[]>> managed_char_arrays; //NOLINT

	public:
		arguments() : raw_char_pointers(1), managed_char_arrays{} {}

		arguments(int argc, const char *const *argv) : arguments{} {
			assert(0 <= argc);
			assert(argv != nullptr);
			assert(argv[argc] == nullptr);
			push_back_range(argv, argv + argc); //NOLINT
		}

		template<typename StringViewMakerInput>
		void push_back(const StringViewMakerInput &input) {
			auto begin = &input;
			auto end = std::next(begin);
			push_back_range(begin, end);
		}

		[[nodiscard]] auto size() const {
			return managed_char_arrays.size();
		}

	private:
		//Not public, see comment for push_back_range.
		void reserve(size_t capacity) {
			managed_char_arrays.reserve(capacity);
			raw_char_pointers.reserve(capacity + 1);
		}
	public:

		//In contrast to containers like std::vector, calling something like reserve here
		//would not prevent multiple push_backs from creating overhead due to the nullptr
		//at the end of the raw pointers. Thus, we provide push_back_range.
		template<typename Iter>
		void push_back_range(Iter begin, Iter end) {
			static_assert(std::is_base_of_v<
				std::random_access_iterator_tag,
				typename std::iterator_traits<Iter>::iterator_category
			>);
			
			if (begin == end) { return; }

			assert(end - begin > 0);
			reserve(size() + static_cast<size_t>(end - begin));
			
			raw_char_pointers.pop_back();
			assert(raw_char_pointers.size() == managed_char_arrays.size());
			for (; begin != end; ++begin) {
				auto sv = detail::StringViewMaker{}(*begin);
				auto size = sv.size();
				auto new_unique_ptr = std::make_unique<char[]>(size + 1); //NOLINT
				auto* new_c_str = new_unique_ptr.get();
				#if defined(__GNUC__)
					#pragma GCC diagnostic push
					#pragma GCC diagnostic ignored "-Wstringop-truncation" //NOLINT
				#endif
					std::strncpy(new_c_str, sv.data(), size);
				#if defined(__GNUC__)
					#pragma GCC diagnostic pop
				#endif
				new_c_str[size] = '\0'; //NOLINT
	
				managed_char_arrays.push_back(std::move(new_unique_ptr));
				raw_char_pointers.push_back(new_c_str);
			}
			raw_char_pointers.emplace_back();
		}

		//We don't consider this to be constant because the user might change
		//the content of the char[] managed by this.
		[[nodiscard]] auto c_args() {
			//Let's make sure the user can't change which char[] are stored,
			//so we can guarantee each entry refers to its corresponding managed char array.
			return static_cast<char *const *>(raw_char_pointers.data());
		}

		[[nodiscard]] auto empty() const {
			return managed_char_arrays.empty();
		}

		//Const access.
		//By using string_views, we implicitely prevent the user from changing the char[].
		[[nodiscard]] auto begin() const {
			return stdc::transform_iterator{
				raw_char_pointers.begin(),
				detail::StringViewMaker{}
			};
		}
		[[nodiscard]] auto end() const {
			assert(!raw_char_pointers.empty());
			return stdc::transform_iterator{
				std::prev(raw_char_pointers.end()),
				detail::StringViewMaker{}
			};
		}		
		[[nodiscard]] auto operator[](size_t pos) const {
			assert(pos < managed_char_arrays.size());
			return detail::StringViewMaker{}(managed_char_arrays[pos].get());
		}
	};

} //namespace stdc