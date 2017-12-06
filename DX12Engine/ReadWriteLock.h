#pragma once
#include <Windows.h>

class ReadWriteLock
{
	SRWLOCK lock;
public:
	ReadWriteLock()
	{
		InitializeSRWLock(&lock);
	}

	void lockExclusive()
	{
		AcquireSRWLockExclusive(&lock);
	}

	void unlockExclusive()
	{
		ReleaseSRWLockExclusive(&lock);
	}

	void lockShared()
	{
		AcquireSRWLockShared(&lock);
	}

	void unlockShared()
	{
		ReleaseSRWLockShared(&lock);
	}

	~ReadWriteLock()
	{
	}
};