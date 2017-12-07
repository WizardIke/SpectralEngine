#pragma once
#include <mutex>

template<typename value_type>
class Queue
{
	std::mutex criticalSection;

	value_type* buffer;
	unsigned int capacity;

	unsigned int readPos = 0u;
	unsigned int writePos = 0u;
	
	void resizeBuffer()
	{
		value_type* tempBuffer = new value_type[static_cast<size_t>(this->capacity * 1.2f)];

		unsigned int oldReadPos = this->readPos;
		this->readPos = 0;
		this->writePos = this->capacity;
		for (unsigned int i = 0; i < this->capacity; ++i) {
			tempBuffer[i] = this->buffer[oldReadPos];
			++oldReadPos;
			if (oldReadPos == this->capacity) oldReadPos = 0u;
		}
		delete[] this->buffer;
		this->buffer = tempBuffer;
		this->capacity = static_cast<unsigned int>(this->capacity * 1.2f);
	}
public:
	Queue(unsigned int initualCapacity) : capacity(initualCapacity)
	{
		buffer = new value_type[initualCapacity];
	}
	~Queue()
	{
		delete buffer;
	}

	void push(value_type&& job)
	{
		criticalSection.lock();
		buffer[writePos] = job;
		++writePos;
		if (writePos == capacity) writePos = 0u;
		if (writePos == readPos) {
			resizeBuffer();
		}
		criticalSection.unlock();
	}

	bool pop(value_type& element)
	{
		criticalSection.lock();
		if (readPos == writePos)
		{
			criticalSection.unlock();
			return false; //empty queue
		}
		element = buffer[readPos];
		++readPos;
		if (readPos == capacity) readPos = 0u;
		criticalSection.unlock();
		return true;
	}
};