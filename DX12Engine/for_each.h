#pragma once
#include <cstddef> //std::size_t
#include <utility> //std::make_index_sequence, std::index_sequence

template<class F, class Tuple, std::size_t... indices>
void for_each(Tuple& tuple, F&& func, std::index_sequence<indices...>) {
	using std::get;
	using expander = int[];
	(void)expander {
		0, ((void)func(get<indices>(tuple)), 0)...
	};
}

template<class F, class Tuple>
void for_each(Tuple& tuple, F&& func)
{
	for_each(tuple, std::forward<F>(func), std::make_index_sequence<tuple_size<Tuple>::value>());
}