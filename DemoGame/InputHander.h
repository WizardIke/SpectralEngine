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
	bool f1Toggled = false;

	int previousMousePosX, previousMousePosY;

	void mouseMoved(long x, long y);

	void leftPressed(GlobalResources& globalResources) { leftDown = true; }
	void rightPressed(GlobalResources& globalResources) { rightDown = true; }
	void upPressed(GlobalResources& globalResources) { upDown = true; }
	void downPressed(GlobalResources& globalResources) { downDown = true; }
	void aPressed(GlobalResources& globalResources) { aDown = true; }
	void dPressed(GlobalResources& globalResources) { dDown = true; }
	void sPressed(GlobalResources& globalResources) { sDown = true; }
	void wPressed(GlobalResources& globalResources) { wDown = true; }
	void xPressed(GlobalResources& globalResources) { xDown = true; }
	void zPressed(GlobalResources& globalResources) { zDown = true; }
	void pageUpPressed(GlobalResources& globalResources) { pageUpDown = true; }
	void pageDownPressed(GlobalResources& globalResources) { pageDownDown = true; }
	void spacePressed(GlobalResources& globalResources) { spaceDown = true; }
	void escapePressed(GlobalResources& globalResources);
	void f1Pressed(GlobalResources& globalResources);
	void f2Pressed(GlobalResources& globalResources);

	void leftReleased(GlobalResources& globalResources) { leftDown = false; }
	void rightReleased(GlobalResources& globalResources) { rightDown = false; }
	void upReleased(GlobalResources& globalResources) { upDown = false; }
	void downReleased(GlobalResources& globalResources) { downDown = false; }
	void aReleased(GlobalResources& globalResources) { aDown = false; }
	void dReleased(GlobalResources& globalResources) { dDown = false; }
	void sReleased(GlobalResources& globalResources) { sDown = false; }
	void wReleased(GlobalResources& globalResources) { wDown = false; }
	void xReleased(GlobalResources& globalResources) { xDown = false; }
	void zReleased(GlobalResources& globalResources) { zDown = false; }
	void pageUpReleased(GlobalResources& globalResources) { pageUpDown = false; }
	void pageDownReleased(GlobalResources& globalResources) { pageDownDown = false; }
	void spaceReleased(GlobalResources& globalResources) { spaceDown = false; }
	void f1Released(GlobalResources& globalResources);
	void f2Released(GlobalResources& globalResources);
};