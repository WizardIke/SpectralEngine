#include "GlobalResources.h"
#include <memory>

#ifdef _WIN32
INT WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
#else
int main()
#endif
{
	GlobalResources game{};
	game.start();
}