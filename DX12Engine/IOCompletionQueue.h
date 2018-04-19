#pragma once
#include <Windows.h>
#undef max
#undef min
#include <limits>

struct IOCompletionPacket
{
	DWORD numberOfBytesTransfered;
	ULONG_PTR completionKey;
	OVERLAPPED* overlapped;
};

class IOCompletionQueue
{
	HANDLE queueHandle;
public:
	IOCompletionQueue(unsigned long maxConcurrentThreads = std::numeric_limits<DWORD>::max())
	{
		queueHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0u, maxConcurrentThreads);
	}
	~IOCompletionQueue()
	{
		CloseHandle(queueHandle);
	}
	/*
	 * Sends a message to this IOCompletionQueue whenever asynchronous IO completes for the file.
	 * A file can only be associated to one IOCompletionQueue and the association lasts until the file handle is closed.
	 * completionKey is in every IO completion packet from the associated file
	 */
	bool associateFile(HANDLE file, ULONG_PTR completionKey = 0u)
	{
		return CreateIoCompletionPort(file, queueHandle, 0u, 0u) != nullptr;
	}

	/*
	 * Retrieves an IOCompletionPacket from this IOCompletionQueue if one is queued.
	 * Also associates the current thread with this IOCompletionQueue. A thread can only be associated with one IOCompletionQueue
	 */
	bool removeIOCompletionPacket(IOCompletionPacket& completionPacket, unsigned long timeoutInMilliseconds = 0u)
	{
		return GetQueuedCompletionStatus(queueHandle, &completionPacket.numberOfBytesTransfered, &completionPacket.completionKey, &completionPacket.overlapped, timeoutInMilliseconds) == TRUE;
	}

	bool addIOCompletionPacket(const IOCompletionPacket& completionPacket)
	{
		return PostQueuedCompletionStatus(queueHandle, completionPacket.numberOfBytesTransfered, completionPacket.completionKey, completionPacket.overlapped) == TRUE;
	}
};