#pragma once
#include <atomic>
#include <cstdint>
#include "Delegate.h"
#include "MultiProducerSingleConsumerQueue.h"
#include "AtomicSinglyLinked.h"

class StreamingRequest : public AtomicSinglyLinked
{
public:
	StreamingRequest() {}
	StreamingRequest(void(*deallocateNode)(StreamingRequest* nodeToDeallocate, void* threadResources, void* globalResources)) : deallocateNode(deallocateNode) {}

	void(*deallocateNode)(StreamingRequest* nodeToDeallocate, void* threadResources, void* globalResources);
	void(*resourceUploaded)(StreamingRequest* request, void* threadResources, void* globalResources);
	void(*streamResource)(StreamingRequest* request, void* threadResources, void* globalResources);
	unsigned long resourceSize;

	unsigned long uploadResourceOffset;
	ID3D12Resource* uploadResource;
	unsigned char* uploadBufferCurrentCpuAddress;

	StreamingRequest* nodeToDeallocate;
	unsigned long numberOfBytesToFree;
	unsigned int copyFenceIndex;
	std::atomic<uint64_t> copyFenceValue;
};