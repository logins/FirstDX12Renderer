#include <Part2.h>
#include <iostream>
#include <D3D12GEPUtils.h>
#include <wrl.h> //For WRL::ComPtr
#include <dxgi1_6.h>
#include <d3dx12.h>

using namespace Microsoft::WRL;

int main()
{
	std::cout << "Hello from Part 2!" << std::endl;

	D3D12GEPUtils::PrintHello();

	Part2::Get()->Initialize();

	Part2::Get()->Run();

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
	m_CmdQueue = D3D12GEPUtils::CreateCommandQueue(graphicsDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_Fence = D3D12GEPUtils::CreateFence(graphicsDevice);
	m_FenceEvent = D3D12GEPUtils::CreateFenceEventHandle();
	uint32_t mainWindowWidth = 1024, mainWindowHeight = 768;

	// Initialize main render window
	D3D12GEPUtils::D3D12Window::D3D12WindowInitInput windowInitInput = {
		L"DX12WindowClass", L"Part2 Main Window",
		graphicsDevice,
		mainWindowWidth, mainWindowHeight, // Window sizes
		mainWindowWidth, mainWindowHeight, // BackBuffer sizes
		m_CmdQueue, MainWndProc, m_Fence
	};
	m_MainWindow.Initialize(windowInitInput);

	for (uint32_t i = 0; i < m_NumCmdAllocators; i++)
	{
		m_CmdAllocators[i] = D3D12GEPUtils::CreateCommandAllocator(graphicsDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
	}

	m_CmdList = D3D12GEPUtils::CreateCommandList(graphicsDevice, m_CmdAllocators[m_MainWindow.GetCurrentBackbufferIndex()], D3D12_COMMAND_LIST_TYPE_DIRECT);

	m_IsInitialized = true;
}

void Part2::Run()
{
	if (!m_IsInitialized)
		return;

	m_MainWindow.ShowWindow();

	// Application's main loop is based on received window messages, specifically WM_PAINT will trigger Update() and Render()
	MSG windowMessage = {};
	while (windowMessage.message != WM_QUIT)
	{
		if (::PeekMessage(&windowMessage, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&windowMessage);
			::DispatchMessage(&windowMessage);
		}
	}
}

void Part2::OnMainWindowPaint()
{
	// Window paint event will trigger game thread update and render methods (as we are in a simple single threaded example)
	Update();

	Render();
}

void Part2::Update()
{
	static uint64_t frameCounter = 0;
	static double elapsedSeconds = 0;
	static std::chrono::high_resolution_clock clock;
	auto t0 = clock.now();

	// TODO update content will go here

	frameCounter++;
	auto t1 = clock.now();
	auto deltaTime = t1 - t0;
	t0 = t1;
	elapsedSeconds += deltaTime.count() * 1e-9; // Conversion from nanoseconds into seconds

	if (elapsedSeconds > 1.0)
	{
		char buffer[500]; auto fps = frameCounter / elapsedSeconds;
		sprintf_s(buffer, 500, "Average FPS: %f\n", fps);
		OutputDebugStringA(buffer);

		frameCounter = 0;
		elapsedSeconds = .0f;
	}
}

void Part2::Render()
{
	auto cmdAllocator = m_CmdAllocators[m_MainWindow.GetCurrentBackbufferIndex()];
	auto backBuffer = m_MainWindow.GetCurrentBackbuffer();

	cmdAllocator->Reset(); // Note: the associated command list needs to be closed
	m_CmdList->Reset(cmdAllocator.Get(), nullptr);

	// Clear render target
	{
		// Transitioning current backbuffer resource to render target state
		CD3DX12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			// We can be sure that the previous state was present because in this application all the render targets
			// are first filled and then presented to the main window repetitevely.
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		m_CmdList->ResourceBarrier(1, &transitionBarrier);

		FLOAT clearColor[] = { .4f, .6f, .9f, 1.f };
		m_CmdList->ClearRenderTargetView(m_MainWindow.GetCurrentRTVDescHandle(), clearColor, 0, nullptr);
	}

	// Execute command list and present current render target from the main window
	{
		CD3DX12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		m_CmdList->ResourceBarrier(1, &transitionBarrier);

		// Mandatory for the command list to close before getting executed by the command queue
		D3D12GEPUtils::ThrowIfFailed(m_CmdList->Close());

		ID3D12CommandList* const cmdLists[] = { m_CmdList.Get() };
		m_CmdQueue->ExecuteCommandLists(1, cmdLists);

		m_MainWindow.Present();
	}
}

void Part2::QuitApplication()
{
	// TODO make sure all the instantiated objects are cleaned up (see output log when closing application if there were some forgotten live objects)
	m_MainWindow.Close();
	uint64_t fenceValue;
	D3D12GEPUtils::FlushCmdQueue(m_CmdQueue, m_Fence, m_FenceEvent, fenceValue);
	::CloseHandle(m_FenceEvent);

	::PostQuitMessage(0); // Causes the application to terminate
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
			QuitApplication();
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
		QuitApplication();
		break;
	}

	return ::DefWindowProcW(InHWND, InMsg, InWParam, InLParam); // Message will be handled by the OS' System Procedure
}
