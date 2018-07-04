#pragma once
#include <atomic>

template<class T>
class MultiProducerSingleConsumerQueue
{
public:
	struct Node
	{
		std::atomic<Node*> next;
		T data;
	};
	
	struct Result
	{
		T* data;
		Node* nodeToDeallocate;
	}
private:
	std::atomic<Node*> head;
	char padding[128u - sizeof(std::atomic<Node*>)]; //put tail on a different cacheline than head
	Node* tail;
public:
	MultiProducerSingleConsumerQueue(Node* stub)
	{
		stub->next.store(nullptr, std::memory_order_relaxed);
		head.store(stub, std::memory_order_relaxed); 
		tail = stub;
	}
	
	void push(Node* first, Node* last)
	{
		last->next.store(nullptr, std::memory_order_relaxed); 
		Node* prev = head.exchange(last, std::memory_order_acq_rel);
		prev->next.store(first, std::memory_order_release);
	}
	
	void push(Node* n) 
	{ 
		n->next.store(nullptr, std::memory_order_relaxed); 
		Node* prev = head.exchange(n, std::memory_order_acq_rel);
		prev->next.store(n, std::memory_order_release);
	} 
	
	Result pop() 
	{ 
		Node* oldTail = tail;
		Node* next = oldTail->next.load(std::memory_order_acquire);
		if (next != nullptr) 
		{ 
			tail = next;
			return {&next->state, oldTail}; 
		} 
		return {nullptr, nullptr}; 
	} 
	
	Node* shutdown()
	{
		return tail;
	}
};
