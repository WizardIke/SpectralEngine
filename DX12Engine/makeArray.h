#pragma once
#include <array>
#include <cstddef>
#include <utility>

template<class T, std::size_t n, class F, std::size_t... indices>
std::enable_if_t<std::is_same_v<T, typename std::invoke_result_t<F>> || std::is_constructible_v<T, typename std::invoke_result_t<F>>, std::array<T, n>> makeArray(std::index_sequence<indices...>, F&& f)
{
	return { {(static_cast<void>(indices), f())...} };
}

template<class T, std::size_t n, class F>
std::enable_if_t<std::is_same_v<T, typename std::invoke_result_t<F>> || std::is_constructible_v<T, typename std::invoke_result_t<F>>, std::array<T, n>> makeArray(F&& f)
{
	return makeArray<T, n, F>(std::make_index_sequence<n>(), std::forward<F>(f));
}

template<std::size_t n, class F>
std::array<typename std::invoke_result_t<F>, n> makeArray(F&& f)
{
	return makeArray<typename std::invoke_result_t<F>, n, F>(std::make_index_sequence<n>(), std::forward<F>(f));
}

template<class T, std::size_t n, class F, std::size_t... indices>
std::enable_if_t<std::is_same_v<T, typename std::invoke_result_t<F, std::size_t>> || std::is_constructible_v<T, typename std::invoke_result_t<F, std::size_t>>, std::array<T, n>> makeArray(std::index_sequence<indices...>, F&& f)
{
	return { {f(indices)...} };
}

template<class T, std::size_t n, class F>
std::enable_if_t<std::is_same_v<T, typename std::invoke_result_t<F, std::size_t>> || std::is_constructible_v<T, typename std::invoke_result_t<F, std::size_t>>, std::array<T, n>> makeArray(F&& f)
{
	return makeArray<T, n, F>(std::make_index_sequence<n>(), std::forward<F>(f));
}

template<std::size_t n, class F>
std::array<typename std::invoke_result_t<F, std::size_t>, n> makeArray(F&& f)
{
	return makeArray<typename std::invoke_result_t<F, std::size_t>, n, F>(std::make_index_sequence<n>(), std::forward<F>(f));
}