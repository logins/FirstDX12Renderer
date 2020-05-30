#include <Part2.h>
#include <iostream>
#include <D3D12GEPUtils.h>


int main()
{
	std::cout << "Hello from Part 2!" << std::endl;

	D3D12GEPUtils::PrintHello();

	Part2::Get()->Initialize();

	//Wait for Enter key press before returning
	getchar();
}

Part2* Part2::m_Instance; // Necessary (as standard 9.4.2.2 specifies) definition of the singleton instance

Part2* Part2::Get()
{
	if (m_Instance == nullptr)
		m_Instance = new Part2();
	return m_Instance;
}

void Part2::Initialize()
{
	// Create all the D3D objects requited for rendering and create the main window
	D3D12GEPUtils::GetMainAdapter(false);

	// TODO initialize main window here

	m_IsInitialized = true;
}

void Part2::OnMainWindowPaint()
{
	// Window paint event will trigger game thread update and render methods (as we are in a simple single threaded example)
	// OnUpdate
	// OnRender
}

LRESULT CALLBACK Part2::MainWndProc(HWND InHWND, UINT InMsg, WPARAM InWParam, LPARAM InLParam)
{
	if (!m_IsInitialized) // Preventing application to process events when all the necessery D3D12 objects are not initialized yet
		return 0;

	bool altKeyDown = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
	switch (InMsg)
	{
	case WM_PAINT:
		OnMainWindowPaint();
		break;
	case WM_SYSKEYDOWN: // When a system key is pressed (e.g. Alt key)
	case  WM_KEYDOWN: // When a non-system key is pressed (e.g. v key)
	{
		switch (InWParam)
		{
		case 'V':
			m_VSyncEnabled = !m_VSyncEnabled;
			break;
		case VK_ESCAPE:
			::PostQuitMessage(0); // Causes the application to terminate
			break;
		case VK_RETURN:
			if (altKeyDown)
			{
			case VK_F11:
				m_MainWindow.SetFullscreenState(!m_MainWindow.IsFullScreen());
			}
			break;
		default:
			break;
		}
	}
	case WM_SYSCHAR:
		break; // Preventing Alt+Enter hotkey to try switching the window to fullscreen since we are manually handling it with Alt+F11
	case WM_SIZE:
	{
		RECT clientRect = {};
		::GetClientRect(InHWND, &clientRect);
		int32_t newWidth = clientRect.right - clientRect.left;
		int32_t newHeight = clientRect.bottom - clientRect.top;
		m_MainWindow.Resize(newWidth, newHeight);
		break;
	}
	case WM_DESTROY: // Called when X is pressed at the top right
		::PostQuitMessage(0);
		break;
	}

	return ::DefWindowProcW(InHWND, InMsg, InWParam, InLParam); // Message will be handled by the OS' System Procedure
}
