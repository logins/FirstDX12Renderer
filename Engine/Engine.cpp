// Engine.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include "pch.h"

#include "Helpers.h"

#include <windows.h> //For window handling
#include <wrl.h> //For WRL::ComPtr
#include <d3d12.h>
#include "Platform/DX12/d3dx12.h" //Helper functions from https://github.com/Microsoft/DirectX-Graphics-Samples/tree/master/Libraries/D3DX12
#include <dxgi1_6.h> //For SwapChain4
#include <shellapi.h>//for CommandLineToArgvW .. getting command line arguments
#include <assert.h>
#include <algorithm>



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
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM) { return 0; };

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

int main()
{

    std::cout << "Hello DX12!\n"; 

	//Wait for Enter key press before returning
	getchar();

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
