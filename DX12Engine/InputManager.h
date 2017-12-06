#pragma once

#include "Window.h"

class InputManager
{
	InputManager(const InputManager&) = delete;
public:
	class MouseMotionObverver
	{
		void(&obververMouseMoved)(void* obverver, float deltaX, float deltaY);
		void* const obverver;
	public:
		MouseMotionObverver(void(&obververMouseMoved)(void* obverver, float deltaX, float deltaY), void* const obverver) : obververMouseMoved(obververMouseMoved), obverver(obverver) {}
		void mouseMoved(float deltaX, float deltaY)
		{
			obververMouseMoved(obverver, deltaX, deltaY);
		}
	};

	InputManager(Window& Window, MouseMotionObverver mouseMotionObverver);
	~InputManager();

	//make this into a bit field
	bool LeftPressed = false;
	bool RightPressed = false;
	bool UpPressed = false;
	bool DownPressed = false;
	bool APressed = false;
	bool DPressed = false;
	bool SPressed = false;
	bool WPressed = false;
	bool XPressed = false;
	bool ZPressed = false;
	bool PgUpPressed = false;
	bool PgDownPressed = false;
	bool SpacePressed = false;

	bool F1Toggled = false;
	bool F2Toggled = false;
	bool F3Toggled = false;
	bool F4Toggled = false;

	int previousMousePosX, previousMousePosY;

	void handleRawInput(LPARAM lParam);

private:
	const int screenWidth, screenHeight;
	MouseMotionObverver mouseMotionObverver;

	UINT rawInputBuffersize = 0u;
	unsigned char* rawInputBuffer;
};