#pragma once

#include <windows.h>
#include <stdint.h>

class Timer
{
public:
	Timer();
	~Timer();

	void start();
	void update();
	float frameTime;
private:
	Timer(const Timer&) = delete;
	uint64_t startTime;
};