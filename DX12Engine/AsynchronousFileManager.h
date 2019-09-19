#pragma once
#include <unordered_map>
#include <cstdint>
#include "File.h"
#include "IOCompletionQueue.h"
#include "SinglyLinked.h"
#include <Windows.h>
#undef min
#undef max

class AsynchronousFileManager
{
private:
	struct ResourceId
	{
		unsigned long long start;
		unsigned long long end;

		bool operator==(const ResourceId& other) const
		{
			return start == other.start && end == other.end;
		}
	};

	struct Hasher
	{
		std::size_t operator()(const ResourceId& key) const
		{
			if constexpr (sizeof(std::size_t) <= 4u)
			{
				unsigned long result = static_cast<unsigned long>(key.start);
				result = result * 31ul + static_cast<unsigned long>(key.start >> 32ull);
				result = result * 31ul + static_cast<unsigned long>(key.end);
				result = result * 31ul + static_cast<unsigned long>(key.end >> 32ull);
				return static_cast<std::size_t>(result);
			}
			else
			{
				unsigned long long result = key.start;
				result = result * 31ull + key.end;
				return static_cast<std::size_t>(result);
			}
		}
	};
public:
	class ReadRequest : public OVERLAPPED, public SinglyLinked, public ResourceId
	{
	public:
		AsynchronousFileManager* asynchronousFileManager;
		//location to put the result
		unsigned char* buffer;
		//amount read
		unsigned long long accumulatedSize;
		//what to do with the result
		void(*fileLoadedCallback)(ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void* tr, const unsigned char* data);
		void(*deleteReadRequest)(ReadRequest& request, void* tr);

		ReadRequest() {}
		ReadRequest(unsigned long long start, unsigned long long end,
			void(*fileLoadedCallback)(ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void* tr, const unsigned char* data),
			void(*deleteRequest)(ReadRequest& request, void* tr)) :
			fileLoadedCallback(fileLoadedCallback),
			deleteReadRequest(deleteRequest)
		{
			this->start = start;
			this->end = end;
		}

		ReadRequest(unsigned long long start, unsigned long long end,
			void(*fileLoadedCallback)(ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void* tr, const unsigned char* data)) :
			fileLoadedCallback(fileLoadedCallback)
		{
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

	File file;
	std::unordered_map<ResourceId, FileData, Hasher> resources;
	IOCompletionQueue& ioCompletionQueue;
	unsigned long long pageSize;

	static bool processIOCompletion(void* tr, DWORD numberOfBytes, LPOVERLAPPED overlapped);
	static bool readFileHelper(void* tr, DWORD, LPOVERLAPPED overlapped);
	static bool discardHelper(void* tr, DWORD, LPOVERLAPPED overlapped);

	static File openFileForReading(const wchar_t* name, IOCompletionQueue& ioCompletionQueue)
	{
		File file(name, File::accessRight::genericRead, File::shareMode::readMode, File::creationMode::openExisting, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING);
		ioCompletionQueue.associateFile(file.native_handle(), (ULONG_PTR)(processIOCompletion));
		return file;
	}
public:
	AsynchronousFileManager(IOCompletionQueue& ioCompletionQueue, const wchar_t* fileName);
	~AsynchronousFileManager();

	IOCompletionQueue& getIoCompletionQueue()
	{
		return ioCompletionQueue;
	}

	void read(ReadRequest& request);
	void discard(ReadRequest& request);
};