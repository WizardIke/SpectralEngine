#pragma once
#include <Windows.h>
#include <fstream>
#include "IOException.h"
#include "WindowsFileCreationException.h"

class ScopedFile
{
public:
#ifdef _WIN32_WINNT
	HANDLE file;
#endif
	enum accessRight : DWORD
	{
		none = 0x00000000L,
		genericAll = 0x10000000L,
		genericExecute = 0x20000000L,
		genericRead = 0x80000000L,
		genericWrite = 0x40000000L,
		Delete = 0x00010000L,
		readControl = 0x00020000L,
		writeDiscretionary = 0x00040000L,
		writeOwner = 0x00080000L,
		synchronize = 0x00100000L,
		//this list is incomplete
	};

	enum shareMode : DWORD
	{
		deleteMode = 0x00000004,
		readMode = 0x00000001,
		writeMode = 0x00000002,
		noSharing = 0x00000000,
	};
	enum creationMode : DWORD
	{
		createNew = 1,
		overwriteOrCreate = 2,
		openExisting = 3,
		openOrCreate = 4,
		overwriteExisting = 5,
	};
	ScopedFile() : file(nullptr) {}
	ScopedFile(LPCWSTR fileName, DWORD accessRight, DWORD shareMode, creationMode creationMode, PCREATEFILE2_EXTENDED_PARAMETERS extendedParameter)
	{
		open(fileName, accessRight, shareMode, creationMode, extendedParameter);
	}

	void open(LPCWSTR fileName, DWORD accessRight, DWORD shareMode, creationMode creationMode, PCREATEFILE2_EXTENDED_PARAMETERS extendedParameter)
	{
#ifdef _WIN32_WINNT
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
		file = CreateFile2(fileName, accessRight, shareMode, creationMode, extendedParameter);
		if (file == INVALID_HANDLE_VALUE) throw WindowsFileCreationException(HRESULT_FROM_WIN32(GetLastError()));
#else
		file = CreateFileW(fileName, accessRight, shareMode, nullptr, creationMode, FILE_ATTRIBUTE_NORMAL, extendedParameter);
		if (file == INVALID_HANDLE_VALUE) throw FileNotFoundException();
#endif
#else
		platform doesn't support ScopedFile
#endif
	}

	enum NotEndOfFile : bool
	{
		True = true,
		False = false,
	};

	template <typename value_type>
	NotEndOfFile read(value_type& buffer, LPOVERLAPPED overlapped)
	{
		DWORD numBytesRead;
		BOOL result = ReadFile(file, &buffer, sizeof(buffer), &numBytesRead, overlapped);
		if (!result)
		{
			if (numBytesRead == 0u) return False;
			throw IOException();
		}
		return True;
	}

	template <typename value_type>
	NotEndOfFile read(value_type& buffer)
	{
		DWORD numBytesRead;
		BOOL result = ReadFile(file, &buffer, sizeof(buffer), &numBytesRead, nullptr);
		if (!result)
		{
			if (numBytesRead == 0u) return False;
			throw IOException();
		}
		return True;
	}

	NotEndOfFile read(void* const buffer, DWORD byteSize, LPOVERLAPPED overlapped)
	{
		DWORD numBytesRead;
		BOOL result = ReadFile(file, buffer, byteSize, &numBytesRead, overlapped);
		if (!result)
		{
			if (numBytesRead == 0u) return False;
			throw IOException();
		}
		return True;
	}

	NotEndOfFile read(void* const buffer, DWORD byteSize)
	{
		DWORD numBytesRead;
		BOOL result = ReadFile(file, buffer, byteSize, &numBytesRead, nullptr);
		if (!result)
		{
			if (numBytesRead == 0u) return False;
			throw IOException();
		}
		return True;
	}

	void close()
	{
		if (file)
		{
			if (!CloseHandle(file)) throw IOException();
			file = nullptr;
		}
	}

	ScopedFile& transfer()
	{
		ScopedFile* ret = this;
		this->file = nullptr;
		return *ret;
	}
};