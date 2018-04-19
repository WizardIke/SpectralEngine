#include "AsynchronousFileManager.h"
#include "SharedResources.h"

void AsynchronousFileManager::checkAndResizeCache(size_t memoryNeeded)
{
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);

	auto availableMemory = (DWORDLONG)(std::min(statex.ullAvailPhys, statex.ullAvailVirtual) * 0.8);
	while ((availableMemory < memoryNeeded || statex.dwMemoryLoad > 90u) && filesHead.next != &filesTail)
	{
		auto data = filesTail.previous;
		VirtualFree(data->allocation, 0u, MEM_RELEASE);
		filesTail.previous = data->previous;
		filesTail.previous->next = &filesTail;
		nonPinnedFiles.erase(data);
		GlobalMemoryStatusEx(&statex);
	}
}

void AsynchronousFileManager::update()
{
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);

	auto availableMemory = (DWORDLONG)(std::min(statex.ullAvailPhys, statex.ullAvailVirtual) * 0.8);
	while (statex.dwMemoryLoad > 90u && filesHead.next != &filesTail)
	{
		auto data = filesTail.previous;
		VirtualFree(data->allocation, 0u, MEM_RELEASE);
		filesTail.previous = data->previous;
		filesTail.previous->next = &filesTail;
		nonPinnedFiles.erase(data);
		GlobalMemoryStatusEx(&statex);
	}
}

AsynchronousFileManager::~AsynchronousFileManager()
{
	while (filesHead.next != &filesTail)
	{
		auto data = filesTail.previous;
		VirtualFree(data->allocation, 0u, MEM_RELEASE);
		filesTail.previous = data->previous;
		filesTail.previous->next = &filesTail;
	}
}

File AsynchronousFileManager::openFileForReading(IOCompletionQueue& ioCompletionQueue, const wchar_t* name)
{
	File file(name, File::accessRight::genericRead, File::shareMode::readMode, File::creationMode::openExisting, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING);
	ioCompletionQueue.associateFile(file.native_handle(), (ULONG_PTR)processIOCompletion);
	return file;
}

bool AsynchronousFileManager::readFile(BaseExecutor* executor, SharedResources& sharedResources, const wchar_t* name, size_t start, size_t end, File file,
	void* requester, void(*completionEvent)(void* requester, BaseExecutor* executor, SharedResources& sharedResources, const uint8_t* data, File file))
{
	IORequest* request;
	{
		std::lock_guard<decltype(mutex)> lock(mutex);
		Key key{ name, start, end };
		auto dataPtr = nonPinnedFiles.find(key);
		if (dataPtr != nonPinnedFiles.end())
		{
			FileData2& oldData = dataPtr.value();
			oldData.next->previous = oldData.previous;
			oldData.previous->next = oldData.next;
			pinnedFiles.insert(std::make_pair(key, FileData{ oldData.allocation,  oldData.data }));
			auto data = oldData.data;
			nonPinnedFiles.erase(dataPtr);
			completionEvent(requester, executor, sharedResources, data, file);
			return true;
		}
		request = requestAllocator.allocate();
	}
	size_t memoryStart = start & ~(sectorSize - 1u);
	size_t memoryEnd = (end + sectorSize - 1u) & ~(sectorSize - 1u);
	size_t memoryNeeded = memoryEnd - memoryStart;
	checkAndResizeCache(memoryNeeded);
	uint8_t* allocation = (uint8_t*)VirtualAlloc(nullptr, memoryNeeded, MEM_RESERVE, PAGE_READWRITE);
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
	request->OffsetHigh = (memoryStart & ~(std::numeric_limits<DWORD>::max() - 1u)) >> 32u;

	DWORD bytesRead = 0u;
	BOOL finished = ReadFile(file.native_handle(), request->buffer, request->sizeToRead, &bytesRead, request);
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
	auto data = pinnedFiles.find(key);
	FileData2 data2;

	data2.allocation = data->second.allocation;
	data2.data = data->second.data;

	data2.next = filesHead.next;
	data2.previous = &filesHead;

	filesHead.next->previous = &data2;
	filesHead.next = &data2;

	nonPinnedFiles.insert(std::pair<Key, FileData2>{key, std::move(data2)});
	pinnedFiles.erase(data);
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
		request->OffsetHigh = (request->memoryStart & ~(std::numeric_limits<DWORD>::max() - 1u)) >> 32u;
		BOOL finished = ReadFile(request->file.native_handle(), request->buffer + request->accumulatedSize, request->sizeToRead - request->accumulatedSize, &bytesRead, request);
		if (finished == true)
		{
			return processIOCompletion(executor, sharedResources, bytesRead, request);
		}
		return GetLastError() != ERROR_IO_PENDING;
	}
	auto data = request->buffer + (request->start & ~(sectorSize - 1u));
	request->completionEvent(request->hEvent, executor, sharedResources, data, request->file);
	{
		std::lock_guard<decltype(fileManager.mutex)> lock(fileManager.mutex);
		fileManager.pinnedFiles.insert(std::pair<const Key, FileData>(Key{ request->name, request->start, request->end }, FileData{ request->buffer, data }));
		fileManager.requestAllocator.deallocate(request);
	}
	return true;
}