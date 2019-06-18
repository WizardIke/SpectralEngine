#pragma once
#include <unordered_map>
#include <cstdint>
#include "File.h"
#include "IOCompletionQueue.h"
#include "SinglyLinked.h"

class AsynchronousFileManager
{
private:
	struct ResourceId
	{
		const wchar_t* filename;
		std::size_t start;
		std::size_t end;

		bool operator==(const ResourceId& other) const
		{
			return filename == other.filename && start == other.start && end == other.end;
		}
	};

	struct Hasher
	{
		std::size_t operator()(const ResourceId& key) const
		{
			std::size_t result = (std::size_t)key.filename;
			result = result * 31u + key.start;
			result = result * 31u + key.end;
			return result;
		}
	};
public:

	class ReadRequest : public OVERLAPPED, public SinglyLinked, public ResourceId
	{
	public:
		AsynchronousFileManager* asynchronousFileManager;
		//location to read
		File file;
		//location to put the result
		unsigned char* buffer;
		//amount read
		std::size_t accumulatedSize;
		//what to do with the result
		void(*fileLoadedCallback)(ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void* executor, void* sharedResources, const unsigned char* data);
		void(*deleteReadRequest)(ReadRequest& request, void* tr, void* gr);

		ReadRequest() {}
		ReadRequest(const wchar_t* filename, File file, std::size_t start, std::size_t end,
			void(*fileLoadedCallback)(ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void* executor, void* sharedResources, const unsigned char* data),
			void(*deleteRequest)(ReadRequest& request, void* executor, void* sharedResources)) :
			file(file),
			fileLoadedCallback(fileLoadedCallback),
			deleteReadRequest(deleteRequest)
		{
			this->filename = filename;
			this->start = start;
			this->end = end;
		}
	};
private:
	struct FileData
	{
		unsigned char* allocation;
		unsigned int userCount;
		ReadRequest* requests;
	};

	std::unordered_map<ResourceId, FileData, Hasher> files;
	IOCompletionQueue& ioCompletionQueue;
	std::size_t pageSize;

	static bool processIOCompletion(void* tr, void* gr, DWORD numberOfBytes, LPOVERLAPPED overlapped);
	static bool readFileHelper(void* tr, void* gr, DWORD, LPOVERLAPPED overlapped);
	static bool discardHelper(void* tr, void* gr, DWORD, LPOVERLAPPED overlapped);
public:
	AsynchronousFileManager(IOCompletionQueue& ioCompletionQueue);
	~AsynchronousFileManager();

	File openFileForReading(const wchar_t* name)
	{
		File file(name, File::accessRight::genericRead, File::shareMode::readMode, File::creationMode::openExisting, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING);
		ioCompletionQueue.associateFile(file.native_handle(), (ULONG_PTR)(processIOCompletion));
		return file;
	}

	void readFile(ReadRequest& request);
	void discard(ReadRequest& request);
};