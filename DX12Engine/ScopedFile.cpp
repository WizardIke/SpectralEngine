#include "ScopedFile.h"

ScopedFile::ScopedFile() : file(nullptr) {}
ScopedFile::ScopedFile(const wchar_t* fileName, DWORD accessRight, DWORD shareMode, creationMode creationMode, PCREATEFILE2_EXTENDED_PARAMETERS extendedParameter)
{
	open(fileName, accessRight, shareMode, creationMode, extendedParameter);
}

ScopedFile::ScopedFile(const ScopedFile& other)
{
	file = other.file;
}

void ScopedFile::open(const wchar_t* fileName, DWORD accessRight, DWORD shareMode, creationMode creationMode, PCREATEFILE2_EXTENDED_PARAMETERS extendedParameter)
{
#ifdef _WIN32_WINNT
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
	file = CreateFile2(fileName, accessRight, shareMode, creationMode, extendedParameter);
	if (file == INVALID_HANDLE_VALUE)
	{
		throw WindowsFileCreationException(HRESULT_FROM_WIN32(GetLastError()));
	}
#else
	file = CreateFileW(fileName, accessRight, shareMode, nullptr, creationMode, FILE_ATTRIBUTE_NORMAL, extendedParameter);
	if (file == INVALID_HANDLE_VALUE) throw FileNotFoundException();
#endif
#else
	platform doesn't support ScopedFile
#endif
}

ScopedFile::NotEndOfFile ScopedFile::read(void* const buffer, uint32_t byteSize, LPOVERLAPPED overlapped)
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

ScopedFile::NotEndOfFile ScopedFile::read(void* const buffer, uint32_t byteSize)
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

void ScopedFile::close()
{
	if (file)
	{
		if (!CloseHandle(file)) throw IOException();
		file = nullptr;
	}
}

void ScopedFile::setPosition(signed long long offset, Position pos)
{
	LONG highBits = (LONG)((offset & ~0xffffffff00000000) >> 32);
	SetFilePointer(file, offset & 0xffffffff, &highBits, pos);
}
unsigned long ScopedFile::getPosition()
{
	return SetFilePointer(file, 0, nullptr, Position::current);
}

size_t ScopedFile::size()
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