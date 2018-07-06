#include "GlobalResources.h"
#include <memory>

#ifdef _WIN32
INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
#else
int main()
#endif
{
	std::unique_ptr<GlobalResources> game(new GlobalResources());
	game->start();
}