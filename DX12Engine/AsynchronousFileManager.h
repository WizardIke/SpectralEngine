#pragma once
#include <unordered_map>
#include <cstdint>
#include "File.h"
#include "IOCompletionQueue.h"
#include <mutex>
#include "FixedSizeAllocator.h"
#include "Delegate.h"

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
		//location to read
		File file;
		const wchar_t* name;
		size_t start;
		size_t end;
		//location to put the result
		uint8_t* buffer;
		//amount read
		size_t accumulatedSize;
		//what to do with the result
		Delegate<void(void* executor, void* sharedResources, const uint8_t* data, File file)> callback;
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

	static bool processIOCompletionHelper(AsynchronousFileManager& fileManager, void* executor, void* sharedResources, DWORD numberOfBytes, LPOVERLAPPED overlapped);
	template<class GlobalResources>
	static bool processIOCompletion(void* executor, void* sharedResources, DWORD numberOfBytes, LPOVERLAPPED overlapped)
	{
		GlobalResources& globalResources = *reinterpret_cast<GlobalResources*>(sharedResources);
		return processIOCompletionHelper(globalResources.asynchronousFileManager, executor, sharedResources, numberOfBytes, overlapped);
	}

	template<class GlobalResources>
	File openFileForReading(IOCompletionQueue& ioCompletionQueue, const wchar_t* name)
	{
		File file(name, File::accessRight::genericRead, File::shareMode::readMode, File::creationMode::openExisting, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING);
		ioCompletionQueue.associateFile(file.native_handle(), (ULONG_PTR)(void*)(processIOCompletion<GlobalResources>));
		return file;
	}

	bool readFile(void* executor, void* sharedResources, const wchar_t* name, size_t start, size_t end, File file,
		void* requester, void(*completionEvent)(void* requester, void* executor, void* sharedResources, const uint8_t* data, File file));
	void discard(const wchar_t* name, size_t start, size_t end);
};