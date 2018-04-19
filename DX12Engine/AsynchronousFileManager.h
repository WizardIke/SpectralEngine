#pragma once
#include <unordered_map>
#include <robin_hash.h>
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
	constexpr static size_t sectorSize = 4u * 1024u; //needs working out at run time
public:
	struct Key
	{
		const wchar_t* name;
		size_t start;
		size_t end;
	};

	struct FileData
	{
		uint8_t* allocation;
		uint8_t* data;
	};

	struct FileData2
	{
		Key key;

		uint8_t* allocation;
		uint8_t* data;
		FileData2* previous;
		FileData2* next;

		FileData2() {}

		FileData2(FileData2&& other) noexcept
		{
			data = other.data;
			next = other.next;
			previous = other.previous;

			next->previous = this;
			previous->next = this;
		}
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

	struct KeySelect
	{
		const Key& operator()(const FileData2& data) const
		{
			return data.key;
		}
	};

	struct ValueSelect
	{
		using value_type = FileData2;

		const FileData2& operator()(const FileData2& data) const
		{
			return data;
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
		void(*completionEvent)(void* requester, BaseExecutor* executor, SharedResources& sharedResources, const uint8_t* data, File file);
	};
private:
	std::mutex mutex;
	FixedSizeAllocator<IORequest> requestAllocator;
	std::unordered_map<Key, FileData, Hasher> pinnedFiles;
	tsl::detail_robin_hash::robin_hash<FileData2, KeySelect, ValueSelect,
		Hasher, std::equal_to<FileData2>, std::allocator<FileData2>, false, tsl::rh::power_of_two_growth_policy<2>> nonPinnedFiles;
	FileData2 filesHead;
	FileData2 filesTail;

	//Drops the least resently used items from the cache to make room for memoryNeeded
	void checkAndResizeCache(size_t memoryNeeded);
public:
	~AsynchronousFileManager();
	File openFileForReading(IOCompletionQueue& ioCompletionQueue, const wchar_t* name);
	bool readFile(BaseExecutor* executor, SharedResources& sharedResources, const wchar_t* name, size_t start, size_t end, File file,
		void* requester, void(*completionEvent)(void* requester, BaseExecutor* executor, SharedResources& sharedResources, const uint8_t* data, File file));
	void discard(const wchar_t* name, size_t start, size_t end);
	static bool processIOCompletion(BaseExecutor* executor, SharedResources& sharedResources, DWORD numberOfBytes, LPOVERLAPPED overlapped);
	void update();
};