#include "GlobalResources.h"
#include <GraphicsAdapterNotFound.h>

#ifdef _WIN32
INT WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
#else
int main()
#endif
{
#ifdef _WIN32
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif
	try
	{
		GlobalResources game{};
		game.start();
#ifdef _WIN32
		CoUninitialize();
#endif
	}
	catch(GraphicsAdapterNotFound) 
	{
#ifdef _WIN32
		CoUninitialize();
#endif
	}
	catch (...)
	{
#ifdef _WIN32
		CoUninitialize();
#endif
	}
}