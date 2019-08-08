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
			if constexpr (sizeof(std::size_t) <= 4u)
			{
				std::size_t result = 2166136261u;
				for (auto current = key.filename; *current != '\0'; ++current)
				{
					result = (result ^ std::size_t{ *current }) * 16777619u;
				}
				result = result * 31u + key.start;
				result = result * 31u + key.end;
				return result;
			}
			else
			{
				std::size_t result = 14695981039346656037u;
				for (auto current = key.filename; *current != '\0'; ++current)
				{
					result = (result ^ std::size_t{ *current }) * 1099511628211u;
				}
				result = result * 31u + key.start;
				result = result * 31u + key.end;
				return result;
			}
		}
	};
public:

	class ReadRequest : public OVERLAPPED, public SinglyLinked, public ResourceId
	{
	public:
		AsynchronousFileManager* asynchronousFileManager;
		File file;
		//location to put the result
		unsigned char* buffer;
		//amount read
		std::size_t accumulatedSize;
		//what to do with the result
		void(*fileLoadedCallback)(ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void* tr, const unsigned char* data);
		void(*deleteReadRequest)(ReadRequest& request, void* tr);

		ReadRequest() {}
		ReadRequest(const wchar_t* filename, File file, std::size_t start, std::size_t end,
			void(*fileLoadedCallback)(ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void* tr, const unsigned char* data),
			void(*deleteRequest)(ReadRequest& request, void* tr)) :
			file(file),
			fileLoadedCallback(fileLoadedCallback),
			deleteReadRequest(deleteRequest)
		{
			this->filename = filename;
			this->start = start;
			this->end = end;
		}

		ReadRequest(const wchar_t* filename, File file, std::size_t start, std::size_t end,
			void(*fileLoadedCallback)(ReadRequest& request, AsynchronousFileManager& asynchronousFileManager, void* tr, const unsigned char* data)) :
			file(file),
			fileLoadedCallback(fileLoadedCallback)
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

	static bool processIOCompletion(void* tr, DWORD numberOfBytes, LPOVERLAPPED overlapped);
	static bool readFileHelper(void* tr, DWORD, LPOVERLAPPED overlapped);
	static bool discardHelper(void* tr, DWORD, LPOVERLAPPED overlapped);
public:
	AsynchronousFileManager(IOCompletionQueue& ioCompletionQueue);
	~AsynchronousFileManager();

	File openFileForReading(const wchar_t* name)
	{
		File file(name, File::accessRight::genericRead, File::shareMode::readMode, File::creationMode::openExisting, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING);
		ioCompletionQueue.associateFile(file.native_handle(), (ULONG_PTR)(processIOCompletion));
		return file;
	}

	IOCompletionQueue& getIoCompletionQueue()
	{
		return ioCompletionQueue;
	}

	void readFile(ReadRequest& request);
	void discard(ReadRequest& request);
};