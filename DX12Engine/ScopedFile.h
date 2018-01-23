#pragma once
#include <Windows.h>
#include <fstream>
#include "IOException.h"
#include "WindowsFileCreationException.h"

class ScopedFile
{
#ifdef _WIN32_WINNT
	HANDLE file;
#endif

	ScopedFile(HANDLE file) : file(file) {}
public:
	enum accessRight : uint32_t
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

	enum shareMode : uint32_t
	{
		deleteMode = 0x00000004,
		readMode = 0x00000001,
		writeMode = 0x00000002,
		noSharing = 0x00000000,
	};
	enum creationMode : uint32_t
	{
		createNew = 1,
		overwriteOrCreate = 2,
		openExisting = 3,
		openOrCreate = 4,
		overwriteExisting = 5,
	};

	enum Position : DWORD
	{
		start = FILE_BEGIN,
		current = FILE_CURRENT,
		end = FILE_END,
	};

	enum NotEndOfFile : bool
	{
		True = true,
		False = false,
	};

	ScopedFile();
	ScopedFile(const wchar_t* fileName, DWORD accessRight, DWORD shareMode, creationMode creationMode, PCREATEFILE2_EXTENDED_PARAMETERS extendedParameter);
	ScopedFile(const ScopedFile& other);
	void open(const wchar_t* fileName, DWORD accessRight, DWORD shareMode, creationMode creationMode, PCREATEFILE2_EXTENDED_PARAMETERS extendedParameter);

	template <typename value_type>
	NotEndOfFile read(value_type& buffer, LPOVERLAPPED overlapped)
	{
		return read(&buffer, sizeof(buffer), overlapped);
	}

	template <typename value_type>
	NotEndOfFile read(value_type& buffer)
	{
		return read(&buffer, sizeof(buffer));
	}

	NotEndOfFile read(void* const buffer, uint32_t byteSize, LPOVERLAPPED overlapped);
	NotEndOfFile read(void* const buffer, uint32_t byteSize);
	void close();
	void setPosition(signed long long offset, Position pos);
	unsigned long getPosition();
	size_t size();
};