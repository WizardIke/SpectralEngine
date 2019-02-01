#include "GlobalResources.h"
#include <GraphicsAdapterNotFound.h>

#ifdef _WIN32
INT WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
#else
int main()
#endif
{
	try
	{
		GlobalResources game{};
		game.start();
	}
	catch(GraphicsAdapterNotFound) {}
}