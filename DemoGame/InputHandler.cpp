#include "InputHander.h"
#include <SharedResources.h>
#include "Executor.h"


InputHandler::InputHandler(Window& Window, MouseMotionObverver mouseMotionObverver) : previousMousePosX(0), previousMousePosY(0), screenWidth(Window.width()),
screenHeight(Window.height()), mouseMotionObverver(mouseMotionObverver) {}

void InputHandler::mouseMoved(long x, long y)
{
	mouseMotionObverver.mouseMoved(static_cast<float>(x) / screenWidth * 10.0f, static_cast<float>(y) / screenWidth * 10.0f);
}

void InputHandler::escapePressed(SharedResources& sharedResources) { sharedResources.nextPhaseJob = Executor::quit; }