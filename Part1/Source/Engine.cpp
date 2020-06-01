// Engine.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include "pch.h"

#include "Helpers.h"


#define WIN32_LEAN_AND_MEAN //This will shrink the inclusion of windows.h to the essential functions
#include <Windows.h> //For window handling
#include <wrl.h> //For WRL::ComPtr
#include <d3d12.h>
#include <d3dx12.h> //Helper functions from https://github.com/Microsoft/DirectX-Graphics-Samples/tree/master/Libraries/D3DX12
#include <dxgi1_6.h> //For SwapChain4
#include <shellapi.h>//for CommandLineToArgvW .. getting command line arguments
#include <assert.h>
#include <algorithm>
#include <chrono>
#include <debugapi.h>


// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

//Number of SwapChain BackBuffers (one per each frame we render concurrently)
const uint8_t g_NumFrames = 3;

//States If using Windows Advanced Rasterization Platform (WARP)
//Allows us to use the full set of advanced rendering features even if not supported
//by current hardware. It can also inspect quality of render result.
bool g_UseWarp = false;

uint32_t g_ClientWidth = 1280;
uint32_t g_ClientHeight = 720;

//Set to true when all the required D3D12 Objects have been initialized
bool g_IsInitialized = false;

//Window Handle
HWND g_hWnd;
//Window rectangle used to store window dimensions when in window mode
RECT g_WindowRect;

//Window callback function
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//D3D12 Objects
using namespace Microsoft::WRL;
ComPtr<ID3D12Device2> g_Device;
ComPtr<ID3D12CommandQueue> g_CommandQueue;
ComPtr<IDXGISwapChain4> g_SwapChain;
ComPtr<ID3D12Resource> g_BackBuffers[g_NumFrames];
ComPtr<ID3D12GraphicsCommandList> g_CommandList;
ComPtr<ID3D12CommandAllocator> g_CommandAllocators[g_NumFrames];
ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;

//Vendor Specific render target view descriptor size
UINT g_RTVDescriptorSize;

//We have a back buffer index because depending of the choosen swap chain Flip Mode Type, 
//back buffer order might not be sequential
UINT g_CurrentBackBufferIndex;


//Synchronization objects
ComPtr<ID3D12Fence> g_Fence;
uint64_t g_FenceValue = 0;
uint64_t g_FrameFenceValues[g_NumFrames] = {};
HANDLE g_FenceEvent;

//NOTE: When you "signal" the Command Queue for a Frame, it means we set a new frame fence value 

//Other window state variables
bool g_VSync = true;
bool g_TearingSupported = false;

bool g_FullScreen = false;


void ParseCommandLineArguments()
{
	int argc;
	wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

	for (size_t i = 0; i < argc; ++i)
	{
		if (::wcscmp(argv[i],L"-w") == 0 || ::wcscmp(argv[i],L"--width") == 0)
		{
			g_ClientWidth = ::wcstol(argv[++i], nullptr, 10);
		}
		else if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0)
		{
			g_ClientHeight = ::wcstol(argv[++i], nullptr, 10);
		}
		else if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
		{
			g_UseWarp = true;
		}
	}

	//Free memory allocated by argv
	::LocalFree(argv);

}

//NOTE: Enabling debug layer after creating the ID3D12Device will cause 
//the runtime to remove the device! Do it always before device creation.
void EnableDebugLayer()
{
#ifdef _DEBUG
	//Always enable the debug layer before doing anything DX12 related
	//so all possible errors generated while creating DX12 objects
	//are caught by the debug layter.
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));

	debugInterface->EnableDebugLayer();

#endif // _DEBUG

}

void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName)
{
	// Register a window class for creating our render window with.
	WNDCLASSEXW windowClass = {};

	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = &WndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInst;
	windowClass.hIcon = ::LoadIcon(hInst, NULL);
	windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = windowClassName;
	windowClass.hIconSm = ::LoadIcon(hInst, NULL);

	static ATOM atom = ::RegisterClassExW(&windowClass);
	assert(atom > 0);
}

HWND CreateHWND(const wchar_t* windowClassName, HINSTANCE hInst,
	const wchar_t* windowTitle, uint32_t width, uint32_t height)
{
	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	// Center the window within the screen. Clamp to 0, 0 for the top-left corner.
	//This is going to be the top-left window corner
	int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
	int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

	HWND hWnd = ::CreateWindowExW(
		NULL,
		windowClassName,
		windowTitle,
		WS_OVERLAPPEDWINDOW,
		windowX,
		windowY,
		windowWidth,
		windowHeight,
		NULL,
		NULL,
		hInst,
		nullptr
	);

	assert(hWnd && "Failed to create window");

	return hWnd;
}

ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp)
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;

#ifdef _DEBUG
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG

	//Create Adapter Factory
	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	if (useWarp)
	{
		//Retrieve the first adapter from the factory (as type Adapter1)
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		//Try to cast it to Adapter4 and if it succeeded, store it
		ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
	}
	else
	{
		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			// Check to see if the adapter can create a D3D12 device without actually 
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
					D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}
	}

	return dxgiAdapter4;
}

ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter)
{
	ComPtr<ID3D12Device2> d3dDevice2;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3dDevice2)));

#ifdef _DEBUG
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(d3dDevice2.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
	}

	//Suppresses the whole category of messages
	//D3D12_MESSAGE_CATEGORY suppressedCategories[2] = { };

	//Suppress messages based on severity level
	D3D12_MESSAGE_SEVERITY suppressedSeverities[] =
	{
		D3D12_MESSAGE_SEVERITY_INFO
	};

	//Suppress messages based on ID
	D3D12_MESSAGE_ID denyIds[] = {
		D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
		D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
		D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
	};

	D3D12_INFO_QUEUE_FILTER newFilter = {};
	//newFilter.DenyList.NumCategories = _countof(suppressedCategories);
	//newFilter.DenyList.pCategoryList = suppressedCategories;
	newFilter.DenyList.NumSeverities = _countof(suppressedSeverities);
	newFilter.DenyList.pSeverityList = suppressedSeverities;
	newFilter.DenyList.NumIDs = _countof(denyIds);
	newFilter.DenyList.pIDList = denyIds;

	ThrowIfFailed(pInfoQueue->PushStorageFilter(&newFilter));


#endif // _DEBUG


	return d3dDevice2;
}

ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> inDevice, D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(inDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

	return d3d12CommandQueue;
}

//In case of NVidia's G-Sync or AMD's FreeSync we can Allow Tearing.
//We can retrieve this information from IDXGIFactory5.
//In this case we are casting IDXGIFactory5 from our IDXGIFactory4: we did not use IDXGIFactory5 because 
//for now version DXGI 1.5 does not support debugging tools.
bool CheckTearingSupport()
{
	BOOL allowTearing = FALSE;
	ComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		ComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory4.As(&factory5)))
		{
			if (FAILED(factory5->CheckFeatureSupport( //NOTE: CheckFeatureSupport is filling allowTearing variable
			DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&allowTearing, sizeof(allowTearing)
			)))
			{
				allowTearing = FALSE;
			}
		}

	}

	return allowTearing == TRUE;
}

ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> cmdQueue, uint32_t width, uint32_t height, uint32_t bufferCount)
{
	ComPtr<IDXGISwapChain4> dxgiSwapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;

	UINT createFactoryFlags = 0;
#ifdef _DEBUG
	createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH; //Will scale backbuffer content to specified width and height (presentation target)
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //discard previous backbuffers content
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; //backbuffer transparency behavior
	// It is recommended to always allow tearing if tearing support is available.
	swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;


	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
		cmdQueue.Get(),
		hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1
	));

	//Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be enabled manually
	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

	//Try casting and store to swapchain4
	ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

	return dxgiSwapChain4;
}

ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device> currentDevice, D3D12_DESCRIPTOR_HEAP_TYPE descHeapType, uint32_t numDescriptors)
{
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.NumDescriptors = numDescriptors;
	descHeapDesc.Type = descHeapType;

	ThrowIfFailed(currentDevice->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

void UpdateRenderTargetViews(ComPtr<ID3D12Device2> currentDevice, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> rtvDescrHeap)
{
	//RTV Descriptor size is vendor-dependendent. 
	//We need to retrieve it in order to know how much space to reserve per each descriptor in the Descriptor Heap
	auto rtvDescriptorSize = currentDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); 

	//To iterate trough the Descriptor Heap, we use a Descriptor Handle initially pointing at the heap start
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescrHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < g_NumFrames; i++)
	{
		ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		//Create RTV and store reference where rtvDescrHeap Handle is currently pointing to
		currentDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

		g_BackBuffers[i] = backBuffer;

		//Increment rtvDescrHeap Handle pointing position
		rtvHandle.Offset(rtvDescriptorSize);

	}

}

ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> currentDevice, D3D12_COMMAND_LIST_TYPE cmdListType)
{
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ThrowIfFailed(currentDevice->CreateCommandAllocator(cmdListType, IID_PPV_ARGS(&cmdAllocator)));

	return cmdAllocator;
}

ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> currentDevice, 
	ComPtr<ID3D12CommandAllocator> cmdAllocator, D3D12_COMMAND_LIST_TYPE cmdListType)
{
	ComPtr<ID3D12GraphicsCommandList> cmdList;

	ThrowIfFailed(currentDevice->CreateCommandList(0, cmdListType, cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&cmdList)));

	//NOTE: By default the Command list will be created in Open state, we manually need to close it!
	//This will allow resetting it at the beginning of the Render function before recording any command.
	ThrowIfFailed(cmdList->Close());

	return cmdList;
}

ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> currentDevice)
{
	ComPtr<ID3D12Fence> fence;

	ThrowIfFailed(currentDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	return fence;
}

//Used for handling CPU fence event
HANDLE CreateFenceEventHandle()
{
	HANDLE fenceEvent;
	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent && "Failed to create fence event.");

	return fenceEvent;
}
//Command Queue Signal Fence (GPU)
uint64_t Signal(ComPtr<ID3D12CommandQueue> cmdQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue)
{
	uint64_t fenceValueForSignal = ++fenceValue;
	ThrowIfFailed(cmdQueue->Signal(fence.Get(), fenceValueForSignal));

	return fenceValueForSignal;
}

//This will stall the main thread up until the requested fence value is reached OR duration has expired
void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent,
	std::chrono::milliseconds duration = std::chrono::milliseconds::max())
{
	if (fence->GetCompletedValue() < fenceValue)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));

		::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count())); //count returns the number of ticks
	}
}

//Flush will stall the main thread up until all the previous rendering commands sent on the command queue have been executed.
//This is done by signaling the referenced fence and waiting for the value to be reached. When that happens, fenceEvent gets executed.
void Flush(ComPtr<ID3D12CommandQueue> cmdQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent)
{
	uint64_t fenceValueForSignal = Signal(cmdQueue, fence, fenceValue);
	WaitForFenceValue(fence, fenceValue, fenceEvent);
}

void Update()
{
	static uint64_t frameCounter = 0;
	static double elapsedSeconds = 0;
	static std::chrono::high_resolution_clock clock;
	static auto t0 = clock.now();

	//Content will go here

	frameCounter++;
	auto t1 = clock.now();
	auto deltaTime = t1 - t0;
	t0 = t1;

	elapsedSeconds += deltaTime.count() * 1e-9; //Conversion from nanoseconds into seconds
	//NOTE: alternatively we can do a duration_cast
	//std::chrono::duration_cast<std::chrono::seconds>(deltaTime).count();
	if (elapsedSeconds > 1.0)// This will ensure the debug print is done at max once per second
	{
		char buffer[500];
		auto fps = frameCounter / elapsedSeconds;
		sprintf_s(buffer, 500, "FPS: %f\n", fps);
		OutputDebugStringA(buffer);

		frameCounter = 0;
		elapsedSeconds = 0.0;

	}
}

void Render()
{
	auto cmdAllocator = g_CommandAllocators[g_CurrentBackBufferIndex];
	auto backBuffer = g_BackBuffers[g_CurrentBackBufferIndex];

	cmdAllocator->Reset();
	g_CommandList->Reset(cmdAllocator.Get(), nullptr);

	//Clear render target
	{
		//Transitioning current backbuffer in render target state
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET); //Note: this barrier will transition ALL subresources to the same state

		g_CommandList->ResourceBarrier(1, &barrier);

		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), g_CurrentBackBufferIndex, g_RTVDescriptorSize);
		g_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	}

	//Present current render target
	{
		//Transition back buffer to present state
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		backBuffer.Get(), //current back buffer
			D3D12_RESOURCE_STATE_RENDER_TARGET, //previous state
			D3D12_RESOURCE_STATE_PRESENT //wanted state
		);

		g_CommandList->ResourceBarrier(1, &barrier);

	}

	//NOTE: Close() must be called on the CommandList before being executed by the command queue!
	ThrowIfFailed(g_CommandList->Close());

	//Finally executing the filled Command List into the Command Queue
	ID3D12CommandList* const cmdLists[] = {
		g_CommandList.Get()
	};

	g_CommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	//Presenting the frame
	UINT syncInterval = g_VSync ? 1 : 0;
	UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0; //NOTE: Present ALSO need to have flag AllowTearing (other than in SwapChain creation)
	ThrowIfFailed(g_SwapChain->Present(syncInterval, presentFlags));

	//Signal fence for the current backbuffer
	//This fence value returned from the Signal operation is used to stall the CPU thread until any (writable - such as the render targets) resources are finished being used
	g_FrameFenceValues[g_CurrentBackBufferIndex] = Signal(g_CommandQueue, g_Fence, g_FenceValue);

	//update BackBuffer index
	g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
	//NOTE: when using Swap Effect Flip Discard, the order of back buffer indices is NOT guaranteed to be sequential!


	//Wait for the just retrieved fence value to be reached
	WaitForFenceValue(g_Fence, g_FrameFenceValues[g_CurrentBackBufferIndex], g_FenceEvent);

}

//NOTE: whenever a resize happens, ALL the backbuffer resources referenced by the SwapChain need to be released.
//In order to do it, we first need to Flush the GPU for any "in-flight" command.
//After swapchainìs resize back buffers, RTVs derscriptor heap is also updated.
void Resize(uint32_t width, uint32_t height)
{
	if (g_ClientWidth != width || g_ClientHeight != height)
	{
		//Don't allow 0 size swap chain back buffers
		g_ClientWidth = std::max(1u, width);
		g_ClientHeight = std::max(1u, height);

		//Flush the GPU queue to make sure the swap chain's back buffers
		//are not being referenced by an in-flight command list.
		Flush(g_CommandQueue, g_Fence, g_FenceValue, g_FenceEvent);
	}

	//NOTE: Any references to the back buffers must be released
	//before the swap chain can be resized!
	//All the per-frame fence values are also reset to the fence value of the current backbuffer index.
	for (int i = 0; i < g_NumFrames; ++i)
	{
		g_BackBuffers[i].Reset();
		g_FrameFenceValues[i] = g_FrameFenceValues[g_CurrentBackBufferIndex];
	}

	//Finally resizing the SwapChain and relative Descriptor Heap
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	ThrowIfFailed(g_SwapChain->GetDesc(&swapChainDesc)); //We get the current swapchain's desc so we can resize with same flags and same bugger color format
	ThrowIfFailed(g_SwapChain->ResizeBuffers(g_NumFrames, g_ClientWidth, g_ClientHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));
	g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
	UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap);

}

//NOTE: This will switch to fullscreen Borderless Window (FSBW) instead of the real one.
//By using a flip-mode swapchain, we don't need Windows to take full ownership of the application
//so we can obtain maximum framerate in borderless as well.
void SetFullscreenState(bool isNowFullscreen)
{
	if (g_FullScreen != isNowFullscreen)
	{
		g_FullScreen = isNowFullscreen;
		if (g_FullScreen)
		{
			//store previous window dimensions
			::GetWindowRect(g_hWnd, &g_WindowRect);

			UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

			::SetWindowLongW(g_hWnd, GWL_STYLE, windowStyle); //Apply borderless window style

			HMONITOR nearestMonitor = ::MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX monitorInfo = {};
			monitorInfo.cbSize = sizeof(MONITORINFOEX); //required to fill cbsize in oder to use this struct type
			::GetMonitorInfo(nearestMonitor, &monitorInfo);

			::SetWindowPos(g_hWnd, HWND_TOP, //This will place the window above all the topmost windows group!
				monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.top,
				monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
				SWP_FRAMECHANGED //Applies chosen window style using the SetWindowLong function. It will trigger a  WM_NCCALCSIZE message to the window, even if the size is not changed
				| SWP_NOACTIVATE //This will make the window to not activate by itself when the position is changed. 
				//If this flag is not present, the window is activated and moved to either the topmost or the non-topmost group, depending on the settings of hWndInsertAfter parameter.
				);
			//The actual call to show the window!
			::ShowWindow(g_hWnd, SW_MAXIMIZE);
		}
		else
		{
			::SetWindowLong(g_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

			::SetWindowPos(g_hWnd, HWND_NOTOPMOST, //This will place the window above all the non-topmost windows!
				g_WindowRect.left,
				g_WindowRect.top,
				g_WindowRect.right - g_WindowRect.left,
				g_WindowRect.bottom - g_WindowRect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE
				);

			::ShowWindow(g_hWnd, SW_NORMAL);
		}

	}
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (g_IsInitialized) //preventing the application to handle events before the necessary dx12 objects are initialised
	{
		bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
		switch (message)
		{
		case WM_PAINT:
			Update();
			Render();
			break;
		case WM_SYSKEYDOWN: //When a system key is pressed (eg. Alt)
		case WM_KEYDOWN: //When a non-system key is pressed
		{
			switch (wParam)
			{
			case 'V':
				g_VSync = !g_VSync;
				break;
			case VK_ESCAPE:
				::PostQuitMessage(0); //Causes the application  to terminate
				break;
			case VK_RETURN:
				if (alt)
				{
			case VK_F11:
				SetFullscreenState(!g_FullScreen);
				}
				break;
			}
			break;
		}
		//NOTE: Alt+Enter is the default way to switch to fullscreen, but in this way it will not take effect
		case WM_SYSCHAR:
			break;
		case WM_SIZE:
		{
			RECT clientRect = {};
			::GetClientRect(g_hWnd, &clientRect);
			int width = clientRect.right-clientRect.left;
			int height = clientRect.bottom-clientRect.top;

			Resize(width, height);
			break;
		}
		case WM_DESTROY: //called when X is pressed in the top right
			::PostQuitMessage(0);
			break;
		default:
			return ::DefWindowProcW(hwnd, message, wParam, lParam); //Message will be handled by the Default Window Procedure !
		}
	}
	else
	{
		return ::DefWindowProcW(hwnd, message, wParam, lParam);
	}

	//NOTE: IF WM_NCCREATE message will be handled retuning 0, window creation will fail!
	return 0;
}

int main()
{

    std::cout << "Hello DX12!\n"; 
	
	// Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
	// Using this awareness context allows the client area of the window 
	// to achieve 100% scaling while still allowing non-client window content to 
	// be rendered in a DPI sensitive fashion.
	::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	//This means that the swap chain buffers will be resized to fill the 
	//total number of screen pixels(true 4K or 8K resolutions) when resizing the client area 
	//of the window instead of scaling the client area based on the DPI scaling settings.

	const wchar_t* windowClassName = L"DX12WindowClass";
	ParseCommandLineArguments();

	//Enable DX12 Debug Layer
	EnableDebugLayer(); //NOTE: this needs to be called BEFORE creating the device!

	g_TearingSupported = CheckTearingSupport();

	HINSTANCE hInstance = GetModuleHandle(NULL);

	RegisterWindowClass(hInstance, windowClassName);
	g_hWnd = CreateHWND(windowClassName, hInstance, L"My first DX12 Window", g_ClientWidth, g_ClientHeight);

	//Initialise global window rect variable
	::GetWindowRect(g_hWnd, &g_WindowRect);

	//Create all the needed DX12 object
	ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(g_UseWarp);
	g_Device = CreateDevice(dxgiAdapter4);
	g_CommandQueue = CreateCommandQueue(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	g_SwapChain = CreateSwapChain(g_hWnd,g_CommandQueue,g_ClientWidth, g_ClientHeight, g_NumFrames);
	g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
	
	g_RTVDescriptorHeap = CreateDescriptorHeap(g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV,g_NumFrames);
	g_RTVDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap);

	//Create Command List and Allocators
	for (int i = 0; i < g_NumFrames; i++)//there needs to be one command allocator per each in-flight render frames
	{
		g_CommandAllocators[i] = CreateCommandAllocator(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	}
	g_CommandList = CreateCommandList(g_Device, g_CommandAllocators[g_CurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);

	//Create fence objects
	g_Fence = CreateFence(g_Device);
	g_FenceEvent = CreateFenceEventHandle();

	//All should be initialised, show the window
	g_IsInitialized = true;

	::ShowWindow(g_hWnd, SW_SHOW);

	//Application Main Loop
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	//Make sure command queue has finished all in-flight commands before closing
	Flush(g_CommandQueue, g_Fence, g_FenceValue, g_FenceEvent);

	//Closing event handle
	::CloseHandle(g_FenceEvent);
	
	//Wait for Enter key press before returning
	getchar();

	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
