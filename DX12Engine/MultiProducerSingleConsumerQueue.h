#pragma once
#include "AtomicSinglyLinked.h"
#include <atomic>

class MultiProducerSingleConsumerQueue
{
public:
	struct Result
	{
		AtomicSinglyLinked* dataNode;
		AtomicSinglyLinked* nodeToDeallocate;
	};
private:
	std::atomic<AtomicSinglyLinked*> head;
	char padding[128u > sizeof(std::atomic<AtomicSinglyLinked*>) ? 128u - sizeof(std::atomic<AtomicSinglyLinked*>) : 1u]; //put tail on a different cacheline than head
	AtomicSinglyLinked* tail;
public:
	MultiProducerSingleConsumerQueue(AtomicSinglyLinked* stub)
	{
		stub->next.store(nullptr, std::memory_order_relaxed);
		head.store(stub, std::memory_order_relaxed); 
		tail = stub;
	}
	
	/*
	 * Pushes all the elements in a linked list onto the queue
	 */
	void push(AtomicSinglyLinked* first, AtomicSinglyLinked* last)
	{
		last->next.store(nullptr, std::memory_order_relaxed); 
		AtomicSinglyLinked* prev = head.exchange(last, std::memory_order_acq_rel);
		prev->next.store(first, std::memory_order_release);
	}
	
	void push(AtomicSinglyLinked* n)
	{ 
		n->next.store(nullptr, std::memory_order_relaxed); 
		AtomicSinglyLinked* prev = head.exchange(n, std::memory_order_acq_rel);
		prev->next.store(n, std::memory_order_release);
	} 
	
	Result pop() 
	{ 
		AtomicSinglyLinked* oldTail = tail;
		AtomicSinglyLinked* next = oldTail->next.load(std::memory_order_acquire);
		if (next != nullptr) 
		{ 
			tail = next;
			return {next, oldTail}; 
		} 
		return {nullptr, nullptr}; 
	} 
	
	/*
	 * The queue cannot be used after this method is called.
	 * Optional, used to reclaim the Node* passed into the constructor.
	 */
	AtomicSinglyLinked* shutdown()
	{
		return tail;
	}
};
