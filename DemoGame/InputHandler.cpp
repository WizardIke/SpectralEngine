#include "InputHander.h"
#include "Assets.h"
#include "Executor.h"


InputHandler::InputHandler(Window& Window, MouseMotionObverver mouseMotionObverver) : previousMousePosX(0), previousMousePosY(0), screenWidth(Window.width()),
screenHeight(Window.height()), mouseMotionObverver(mouseMotionObverver) {}

void InputHandler::mouseMoved(long x, long y)
{
	mouseMotionObverver.mouseMoved(static_cast<float>(x) / screenWidth * 10.0f, static_cast<float>(y) / screenWidth * 10.0f);
}

void InputHandler::escapePressed(SharedResources& sharedResources) { sharedResources.nextPhaseJob = Executor::quit; }

void InputHandler::f1Pressed(SharedResources& sr)
{
	if (!f1Down)
	{
		f1Down = true;
		if (f1Toggled)
		{
			f1Toggled = false;
			Assets& sharedResources = reinterpret_cast<Assets&>(sr);
			sharedResources.userInterface().setDisplayVirtualFeedbackTexture(false);
		}
		else
		{
			f1Toggled = true;
			Assets& sharedResources = reinterpret_cast<Assets&>(sr);
			sharedResources.userInterface().setDisplayVirtualFeedbackTexture(true);
		}
	}
}

void InputHandler::f1Released(SharedResources& sr)
{
	if (f1Down)
	{
		f1Down = false;
	}
}