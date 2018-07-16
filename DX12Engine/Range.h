#pragma once
#include <type_traits>

template<class Iterator1, class Iterator2 = Iterator1>
class Range
{
	Iterator1 mBegin;
	Iterator2 mEnd;

	template<class It1, class It2, class NewType, NewType (*f)(decltype(*std::declval<It1>()) it)>
	class Mapped : public It1
	{
		friend class Range<Iterator1, Iterator2>;
		Mapped(It1 current) : It1(current) {}
	public:
		using value_type = typename std::remove_reference<NewType>::type;
		using difference_type = typename std::iterator_traits<It1>::difference_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = typename std::iterator_traits<It1>::iterator_category;

		Mapped() {}
		Mapped(const Mapped& other) = default;

		NewType operator*()
		{
			return f(**static_cast<It1*>(this));
		}

		pointer operator->()
		{
			return &f(**static_cast<It1*>(this));
		}

		bool operator!=(const It2& other)
		{
			return *((It1*)this) != other;
		}

		bool operator!=(const Mapped<It1, It2, NewType, f>& other)
		{
			return *((It1*)this) != (It1&)other;
		}

		bool operator==(const It2& other)
		{
			return *((It1*)this) == other;
		}

		bool operator==(const Mapped<It1, It2, NewType, f>& other)
		{
			return *((It1*)this) == (It1&)other;
		}
	};

	template<class NewType, NewType(*f)(decltype(*std::declval<Iterator1>()) value)>
	using MappedRange = std::conditional_t<std::is_same_v<Iterator1, Iterator2>, Range<Mapped<Iterator1, Iterator2, NewType, f>, Mapped<Iterator2, Iterator1, NewType, f>>, Range<Mapped<Iterator1, Iterator2, NewType, f>, Iterator2>>;
public:
	Range(Iterator1 begin1, Iterator2 end1) : mBegin(std::move(begin1)), mEnd(std::move(end1)) {}
	template<class T>
	Range(T& t) : mBegin(std::begin(t)), mEnd(std::end(t)) {}
	Iterator1 begin() { return mBegin; }
	Iterator2 end() { return mEnd; }

	bool empty()
	{
		return mBegin == mEnd;
	}

	template<class NewType, NewType(*f)(decltype(*std::declval<Iterator1>()) value)>
	MappedRange<NewType, f> map()
	{
		return { mBegin, mEnd };
	}
};

template<class Iterator1, class Iterator2 = Iterator1>
Range<Iterator1, Iterator2> range(Iterator1 begin, Iterator2 end) { return Range<Iterator1, Iterator2>(begin, end); }

template<class T>
auto range(T& container) -> Range<decltype(container.begin()), decltype(container.end())>
{
	return Range<decltype(container.begin()), decltype(container.end())>(container);
}