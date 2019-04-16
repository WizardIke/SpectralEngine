#pragma once
#include <functional>
#include <vector>
#include <cassert>

/*
* Implements a PriorityQueue using a heap.
* top() returns a refernce an item for which Compare()(item, otherItem) returns false for all values of otherItem in the PriorityQueue.
*	This means that Compare can be std::less<T> for a max PriorityQueue and std::greater<T> for a min PriorityQueue.
* top() always returns a refernce to the same item unless an item is added, removed or has its priority changed.
* Compare must provide a strict weak ordering.
*/
template<class T, class Container = std::vector<T>, class Compare = std::less<typename Container::value_type>>
class PriorityQueue
{
	class CompareCleaner : private Compare
	{
	public:
		using Compare::operator();
	};

	class Impl : public CompareCleaner
	{
	public:
		Container data;
	};

	Impl impl;
public:
	using container_type = Container;
	using value_compare = Compare;
	using value_type = typename Container::value_type;
	using size_type = typename Container::size_type;
	using reference = typename Container::reference;
	using const_reference = typename Container::const_reference;
	using iterator = typename Container::iterator;
	using const_iterator = typename Container::const_iterator;
private:
	void moveUp(size_type index)
	{
		using std::swap;
		const size_type parentIndex = (index - (size_type)1u) / (size_type)2u;
		if(index != (size_type)0u)
		{
			if(impl(impl.data[parentIndex], impl.data[index]))
			{
				swap(impl.data[parentIndex], impl.data[index]);
				moveUp(parentIndex);
			}
		}
	}

	void moveDown(size_type index = (size_type)0u)
	{
		using std::swap;
		const size_type child1 = (size_type)2u * index + (size_type)1u;
		const size_type child2 = (size_type)2u * index + (size_type)2u;
		if(child1 < impl.data.size())
		{
			if(child2 < impl.data.size())
			{
				if(impl(impl.data[index], impl.data[child1]) || impl(impl.data[index], impl.data[child2]))
				{
					if(impl(impl.data[child2], impl.data[child1]))
					{
						swap(impl.data[child1], impl.data[index]);
						moveDown(child1);
					}
					else
					{
						swap(impl.data[child2], impl.data[index]);
						moveDown(child2);
					}
				}
			}
			else
			{
				if(impl(impl.data[index], impl.data[child1]))
				{
					swap(impl.data[child1], impl.data[index]);
				}
			}
		}
	}
public:
	PriorityQueue& operator=(const PriorityQueue& other)
	{
		impl.data = other.impl.data;
		return *this;
	}

	PriorityQueue& operator=(PriorityQueue&& other)
	{
		impl.data = std::move(other.impl.data);
		return *this;
	}

	reference top()
	{
		return impl.data[(size_type)0u];
	}

	bool empty()
	{
		return impl.data.empty();
	}

	size_t size()
	{
		return impl.data.size();
	}

	void push(const_reference value)
	{
		static_assert(std::is_same<T, value_type>(), "T must be the same as the typename Container::value_type");
		auto index = impl.data.size();
		impl.data.push_back(value);
		moveUp(index);
	}

	void push(value_type&& value)
	{
		static_assert(std::is_same<T, value_type>(), "T must be the same as the typename Container::value_type");
		auto index = impl.data.size();
		impl.data.push_back(std::move(value));
		moveUp(index);
	}

	template<class... Args>
	void emplace(Args&&... args)
	{
		static_assert(std::is_same<T, value_type>(), "T must be the same as the typename Container::value_type");
		auto index = impl.data.size();
		impl.data.emplace_back(std::forward<Args>(args)...);
		moveUp(index);
	}

	void pop()
	{
		assert(impl.data.size() != (size_type)0u);
		if(impl.data.size() != (size_type)1u)
		{
			impl.data[(size_type)0u] = std::move(*(impl.data.end() - (size_type)1u));
			impl.data.pop_back();
			moveDown((size_type)0u);
		}
		else
		{
			impl.data.pop_back();
		}
	}

	value_type popAndGet()
	{
		assert(impl.data.size() != (size_type)0u);
		value_type ret = std::move(impl.data[(size_type)0u]);
		if(impl.data.size() != (size_type)1u)
		{
			impl.data[(size_type)0u].~value_type();
			new(&impl.data[(size_type)0u]) value_type(std::move(*(impl.data.end() - (size_type)1u)));
			impl.data.pop_back();
			moveDown((size_type)0u);
		}
		else
		{
			impl.data.pop_back();
		}
		return std::move(ret);
	}

	void swap(PriorityQueue& other)
	{
		using std::swap;
		swap(impl.data, other.impl.data);
	}

	iterator begin()
	{
		return impl.data.begin();
	}

	const_iterator begin() const
	{
		return impl.data.begin();
	}

	iterator end()
	{
		return impl.data.end();
	}

	const_iterator end() const
	{
		return impl.data.end();
	}

	void priorityIncreased(reference value)
	{
		auto index = &value - &impl.data[(size_type)0u];
		moveUp(index);
	}

	void erase(iterator pos)
	{
		auto dataEnd = impl.data.end();
		--dataEnd;
		if(pos != dataEnd)
		{
			*pos = std::move(*dataEnd);
			impl.data.pop_back();
			auto index = size_type(pos - impl.data.begin());
			if(index == (size_type)0u) moveDown(index);
			else
			{
				auto parentIndex = (index - (size_type)1u) / (size_type)2u;
				if(impl(impl.data[index], impl.data[parentIndex])) moveDown(index);
				else moveUp(index);
			}
		}
		else
		{
			impl.data.pop_back();
		}
	}

	template<class EqualTo>
	iterator find(const_reference value, EqualTo equal = std::equal_to<value_type>())
	{
		const auto dataEnd = impl.data.end();
		for(auto start = impl.data.begin(); start != dataEnd; ++start)
		{
			if(equal(*start, value))
			{
				return start;
			}
		}
		return dataEnd;
	}

	template<class EqualTo>
	const_iterator find(const_reference value, EqualTo equal = std::equal_to<value_type>()) const
	{
		const auto dataEnd = impl.data.end();
		for(auto start = impl.data.begin(); start != dataEnd; ++start)
		{
			if(equal(*start, value))
			{
				return start;
			}
		}
		return dataEnd;
	}

	void clear()
	{
		impl.data.clear();
	}
};

template<class... Ts>
void swap(PriorityQueue<Ts...>& rhs, PriorityQueue<Ts...>& lhs)
{
	rhs.swap(lhs);
}