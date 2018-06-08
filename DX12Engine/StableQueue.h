#pragma once
#include <memory>

/* references and pointers to objects in a StableQueue remain valid even if the queue resizes.
 */

template<class value_type, size_t mSlabSize>
struct StableQueue_Slab
{
	constexpr static auto slabSize = mSlabSize;
	value_type data[slabSize];
	StableQueue_Slab* next;
};

template<class T, class Allocator = std::allocator<StableQueue_Slab<T, 32u>>>
class StableQueue
{
public:
	using value_type = T;
	using reference = value_type&;
	using const_reference = const value_type&;
private:
	using Slab = typename std::allocator_traits<Allocator>::value_type;
	constexpr static auto slabSize = Slab::slabSize;
	Allocator allocator;
	Slab* freeList = nullptr;
	value_type* mFrontSlabEnd;
	value_type* mFront = nullptr;
	value_type* mBackSlabEnd = nullptr;
	value_type* mBack = nullptr;

	void growBack()
	{
		Slab* newSlab;
		if (freeList != nullptr)
		{
			newSlab = freeList;
			freeList = freeList->next;
		}
		else
		{
			newSlab = allocator.allocate(1u);
		}
		
		if (mBack != nullptr)
		{
			Slab* oldSlab = reinterpret_cast<Slab*>(mBack - slabSize);
			oldSlab->next = newSlab;
		}
		else
		{
			mFront = &newSlab->data[0];
			mFrontSlabEnd = mFront + slabSize;
		}
		newSlab->next = nullptr;
		mBack = &newSlab->data[0];
		mBackSlabEnd = mBack + slabSize;
	}

	void shrinkFront()
	{
		Slab* oldSlab = reinterpret_cast<Slab*>(mFront - slabSize);
		Slab* newSlab = oldSlab->next;
		oldSlab->next = freeList;
		freeList = oldSlab;
		if (newSlab == nullptr)
		{
			mBackSlabEnd = nullptr;
			mBack = nullptr;
			mFront = nullptr;
		}
		else
		{
			mFront = &newSlab->data[0];
			mFrontSlabEnd = mFront + slabSize;
		}
	}
public:
	~StableQueue()
	{
		while (!empty())
		{
			pop_front();
		}
		while (freeList != nullptr)
		{
			Slab* temp = freeList;
			freeList = freeList->next;
			allocator.deallocate(temp, 1u);
		}
	}

	void push_back(const_reference value)
	{
		if (mBack == mBackSlabEnd)
		{
			growBack();
		}
		new(mBack) value_type(value);
		++mBack;
	}

	void push_back(value_type&& value)
	{
		if (mBack == mBackSlabEnd)
		{
			growBack();
		}
		new(mBack) value_type(std::move(value));
		++mBack;
	}

	template<class... Args>
	void emplace_back(Args&&... args)
	{
		if (mBack == mBackSlabEnd)
		{
			growBack();
		}
		new(mBack) value_type(std::farword<Args>(args)...);
		++mBack;
	}

	void pop_front()
	{
		mFront->~value_type();
		++mFront;
		if (mFront == mFrontSlabEnd)
		{
			shrinkFront();
		}
	}

	reference front()
	{
		return *mFront;
	}

	const_reference front() const
	{
		return *mFront;
	}

	reference back()
	{
		return *(mBack - 1u);
	}

	const_reference back() const
	{
		return *(mBack - 1u);
	}

	bool empty() const
	{
		return mBack == mFront;
	}
};