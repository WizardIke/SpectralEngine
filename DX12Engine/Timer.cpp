#include "Timer.h"
#include "UnkownCpuFrequencyException.h"

Timer::Timer() 
{
	uint64_t frequency;
	QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
	if (!frequency) { throw UnkownCpuFrequencyException(); }
}

Timer::~Timer() {}

void Timer::update()
{
	uint64_t currentTime;
	uint64_t frequency;
	QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
	QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
	frameTime = (float)(currentTime - startTime) / (float)(frequency);
	startTime = currentTime;
}

void Timer::start()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
	frameTime = 0.0f;
}