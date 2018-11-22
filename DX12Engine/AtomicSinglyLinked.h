#pragma once
#include <atomic>

class AtomicSinglyLinked
{
public:
	std::atomic<AtomicSinglyLinked*> next;
};