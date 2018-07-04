#include "GlobalResources.h"
#include <memory>

#ifdef _WIN32
#ifdef UNICODE
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pScmdline, int iCmdshow)
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
#endif
#else
int main()
#endif
{
	std::unique_ptr<GlobalResources> game(new GlobalResources());
	game->start();
}