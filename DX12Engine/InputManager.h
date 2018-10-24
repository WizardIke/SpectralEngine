#pragma once

#include "Window.h"
#include <memory>
#undef min
#undef max

namespace Keys
{
	constexpr UINT left = VK_LEFT;
	constexpr UINT right = VK_RIGHT;
	constexpr UINT up = VK_UP;
	constexpr UINT down = VK_DOWN;
	constexpr UINT a = 0x41;
	constexpr UINT d = 0x44;
	constexpr UINT s = 0x53;
	constexpr UINT w = 0x57;
	constexpr UINT x = 0x58;
	constexpr UINT z = 0x5A;
	constexpr UINT pageUp = VK_PRIOR;
	constexpr UINT pageDown = VK_NEXT;
	constexpr UINT space = VK_SPACE;
	constexpr UINT escape = VK_ESCAPE;
	constexpr UINT f1 = VK_F1;
	constexpr UINT f2 = VK_F2;
	constexpr UINT leftMouse = VK_LBUTTON;
	constexpr UINT rightMouse = VK_RBUTTON;
	constexpr UINT middleMouse = VK_MBUTTON;
	constexpr UINT leftShift = VK_LSHIFT;
	constexpr UINT rightShift = VK_RSHIFT;
	constexpr UINT leftControl = VK_LCONTROL;
	constexpr UINT rightControl = VK_RCONTROL;
	constexpr UINT numpad0 = VK_NUMPAD0;
	constexpr UINT numpad1 = VK_NUMPAD1;
	constexpr UINT numpad2 = VK_NUMPAD2;
	constexpr UINT numpad3 = VK_NUMPAD3;
	constexpr UINT numpad4 = VK_NUMPAD4;
	constexpr UINT numpad5 = VK_NUMPAD5;
	constexpr UINT numpad6 = VK_NUMPAD6;
	constexpr UINT numpad7 = VK_NUMPAD7;
	constexpr UINT numpad8 = VK_NUMPAD8;
	constexpr UINT numpad9 = VK_NUMPAD9;
	constexpr UINT rightAlt = VK_RMENU;
	constexpr UINT leftAlt = VK_LMENU;
	constexpr UINT numpadEnter = VK_SEPARATOR;
	constexpr UINT numpadDecimal = VK_DECIMAL;
}

template<class SharedResources>
struct BaseInputHandler
{
	using GlobalResources = SharedResources;
	void mouseMoved(long x, long y) {}
	//return true to skip default proccessing
	bool keyDown(UINT keyCode, UINT scanCode, SharedResources& sharedResources) {}
	bool keyUp(UINT keyCode, UINT scanCode, SharedResources& sharedResources) {}
};

class InputManager
{
public:
	InputManager()
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
		if (success == FALSE) { throw false; }
	}
	~InputManager() {}

	template<class InputHandler>
	void handleRawInput(LPARAM lParam, InputHandler& inputHandler, typename InputHandler::GlobalResources& sharedResources)
	{
		RAWINPUT rawInput;
		UINT size = sizeof(RAWINPUT);
		GetRawInputData((HRAWINPUT)(lParam), RID_INPUT, (void*)&rawInput, &size,
			sizeof(RAWINPUTHEADER));
		if(rawInput.header.dwType == RIM_TYPEKEYBOARD)
		{
			const RAWKEYBOARD& rawKeyboard = rawInput.data.keyboard;
			UINT virtualKey = rawKeyboard.VKey;
			UINT scanCode = rawKeyboard.MakeCode;
			UINT flags = rawKeyboard.Flags;
			if(virtualKey == 255)
			{
				// discard "fake keys" which are part of an escaped sequence
				return;
			}
			else if(virtualKey == VK_SHIFT)
			{
				// correct left-hand / right-hand SHIFT
				virtualKey = MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX);
			}
			else if(virtualKey == VK_NUMLOCK)
			{
				// correct PAUSE/BREAK and NUM LOCK silliness, and set the extended bit
				scanCode = (MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC) | 0x100);
			}
			const bool isE0 = ((flags & RI_KEY_E0) != 0);
			const bool isE1 = ((flags & RI_KEY_E1) != 0);

			if(isE1)
			{
				// for escaped sequences, turn the virtual key into the correct scan code using MapVirtualKey.
				// however, MapVirtualKey is unable to map VK_PAUSE (this is a known bug), hence we map that by hand.
				if(virtualKey == VK_PAUSE)
					scanCode = 0x45;
				else
					scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);
			}
			switch(virtualKey)
			{
				// right-hand CONTROL and ALT have their e0 bit set
			case VK_CONTROL:
				if(isE0)
					virtualKey = Keys::rightControl;
				else
					virtualKey = Keys::leftControl;
				break;

			case VK_MENU:
				if(isE0)
					virtualKey = Keys::rightAlt;
				else
					virtualKey = Keys::leftAlt;
				break;

				// NUMPAD ENTER has its e0 bit set
			case VK_RETURN:
				if(isE0)
					virtualKey = Keys::numpadEnter;
				break;

				// the standard INSERT, DELETE, HOME, END, PRIOR and NEXT keys will always have their e0 bit set, but the
				// corresponding keys on the NUMPAD will not.
			case VK_INSERT:
				if(!isE0)
					virtualKey = Keys::numpad0;
				break;

			case VK_DELETE:
				if(!isE0)
					virtualKey = Keys::numpadDecimal;
				break;

			case VK_HOME:
				if(!isE0)
					virtualKey = Keys::numpad7;
				break;

			case VK_END:
				if(!isE0)
					virtualKey = Keys::numpad1;
				break;

			case VK_PRIOR:
				if(!isE0)
					virtualKey = Keys::numpad9;
				break;

			case VK_NEXT:
				if(!isE0)
					virtualKey = Keys::numpad3;
				break;

				// the standard arrow keys will always have their e0 bit set, but the
				// corresponding keys on the NUMPAD will not.
			case VK_LEFT:
				if(!isE0)
					virtualKey = Keys::numpad4;
				break;

			case VK_RIGHT:
				if(!isE0)
					virtualKey = Keys::numpad6;
				break;

			case VK_UP:
				if(!isE0)
					virtualKey = Keys::numpad8;
				break;

			case VK_DOWN:
				if(!isE0)
					virtualKey = Keys::numpad2;
				break;

				// NUMPAD 5 doesn't have its e0 bit set
			case VK_CLEAR:
				if(!isE0)
					virtualKey = Keys::numpad5;
				break;
			}
			const bool released = ((flags & RI_KEY_BREAK) != 0);
			scanCode |= ((UINT)isE0) << 8u;

			bool handled;
			if(released)
			{
				handled = inputHandler.keyUp(virtualKey, scanCode, sharedResources);
			}
			else
			{
				handled = inputHandler.keyDown(virtualKey, scanCode, sharedResources);
			}
			if(!handled)
			{
				DefRawInputProc(&rawInput, 1, sizeof(RAWINPUTHEADER));
			}
		}
	}

	static void getKeyName(unsigned long scanCode, char* buffer, unsigned int bufferLength)
	{
		LONG key = (LONG)(((UINT)scanCode << 16));
		GetKeyNameTextA(key, buffer, bufferLength);
	}

	static void getKeyName(unsigned long scanCode, wchar_t* buffer, unsigned int bufferLength)
	{
		LONG key = (LONG)(((UINT)scanCode << 16));
		GetKeyNameTextW(key, buffer, bufferLength);
	}
};