#pragma once

#include "Window.h"
#include <memory>
#undef min
#undef max

template<class SharedResources>
struct BaseInputHandler
{
	using GlobalResources = SharedResources;
	void leftMousePressed(SharedResources& sharedResources) {}
	void mouseMoved(long x, long y) {}

	void leftPressed(SharedResources& sharedResources) {}
	void rightPressed(SharedResources& sharedResources) {}
	void upPressed(SharedResources& sharedResources) {}
	void downPressed(SharedResources& sharedResources) {}
	void aPressed(SharedResources& sharedResources) {}
	void dPressed(SharedResources& sharedResources) {}
	void sPressed(SharedResources& sharedResources) {}
	void wPressed(SharedResources& sharedResources) {}
	void xPressed(SharedResources& sharedResources) {}
	void zPressed(SharedResources& sharedResources) {}
	void pageUpPressed(SharedResources& sharedResources) {}
	void pageDownPressed(SharedResources& sharedResources) {}
	void spacePressed(SharedResources& sharedResources) {}
	void escapePressed(SharedResources& sharedResources) {}
	void f1Pressed(SharedResources& sharedResources) {}
	void f2Pressed(SharedResources& sharedResources) {}

	void leftReleased(SharedResources& sharedResources) {}
	void rightReleased(SharedResources& sharedResources) {}
	void upReleased(SharedResources& sharedResources) {}
	void downReleased(SharedResources& sharedResources) {}
	void aReleased(SharedResources& sharedResources) {}
	void dReleased(SharedResources& sharedResources) {}
	void sReleased(SharedResources& sharedResources) {}
	void wReleased(SharedResources& sharedResources) {}
	void xReleased(SharedResources& sharedResources) {}
	void zReleased(SharedResources& sharedResources) {}
	void pageUpReleased(SharedResources& sharedResources) {}
	void pageDownReleased(SharedResources& sharedResources) {}
	void spaceReleased(SharedResources& sharedResources) {}
	void escapeReleased(SharedResources& sharedResources) {}
	void f1Released(SharedResources& sr) {}
	void f2Released(SharedResources& sr) {}
};

class InputManager
{
	unsigned long rawInputBuffersize = 0u;
	std::unique_ptr<unsigned char[]> rawInputBuffer;
public:
	InputManager() :
		rawInputBuffer(nullptr)
	{
		RAWINPUTDEVICE inputDevices[2];
		inputDevices[0].usUsagePage = 0x01;
		inputDevices[0].usUsage = 0x02;
		inputDevices[0].dwFlags = RIDEV_NOLEGACY;   // adds HID mouse and also ignores legacy mouse messages
		inputDevices[0].hwndTarget = nullptr;

		inputDevices[1].usUsagePage = 0x01;
		inputDevices[1].usUsage = 0x06;
		inputDevices[1].dwFlags = RIDEV_NOLEGACY;   // adds HID keyboard and also ignores legacy keyboard messages
		inputDevices[1].hwndTarget = nullptr;


		BOOL success = RegisterRawInputDevices(inputDevices, sizeof(inputDevices) / sizeof(inputDevices[0]), sizeof(RAWINPUTDEVICE));
		if (!success) { throw false; }
	}
	~InputManager() {}

	template<class InputHandler>
	void handleRawInput(LPARAM lParam, InputHandler& inputHandler, typename InputHandler::GlobalResources& sharedResources)
	{
		UINT requiredInputBufferSize = 0u;
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &requiredInputBufferSize, sizeof(RAWINPUTHEADER)); //get the required buffer size
		if (requiredInputBufferSize > rawInputBuffersize)
		{
			rawInputBuffer.reset(new unsigned char[requiredInputBufferSize]);
			rawInputBuffersize = requiredInputBufferSize;
		}

		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawInputBuffer.get(), &requiredInputBufferSize, sizeof(RAWINPUTHEADER));

		RAWINPUT& raw = (RAWINPUT&)*rawInputBuffer.get();


		if (raw.header.dwType == RIM_TYPEMOUSE)
		{
			switch (raw.data.mouse.usButtonFlags)
			{
			case RI_MOUSE_LEFT_BUTTON_DOWN:
				inputHandler.leftMousePressed(sharedResources);
				break;
			}
			if (raw.data.mouse.lLastX | raw.data.mouse.lLastY)
			{
				inputHandler.mouseMoved(raw.data.mouse.lLastX, raw.data.mouse.lLastY);
			}
		}
		else if (raw.header.dwType == RIM_TYPEKEYBOARD)
		{
			if (raw.data.keyboard.Flags == RI_KEY_MAKE)
			{
				switch (raw.data.keyboard.VKey)
				{
				case VK_LEFT:
					inputHandler.leftPressed(sharedResources);
					break;
				case VK_RIGHT:
					inputHandler.rightPressed(sharedResources);
					break;
				case VK_UP:
					inputHandler.upPressed(sharedResources);
					break;
				case VK_DOWN:
					inputHandler.downPressed(sharedResources);
					break;
				case 0x41: // A
					inputHandler.aPressed(sharedResources);
					break;
				case 0x44: // D
					inputHandler.dPressed(sharedResources);
					break;
				case 0x53: // S
					inputHandler.sPressed(sharedResources);
					break;
				case 0x57: // W
					inputHandler.wPressed(sharedResources);
					break;
				case 0x58: // X
					inputHandler.xPressed(sharedResources);
					break;
				case 0x5A: // Z
					inputHandler.zPressed(sharedResources);
					break;
				case VK_PRIOR: // Page up
					inputHandler.pageUpPressed(sharedResources);
					break;
				case VK_NEXT: // Page Down
					inputHandler.pageDownPressed(sharedResources);
					break;
				case VK_SPACE:
					inputHandler.spacePressed(sharedResources);
					break;
				case VK_ESCAPE:
					inputHandler.escapePressed(sharedResources);
					break;
				case VK_F1:
					inputHandler.f1Pressed(sharedResources);
					break;
				case VK_F2:
					inputHandler.f2Pressed(sharedResources);
					break;
				default:
					PRAWINPUT rawInputPtr = &raw;
					DefRawInputProc(&rawInputPtr, 1u, sizeof(RAWINPUTHEADER));
					break;
				}
			}
			else if (raw.data.keyboard.Flags == RI_KEY_BREAK)
			{
				switch (raw.data.keyboard.VKey)
				{
				case VK_LEFT:
					inputHandler.leftReleased(sharedResources);
					break;
				case VK_RIGHT:
					inputHandler.rightReleased(sharedResources);
					break;
				case VK_UP:
					inputHandler.upReleased(sharedResources);
					break;
				case VK_DOWN:
					inputHandler.downReleased(sharedResources);
					break;
				case 0x41: // A
					inputHandler.aReleased(sharedResources);
					break;
				case 0x44: // D
					inputHandler.dReleased(sharedResources);
					break;
				case 0x53: // S
					inputHandler.sReleased(sharedResources);
					break;
				case 0x57: // W
					inputHandler.wReleased(sharedResources);
					break;
				case 0x58: // X
					inputHandler.xReleased(sharedResources);
					break;
				case 0x5A: // Z
					inputHandler.zReleased(sharedResources);
					break;
				case VK_PRIOR: // Page up
					inputHandler.pageUpReleased(sharedResources);
					break;
				case VK_NEXT: // Page Down
					inputHandler.pageDownReleased(sharedResources);
					break;
				case VK_SPACE:
					inputHandler.spaceReleased(sharedResources);
					break;
				case VK_ESCAPE:
					inputHandler.escapeReleased(sharedResources);
					break;
				case VK_F1:
					inputHandler.f1Released(sharedResources);
					break;
				case VK_F2:
					inputHandler.f2Released(sharedResources);
					break;
				default:
					PRAWINPUT rawInputPtr = &raw;
					DefRawInputProc(&rawInputPtr, 1u, sizeof(RAWINPUTHEADER));
					break;
				}
			}
		}
	}
};