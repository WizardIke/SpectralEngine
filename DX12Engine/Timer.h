#pragma once
#include <chrono>

class Timer
{
	std::chrono::time_point<std::chrono::high_resolution_clock> oldTime;
	float mFrameTime;
public:
	void start();
	void update();
	float frameTime()
	{
		return mFrameTime;
	}
};