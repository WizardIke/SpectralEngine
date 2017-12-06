#pragma once
#include <iostream>
#include "Array.h"
#include "DynamicArray.h"
#include <array>
#include <stdio.h>

class Integer
{
	int i;
public:
	Integer(int i) : i(i) {}

	operator int() { return i; }
};

void arrayTest(ArrayPointer<int> a)
{
	for (const auto &b : a) std::cout << b << '\n';
	a.reverse();
}

int main()
{
	/*
	Array<long long, 497> a([](size_t i) {return i * i; });
	for (const auto b : a)
	{
		std::cout << b << '\n';
	}
	Array<int, 5> a2 = { {1, 4, 3, 6, 5 } };
	for (const auto b : a2)
	{
		std::cout << b << '\n';
	}
	Array<int, 3> a3 = { { 1, 1, 2 } };
	for (const auto b : a3) std::cout << b << '\n';

	std::array<int, 3> a4{ 12 , 1, 4};
	for (const auto b : a4) std::cout << b << '\n';

	Array<Integer, 4> a5([](size_t i) {return Integer(static_cast<int>(i + i)); });
	for (auto b : a5) std::cout << (int)b << '\n';

	std::array<Integer, 4> a6 = {0, 2, 4, 6};
	Array<Integer, 4> a9 = { { 0, 2, 4, 6} };
	Array<float, 497> a10;
	for (auto b : a9) std::cout << (int)b << '\n';
	std::cout << '\n' << std::endl;
	arrayTest(a3);
	std::cout << '\n';
	for (const auto& b : a3) std::cout << b << '\n';
	*/
	//Array<long long, 100000> a12([](size_t i) {return i * i; });
	//for (const auto b : a12) std::cout << b << '\n';
	/*
	Array<int, 10> a13;
	for (const auto b : a13) std::cout << b << '\n';

	std::cout << '\n';
	Array<int, 10> a14{ {0} };
	for (const auto b : a14) std::cout << b << '\n';

	DynamicArray<int> d1(ArraySize(2));
	std::cout << '\n';
	for (const auto b : d1) std::cout << b << '\n';

	DynamicArray<int> d2(ArraySize(20), [](std::size_t i) {return (int)i + 2; });
	std::cout << '\n';
	for (const auto b : d2) std::cout << b << '\n';

	DynamicArray<int> d3(ArraySize(20), DynamicArray<int>::doNotInitialize);
	std::cout << '\n';
	for (const auto b : d3) std::cout << b << '\n';

	std::cout << '\n';
	Array<int, 10> a15{ Array<int, 10>::doNotInitialize };
	for (const auto b : a15) std::cout << b << '\n';

	DynamicArray<int> d4(ArraySize(20), { 1, 5, 7, 2 });
	std::cout << '\n';
	for (const auto b : d4) std::cout << b << '\n';
	*/
	Array<long long, 1000> a12([](size_t i) {return i * i; });
	for (auto b : a12) std::cout << b << '\n';

	Array<float, 1000> a13([](size_t i, float& element) {element = (float)(i * i); });
	for (auto b : a13) std::cout << b << '\n';
}