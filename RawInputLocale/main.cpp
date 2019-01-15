#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <memory>
#include <string>

// Switch this to see the difference
constexpr bool USE_NOLEGACY_RAW_INPUT = false;

static void SetKeyState(BYTE& KeyState, bool IsDown)
{
	if (IsDown)
	{
		// Set 'down' bit 0x80, toggle 'toggle' bit 0x01
		KeyState = 0x80 | (0x01 ^ (KeyState & 0x01));
	}
	else
	{
		// Clear 'down' bit 0x80
		KeyState &= ~0x80;
	}
}
//---------------------------------------------------------------------

// Window procedure for GUI windows
LONG WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INPUT:
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

			LRESULT lr = ::DefRawInputProc(&pData, 1, sizeof(RAWINPUTHEADER));
			if (lr != S_OK) ::OutputDebugString("Error inside DefRawInputProc\n");

			// Keyboard must generate legacy messages only if no one processed a raw input.
			// We don't count repeats here but it can be added if necessary. There is no much profit
			// from a correct repeat count since we generate a separate message for each repeat.
			if (USE_NOLEGACY_RAW_INPUT && pData->header.dwType == RIM_TYPEKEYBOARD)
			{
				// For now we always assume key was not down. Might be fixed if necessary.
				constexpr bool WasDown = false;

				HWND hWndFocus = ::GetFocus();
				RAWKEYBOARD& KbData = pData->data.keyboard;
				const SHORT VKey = KbData.VKey;
				const BYTE ScanCode = static_cast<BYTE>(KbData.MakeCode);
				const bool IsExt = (KbData.Flags & RI_KEY_E0);

				LPARAM KbLParam = (1 << 0) | (ScanCode << 16);
				if (IsExt) KbLParam |= (1 << 24);

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
				if (hWndReceiver) ::PostMessage(hWndReceiver, KbData.Message, VKey, KbLParam);

				// Posted keyboard messages don't change the thread keyboard state so we must update it manually
				// for accelerators and some other windows internals (like alt codes Alt + Numpad NNN) to work.
				if (VKey < 256)
				{
					BYTE Keys[256];
					::GetKeyboardState(Keys);

					const bool IsKeyDown = (KbData.Message == WM_KEYDOWN || KbData.Message == WM_SYSKEYDOWN);
					SetKeyState(Keys[VKey], IsKeyDown);

					switch (VKey)
					{
						case VK_SHIFT:
						{
							const UINT VKeyH = ::MapVirtualKey(ScanCode, MAPVK_VSC_TO_VK_EX);
							SetKeyState(Keys[VKeyH == VK_RSHIFT ? VK_RSHIFT : VK_LSHIFT], IsKeyDown);
							break;
						}
						case VK_CONTROL:
						{
							const SHORT VKeyH = IsExt ? VK_RCONTROL : VK_LCONTROL;
							SetKeyState(Keys[VKeyH], IsKeyDown);
							break;
						}
						case VK_MENU:
						{
							const SHORT VKeyH = IsExt ? VK_RMENU : VK_LMENU;
							SetKeyState(Keys[VKeyH], IsKeyDown);
							break;
						}
					}

					::SetKeyboardState(Keys);
				}
			}

			free(pRawInputBuffer);

			break;
		}

		// In our case this is a valid indicator
		case WM_INPUTLANGCHANGE: ::OutputDebugString("WM_INPUTLANGCHANGE\n"); break;

		case WM_CHAR:
		case WM_SYSCHAR:
		{
			// Note that Alt+000 (numpad alt codes) work, for example Alt+123 prints '{'
			::OutputDebugString((std::string("Character: ") + (char)wParam + '\n').c_str());
			return 0;
		}

		case WM_DESTROY: ::PostQuitMessage(0); return 0;
	}

	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	// Create main window (can create multiple, but it doesn't matter)

	WNDCLASSEX WndClass2;
	memset(&WndClass2, 0, sizeof(WndClass2));
	WndClass2.cbSize        = sizeof(WndClass2);
	WndClass2.style         = CS_DBLCLKS;
	WndClass2.lpfnWndProc   = WindowProc;
	WndClass2.hInstance     = hInstance;
	WndClass2.hIcon         = ::LoadIcon(NULL, IDI_APPLICATION);
	WndClass2.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WndClass2.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
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

	// Register raw input for keyboards

	RAWINPUTDEVICE RawInputDevices;
	RawInputDevices.usUsagePage = 0x01; // Generic
	RawInputDevices.usUsage = 0x06; // Keyboard
	RawInputDevices.dwFlags = USE_NOLEGACY_RAW_INPUT ? RIDEV_NOLEGACY : 0;
	RawInputDevices.hwndTarget = NULL;
	if (::RegisterRawInputDevices(&RawInputDevices, 1, sizeof(RAWINPUTDEVICE)) == FALSE)
	{
		::OutputDebugString("ERROR: can't register raw input devices");
		return 0;
	}

	// Message loop

	MSG Msg;
	while (true)
	{
		if (!::PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) continue;
		if (Msg.message == WM_QUIT) break;
		::TranslateMessage(&Msg);
		::DispatchMessage(&Msg);
	}

	::UnregisterClass((const char*)aGUIWndClass, hInstance);

	return 0; 
}
//---------------------------------------------------------------------
