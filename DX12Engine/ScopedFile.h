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
	ScopedFile() : file(nullptr) {}
	ScopedFile(const wchar_t* fileName, DWORD accessRight, DWORD shareMode, creationMode creationMode, PCREATEFILE2_EXTENDED_PARAMETERS extendedParameter)
	{
		open(fileName, accessRight, shareMode, creationMode, extendedParameter);
	}

	ScopedFile(const ScopedFile& other)
	{
		file = other.file;
	}

	void open(const wchar_t* fileName, DWORD accessRight, DWORD shareMode, creationMode creationMode, PCREATEFILE2_EXTENDED_PARAMETERS extendedParameter)
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

	NotEndOfFile read(void* const buffer, uint32_t byteSize, LPOVERLAPPED overlapped)
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

	NotEndOfFile read(void* const buffer, uint32_t byteSize)
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

	enum Position : DWORD
	{
		start = FILE_BEGIN,
		current = FILE_CURRENT,
		end = FILE_END,
	};

	void setPosition(signed long long offset, Position pos)
	{
		LONG highBits = (LONG)((offset & ~0xffffffff00000000) >> 32);
		SetFilePointer(file, offset & 0xffffffff, &highBits, pos);
	}
	unsigned long getPosition()
	{
		return SetFilePointer(file, 0, nullptr, Position::current);
	}

	size_t size()
	{
		LARGE_INTEGER FileSize = { 0 };
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
		FILE_STANDARD_INFO fileInfo;
		if (!GetFileInformationByHandleEx(file, FileStandardInfo, &fileInfo, sizeof(fileInfo)))
		{
			throw HresultException(HRESULT_FROM_WIN32(GetLastError()));
		}
		FileSize = fileInfo.EndOfFile;
#else
		GetFileSizeEx(TextureFile.file, &FileSize);
#endif
		constexpr size_t ltSize = sizeof(LARGE_INTEGER); //visual studio complains about sizeof(LARGE_INTEGER) not being a constant expression if written in the #if
		constexpr size_t sSize = sizeof(size_t);
#if (ltSize > sSize)
		return (size_t)FileSize.LowPart;
#else
		return (size_t)FileSize.QuadPart;
#endif
		
	}
};