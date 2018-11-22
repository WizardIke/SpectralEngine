#include <new>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <windows.h>

template<std::size_t size, std::size_t alignment>
class MultiThreadedPoolAllocator
{
private:
	static constexpr std::size_t myMax(std::size_t x, std::size_t y)
	{
		return x > y ? x : y;
	}
	
	static constexpr std::size_t ceilToMultiple(std::size_t size2, std::size_t alignment2)
	{
		return size2 % alignment2 == 0 ? size2 : size2 + (alignment2 - size2 % alignment2);
	}

	static constexpr std::size_t minSize = ceilToMultiple(myMax(size, sizeof(char*)),
		ceilToMultiple(ceilToMultiple(myMax(alignment, alignof(char*)), alignment),
			alignof(char*)));
	static constexpr std::size_t minAlignment = ceilToMultiple(ceilToMultiple(
		myMax(alignment, alignof(char*)), alignment), alignof(char*));
	
#if __cplusplus >= 201703L
	static constexpr std::size_t hardwareDestructiveInterferenceSize 
		= std::hardware_destructive_interference_size;
#else
	static constexpr std::size_t hardwareDestructiveInterferenceSize = 64u;
#endif
	
	class FreeItem
	{
	public:
		FreeItem* next;
	};
	
	class MemPage
	{
		class Data
		{
		public:
			MemPage* next; //Read and written by single thread
			FreeItem* localFreeItems; //Read and written by single thread

			char padding[MultiThreadedPoolAllocator::hardwareDestructiveInterferenceSize > sizeof(MemPage*) + sizeof(FreeItem*) ?
				MultiThreadedPoolAllocator::hardwareDestructiveInterferenceSize - sizeof(MemPage*) - sizeof(FreeItem*) : 1u]; //Needs to be padded to prevent false sharing
			
			MultiThreadedPoolAllocator* owningAllocator; //Read by multiple threads

			char padding2[MultiThreadedPoolAllocator::hardwareDestructiveInterferenceSize > sizeof(MultiThreadedPoolAllocator*) ?
				MultiThreadedPoolAllocator::hardwareDestructiveInterferenceSize - sizeof(MultiThreadedPoolAllocator*) : 1u]; //Needs to be padded to prevent false sharing

			std::atomic<FreeItem*> returnedFreeItems; //Read and written by multiple threads

			Data(MemPage* next, MultiThreadedPoolAllocator* owningAllocator) :
				next{next},
				owningAllocator{owningAllocator},
				returnedFreeItems{nullptr},
				localFreeItems{nullptr} {}
		};
		static constexpr std::size_t startOffset =
			MultiThreadedPoolAllocator::ceilToMultiple(sizeof(Data),
				MultiThreadedPoolAllocator::minAlignment);
	public:
		Data data;

		MemPage(MultiThreadedPoolAllocator* owningAllocator, std::size_t totalSize,
			char* mem, MemPage* next) :
			data(next, owningAllocator)
		{
			constexpr auto minSize = MultiThreadedPoolAllocator::minSize;
			assert(startOffset + minSize >= totalSize && "Allocating a memory page that doesn't have enough space for any pooled items");
			const char* theEnd = mem + totalSize - minSize;
			for(char* current = mem + startOffset; 
				current <= theEnd; current += minSize)
			{
				FreeItem* const newPtr = new(current) FreeItem{data.localFreeItems};
				data.localFreeItems = newPtr;
			}
		}
	};
	
	MemPage* freePagesList;
	std::size_t virtualAllocationGranularity;

	char padding[hardwareDestructiveInterferenceSize > sizeof(MemPage*) + sizeof(std::size_t) ?
		hardwareDestructiveInterferenceSize - sizeof(MemPage*) - sizeof(std::size_t) : 1u]; //Needs to be padded to prevent false sharing

	std::atomic<MemPage*> returnedFreePages;
public:
	MultiThreadedPoolAllocator(std::size_t virtualAllocationGranularity) :
		freePagesList{nullptr},
		virtualAllocationGranularity(virtualAllocationGranularity),
		returnedFreePages{nullptr} {}
			
	void* allocate()
	{
		if(freePagesList != nullptr)
		{
			MemPage* const page = freePagesList;
			FreeItem* freeItem = page->data.localFreeItems;
			if(freeItem != nullptr)
			{
				page->data.localFreeItems = freeItem->next;
				freeItem->~FreeItem();
				return freeItem;
			}
			while(true)
			{
				//Get items from returned free list
				FreeItem* returedItems = page->data.returnedFreeItems.load(std::memory_order_relaxed);
				while(!page->data.returnedFreeItems.compare_exchange_weak(
					returedItems,
					nullptr,
					std::memory_order_acquire,
					std::memory_order_relaxed)) {};
				if(returedItems != nullptr)
				{
					//Add returedItems to localFreeItems
					page->data.localFreeItems = returedItems->next;
					//Return first item from localFreeItems
					returedItems->~FreeItem();
					return returedItems;
				}
				//Try set returned free list to &owningAllocator because mem page is fully allocated
				FreeItem* expected = nullptr;
				FreeItem* owningAllocator = reinterpret_cast<FreeItem*>(page->data.owningAllocator);
				if(page->data.returnedFreeItems.compare_exchange_weak(
					expected,
					owningAllocator,
					std::memory_order_relaxed,
					std::memory_order_relaxed))
				{
					//Remove page from freePages list
					freePagesList = freePagesList->data.next;
					return allocate();
				}
			}
		}
		//Try to recover returned mem pages
		MemPage* returnedPages = returnedFreePages.load(std::memory_order_relaxed);
		while(!returnedFreePages.compare_exchange_weak(
			returnedPages,
			nullptr,
			std::memory_order_acquire,
			std::memory_order_relaxed)) {};
		if(returnedPages != nullptr)
		{
			freePagesList = returnedPages;
		}
		else
		{
			//Allocate mem page using virtual alloc
			char* newPageMemory = reinterpret_cast<char*>(
				VirtualAlloc(nullptr, virtualAllocationGranularity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
			MemPage* newPage = new(newPageMemory) MemPage{this, virtualAllocationGranularity,
				newPageMemory, freePagesList};
			freePagesList = newPage;
		}
		return allocate();
	}
		
	void deallocate(void* ptr)
	{
		//Get address of mem page
		MemPage* page = reinterpret_cast<MemPage*>(
			reinterpret_cast<std::uintptr_t>(ptr) 
			& static_cast<std::uintptr_t>(virtualAllocationGranularity - 1u));
		if(page->data.owningAllocator == this)
		{
			//Add ptr to free items page
			FreeItem* const oldFreeItems = page->data.localFreeItems;
			page->data.localFreeItems = new(ptr) FreeItem{oldFreeItems};
			//If page was fully allocated, add page to freePagesList of this allocator
			if(oldFreeItems == nullptr)
			{
				FreeItem* expected = reinterpret_cast<FreeItem*>(this);
				if(page->data.returnedFreeItems.compare_exchange_strong(
					expected,
					nullptr,
					std::memory_order_relaxed,
					std::memory_order_relaxed))
				{
					page->data.next = freePagesList;
					freePagesList = page;
				}
			}
		}
		else
		{
			//Add ptr to returned free items of page
			FreeItem* const pageAllocator = reinterpret_cast<FreeItem*>(page->data.owningAllocator);
			FreeItem* oldReturnedFreeItems = page->data.returnedFreeItems
				.load(std::memory_order_relaxed);
			FreeItem* newReturnedFreeItems;
			bool wasFullyAllocated;
			do
			{
				if(oldReturnedFreeItems == pageAllocator)
				{
					newReturnedFreeItems = new(ptr) FreeItem{nullptr};
					wasFullyAllocated = true;
				}
				else
				{
					newReturnedFreeItems = new(ptr) FreeItem{oldReturnedFreeItems};
					wasFullyAllocated = false;
				}
			} while(!page->data.returnedFreeItems.compare_exchange_weak(
				oldReturnedFreeItems,
				newReturnedFreeItems,
				std::memory_order_release,
				std::memory_order_relaxed));
			//If page was fully allocated, add page to returnedFreePages of owningAllocator
			if(wasFullyAllocated)
			{
				std::atomic<MemPage*>& returnedFreePages = page->data.owningAllocator
					->returnedFreePages;
				page->data.next = returnedFreePages.load(
					std::memory_order_relaxed);
				while(!returnedFreePages.compare_exchange_weak(
					page->data.next,
					page,
					std::memory_order_release,
					std::memory_order_relaxed)) {}
			}
		}
	}
};