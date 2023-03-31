#pragma once

#include <array>

template<class Function, std::size_t... Indices>
constexpr auto make_array_helper(Function f, std::index_sequence<Indices...>)
->std::array<decltype(f(std::size_t(0))), sizeof...(Indices)>
{
	return { { f(Indices)... } };
}

template<int N, class Function>
constexpr auto make_array(Function f)
->std::array<decltype(f(std::size_t(0))), N>
{
	return make_array_helper(f, std::make_index_sequence<N>{});
}
