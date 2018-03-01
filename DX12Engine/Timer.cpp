#include "Timer.h"

void Timer::start()
{
	oldTime = std::chrono::high_resolution_clock::now();
	mFrameTime = 0.f;
}
void Timer::update()
{
	auto newTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> diff = newTime - oldTime;
	mFrameTime = diff.count();
	oldTime = newTime;
}