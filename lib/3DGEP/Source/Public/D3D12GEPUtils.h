#ifndef D3D12GEPUtils_h__
#define D3D12GEPUtils_h__

#include <iostream>
#include <chrono>
#include <Windows.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>

#ifdef max
#undef max // This is needed to avoid conflicts with functions called max(), like chrono::milliseconds::max()
#endif

namespace D3D12GEPUtils {

	using namespace Microsoft::WRL;

	void PrintHello();

	ComPtr<IDXGIAdapter4> GetMainAdapter(bool InUseWarp);

	ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> InAdapter);

	ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> InDevice, D3D12_COMMAND_LIST_TYPE InType);

	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType, UINT InNumDescriptors);

	ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> InDevice, D3D12_COMMAND_LIST_TYPE InCmdListType);

	ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> InDevice, ComPtr<ID3D12CommandAllocator> InCmdAllocator, D3D12_COMMAND_LIST_TYPE InCmdListType);

	ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> InDevice);

	HANDLE CreateFenceEventHandle();

	uint64_t SignalCmdQueue(ComPtr<ID3D12CommandQueue> InCmdQueue, ComPtr<ID3D12Fence> InFence, uint64_t& OutFenceValue);

	// Stalls the thread up until the InFenceEvent is signaled with InFenceValue, or when optional InMaxDuration has passed
	void WaitForFenceValue(ComPtr<ID3D12Fence> InFence, uint64_t InFenceValue, HANDLE InFenceEvent, std::chrono::milliseconds InMaxDuration = std::chrono::milliseconds::max());

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw std::exception();
		}
	}

	class D3D12Window
	{

	public:
		D3D12Window() {};
		void Initialize(const wchar_t* InWindowClassName, const wchar_t* InWindowTitle, 
			uint32_t InWinWidth, uint32_t InWinHeight, 
			uint32_t InBufWidth, uint32_t InBufHeight,
			ComPtr<ID3D12CommandQueue> InCmdQueue,
			WNDPROC InWndProc
			);

		inline bool IsFullScreen() { return m_IsFullScreen; };
		void SetFullscreenState(bool InNowFullScreen);

		void Resize(int32_t InNewWidth, int32_t InNewHeight);
	private:

		void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName, WNDPROC InWndProc);

		void CreateHWND(const wchar_t* InWindowClassName, HINSTANCE InHInstance,
			const wchar_t* InWindowTitle, uint32_t width, uint32_t height);

		void CreateSwapChain(ComPtr<ID3D12CommandQueue> InCmdQueue, uint32_t InBufWidth, uint32_t InBufHeight);

		void UpdateRenderTargetViews();

		
		// Default number of buffers handled by the swapchain
		static const uint32_t m_DefaultBufferCount = 3;

		ComPtr<ID3D12Device2> m_CurrentDevice;
		HWND m_HWND;
		ComPtr<IDXGISwapChain4> m_SwapChain;
		ComPtr<ID3D12Resource> m_BackBuffers[m_DefaultBufferCount];
		UINT m_CurrentBackBufferIndex = 0;
		ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;

		bool m_IsFullScreen = false;

		bool m_IsInitialized = false;
	};
}


#endif // D3D12GEPUtils_h__
