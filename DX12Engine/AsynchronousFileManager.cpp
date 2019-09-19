#include "AsynchronousFileManager.h"
#include <Windows.h>
#include <limits.h>

AsynchronousFileManager::AsynchronousFileManager(IOCompletionQueue& ioCompletionQueue1, const wchar_t* fileName) :
	ioCompletionQueue(ioCompletionQueue1),
	file(openFileForReading(fileName, ioCompletionQueue1))
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	pageSize = systemInfo.dwPageSize;
}

AsynchronousFileManager::~AsynchronousFileManager()
{
	file.close();
}

bool AsynchronousFileManager::readFileHelper(void* tr, DWORD, LPOVERLAPPED overlapped)
{
	auto& request = *static_cast<ReadRequest*>(overlapped);
	const auto pageSize = request.asynchronousFileManager->pageSize;
	auto& resources = request.asynchronousFileManager->resources;

	const auto memoryStart = request.start & ~(pageSize - 1ull);
	const auto memoryEnd = (request.end + pageSize - 1ull) & ~(pageSize - 1ull);
	const auto memoryNeeded = memoryEnd - memoryStart;

	unsigned char* allocation;
	auto dataPtr = resources.find(request);
	if(dataPtr != resources.end())
	{
		auto& dataDescriptor = dataPtr->second;
		++dataDescriptor.userCount;
		allocation = dataDescriptor.allocation;

		if(dataDescriptor.userCount == 1u)
		{
			auto result = ReclaimVirtualMemory(allocation, static_cast<SIZE_T>(memoryNeeded));
			if(result == ERROR_SUCCESS)
			{
				//We successfully reclaimed the data, so we can complete the request to load it.
				auto data = allocation + request.start - memoryStart;
				request.fileLoadedCallback(request, *request.asynchronousFileManager, tr, data);
				return true;
			}
			else
			{
				if(result != ERROR_BUSY) //ERROR_BUSY means the virtual memory was reclaimed but not its contents.
				{
					return false;
				}
				request.next = dataDescriptor.requests;
				dataDescriptor.requests = &request;
			}
		}
		else if(dataDescriptor.requests == nullptr)
		{
			//The resource is already loaded.
			auto data = allocation + request.start - memoryStart;
			request.fileLoadedCallback(request, *request.asynchronousFileManager, tr, data);
			return true;
		}
		else
		{
			//The resource isn't loaded yet.
			request.next = dataDescriptor.requests;
			dataDescriptor.requests = &request;
			return true; //Some other request is already loading the data. 
		}
	}
	else
	{
		allocation = (unsigned char*)VirtualAlloc(nullptr, static_cast<SIZE_T>(memoryNeeded), MEM_COMMIT, PAGE_READWRITE);
		request.next = nullptr;
		resources.insert(std::pair<const ResourceId, FileData>(request, FileData{allocation, 1u, &request}));
	}

	request.buffer = allocation;
	request.accumulatedSize = 0u;
	request.hEvent = nullptr;
	request.Offset = static_cast<DWORD>(memoryStart);
	request.OffsetHigh = static_cast<DWORD>(memoryStart >> (sizeof(DWORD) * CHAR_BIT));

	DWORD bytesRead = 0u;
	const DWORD maxReadableAmount = std::numeric_limits<DWORD>::max() & ~static_cast<DWORD>(pageSize - 1u);
	BOOL finished = ReadFile(request.asynchronousFileManager->file.native_handle(), request.buffer,
		memoryNeeded > maxReadableAmount ? maxReadableAmount : static_cast<DWORD>(memoryNeeded), &bytesRead, &request);
	if (finished == FALSE && GetLastError() != ERROR_IO_PENDING)
	{
		return false;
	}
	return true;
}

bool AsynchronousFileManager::discardHelper(void* tr, DWORD, LPOVERLAPPED overlapped)
{
	auto& request = *static_cast<ReadRequest*>(overlapped);
	const auto pageSize = request.asynchronousFileManager->pageSize;
	auto& resources = request.asynchronousFileManager->resources;

	const auto memoryStart = request.start & ~(pageSize - 1ull);
	const auto memoryEnd = (request.end + pageSize - 1ull) & ~(pageSize - 1ull);
	const auto memoryNeeded = memoryEnd - memoryStart;
	FileData& dataDescriptor = resources.find(request)->second;
	--dataDescriptor.userCount;
	if (dataDescriptor.userCount == 0u)
	{
		//The resource is no longer needed in memory.
		OfferVirtualMemory(dataDescriptor.allocation, static_cast<SIZE_T>(memoryNeeded), OFFER_PRIORITY::VmOfferPriorityLow);
	}

	request.deleteReadRequest(request, tr);
	return true;
}

 bool AsynchronousFileManager::processIOCompletion(void* tr, DWORD numberOfBytes, LPOVERLAPPED overlapped)
{
	 ReadRequest* request = static_cast<ReadRequest*>(overlapped);
	 AsynchronousFileManager& fileManager = *request->asynchronousFileManager;

	 request->accumulatedSize += numberOfBytes;
	 const auto sectorSize = fileManager.pageSize;
	 const auto memoryStart = request->start & ~(sectorSize - 1u);
	 const auto memoryEnd = (request->end + sectorSize - 1u) & ~(sectorSize - 1u);
	 const auto sizeToRead = memoryEnd - memoryStart;
	 if ((request->accumulatedSize < (request->end - memoryStart)) && request->accumulatedSize != sizeToRead)
	 {
		 request->accumulatedSize = request->accumulatedSize & ~(sectorSize - 1u);
		 const auto currentPosition = memoryStart + request->accumulatedSize;
		 request->Offset = static_cast<DWORD>(currentPosition);
		 request->OffsetHigh = static_cast<DWORD>(currentPosition >> (sizeof(DWORD) * CHAR_BIT));
		 DWORD bytesRead = 0u;
		 const auto remainingAmountToRead = sizeToRead - request->accumulatedSize;
		 const DWORD maxReadableAmount = std::numeric_limits<DWORD>::max() & ~static_cast<DWORD>(sectorSize - 1u);
		 BOOL finished = ReadFile(fileManager.file.native_handle(), request->buffer + request->accumulatedSize,
			 remainingAmountToRead > maxReadableAmount ? maxReadableAmount : static_cast<DWORD>(remainingAmountToRead), &bytesRead, request);
		 if (finished == FALSE && GetLastError() != ERROR_IO_PENDING)
		 {
			 return false;
		 }
		 return true;
	 }
	 auto data = request->buffer + (request->start & (sectorSize - 1u));

	 FileData& dataDescriptor = fileManager.resources.find(*request)->second;
	 ReadRequest* requests = dataDescriptor.requests;
	 dataDescriptor.requests = nullptr;
	 do
	 {
		 ReadRequest& temp = *requests;
		 requests = static_cast<ReadRequest*>(requests->next); //Allow reuse of next
		 temp.fileLoadedCallback(temp, fileManager, tr, data);
	 } while (requests != nullptr);
	 return true;
}

 void AsynchronousFileManager::read(ReadRequest& request)
 {
	 request.asynchronousFileManager = this;
	 IOCompletionPacket task;
	 task.numberOfBytesTransfered = 0u;
	 task.overlapped = &request;
	 task.completionKey = reinterpret_cast<ULONG_PTR>(readFileHelper);
	 ioCompletionQueue.push(task);
 }

 void AsynchronousFileManager::discard(ReadRequest& request)
 {
	 request.asynchronousFileManager = this;
	 IOCompletionPacket task;
	 task.numberOfBytesTransfered = 0u;
	 task.overlapped = &request;
	 task.completionKey = reinterpret_cast<ULONG_PTR>(discardHelper);
	 ioCompletionQueue.push(task);
 }