#include "InputManager.h"
#include "HresultException.h"

InputManager::InputManager(Window& Window, MouseMotionObverver mouseMotionObverver) : previousMousePosX(0), previousMousePosY(0), screenWidth(Window.width()),
	screenHeight(Window.height()), mouseMotionObverver(mouseMotionObverver), rawInputBuffer(nullptr)
{
	RAWINPUTDEVICE rawMouse;
	rawMouse.usUsagePage = 0x01;
	rawMouse.usUsage = 0x02;
	rawMouse.dwFlags = RIDEV_NOLEGACY;   // adds HID mouse and also ignores legacy mouse messages
	rawMouse.hwndTarget = nullptr;

	BOOL success = RegisterRawInputDevices(&rawMouse, 1u, sizeof(RAWINPUTDEVICE));
	if (!success) {throw false;}
}

InputManager::~InputManager()
{
	if(rawInputBuffer) delete[] rawInputBuffer;
}

void InputManager::handleRawInput(LPARAM lParam)
{
	UINT requiredInputBufferSize;
	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &requiredInputBufferSize, sizeof(RAWINPUTHEADER)); //get the required buffer size
	if (requiredInputBufferSize > rawInputBuffersize)
	{
		delete[] rawInputBuffer;
		rawInputBuffer = nullptr;
		rawInputBuffer = new unsigned char[requiredInputBufferSize];
		rawInputBuffersize = requiredInputBufferSize;
	}

	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawInputBuffer, &requiredInputBufferSize, sizeof(RAWINPUTHEADER));

	RAWINPUT* raw = (RAWINPUT*)rawInputBuffer;

	
	if (raw->header.dwType == RIM_TYPEMOUSE)
	{
		switch (raw->data.mouse.usButtonFlags)
		{
		case RI_MOUSE_LEFT_BUTTON_DOWN :

			break;
		}
		if (raw->data.mouse.lLastX | raw->data.mouse.lLastY)
		{
			mouseMotionObverver.mouseMoved(static_cast<float>(raw->data.mouse.lLastX) / screenWidth * 10.0f, static_cast<float>(raw->data.mouse.lLastY) / screenWidth * 10.0f);
		}
	}
	else if (raw->header.dwType == RIM_TYPEKEYBOARD)
	{
		/*
		hResult = StringCchPrintf(szTempOutput, STRSAFE_MAX_CCH, TEXT(" Kbd: make=%04x Flags:%04x Reserved:%04x ExtraInformation:%08x, msg=%04x VK=%04x \n"),
		raw->data.keyboard.MakeCode,
		raw->data.keyboard.Flags,
		raw->data.keyboard.Reserved,
		raw->Data.keyboard.ExtraInformation,
		raw->data.keyboard.Message,
		raw->data.keyboard.VKey);
		if (FAILED(hResult))
		{
		// TODO: write error handler
		}
		OutputDebugString(szTempOutput);
		*/
	}
}