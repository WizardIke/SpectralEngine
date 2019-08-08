#include "GlobalResources.h"
#include <GraphicsAdapterNotFound.h>

#ifdef _WIN32
namespace
{
	struct ComEnvironment
	{
		ComEnvironment()
		{
			CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		}

		~ComEnvironment()
		{
			CoUninitialize();
		}
	};
}
#endif

#ifdef _WIN32
INT WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
#else
int main()
#endif
{
#ifdef _WIN32
	ComEnvironment com;
#endif
	try
	{
		GlobalResources game{};
		game.start();
	}
	catch(GraphicsAdapterNotFound) {}
}