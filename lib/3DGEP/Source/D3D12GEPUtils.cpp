#include "D3D12GEPUtils.h"
#include "D3D12UtilsInternal.h"
#include <assert.h>
#include <algorithm>
#include <d3dx12.h>

#ifdef max
#undef max // This is needed to avoid conflicts with functions called max(), like chrono::milliseconds::max()
#undef min
#endif

namespace D3D12GEPUtils
{
	void D3D12Window::Initialize(D3D12WindowInitInput InInitParams)
	{
		HINSTANCE InHInstance = ::GetModuleHandle(NULL); //Created windows will always refer to the current application instance
		m_CurrentDevice = InInitParams.graphicsDevice;
		m_CommandQueue = InInitParams.CmdQueue;
		m_Fence = InInitParams.Fence;
		m_FenceEvent = D3D12GEPUtils::CreateFenceEventHandle();
		for (uint32_t i = 0; i < m_DefaultBufferCount; i++)
		{
			m_CmdAllocators[i] = D3D12GEPUtils::CreateCommandAllocator(m_CurrentDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
		}
		// RTV Descriptor Size is vendor-dependent.
		// We need to retrieve it in order to know how much space to reserve per each descriptor in the Descriptor Heap
		m_RTVDescIncrementSize = m_CurrentDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_TearingSupported = CheckTearingSupport();
		// TODO check vsync support and store it to m_vSync
		
		RegisterWindowClass(InHInstance, InInitParams.WindowClassName, InInitParams.WndProc);

		CreateHWND(InInitParams.WindowClassName, InHInstance, InInitParams.WindowTitle, InInitParams.WinWidth, InInitParams.WinHeight);

		CreateSwapChain(InInitParams.CmdQueue, InInitParams.BufWidth, InInitParams.BufHeight);

		m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

		m_CmdList = D3D12GEPUtils::CreateCommandList(m_CurrentDevice, m_CmdAllocators[m_CurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);

		m_RTVDescriptorHeap = D3D12GEPUtils::CreateDescriptorHeap(m_CurrentDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_DefaultBufferCount);

		UpdateRenderTargetViews();

		m_IsInitialized = true;

	}

	void D3D12Window::ShowWindow()
	{
		::ShowWindow(m_HWND, SW_SHOW);
	}

	void D3D12Window::Close()
	{
		::CloseHandle(m_FenceEvent);
	}

	void D3D12Window::Present()
	{
		UINT presentFlags = m_TearingSupported && !IsVSyncEnabled() ? DXGI_PRESENT_ALLOW_TEARING : 0;
		ThrowIfFailed(m_SwapChain->Present(IsVSyncEnabled(), presentFlags));

		// Signal fence for the current backbuffer "on the fly"
		uint64_t currentFenceValue;
		D3D12GEPUtils::SignalCmdQueue(m_CommandQueue, m_Fence, currentFenceValue);
		m_FrameFenceValues[m_CurrentBackBufferIndex] = currentFenceValue;

		// Update current backbuffer index and wait for the present operation to be finished (it will when the fence value will be reached)
		// Note: the present operation changed the swapchain's current backbuffer index to the next available !
		m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

		WaitForFenceValue(m_Fence, currentFenceValue, m_FenceEvent);
	}

	ComPtr<ID3D12GraphicsCommandList> D3D12Window::ResetCmdListWithCurrentAllocator()
{
		ID3D12CommandAllocator* currentCmdAllocator = m_CmdAllocators[m_CurrentBackBufferIndex].Get();
		// We assume that the current allocator can be reset (all the commands previously queued in it have to be completed)
		// and the corresponding command list was closed.
		currentCmdAllocator->Reset();

		m_CmdList->Reset(currentCmdAllocator, nullptr); // A dummy initial pipeline state will be set by the runtime

		return m_CmdList;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE D3D12Window::GetCurrentRTVDescHandle()
{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
			m_CurrentBackBufferIndex, m_RTVDescIncrementSize);
	}

	void D3D12Window::SetFullscreenState(bool InNowFullScreen)
	{
		if (m_IsInitialized && (m_IsFullScreen == InNowFullScreen))
			return;
		
		if (InNowFullScreen)
		{
			// Store previous window dimensions to be able to turn back to window mode
			::GetWindowRect(m_HWND, &m_WindowModeRect);

			UINT fullscreenStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
			::SetWindowLongW(m_HWND, GWL_STYLE, fullscreenStyle);

			HMONITOR nearestMonitor = ::MonitorFromWindow(m_HWND, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX monitorInfo = {};
			monitorInfo.cbSize = sizeof(MONITORINFOEX); // Fill cbSize is required to use MONITORINFOEX
			::GetMonitorInfo(nearestMonitor, &monitorInfo);

			::SetWindowPos(m_HWND, HWND_TOP, // This will place window on top of all the others
				monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.top,
				monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
				SWP_FRAMECHANGED | // Will apply the window style set with ::SetWindowLongW and send a WM_NCCALCSIZE message to the window
				SWP_NOACTIVATE // Do not trigger window activation while repositioning
				);
			::ShowWindow(m_HWND, SW_MAXIMIZE); // The actual window set fullscreen command
		} 
		else // From fullscreen to window
		{
			::SetWindowLong(m_HWND, GWL_STYLE, WS_OVERLAPPEDWINDOW);

			::SetWindowPos(m_HWND, HWND_NOTOPMOST, // Generic window setting
				m_WindowModeRect.left,
				m_WindowModeRect.top,
				m_WindowModeRect.right - m_WindowModeRect.left,
				m_WindowModeRect.bottom - m_WindowModeRect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE
				);
			::ShowWindow(m_HWND, SW_NORMAL);
		}

		m_IsFullScreen = InNowFullScreen;
	}

	// Note: When a window resize happens, all the swapchain backbuffers need to be released.
	// Before doing it, we need to wait for the GPU to be done with them, by flushing.
	void D3D12Window::Resize(uint32_t InNewWidth, uint32_t InNewHeight)
	{
		// Don't allow 0 size swap chain buffers
		InNewWidth = std::max(1u, InNewWidth);
		InNewHeight = std::max(1u, InNewHeight);

		// Flush GPU to ensure no commands are "in-flight" on the backbuffers
		D3D12GEPUtils::FlushCmdQueue(m_CommandQueue, m_Fence, m_FenceEvent, m_LastSeenFenceValue);

		// All the backbuffers references must be released before resizing the swapchain
		for (int32_t i = 0; i < m_DefaultBufferCount; i++)
		{
			m_BackBuffers[i].Reset();
			m_FrameFenceValues[i] = m_FrameFenceValues[m_CurrentBackBufferIndex];
		}

		// Resizing swapchain and relative backbuffers' descriptor heap
		DXGI_SWAP_CHAIN_DESC swapchain_desc = {};
		ThrowIfFailed(m_SwapChain->GetDesc(&swapchain_desc)); // Maintain previous swapchain settings after resizing
		ThrowIfFailed(m_SwapChain->ResizeBuffers(m_DefaultBufferCount, InNewWidth, InNewHeight, swapchain_desc.BufferDesc.Format, swapchain_desc.Flags));
		m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
		UpdateRenderTargetViews();
	}

	void D3D12Window::CreateHWND(const wchar_t* InWindowClassName, HINSTANCE InHInstance, const wchar_t* InWindowTitle, uint32_t width, uint32_t height)
	{
		int32_t screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
		int32_t screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

		RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
		::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		int32_t windowWidth = windowRect.right - windowRect.left;
		int32_t windowHeight = windowRect.bottom - windowRect.top;

		//Center window within the screen. Clamp to 0,0 for the top-left corner.
		//This is going to be the top-left corner
		int32_t winTLx = std::max<int>(0, (screenWidth - windowWidth) / 2);
		int32_t winTLy = std::max<int>(0, (screenHeight - windowHeight) / 2);

		HWND currentHWND = ::CreateWindowExW(
			NULL,
			InWindowClassName,
			InWindowTitle,
			WS_OVERLAPPEDWINDOW,
			winTLx,
			winTLy,
			windowWidth,
			windowHeight,
			NULL,
			NULL,
			InHInstance,
			nullptr
		);

		//Check for valid return value
		assert(currentHWND && "Failed to create the HWND for the current window.");

		m_HWND = currentHWND;
	}

	void D3D12Window::CreateSwapChain(ComPtr<ID3D12CommandQueue> InCmdQueue, uint32_t InBufWidth, uint32_t InBufHeight)
	{
		ComPtr<IDXGIFactory4> dxgiFactory4;
		UINT createFactoryFlags = 0;
#if _DEBUG
		createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
		ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc= {};
		swapChainDesc.Width = InBufWidth;
		swapChainDesc.Height = InBufHeight;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Unsigned normalized integers, 8 bits per channel
		swapChainDesc.Stereo = FALSE;
		swapChainDesc.SampleDesc = { 1, 0 }; // Num samples per pixel and quality
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = m_DefaultBufferCount;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH; // Stretch buffer content to match window size
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Discards previous backbuffers content
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapChainDesc.Flags = m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0; // Tearing feature is required for variable refresh rate displays.
		// Allow Tearing has also to be set in Present settings.  It can only be used in windowed mode. 
		// To use this flag in full screen Win32 apps, the application should present to a fullscreen borderless window
		// and disable automatic ALT+ENTER fullscreen switching using IDXGIFactory::MakeWindowAssociation.
		// https://docs.microsoft.com/en-gb/windows/win32/direct3ddxgi/dxgi-present

		ComPtr<IDXGISwapChain1> swapChain1;
		ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
			InCmdQueue.Get(),
			m_HWND,
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain1
		));

		// Disable Alt+Enter hotkey, necessery for allow tearing feature
		ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(m_HWND, DXGI_MWA_NO_ALT_ENTER));

		// Cast to swapchain 4 and assign to member variable
		ThrowIfFailed(swapChain1.As(&m_SwapChain));
	}

	void D3D12Window::UpdateRenderTargetViews()
	{
		//Getting a descriptor handle to iterate trough the descriptor heap elements
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescHeapHandle(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT bufferIdx = 0; bufferIdx < m_DefaultBufferCount; bufferIdx++)
		{
			ComPtr<ID3D12Resource> backBuffer;
			// Retrieve interface to the current backBuffer
			ThrowIfFailed(m_SwapChain->GetBuffer(bufferIdx, IID_PPV_ARGS(&backBuffer)));
			// Create the RTV and store the reference of the current backBuffer, which is where rtvDescHeapHandle is pointing to
			// Either the pointer to the resource or the resource descriptor must be provided to create the view.
			m_CurrentDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvDescHeapHandle);

			m_BackBuffers[bufferIdx] = backBuffer;

			// Move the CPU descriptor handle to the next element on the heap
			rtvDescHeapHandle.Offset(m_RTVDescIncrementSize);
		}
	}

	void D3D12Window::RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName, WNDPROC InWndProc)
	{
		// Creating a window class to represent out render window
		WNDCLASSEXW windowClass = {};

		windowClass.cbSize = sizeof(WNDCLASSEXW);
		windowClass.style = CS_HREDRAW | CS_VREDRAW; // Redraw window if movement or size adjustments horizontally or vertically
		windowClass.lpfnWndProc = InWndProc;
		windowClass.cbClsExtra = 0; // Extra bytes to allocate after window class
		windowClass.cbWndExtra = 0; // Extra bytes to allocate after window instance
		windowClass.hInstance = hInst;
		windowClass.hIcon = ::LoadIcon(hInst, NULL); // Default icon
		windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // +1 must be added by documentation
		windowClass.lpszMenuName = NULL;
		windowClass.lpszClassName = windowClassName;
		windowClass.hIconSm = ::LoadIcon(hInst, NULL);

		static ATOM atom = ::RegisterClassExW(&windowClass);
		assert(atom > 0);
	}

	void PrintHello()
	{
		std::cout << "Hello From D3D12GEPUtils Library!" << std::endl;

		D3D12GEPUtils::ThisIsMyInternalFunction();
	}

	ComPtr<IDXGIAdapter4> GetMainAdapter(bool InUseWarp)
	{
		// Note: warp is a virtual platform that provides backward compatibility to DX12 for older graphics devices.
		// More info at: https://docs.microsoft.com/en-us/windows/win32/direct3darticles/directx-warp#what-is-warp 
		ComPtr<IDXGIFactory4> dxgiFactory;
		UINT createFactoryFlags = 0;

#ifdef _DEBUG
		createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
		ThrowIfFailed(::CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

		ComPtr<IDXGIAdapter1> dxgiAdapter1;
		ComPtr<IDXGIAdapter4> dxgiAdapter4;

		if (InUseWarp)
		{
			// Retrieve the first adapter found from the factory
			ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
			// Try casting it to adapter4
			ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
		}
		else
		{
			SIZE_T maxDedicatedVideoMemory = 0;
			// Inspecting all the devices found and taking the one with largest video memory
			for (UINT i =0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
			{
				DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
				dxgiAdapter1->GetDesc1(&dxgiAdapterDesc);
				if ((dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 // Avoid the adapter called the "Microsoft Basic Render Driver" adapter, which is a default adapter without display outputs
					&& SUCCEEDED(::D3D12CreateDevice(dxgiAdapter1.Get(),
						D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)) // Check if current adapter can create a D3D12 device without actually creating it
					&& dxgiAdapterDesc.DedicatedVideoMemory > maxDedicatedVideoMemory
					)
				{
					maxDedicatedVideoMemory = dxgiAdapterDesc.DedicatedVideoMemory;
					ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
				}
			}
		}
		return dxgiAdapter4;
	}

	ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> InAdapter)
	{
		ComPtr<ID3D12Device2> d3dDevice2;
		ThrowIfFailed(::D3D12CreateDevice(InAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&d3dDevice2)));
#if _DEBUG // Adding Info Queue filters
		ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(d3dDevice2.As(&infoQueue)))
		{
			// When a message with the following severities passes trough the Storage Filter, the program will break.
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
		}
		else
		{
			assert("We were not able to get the InfoQueue from device... Maybe the Debug Layer is disabled..");
		}
		// Following message severities will be suppressed
		D3D12_MESSAGE_SEVERITY suppressedSeverities[] = { D3D12_MESSAGE_SEVERITY_INFO };

		// Following messages will be suppressed based on their id
		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
		};

		D3D12_INFO_QUEUE_FILTER infoQueueFilter = {};
		infoQueueFilter.DenyList.NumSeverities = _countof(suppressedSeverities);
		infoQueueFilter.DenyList.pSeverityList = suppressedSeverities;
		infoQueueFilter.DenyList.NumIDs = _countof(denyIds);
		infoQueueFilter.DenyList.pIDList = denyIds;
		// Note: we can also deny entire message categories
		ThrowIfFailed(infoQueue->PushStorageFilter(&infoQueueFilter));
#endif
		return d3dDevice2;
	}

	ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> InDevice, D3D12_COMMAND_LIST_TYPE InType)
	{
		ComPtr<ID3D12CommandQueue> d3d12CmdQueue;

		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		cmdQueueDesc.Type = InType;
		cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		cmdQueueDesc.NodeMask = 0; // Assuming using 1 GPU
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

		ThrowIfFailed(InDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&d3d12CmdQueue)));

		return d3d12CmdQueue;
	}

	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType, UINT InNumDescriptors)
	{
		ComPtr<ID3D12DescriptorHeap> descrHeap;

		D3D12_DESCRIPTOR_HEAP_DESC descrHeapDesc = {};
		descrHeapDesc.Type = InType;
		descrHeapDesc.NumDescriptors = InNumDescriptors;
		descrHeapDesc.NodeMask = 0;
		descrHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		ThrowIfFailed(InDevice->CreateDescriptorHeap(&descrHeapDesc, IID_PPV_ARGS(&descrHeap)));

		return descrHeap;
	}

	ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> InDevice, D3D12_COMMAND_LIST_TYPE InCmdListType)
	{
		ComPtr<ID3D12CommandAllocator> cmdAllocator;
		ThrowIfFailed(InDevice->CreateCommandAllocator(InCmdListType, IID_PPV_ARGS(&cmdAllocator)));
		return cmdAllocator;
	}

	ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> InDevice, ComPtr<ID3D12CommandAllocator> InCmdAllocator, D3D12_COMMAND_LIST_TYPE InCmdListType)
	{
		ComPtr<ID3D12GraphicsCommandList> cmdList;
		ThrowIfFailed(InDevice->CreateCommandList(0, InCmdListType, InCmdAllocator.Get(), nullptr, IID_PPV_ARGS(&cmdList)));
		// Note: PSO parameter is optional and we can initialize the command list with it
		// Note: the initial state of a newly created command list is Open, so we manually need to close it.
		ThrowIfFailed(cmdList->Close()); // Close it because the beginning of the render method will execute reset on the cmdList
	
		return cmdList;
	}	

	ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> InDevice)
	{
		ComPtr<ID3D12Fence> fence;
		ThrowIfFailed(InDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
		return fence;
	}

	HANDLE CreateFenceEventHandle()
	{
		HANDLE eventHandle;
		eventHandle = ::CreateEvent(
			NULL,	// Handle cannot be inherited by child processes
			FALSE,	// This is going to be an auto-reset event object, after being signaled, it will automatically return in non-signaled state
			FALSE,	// Initial event state is non-signaled 
			NULL);	// Name of the event object, for events comparison
		assert(eventHandle && "[D3D12GEPUtils] Failed to create fence event.");

		return eventHandle;
	}

	void WaitForFenceValue(ComPtr<ID3D12Fence> InFence, uint64_t InFenceValue, HANDLE InFenceEvent, std::chrono::milliseconds InMaxDuration /*= std::chrono::milliseconds::max()*/)
	{
		if (InFence->GetCompletedValue() < InFenceValue)
		{
			ThrowIfFailed(InFence->SetEventOnCompletion(InFenceValue, InFenceEvent));

			::WaitForSingleObject(InFenceEvent, static_cast<DWORD>(InMaxDuration.count())); // count() returns the number of ticks
		}
	}

	void SignalCmdQueue(ComPtr<ID3D12CommandQueue> InCmdQueue, ComPtr<ID3D12Fence> InFence, uint64_t& OutFenceValue)
	{
		++OutFenceValue;

		ThrowIfFailed(InCmdQueue->Signal(InFence.Get(), OutFenceValue));
	}

	void FlushCmdQueue(ComPtr<ID3D12CommandQueue> InCmdQueue, ComPtr<ID3D12Fence> InFence, HANDLE InFenceEvent, uint64_t& OutFenceValue)
	{
		SignalCmdQueue(InCmdQueue, InFence, OutFenceValue);

		WaitForFenceValue(InFence, OutFenceValue, InFenceEvent);
	}

	// Note: enabling debug layer after creating a device will cause the runtime to remove the device.
	// Always enable debug layer before the device to be created.
	void EnableDebugLayer()
	{
#ifdef _DEBUG
		ComPtr<ID3D12Debug> debugInterface;
		ThrowIfFailed(::D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));

		debugInterface->EnableDebugLayer();
#endif
	}

}

