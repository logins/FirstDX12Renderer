#include <Part2.h>
#include <iostream>
#include <D3D12GEPUtils.h>
#include <wrl.h> //For WRL::ComPtr
#include <dxgi1_6.h>

using namespace Microsoft::WRL;

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
	ComPtr<IDXGIAdapter4> adapter = D3D12GEPUtils::GetMainAdapter(false);
	// Note: Debug Layer needs to be created before creating the Device
	D3D12GEPUtils::EnableDebugLayer();
	ComPtr<ID3D12Device2> graphicsDevice = D3D12GEPUtils::CreateDevice(adapter);
	ComPtr<ID3D12CommandQueue> cmdQueue = D3D12GEPUtils::CreateCommandQueue(graphicsDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
	ComPtr<ID3D12Fence> fence = D3D12GEPUtils::CreateFence(graphicsDevice);
	uint32_t mainWindowWidth = 1024, mainWindowHeight = 768;

	// Initialize main render window
	D3D12GEPUtils::D3D12Window::D3D12WindowInitInput windowInitInput = {
		L"DX12WindowClass", L"Part2 Main Window",
		graphicsDevice,
		mainWindowWidth, mainWindowHeight, // Window sizes
		mainWindowWidth, mainWindowHeight, // BackBuffer sizes
		cmdQueue, MainWndProc, fence
	};
	m_MainWindow.Initialize(windowInitInput);

	m_IsInitialized = true;
}

void Part2::OnMainWindowPaint()
{
	// Window paint event will trigger game thread update and render methods (as we are in a simple single threaded example)
	// OnUpdate
	// OnRender
}

LRESULT CALLBACK Part2::MainWndProc_Internal(HWND InHWND, UINT InMsg, WPARAM InWParam, LPARAM InLParam)
{
	// Preventing application to process events when all the necessery D3D12 objects are not initialized yet
	// Note: Do Not Return 0, otherwise HWND window creation will fail with no message!!
	if (!m_IsInitialized)
		return ::DefWindowProcW(InHWND, InMsg, InWParam, InLParam);

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
