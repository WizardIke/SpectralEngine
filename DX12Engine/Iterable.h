#pragma once

template<class Iterator>
class Iterable
{
	Iterator mBegin, mEnd;
public:
	Iterable(Iterator begin, Iterator end) : mBegin(std::move(begin)), mEnd(std::move(end)) {}
	template<class T>
	Iterable(T& t) : mBegin(t.begin()), mEnd(t.end()) {}
	Iterator begin() { return mBegin; }
	Iterator end() { return mEnd; }
};