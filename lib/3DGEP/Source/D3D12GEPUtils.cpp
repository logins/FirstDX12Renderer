#include "D3D12GEPUtils.h"
#include "D3D12UtilsInternal.h"
#include <assert.h>
#include <algorithm>

namespace D3D12GEPUtils
{
	D3D12Window::D3D12Window(const wchar_t* InWindowClassName, const wchar_t* InWindowTitle,
		uint32_t InWinWidth, uint32_t InWinHeight,
		uint32_t InBufWidth, uint32_t InBufHeight,
		ComPtr<ID3D12CommandQueue> InCmdQueue)
	{
		HINSTANCE InHInstance = ::GetModuleHandle(NULL); //Created windows will always refer to the current application instance

		RegisterWindowClass(InHInstance, InWindowClassName);

		CreateHWND(InWindowClassName, InHInstance, InWindowTitle, InWinWidth, InWinHeight);

		CreateSwapChain(InCmdQueue, InBufWidth, InBufHeight);

		m_IsInitialized = true;
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
		swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0; // Tearing feature is required for variable refresh rate displays.
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

	void D3D12Window::RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName)
	{
		// Creating a window class to represent out render window
		WNDCLASSEXW windowClass = {};

		windowClass.cbSize = sizeof(WNDCLASSEXW);
		windowClass.style = CS_HREDRAW | CS_VREDRAW; // Redraw window if movement or size adjustments horizontally or vertically
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

}

