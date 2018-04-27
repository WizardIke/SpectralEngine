#include "AsynchronousFileManager.h"
#include "SharedResources.h"
#include <Windows.h>

AsynchronousFileManager::AsynchronousFileManager()
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	sectorSize = systemInfo.dwPageSize;
}

AsynchronousFileManager::~AsynchronousFileManager() {}

File AsynchronousFileManager::openFileForReading(IOCompletionQueue& ioCompletionQueue, const wchar_t* name)
{
	File file(name, File::accessRight::genericRead, File::shareMode::readMode, File::creationMode::openExisting, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING);
	ioCompletionQueue.associateFile(file.native_handle(), (ULONG_PTR)processIOCompletion);
	return file;
}

bool AsynchronousFileManager::readFile(BaseExecutor* executor, SharedResources& sharedResources, const wchar_t* name, size_t start, size_t end, File file,
	void* requester, void(*completionEvent)(void* requester, BaseExecutor* executor, SharedResources& sharedResources, const uint8_t* data, File file))
{
	size_t memoryStart = start & ~(sectorSize - 1u);
	size_t memoryEnd = (end + sectorSize - 1u) & ~(sectorSize - 1u);
	size_t memoryNeeded = memoryEnd - memoryStart;

	uint8_t* allocation;
	bool dataFound = false;
	Key key{ name, start, end };
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		
		auto dataPtr = files.find(key);
		if (dataPtr != files.end())
		{
			dataFound = true;
			allocation = dataPtr->second.allocation;
		}
		else
		{
			allocation = (uint8_t*)VirtualAlloc(nullptr, memoryNeeded, MEM_RESERVE, PAGE_READWRITE);
			auto data = allocation + (start & ~(sectorSize - 1u));
			files.insert(std::pair<const Key, FileData>(key, FileData{ allocation, data, memoryNeeded }));
		}
	}
	
	if (dataFound)
	{
		auto result = ReclaimVirtualMemory(allocation, memoryNeeded);
		if (result == ERROR_SUCCESS)
		{
			auto data = allocation + (start & ~(sectorSize - 1u));
			completionEvent(requester, executor, sharedResources, data, file);
			return true;
		}
		else if(result != ERROR_BUSY) //ERROR_BUSY means the virtual memory was reclaimed but not its contents.
		{
			return false;
		}
	}

	IORequest* request;
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		request = requestAllocator.allocate();
	}

	request->buffer = allocation;
	request->name = name;
	request->start = start;
	request->end = end;
	request->sizeToRead = memoryNeeded;
	request->accumulatedSize = 0u;
	request->completionEvent = completionEvent;
	request->hEvent = requester;
	request->memoryStart = memoryStart;
	request->file = file;
	request->Offset = memoryStart & (std::numeric_limits<DWORD>::max() - 1u);
	request->OffsetHigh = (DWORD)((memoryStart & ~(size_t)(std::numeric_limits<DWORD>::max() - 1u)) >> 32u);

	DWORD bytesRead = 0u;
	BOOL finished = ReadFile(file.native_handle(), request->buffer, (DWORD)request->sizeToRead, &bytesRead, request);
	if (finished == FALSE && GetLastError() != ERROR_IO_PENDING)
	{
		return false;
	}
	return true;
}

void AsynchronousFileManager::discard(const wchar_t* name, size_t start, size_t end)
{
	Key key{ name, start, end };
	std::lock_guard<decltype(mutex)> lock(mutex);
	auto data = files.find(key);
	OfferVirtualMemory(data->second.allocation, data->second.allocationSize, OFFER_PRIORITY::VmOfferPriorityLow);
}

 bool AsynchronousFileManager::processIOCompletion(BaseExecutor* executor, SharedResources& sharedResources, DWORD numberOfBytes, LPOVERLAPPED overlapped)
{
	AsynchronousFileManager& fileManager = sharedResources.asynchronousFileManager;
	IORequest* request = reinterpret_cast<IORequest*>(overlapped);
	request->accumulatedSize += numberOfBytes;
	if (request->accumulatedSize != request->sizeToRead)
	{
		DWORD bytesRead = 0u;
		request->memoryStart = request->memoryStart + request->accumulatedSize;
		request->Offset = request->memoryStart & (std::numeric_limits<DWORD>::max() - 1u);
		request->OffsetHigh = (DWORD)((request->memoryStart & ~(size_t)(std::numeric_limits<DWORD>::max() - 1u)) >> 32u);
		BOOL finished = ReadFile(request->file.native_handle(), request->buffer + request->accumulatedSize, (DWORD)(request->sizeToRead - request->accumulatedSize), &bytesRead, request);
		if (finished == TRUE)
		{
			return processIOCompletion(executor, sharedResources, bytesRead, request);
		}
		return GetLastError() != ERROR_IO_PENDING;
	}
	auto data = request->buffer + (request->start & ~(fileManager.sectorSize - 1u));
	request->completionEvent(request->hEvent, executor, sharedResources, data, request->file);
	{
		std::lock_guard<decltype(fileManager.mutex)> lock(fileManager.mutex);
		fileManager.requestAllocator.deallocate(request);
	}
	return true;
}