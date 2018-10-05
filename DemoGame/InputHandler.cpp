#include "InputHander.h"
#include "GlobalResources.h"
#include "ThreadResources.h"


InputHandler::InputHandler(Window& Window, MouseMotionObverver mouseMotionObverver) : previousMousePosX(0), previousMousePosY(0), screenWidth(Window.width()),
screenHeight(Window.height()), mouseMotionObverver(mouseMotionObverver) {}

void InputHandler::mouseMoved(long x, long y)
{
	mouseMotionObverver.mouseMoved(static_cast<float>(x) / screenWidth * 10.0f, static_cast<float>(y) / screenWidth * 10.0f);
}

void InputHandler::escapePressed(GlobalResources& globalResources) { globalResources.taskShedular.setNextPhaseTask(ThreadResources::quit); }

void InputHandler::f1Pressed(GlobalResources& globalResources)
{
	if(!f1Down)
	{
		f1Down = true;
		f1Toggled = !f1Toggled;
		globalResources.userInterface.setDisplayVirtualFeedbackTexture(f1Toggled);
	}
}

void InputHandler::f1Released(GlobalResources& globalResources)
{
	if(f1Down)
	{
		f1Down = false;
	}
}

void InputHandler::f2Pressed(GlobalResources& globalResources)
{
	if(!f2Down)
	{
		f2Down = true;
		globalResources.window.setVSync(!globalResources.window.getVSync());
	}
}

void InputHandler::f2Released(GlobalResources& globalResources)
{
	if(f2Down)
	{
		f2Down = false;
	}
}