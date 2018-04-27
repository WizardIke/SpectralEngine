#include "ScopedFile.h"

File::File(const wchar_t* fileName, DWORD accessRight, DWORD shareMode, creationMode creationMode, DWORD fileAttributes, HANDLE templateFile)
{
	open(fileName, accessRight, shareMode, creationMode, fileAttributes, templateFile);
}

File::File(const File& other)
{
	file = other.file;
}

void File::open(const wchar_t* fileName, DWORD accessRight, DWORD shareMode, creationMode creationMode, DWORD fileAttributes, HANDLE templateFile)
{
#ifdef _WIN32_WINNT
	file = CreateFileW(fileName, accessRight, shareMode, nullptr, creationMode, fileAttributes, templateFile);
	if (file == INVALID_HANDLE_VALUE) throw WindowsFileCreationException(HRESULT_FROM_WIN32(GetLastError()));
#else
	platform doesn't support ScopedFile
#endif
}

File::NotEndOfFile File::read(void* const buffer, uint32_t byteSize, LPOVERLAPPED overlapped)
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

File::NotEndOfFile File::read(void* const buffer, uint32_t byteSize)
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

void File::close()
{
	if (!CloseHandle(file)) throw IOException();
}

void File::setPosition(signed long long offset, Position pos)
{
	LONG highBits = (LONG)((offset & ~0xffffffff00000000) >> 32);
	SetFilePointer(file, offset & 0xffffffff, &highBits, pos);
}
unsigned long File::getPosition()
{
	return SetFilePointer(file, 0, nullptr, Position::current);
}

size_t File::size()
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

void File::write(void* const buffer, uint32_t byteSize)
{
	DWORD numberOfBytesWritten;
	BOOL result = WriteFile(file, buffer, byteSize, &numberOfBytesWritten, nullptr);
	if (result == FALSE || numberOfBytesWritten != byteSize)
	{
		throw IOException();
	}
}