#include "AsynchronousFileManager.h"
#include <Windows.h>

AsynchronousFileManager::AsynchronousFileManager()
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	sectorSize = systemInfo.dwPageSize;
}

AsynchronousFileManager::~AsynchronousFileManager() {}

bool AsynchronousFileManager::readFile(void* executor, void* sharedResources, const wchar_t* name, size_t start, size_t end, File file,
	void* requester, void(*completionEvent)(void* requester, void* executor, void* sharedResources, const uint8_t* data, File file))
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
			allocation = (uint8_t*)VirtualAlloc(nullptr, memoryNeeded, MEM_COMMIT, PAGE_READWRITE);
			auto data = allocation + start - memoryStart;
			files.insert(std::pair<const Key, FileData>(key, FileData{ allocation }));
		}
	}
	
	if (dataFound)
	{
		auto result = ReclaimVirtualMemory(allocation, memoryNeeded);
		if (result == ERROR_SUCCESS)
		{
			auto data = allocation + start - memoryStart;
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
	request->accumulatedSize = 0u;
	request->callback = { requester, completionEvent };
	request->hEvent = nullptr;
	request->file = file;
	request->Offset = memoryStart & (std::numeric_limits<DWORD>::max() - 1u);
	request->OffsetHigh = (DWORD)((memoryStart & ~(size_t)(std::numeric_limits<DWORD>::max() - 1u)) >> 32u);

	DWORD bytesRead = 0u;
	BOOL finished = ReadFile(file.native_handle(), request->buffer, (DWORD)memoryNeeded, &bytesRead, request);
	if (finished == FALSE && GetLastError() != ERROR_IO_PENDING)
	{
#ifndef ndebug
		auto lastError = GetLastError();
#endif
		return false;
	}
	return true;
}

void AsynchronousFileManager::discard(const wchar_t* name, size_t start, size_t end)
{
	Key key{ name, start, end };
	size_t memoryStart = start & ~(sectorSize - 1u);
	size_t memoryEnd = (end + sectorSize - 1u) & ~(sectorSize - 1u);
	size_t memoryNeeded = memoryEnd - memoryStart;
	FileData fileData;
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		fileData = files.find(key)->second;
	}
	OfferVirtualMemory(fileData.allocation, memoryNeeded, OFFER_PRIORITY::VmOfferPriorityLow);
}

 bool AsynchronousFileManager::processIOCompletionHelper(AsynchronousFileManager& fileManager, void* executor, void* sharedResources, DWORD numberOfBytes, LPOVERLAPPED overlapped)
{
	IORequest* request = reinterpret_cast<IORequest*>(overlapped);
	request->accumulatedSize += numberOfBytes;
	const std::size_t sectorSize = fileManager.sectorSize;
	const std::size_t memoryStart = request->start & ~(sectorSize - 1u);
	const std::size_t memoryEnd = (request->end + sectorSize - 1u) & ~(sectorSize - 1u);
	const std::size_t sizeToRead = memoryEnd - memoryStart;
	if ((request->accumulatedSize < (request->end - memoryStart)) && request->accumulatedSize != sizeToRead)
	{
		request->accumulatedSize = request->accumulatedSize & ~(sectorSize - 1u);
		const std::size_t currentPosition = memoryStart + request->accumulatedSize;
		request->Offset = currentPosition & (std::numeric_limits<DWORD>::max() - 1u);
		request->OffsetHigh = (DWORD)((currentPosition & ~(size_t)(std::numeric_limits<DWORD>::max() - 1u)) >> 32u);
		DWORD bytesRead = 0u;
		BOOL finished = ReadFile(request->file.native_handle(), request->buffer + request->accumulatedSize, (DWORD)(sizeToRead - request->accumulatedSize), &bytesRead, request);
		if (finished == FALSE && GetLastError() != ERROR_IO_PENDING)
		{
#ifndef NDEBUG
			auto lastError = GetLastError();
#endif
			return false;
		}
		return true;
	}
	auto data = request->buffer + (request->start & (fileManager.sectorSize - 1u));
	File file = request->file;
	{
		std::lock_guard<decltype(mutex)> lock(fileManager.mutex);
		fileManager.requestAllocator.deallocate(request);
	}
	request->callback(executor, sharedResources, data, request->file);
	return true;
}