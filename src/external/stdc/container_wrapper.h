#pragma once

#include "get.h"
#include "type_traits.h"

#include <array>

namespace stdc {

template<typename Array>
class ExposedArray {
private :
	using This = ExposedArray<Array>;

public:
	using base_container = Array;

	using value_type = typename base_container::value_type;
	using reference = typename base_container::reference;
	using const_reference = typename base_container::const_reference;
	using size_type = typename base_container::size_type;

	//For get interface.
	static_assert(std::is_same_v<size_t, size_type>);

protected:
	base_container m_exposed_array;

public:
	[[nodiscard]] constexpr auto operator[](size_type index) const -> const_reference {
		return m_exposed_array[index];
	}
	[[nodiscard]] constexpr auto operator[](size_type index) -> reference {
		return m_exposed_array[index];
	}

//Protected special member functions.
protected:
	//Only allow moving the container in to prevent misuse.
	explicit constexpr ExposedArray(base_container &&exposed_array) :
		m_exposed_array{std::move(exposed_array)}
	{}

	~ExposedArray() noexcept = default;
	constexpr ExposedArray(const This &) = default;
	constexpr auto operator=(const This &) & -> ExposedArray & = default;
	constexpr ExposedArray(This &&) noexcept = default;
	constexpr auto operator=(This &&) & noexcept -> ExposedArray & = default;

public:
	template<size_type I>
	[[nodiscard]] constexpr auto &get() & {
		using stdc::get;
		return get<I>(m_exposed_array);
	}

	template<size_type I>
	[[nodiscard]] constexpr const auto &get() const & {
		using stdc::get;
		return get<I>(m_exposed_array);
	}

	template<size_type I>
	[[nodiscard]] constexpr auto &&get() && {
		using stdc::get;
		return get<I>(std::move(m_exposed_array));
	}

	template<size_type I>
	[[nodiscard]] constexpr const auto &&get() const && {
		using stdc::get;
		return get<I>(std::move(m_exposed_array));
	}

};

template<size_t I, typename Array>
[[nodiscard]] constexpr const auto &get(const ExposedArray<Array> &exposed_array) noexcept {
	return exposed_array.template get<I>();
}

template<size_t I, typename Array>
[[nodiscard]] constexpr auto &get(ExposedArray<Array> &exposed_array) noexcept {
	return exposed_array.template get<I>();
}

template<size_t I, typename Array>
[[nodiscard]] constexpr const auto &&get(const ExposedArray<Array> &&exposed_array) noexcept {
	return std::move(exposed_array).template get<I>();
}

template<size_t I, typename Array>
[[nodiscard]] constexpr auto &&get(ExposedArray<Array> &&exposed_array) noexcept {
	return std::move(exposed_array).template get<I>();
}

} //namespace stdc

namespace std {
template<typename Array>
struct tuple_size<stdc::ExposedArray<Array>> : public tuple_size<Array> {};

template<size_t I, typename Array>
struct tuple_element<I, stdc::ExposedArray<Array>> : public tuple_element<I, Array> {};
} //namespace std

namespace stdc {

template<typename Container>
class ConstExposedContainer {
private :
	using This = ConstExposedContainer<Container>;

public:
	using base_container = Container;

	using value_type = typename base_container::value_type;
	using const_reference = typename base_container::const_reference;
	using const_iterator = typename base_container::const_iterator;
	using size_type = typename base_container::size_type;

protected:
	base_container m_const_exposed_container;

public:
	[[nodiscard]] constexpr auto operator[](size_type index) const -> const_reference {
		return m_const_exposed_container[index];
	}

	[[nodiscard]] constexpr auto begin() const -> const_iterator { return m_const_exposed_container.begin(); }
	[[nodiscard]] constexpr auto end() const -> const_iterator { return m_const_exposed_container.end(); }
	[[nodiscard]] constexpr auto cbegin() const -> const_iterator { return m_const_exposed_container.cbegin(); }
	[[nodiscard]] constexpr auto cend() const -> const_iterator { return m_const_exposed_container.cend(); }

	[[nodiscard]] constexpr auto size() const -> size_type { return m_const_exposed_container.size(); }
	[[nodiscard]] constexpr auto empty() const -> bool { return m_const_exposed_container.empty(); }

//Protected special member functions.
protected:
	//Only allow moving the container in to prevent misuse.
	explicit constexpr ConstExposedContainer(base_container &&const_exposed_container) :
		m_const_exposed_container{std::move(const_exposed_container)}
	{}

	~ConstExposedContainer() noexcept = default;
	constexpr ConstExposedContainer(const This &) = default;
	constexpr auto operator=(const This &) & -> ConstExposedContainer & = default;
	constexpr ConstExposedContainer(This &&) noexcept = default;
	constexpr auto operator=(This &&) & noexcept -> ConstExposedContainer & = default;
};

} //namespace stdc
