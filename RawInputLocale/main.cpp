#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <memory>
#include <string>

// Switch this to see the difference
constexpr bool USE_NOLEGACY_RAW_INPUT = true;

// Window procedure for GUI windows
LONG WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_DESTROY:
		{
			::PostQuitMessage(0);
			return 0;
		}

		case WM_KEYDOWN: ::OutputDebugString("WM_KEYDOWN\n"); break;
		case WM_SYSKEYDOWN: ::OutputDebugString("WM_KEYDOWN\n"); break;
		case WM_KEYUP: ::OutputDebugString("WM_KEYUP\n"); break;
		case WM_SYSKEYUP: ::OutputDebugString("WM_SYSKEYUP\n"); break;

		// In our case this is a valid indicator
		case WM_INPUTLANGCHANGE: ::OutputDebugString("WM_INPUTLANGCHANGE\n"); break;

		case WM_CHAR:
		case WM_SYSCHAR:
		{
			// Note that Alt+000 (numpad alt codes) work, for example Alt+123 prints '{'
			::OutputDebugString((std::string("Character: ") + (char)wParam + '\n').c_str());
			return 0;
		}
	}

	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------

// Window procedure for a message-only window (primarily for raw input processing,
// because RIDEV_DEVNOTIFY / WM_INPUT_DEVICE_CHANGE doesn't work with NULL hWnd)
LONG WINAPI MessageOnlyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_INPUT_DEVICE_CHANGE)
	{
		if (wParam == GIDC_ARRIVAL)
			::OutputDebugString("Raw input device arrived\n");
		else if (wParam == GIDC_REMOVAL)
			::OutputDebugString("Raw input device removed\n");
		return 0;
	}
	else if (uMsg == WM_INPUT)
	{
		UINT DataSize;
		if (::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &DataSize, sizeof(RAWINPUTHEADER)) == (UINT)-1 ||
			!DataSize)
		{
			return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		void* pRawInputBuffer = malloc(DataSize);

		if (::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pRawInputBuffer, &DataSize, sizeof(RAWINPUTHEADER)) == (UINT)-1)
		{
			return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		PRAWINPUT pData = (PRAWINPUT)pRawInputBuffer;

		// Instead of invoking real handlers we are emulating a handled state here so that each second key is handled
		static bool RawInputWasHandled = false;
		if (!(pData->data.keyboard.Flags & RI_KEY_BREAK))
			RawInputWasHandled = !RawInputWasHandled;

		if (RawInputWasHandled)
		{
			::OutputDebugString("Raw input was handled, no legacy messages must be sent\n");
		}
		else
		{
			::OutputDebugString("Raw input was not handled, legacy message will be sent\n");

			::DefRawInputProc(&pData, 1, sizeof(RAWINPUTHEADER));

			// Keyboard must generate legacy messages only if no one processed a raw input.
			// We don't count repeats here but it can be added if necessary. There is no much profit
			// from a correct repeat count since we generate a separate message for each repeat.
			if (USE_NOLEGACY_RAW_INPUT && pData->header.dwType == RIM_TYPEKEYBOARD)
			{
				// For now we always assume key was not down. Might be fixed if necessary.
				constexpr bool WasDown = false;

				HWND hWndFocus = ::GetFocus();
				RAWKEYBOARD& KbData = pData->data.keyboard;

				LPARAM KbLParam = (1 << 0) | (KbData.MakeCode << 16);
				if (KbData.Flags & RI_KEY_E0) KbLParam |= (1 << 24);

				switch (KbData.Message)
				{
					case WM_KEYDOWN:
					{
						if (WasDown) KbLParam |= (1 << 30);
						break;
					}
					case WM_KEYUP:
					{
						KbLParam |= ((1 << 30) | (1 << 31));
						break;
					}
					case WM_SYSKEYDOWN:
					{
						if (hWndFocus) KbLParam |= (1 << 29);
						if (WasDown) KbLParam |= (1 << 30);
						break;
					}
					case WM_SYSKEYUP:
					{
						KbLParam |= ((1 << 30) | (1 << 31));
						if (hWndFocus) KbLParam |= (1 << 29);
						break;
					}
					default:
					{
						return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
					}
				}

				HWND hWndReceiver = hWndFocus ? hWndFocus : ::GetActiveWindow();
				if (hWndReceiver) ::PostMessage(hWndReceiver, KbData.Message, KbData.VKey, KbLParam);
			}
		}

		free(pRawInputBuffer);
	}

	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	// Create message-only window for raw input processing

	WNDCLASSEX WndClass;
	memset(&WndClass, 0, sizeof(WndClass));
	WndClass.cbSize        = sizeof(WndClass);
	WndClass.style         = 0;
	WndClass.lpfnWndProc   = MessageOnlyWindowProc;
	WndClass.cbClsExtra    = 0;
	WndClass.cbWndExtra    = 0;
	WndClass.hInstance     = hInstance;
	WndClass.hIcon         = NULL;
	WndClass.hIconSm       = NULL;
	WndClass.hCursor       = NULL;
	WndClass.hbrBackground = NULL;
	WndClass.lpszMenuName  = NULL;
	WndClass.lpszClassName = "RawInputLocale::MessageOnlyWindow";

	ATOM aMessageOnlyWndClass = ::RegisterClassEx(&WndClass);
	if (!aMessageOnlyWndClass) return 0;

	HWND hWndMessageOnly = ::CreateWindowEx(0, (const char*)aMessageOnlyWndClass, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	if (!hWndMessageOnly) return 0;

	// Register raw input for keyboards

	RAWINPUTDEVICE RawInputDevices;
	RawInputDevices.usUsagePage = 0x01; // Generic
	RawInputDevices.usUsage = 0x06; // Keyboard
	RawInputDevices.dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY | RIDEV_NOHOTKEYS;
	if (USE_NOLEGACY_RAW_INPUT)
		RawInputDevices.dwFlags |= RIDEV_NOLEGACY;
	RawInputDevices.hwndTarget = hWndMessageOnly;
	if (::RegisterRawInputDevices(&RawInputDevices, 1, sizeof(RAWINPUTDEVICE)) == FALSE)
	{
		::OutputDebugString("ERROR: can't register raw input devices");
		return 0;
	}

	// Create main window (can create multiple, but it doesn't matter)

	WNDCLASSEX WndClass2;
	memset(&WndClass2, 0, sizeof(WndClass2));
	WndClass2.cbSize        = sizeof(WndClass2);
	WndClass2.style         = CS_DBLCLKS;
	WndClass2.lpfnWndProc   = WindowProc;
	WndClass2.cbClsExtra    = 0;
	WndClass2.cbWndExtra    = 0;
	WndClass2.hInstance     = hInstance;
	WndClass2.hIcon         = ::LoadIcon(NULL, IDI_APPLICATION);
	WndClass2.hIconSm       = NULL;
	WndClass2.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WndClass2.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	WndClass2.lpszMenuName  = NULL;
	WndClass2.lpszClassName = "RawInputLocale::MainWindow";

	ATOM aGUIWndClass = ::RegisterClassEx(&WndClass2);
	if (!aGUIWndClass) return 0;

	HWND hWnd = ::CreateWindowEx(
		0,
		(const char*)aGUIWndClass,
		"Raw input locale (Main Window)",
		(WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE),
		50, 50, 800, 600,
		NULL, NULL, hInstance, NULL);

	if (!hWnd) return 0;

	::ShowWindow(hWnd, SW_SHOWDEFAULT);

	// Message loop

	MSG Msg;
	while (true)
	{
		if (!::PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			::Sleep(17);
			continue;
		}

		if (Msg.message == WM_QUIT) break;

		if (USE_NOLEGACY_RAW_INPUT)
		{
			// Posted keyboard messages don't change the thread keyboard state so we must update it manually
			// for accelerators and some other windows internals (like alt codes Alt + Numpad NNN) to work.
			// "The status changes as a thread removes keyboard messages from its message queue" (c) Docs
			const bool IsKeyDown = (Msg.message == WM_KEYDOWN || Msg.message == WM_SYSKEYDOWN);
			const bool IsKeyUp = (Msg.message == WM_KEYUP || Msg.message == WM_SYSKEYUP);
			if (IsKeyDown || IsKeyUp)
			{
				if (Msg.wParam < 256)
				{
					BYTE Keys[256];
					::GetKeyboardState(Keys);

					BYTE KeyState = IsKeyDown ? 0x80 : 0x00;

					Keys[Msg.wParam] = KeyState;
					switch (Msg.wParam)
					{
						case VK_SHIFT:
						{
							// Setting VK_SHIFT bit 0x80 here disables a language switching hotkey Alt+Shift,
							// but it doesn't work with RIDEV_NOLEGACY anyway
							//Keys[VK_SHIFT] = 0;

							const UINT VKey = ::MapVirtualKey((Msg.lParam & 0x00ff0000) >> 16, MAPVK_VSC_TO_VK_EX);
							Keys[VKey] = KeyState;
							break;
						}
						case VK_CONTROL:
						{
							const UINT VKey = (Msg.lParam & 0x01000000) ? VK_RCONTROL : VK_LCONTROL;
							Keys[VKey] = KeyState;
							break;
						}
						case VK_MENU:
						{
							const UINT VKey = (Msg.lParam & 0x01000000) ? VK_RMENU : VK_LMENU;
							Keys[VKey] = KeyState;
							break;
						}
					}

					::SetKeyboardState(Keys);
				}
			}
		}

		::TranslateMessage(&Msg);
		::DispatchMessage(&Msg);
	}

	// Unregister keyboards from raw input and destroy created objects

	RawInputDevices.dwFlags = RIDEV_REMOVE;
	RawInputDevices.hwndTarget = NULL;
	::RegisterRawInputDevices(&RawInputDevices, 1, sizeof(RAWINPUTDEVICE));

	::DestroyWindow(hWndMessageOnly);
	::UnregisterClass((const char*)aGUIWndClass, hInstance);
	::UnregisterClass((const char*)aMessageOnlyWndClass, hInstance);

	return 0; 
}
//---------------------------------------------------------------------
