#include "AsynchronousFileManager.h"
#include <Windows.h>

AsynchronousFileManager::AsynchronousFileManager()
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	sectorSize = systemInfo.dwPageSize;
}

AsynchronousFileManager::~AsynchronousFileManager() {}

bool AsynchronousFileManager::readFile(void* executor, void* sharedResources, IORequest* request)
{
	size_t memoryStart = request->start & ~(sectorSize - 1u);
	size_t memoryEnd = (request->end + sectorSize - 1u) & ~(sectorSize - 1u);
	size_t memoryNeeded = memoryEnd - memoryStart;

	unsigned char* allocation;
	bool dataFound = false;
	Key key{request->filename, request->start, request->end };
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
			allocation = (unsigned char*)VirtualAlloc(nullptr, memoryNeeded, MEM_COMMIT, PAGE_READWRITE);
			files.insert(std::pair<const Key, FileData>(key, FileData{ allocation }));
		}
	}
	
	if (dataFound)
	{
		auto result = ReclaimVirtualMemory(allocation, memoryNeeded);
		if (result == ERROR_SUCCESS)
		{
			auto data = allocation + request->start - memoryStart;
			request->fileLoadedCallback(*request, executor, sharedResources, data);
			return true;
		}
		else if(result != ERROR_BUSY) //ERROR_BUSY means the virtual memory was reclaimed but not its contents.
		{
			return false;
		}
	}

	request->buffer = allocation;
	request->accumulatedSize = 0u;
	request->hEvent = nullptr;
	request->Offset = memoryStart & (std::numeric_limits<DWORD>::max() - 1u);
	request->OffsetHigh = (DWORD)((memoryStart & ~(size_t)(std::numeric_limits<DWORD>::max() - 1u)) >> 32u);

	DWORD bytesRead = 0u;
	BOOL finished = ReadFile(request->file.native_handle(), request->buffer, (DWORD)memoryNeeded, &bytesRead, request);
	if (finished == FALSE && GetLastError() != ERROR_IO_PENDING)
	{
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
	IORequest* request = static_cast<IORequest*>(overlapped);
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
			return false;
		}
		return true;
	}
	auto data = request->buffer + (request->start & (fileManager.sectorSize - 1u));
	request->fileLoadedCallback(*request, executor, sharedResources, data);
	return true;
}