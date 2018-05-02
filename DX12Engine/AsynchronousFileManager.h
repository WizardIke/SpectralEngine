#pragma once
#include <unordered_map>
#include <cstdint>
#include "ScopedFile.h"
#include "IOCompletionQueue.h"
#include "Range.h"
#include <mutex>
#include "FixedSizeAllocator.h"
class BaseExecutor;
class SharedResources;

class AsynchronousFileManager
{
public:
	struct Key
	{
		const wchar_t* name;
		size_t start;
		size_t end;

		bool operator==(const Key& other) const
		{
			return name == other.name && start == other.start && end == other.end;
		}
	};

	struct FileData
	{
		uint8_t* allocation;
		uint8_t* data;
		std::size_t allocationSize;
	};

	struct Hasher
	{
		size_t operator()(const Key& key) const
		{
			size_t result = (size_t)key.name;
			result = result * 31u + key.start;
			result = result * 31u + key.end;
			return result;
		}
	};

	struct IORequest : OVERLAPPED
	{
		const wchar_t* name;
		uint8_t* buffer;
		size_t start;
		size_t end;
		size_t sizeToRead;
		size_t accumulatedSize;
		size_t memoryStart;
		File file;
		void* requester;
		void(*completionEvent)(void* requester, BaseExecutor* executor, SharedResources& sharedResources, const uint8_t* data, File file);
	};
private:
	std::mutex mutex;
	FixedSizeAllocator<IORequest> requestAllocator;
	using HashMap = std::unordered_map<Key, FileData, Hasher>;
	HashMap files;
	std::size_t sectorSize;
public:
	AsynchronousFileManager();
	~AsynchronousFileManager();
	File openFileForReading(IOCompletionQueue& ioCompletionQueue, const wchar_t* name);
	bool readFile(BaseExecutor* executor, SharedResources& sharedResources, const wchar_t* name, size_t start, size_t end, File file,
		void* requester, void(*completionEvent)(void* requester, BaseExecutor* executor, SharedResources& sharedResources, const uint8_t* data, File file));
	void discard(const wchar_t* name, size_t start, size_t end);
	static bool processIOCompletion(BaseExecutor* executor, SharedResources& sharedResources, DWORD numberOfBytes, LPOVERLAPPED overlapped);
};