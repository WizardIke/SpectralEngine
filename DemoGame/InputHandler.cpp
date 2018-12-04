#include "InputHander.h"
#include "GlobalResources.h"
#include "ThreadResources.h"


InputHandler::InputHandler(Window& Window, MouseMotionObverver mouseMotionObverver) : previousMousePosX(0), previousMousePosY(0), screenWidth(Window.width()),
screenHeight(Window.height()), mouseMotionObverver(mouseMotionObverver) {}

void InputHandler::mouseMoved(long x, long y)
{
	mouseMotionObverver.mouseMoved(static_cast<float>(x) / screenWidth * 10.0f, static_cast<float>(y) / screenWidth * 10.0f);
}

bool InputHandler::keyDown(UINT keyCode, UINT, GlobalResources& globalResources)
{
	switch(keyCode)
	{
	case Keys::left:
		leftDown = true;
		return true;
	case Keys::right:
		rightDown = true;
		return true;
	case Keys::up:
		upDown = true;
		return true;
	case Keys::down:
		downDown = true;
		return true;
	case Keys::a:
		aDown = true;
		return true;
	case Keys::d:
		dDown = true;
		return true;
	case Keys::s:
		sDown = true;
		return true;
	case Keys::w:
		wDown = true;
		return true;
	case Keys::x:
		xDown = true;
		return true;
	case Keys::z:
		zDown = true;
		return true;
	case Keys::pageUp:
		pageUpDown = true;
		return true;
	case Keys::pageDown:
		pageDownDown = true;
		return true;
	case Keys::space:
		spaceDown = true;
		return true;
	case Keys::escape:
		globalResources.taskShedular.setNextPhaseTask(ThreadResources::quit);
		return true;
	case Keys::f1:
		if(!f1Down)
		{
			f1Down = true;
			f1Toggled = !f1Toggled;
			globalResources.userInterface.setDisplayVirtualFeedbackTexture(f1Toggled);
		}
		return true;
	case Keys::f2:
		if(!f2Down)
		{
			f2Down = true;
			globalResources.window.setVSync(!globalResources.window.getVSync());
		}
		return true;
	default:
		return false;
	}
}

bool InputHandler::keyUp(UINT keyCode, UINT, GlobalResources&)
{
	switch(keyCode)
	{
	case Keys::left:
		leftDown = false;
		return true;
	case Keys::right:
		rightDown = false;
		return true;
	case Keys::up:
		upDown = false;
		return true;
	case Keys::down:
		downDown = false;
		return true;
	case Keys::a:
		aDown = false;
		return true;
	case Keys::d:
		dDown = false;
		return true;
	case Keys::s:
		sDown = false;
		return true;
	case Keys::w:
		wDown = false;
		return true;
	case Keys::x:
		xDown = false;
		return true;
	case Keys::z:
		zDown = false;
		return true;
	case Keys::pageUp:
		pageUpDown = false;
		return true;
	case Keys::pageDown:
		pageDownDown = false;
		return true;
	case Keys::space:
		spaceDown = false;
		return true;
	case Keys::f1:
		if(f1Down)
		{
			f1Down = false;
		}
		return true;
	case Keys::f2:
		if(f2Down)
		{
			f2Down = false;
		}
		return true;
	default:
		return false;
	}
}