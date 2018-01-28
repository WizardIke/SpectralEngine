#pragma once
#include <InputManager.h>
#include <cassert>
class SharedResources;

class InputHandler : public BaseInputHandler
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

	bool F1Toggled = false;
	bool F2Toggled = false;
	bool F3Toggled = false;
	bool F4Toggled = false;

	int previousMousePosX, previousMousePosY;

	void mouseMoved(long x, long y);

	void leftPressed(SharedResources& sharedResources) { leftDown = true; }
	void rightPressed(SharedResources& sharedResources) { rightDown = true; }
	void upPressed(SharedResources& sharedResources) { upDown = true; }
	void downPressed(SharedResources& sharedResources) { downDown = true; }
	void aPressed(SharedResources& sharedResources) { aDown = true; }
	void dPressed(SharedResources& sharedResources) { dDown = true; }
	void sPressed(SharedResources& sharedResources) { sDown = true; }
	void wPressed(SharedResources& sharedResources) { wDown = true; }
	void xPressed(SharedResources& sharedResources) { xDown = true; }
	void zPressed(SharedResources& sharedResources) { zDown = true; }
	void pageUpPressed(SharedResources& sharedResources) { pageUpDown = true; }
	void pageDownPressed(SharedResources& sharedResources) { pageDownDown = true; }
	void spacePressed(SharedResources& sharedResources) { spaceDown = true; }
	void escapePressed(SharedResources& sharedResources);

	void leftReleased(SharedResources& sharedResources) { leftDown = false; }
	void rightReleased(SharedResources& sharedResources) { rightDown = false; }
	void upReleased(SharedResources& sharedResources) { upDown = false; }
	void downReleased(SharedResources& sharedResources) { downDown = false; }
	void aReleased(SharedResources& sharedResources) { aDown = false; }
	void dReleased(SharedResources& sharedResources) { dDown = false; }
	void sReleased(SharedResources& sharedResources) { sDown = false; }
	void wReleased(SharedResources& sharedResources) { wDown = false; }
	void xReleased(SharedResources& sharedResources) { xDown = false; }
	void zReleased(SharedResources& sharedResources) { zDown = false; }
	void pageUpReleased(SharedResources& sharedResources) { pageUpDown = false; }
	void pageDownReleased(SharedResources& sharedResources) { pageDownDown = false; }
	void spaceReleased(SharedResources& sharedResources) { spaceDown = false; }
};