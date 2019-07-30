#pragma once
#include <InputManager.h>
#include <cassert>
class GlobalResources;

class InputHandler : public BaseInputHandler<GlobalResources>
{
	class MouseMotionObverver
	{
		void(*obververMouseMoved)(void* obverver, float deltaX, float deltaY);
		void* const obverver;
	public:
		MouseMotionObverver(void(*obververMouseMoved)(void* obverver, float deltaX, float deltaY), void* const obverver) :
			obververMouseMoved(obververMouseMoved), obverver(obverver)
		{
			assert(obververMouseMoved != nullptr);
		}
		void mouseMoved(float deltaX, float deltaY)
		{
			obververMouseMoved(obverver, deltaX, deltaY);
		}
	};

	const int screenWidth, screenHeight;
	MouseMotionObverver mouseMotionObverver;
public:
	InputHandler(Window& window, MouseMotionObverver mouseMotionObverver);
	//make this into a bit field
	bool leftDown = false;
	bool rightDown = false;
	bool upDown = false;
	bool downDown = false;
	bool aDown = false;
	bool dDown = false;
	bool sDown = false;
	bool wDown = false;
	bool xDown = false;
	bool zDown = false;
	bool pageUpDown = false;
	bool pageDownDown = false;
	bool spaceDown = false;
	bool f1Down = false;
	bool f2Down = false;
	bool f11Down = false;
	bool f1Toggled = false;

	int previousMousePosX, previousMousePosY;

	void mouseMoved(long x, long y);

	bool keyUp(UINT keyCode, UINT scanCode, GlobalResources& sharedResources);
	bool keyDown(UINT keyCode, UINT scanCode, GlobalResources& sharedResources);
};