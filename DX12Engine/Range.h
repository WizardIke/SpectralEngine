#pragma once

template<class Iterator1, class Iterator2 = Iterator1>
class Range
{
	Iterator1 mBegin;
	Iterator2 mEnd;
	template<class NewType, NewType& (*f)(typename std::iterator_traits<Iterator1>::value_type& it)>
	class Mapped : public Iterator1
	{
	public:
		using value_type = NewType;
		using difference_type = typename std::iterator_traits<Iterator1>::difference_type;
		using pointer = typename std::iterator_traits<Iterator1>::pointer;
		using reference = typename std::iterator_traits<Iterator1>::reference;
		using iterator_category = typename std::iterator_traits<Iterator1>::iterator_category;

		Mapped(Iterator1 current) : Iterator1(current) {}
		Mapped(const Mapped& other) = default;

		NewType& operator*()
		{
			return f(**static_cast<Iterator1*>(this));
		}

		NewType* operator->()
		{
			return &f(**static_cast<Iterator1*>(this));
		}

		bool operator!=(const Iterator2& other)
		{
			return *((Iterator1*)this) != other;
		}

		bool operator==(const Iterator2& other)
		{
			return *((Iterator1*)this) == other;
		}
	};
public:
	Range(Iterator1 begin, Iterator2 end) : mBegin(std::move(begin)), mEnd(std::move(end)) {}
	template<class T>
	Range(T& t) : mBegin(std::begin(t)), mEnd(std::end(t)) {}
	Iterator1 begin() { return mBegin; }
	Iterator2 end() { return mEnd; }

	bool empty()
	{
		return mBegin == mEnd;
	}

	template<class NewType, NewType&(*f)(typename std::iterator_traits<Iterator1>::value_type& it)>
	Range<Mapped<NewType, f>, Iterator2> map()
	{
		return { mBegin, mEnd };
	}
};

template<class Iterator1, class Iterator2 = Iterator1>
Range<Iterator1, Iterator2> range(Iterator1 begin, Iterator2 end) { return Range<Iterator1, Iterator2>(begin, end); }