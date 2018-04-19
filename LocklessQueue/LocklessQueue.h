#pragma once

#include <atomic>
#include <utility>
#include "Array.h"

template<class ElementType, unsigned int capacity>
class LocklessQueue
{
	std::atomic<unsigned int> readPos = 0u;
	std::atomic<unsigned int> writePos = 0u;
	ElementType data[capacity];
public:
	void push(ElementType&& element)
	{
		unsigned int oldWritePos = writePos.fetch_add(1u);
		data[oldWritePos % capacity] = std::move(element);
	}

	void push(ElementType* elements, unsigned int numElements)
	{
		unsigned int oldWritePos = writePos.fetch_add(numElements); //must be sequencally consistent
		unsigned int end = (oldWritePos + numElements) % capacity;
		unsigned int i = 0u;
		for (oldWritePos %= capacity; oldWritePos != end; oldWritePos = (oldWritePos + 1u) % capacity, ++i)
		{
			data[oldWritePos] = elements[i];
		}
	}

	bool pop(ElementType& element)
	{
		unsigned int oldReadPos = readPos.load();
		bool empty = false;
		do
		{
			if (oldReadPos != writePos.load())
			{
				empty = true;
				break;
			}
		} while (!readPos.compare_exchange_weak(oldReadPos, oldReadPos + 1u));

		if (!empty)
		{
			element = std::move(data[oldReadPos]);
			return true;
		}
		return false;
	}
};

/* capacity should be a power of two */
template<class ElementType, unsigned long capacity>
class LocklessQueueSingleConsumerMultipleProducers
{
	unsigned long readPos = 0u;
	std::atomic<unsigned long> writePos = 0u;
	Array<ElementType, capacity> data;

	template<typename value_type>
	friend class LocklessQueueSingleConsumerMultipleProducersReference;
public:
	LocklessQueueSingleConsumerMultipleProducers() {}
	LocklessQueueSingleConsumerMultipleProducers(LocklessQueueSingleConsumerMultipleProducers<ElementType, capacity>&& other) : readPos(std::move(other.readPos)), writePos(other.writePos.load()),
		data(std::move(other.data)) {}
	void push(ElementType&& element)
	{
		unsigned long oldWritePos = writePos.fetch_add(1u, std::memory_order_seq_cst); //makes new element available to early
		data[oldWritePos % capacity] = std::move(element);
	}

	void push(ElementType* elements, unsigned long numElements)
	{
		unsigned long oldWritePos = writePos.fetch_add(numElements); //must be sequencally consistent
		unsigned long end = (oldWritePos + numElements) % capacity;
		unsigned long i = 0u;
		for (oldWritePos %= capacity; oldWritePos != end; oldWritePos = (oldWritePos + 1u) % capacity, ++i)
		{
			data[oldWritePos] = elements[i];
		}
	}

	bool pop(ElementType& element)
	{
		if (readPos != writePos.load())
		{
			++readPos;
			element = std::move(data[readPos - 1u]);
			return true;
		}
		return false;
	}
};

template<typename value_type>
class LocklessQueueSingleConsumerMultipleProducersReference
{
	unsigned char* queue;
	unsigned long capacity;

	typedef LocklessQueueSingleConsumerMultipleProducers<value_type, 1024> LocklessQueueSingleConsumerMultipleProducers_t;
	unsigned long& readPos()
	{
		return *reinterpret_cast<unsigned long* const>(queue + offsetof(LocklessQueueSingleConsumerMultipleProducers_t, readPos));
	}
	std::atomic<unsigned long>& writePos()
	{
		return *reinterpret_cast<std::atomic<unsigned long>* const>(queue + offsetof(LocklessQueueSingleConsumerMultipleProducers_t, writePos));
	}
	value_type* const data()
	{
		return reinterpret_cast<value_type* const>(queue + offsetof(LocklessQueueSingleConsumerMultipleProducers_t, data));
	}
public:
	LocklessQueueSingleConsumerMultipleProducersReference() {}
	template<unsigned long capacity>
	LocklessQueueSingleConsumerMultipleProducersReference(LocklessQueueSingleConsumerMultipleProducers<value_type, capacity>& queue) : queue(reinterpret_cast<unsigned char*>(&queue)), capacity(capacity) {}

	template<unsigned long capacity>
	void operator=(LocklessQueueSingleConsumerMultipleProducers<value_type, capacity>& other)
	{
		queue = reinterpret_cast<unsigned char*>(&other);
		this->capacity = capacity;
	}

	void push(value_type&& element)
	{
		unsigned int oldWritePos = writePos().fetch_add(1u); //must be sequencally consistent
		data()[oldWritePos % capacity] = std::move(element);
	}

	void push(value_type* elements, unsigned int numElements)
	{
		unsigned int oldWritePos = writePos().fetch_add(numElements); //must be sequencally consistent
		unsigned int end = (oldWritePos + numElements) % capacity;
		unsigned int i = 0u;
		for (oldWritePos %= capacity; oldWritePos != end; oldWritePos = (oldWritePos + 1u) % capacity, ++i)
		{
			data()[oldWritePos] = elements[i];
		}
	}

	bool pop(value_type& element)
	{
		if (readPos() != writePos().load())
		{
			++(readPos());
			element = std::move(data()[readPos()]);
			return true;
		}
		return false;
	}
};