#pragma once
#include <unordered_map>
#include <cstdint>
#include "File.h"
#include "IOCompletionQueue.h"
#include <mutex>

class AsynchronousFileManager
{
public:
	struct Key
	{
		const wchar_t* name;
		std::size_t start;
		std::size_t end;

		bool operator==(const Key& other) const
		{
			return name == other.name && start == other.start && end == other.end;
		}
	};

	struct FileData
	{
		unsigned char* allocation;
	};

	struct Hasher
	{
		std::size_t operator()(const Key& key) const
		{
			std::size_t result = (std::size_t)key.name;
			result = result * 31u + key.start;
			result = result * 31u + key.end;
			return result;
		}
	};

	class IORequest : public OVERLAPPED
	{
	public:
		//location to read
		File file;
		const wchar_t* filename;
		std::size_t start;
		std::size_t end;
		//location to put the result
		unsigned char* buffer;
		//amount read
		std::size_t accumulatedSize;
		//what to do with the result
		void(*fileLoadedCallback)(IORequest& request, void* executor, void* sharedResources, const unsigned char* data);

		IORequest() {}
		IORequest(const wchar_t* filename, File file, std::size_t start, std::size_t end,
			void(*fileLoadedCallback)(IORequest& request, void* executor, void* sharedResources, const unsigned char* data)) :
			filename(filename),
			file(file),
			start(start),
			end(end),
			fileLoadedCallback(fileLoadedCallback)
		{}
	};
private:
	std::mutex mutex;
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

	bool readFile(void* executor, void* sharedResources, IORequest* request);
	void discard(const wchar_t* name, std::size_t start, std::size_t end);
};