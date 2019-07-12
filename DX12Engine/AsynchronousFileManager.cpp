#include "AsynchronousFileManager.h"
#include <Windows.h>

AsynchronousFileManager::AsynchronousFileManager(IOCompletionQueue& ioCompletionQueue1) :
	ioCompletionQueue(ioCompletionQueue1)
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	pageSize = systemInfo.dwPageSize;
}

AsynchronousFileManager::~AsynchronousFileManager() {}

bool AsynchronousFileManager::readFileHelper(void* tr, DWORD, LPOVERLAPPED overlapped)
{
	auto& request = *static_cast<ReadRequest*>(overlapped);
	const auto pageSize = request.asynchronousFileManager->pageSize;
	auto& files = request.asynchronousFileManager->files;

	std::size_t memoryStart = request.start & ~(pageSize - 1u);
	std::size_t memoryEnd = (request.end + pageSize - 1u) & ~(pageSize - 1u);
	std::size_t memoryNeeded = memoryEnd - memoryStart;

	unsigned char* allocation;
	auto dataPtr = files.find(request);
	if(dataPtr != files.end())
	{
		auto& dataDescriptor = dataPtr->second;
		++dataDescriptor.userCount;
		allocation = dataDescriptor.allocation;

		if(dataDescriptor.userCount == 1u)
		{
			auto result = ReclaimVirtualMemory(allocation, memoryNeeded);
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
		allocation = (unsigned char*)VirtualAlloc(nullptr, memoryNeeded, MEM_COMMIT, PAGE_READWRITE);
		request.next = nullptr;
		files.insert(std::pair<const ResourceId, FileData>(request, FileData{allocation, 1u, &request}));
	}

	request.buffer = allocation;
	request.accumulatedSize = 0u;
	request.hEvent = nullptr;
	request.Offset = memoryStart & (std::numeric_limits<DWORD>::max() - 1u);
	request.OffsetHigh = (DWORD)((memoryStart & ~(std::size_t)(std::numeric_limits<DWORD>::max() - 1u)) >> 32u);

	DWORD bytesRead = 0u;
	BOOL finished = ReadFile(request.file.native_handle(), request.buffer, (DWORD)memoryNeeded, &bytesRead, &request);
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
	auto& files = request.asynchronousFileManager->files;

	std::size_t memoryStart = request.start & ~(pageSize - 1u);
	std::size_t memoryEnd = (request.end + pageSize - 1u) & ~(pageSize - 1u);
	std::size_t memoryNeeded = memoryEnd - memoryStart;
	FileData& dataDescriptor = files.find(request)->second;
	--dataDescriptor.userCount;
	if (dataDescriptor.userCount == 0u)
	{
		//The resource is no longer needed in memory.
		OfferVirtualMemory(dataDescriptor.allocation, memoryNeeded, OFFER_PRIORITY::VmOfferPriorityLow);
	}

	request.deleteReadRequest(request, tr);
	return true;
}

 bool AsynchronousFileManager::processIOCompletion(void* tr, DWORD numberOfBytes, LPOVERLAPPED overlapped)
{
	 ReadRequest* request = static_cast<ReadRequest*>(overlapped);
	 AsynchronousFileManager& fileManager = *request->asynchronousFileManager;

	 request->accumulatedSize += numberOfBytes;
	 const std::size_t sectorSize = fileManager.pageSize;
	 const std::size_t memoryStart = request->start & ~(sectorSize - 1u);
	 const std::size_t memoryEnd = (request->end + sectorSize - 1u) & ~(sectorSize - 1u);
	 const std::size_t sizeToRead = memoryEnd - memoryStart;
	 if ((request->accumulatedSize < (request->end - memoryStart)) && request->accumulatedSize != sizeToRead)
	 {
		 request->accumulatedSize = request->accumulatedSize & ~(sectorSize - 1u);
		 const std::size_t currentPosition = memoryStart + request->accumulatedSize;
		 request->Offset = currentPosition & (std::numeric_limits<DWORD>::max() - 1u);
		 request->OffsetHigh = (DWORD)((currentPosition & ~(std::size_t)(std::numeric_limits<DWORD>::max() - 1u)) >> 32u);
		 DWORD bytesRead = 0u;
		 BOOL finished = ReadFile(request->file.native_handle(), request->buffer + request->accumulatedSize, (DWORD)(sizeToRead - request->accumulatedSize), &bytesRead, request);
		 if (finished == FALSE && GetLastError() != ERROR_IO_PENDING)
		 {
			 return false;
		 }
		 return true;
	 }
	 auto data = request->buffer + (request->start & (fileManager.pageSize - 1u));

	 FileData& dataDescriptor = fileManager.files.find(*request)->second;
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

 void AsynchronousFileManager::readFile(ReadRequest& request)
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