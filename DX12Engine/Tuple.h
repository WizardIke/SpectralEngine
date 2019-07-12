#pragma once
#include <cstddef> //std::size_t
#include <type_traits> //std::integral_constant

/* 
 * Similar to std::tuple but is an aggregate
 */
template<class... Ts>
struct Tuple {};

template<class T, class T2, class... Ts>
struct Tuple<T, T2, Ts...>
{
	T head;
	Tuple<T2, Ts...> tail;
};

template<class T>
struct Tuple<T>
{
	T head;
};

template<class... Ts>
Tuple(Ts...)->Tuple<Ts...>;

template<std::size_t i, class T>
struct tuple_element {};

template<std::size_t i, class T, class... Ts>
struct tuple_element<i, Tuple<T, Ts...>>
{
	using type = typename tuple_element<i - 1u, Tuple<Ts...>>::type;
};

template<class T, class... Ts>
struct tuple_element<0u, Tuple<T, Ts...>>
{
	using type = T;
};

template<std::size_t i, class T>
struct tuple_element<i, const T> {
	using type = const typename tuple_element<i, T>::type;
};

template<std::size_t i, class T>
struct tuple_element<i, volatile T> {
	using type = volatile typename tuple_element<i, T>::type;
};

template<std::size_t i, class T>
struct tuple_element<i, const volatile T> {
	using type = const volatile typename tuple_element<i, T>::type;
};

template <std::size_t i, class T>
using tuple_element_t = typename tuple_element<i, T>::type;

template<std::size_t i, class... Ts>
constexpr typename tuple_element<i, Tuple<Ts...> >::type& get(Tuple<Ts...>& t) noexcept
{
	if constexpr (i == 0u)
	{
		return t.head;
	}
	else
	{
		return get<i - 1u>(t.tail);
	}
}

template<std::size_t i, class... Ts>
constexpr typename tuple_element<i, Tuple<Ts...>>::type&& get(Tuple<Ts...>&& t) noexcept
{
	if constexpr (i == 0u)
	{
		return std::move(t.head);
	}
	else
	{
		return get<i - 1u>(std::move(t.tail));
	}
}

template<std::size_t i, class... Ts>
constexpr typename tuple_element<i, Tuple<Ts...>>::type const& get(const Tuple<Ts...>& t) noexcept
{
	if constexpr (i == 0u)
	{
		return t.head;
	}
	else
	{
		return get<i - 1u>(t.tail);
	}
}

template<std::size_t i, class... Ts>
constexpr typename tuple_element<i, Tuple<Ts...>>::type const&& get(const Tuple<Ts...>&& t) noexcept
{
	if constexpr (i == 0u)
	{
		return std::move(t.head);
	}
	else
	{
		return get<i - 1u>(std::move(t.tail));
	}
}

template<class T>
class tuple_size;
template<class... Ts>
class tuple_size<Tuple<Ts...>> : public std::integral_constant<std::size_t, sizeof...(Ts)> { };

template<class T>
class tuple_size<const T> : public std::integral_constant<std::size_t, tuple_size<T>::value> { };

template<class T>
class tuple_size<volatile T> : public std::integral_constant<std::size_t, tuple_size<T>::value> { };

template<class T>
class tuple_size<const volatile T> : public std::integral_constant<std::size_t, tuple_size<T>::value> { };

template<class T>
inline constexpr std::size_t tuple_size_v = tuple_size<T>::value;

template<class... Ts>
constexpr Tuple<Ts&&...> forward_as_tuple(Ts&&... args) noexcept
{
	return Tuple<Ts&&...>{std::forward<Ts>(args)...};
}