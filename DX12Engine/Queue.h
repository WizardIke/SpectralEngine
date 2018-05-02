#pragma once
#include <mutex>
#include <memory>

template<typename value_type, class Allocator = std::allocator<value_type>>
class Queue : private Allocator
{
	std::mutex criticalSection;

	value_type* buffer;
	value_type* capacityEnd;

	value_type* readPos;
	value_type* writePos;
	
	void resizeBuffer()
	{
		const size_t capacity = capacityEnd - buffer;
		const size_t newCapacity = capacity + (capacity >> 1u);
		value_type* tempBuffer = this->allocate(newCapacity);

		value_type* oldReadPos = readPos;
		value_type* const tempEnd = tempBuffer + capacity;
		for (value_type* i = tempBuffer; i != tempEnd; ++i)
		{
			*i = std::move(*oldReadPos);
			oldReadPos->~value_type();
			++oldReadPos;
			if (oldReadPos == capacityEnd) oldReadPos = buffer;
		}
		delete[] buffer;
		buffer = tempBuffer;
		readPos = tempBuffer;
		writePos = tempEnd;
		capacityEnd = tempBuffer + newCapacity;
	}
public:
	Queue(size_t initualCapacity)
	{
		if (initualCapacity < 2u) initualCapacity = 2u;
		buffer = this->allocate(initualCapacity);
		capacityEnd = buffer + initualCapacity;
		readPos = buffer;
		writePos = buffer;
	}
	~Queue()
	{
		for (value_type* i = readPos; i != writePos; ++i)
		{
			i->~value_type();
		}
		deallocate(buffer, capacityEnd - buffer);
	}

	void push(value_type&& job)
	{
		std::lock_guard<decltype(criticalSection)> lock(criticalSection);
		new(writePos) value_type(std::move(job));
		++writePos;
		if (writePos == capacityEnd) writePos = buffer;
		if (writePos == readPos) 
		{
			resizeBuffer();
		}
	}

	bool pop(value_type& element)
	{
		std::lock_guard<decltype(criticalSection)> lock(criticalSection);
		if (readPos == writePos)
		{
			return false; //empty queue
		}
		element = std::move(*readPos);
		readPos->~value_type();
		++readPos;
		if (readPos == capacityEnd) readPos = buffer;
		return true;
	}
};