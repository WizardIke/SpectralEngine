#pragma once
#include <tuple>
#include <utility>

template<class F, class...Ts, std::size_t...Is>
void for_each(std::tuple<Ts...> & tuple, F func, std::index_sequence<Is...>) {
	using expander = int[];
	(void)expander {
		0, ((void)func(std::get<Is>(tuple)), 0)...
	};
}

template<class F, class...Ts>
void for_each(std::tuple<Ts...> & tuple, F func) {
	for_each(tuple, func, std::make_index_sequence<sizeof...(Ts)>());
}