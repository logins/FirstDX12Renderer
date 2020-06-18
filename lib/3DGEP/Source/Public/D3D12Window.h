#ifndef D3D12Window_h__
#define D3D12Window_h__

#include <iostream>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12.h>
#include <D3D12CommandQueue.h>

namespace D3D12GEPUtils {

	using namespace Microsoft::WRL;

	class D3D12Window
	{

	public:
		// Struct containing the many params used to initialize a D3D12Window
		struct D3D12WindowInitInput
		{
			const wchar_t* WindowClassName; const wchar_t* WindowTitle;
			ComPtr<ID3D12Device2> graphicsDevice;
			uint32_t WinWidth; uint32_t WinHeight;
			uint32_t BufWidth; uint32_t BufHeight; WNDPROC WndProc;
		};

		D3D12Window() = delete;
		D3D12Window(D3D12GEPUtils::D3D12CommandQueue& InCmdQueue) : m_CmdQueue(InCmdQueue) {};

		void Initialize(D3D12WindowInitInput InInitParams);

		void ShowWindow();

		void Close();

		void Present();

		uint32_t GetCurrentBackbufferIndex() const { return m_CurrentBackBufferIndex; }

		ComPtr<ID3D12Resource> GetCurrentBackbuffer() { return m_BackBuffers[m_CurrentBackBufferIndex]; }
		ComPtr<ID3D12Resource> GetBackbufferAtIndex(uint32_t InIdx) { InIdx < m_DefaultBufferCount ? m_BackBuffers[InIdx] : nullptr; }

		// Returns the CPU descriptor handle of the render target view of the backbuffer at the current index
		CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVDescHandle();

		D3D12_CPU_DESCRIPTOR_HANDLE GetCuttentDSVDescHandle();

		inline bool IsFullScreen() { return m_IsFullScreen; };
		void SetFullscreenState(bool InNowFullScreen);

		bool IsVSyncEnabled() const { return m_VSync; }
		void SetVSync(bool InNowEnabled) { m_VSync = InNowEnabled; }

		void Resize(uint32_t InNewWidth, uint32_t InNewHeight);
		UINT GetFrameWidth() const { return m_FrameWidth; }
		UINT GetFrameHeight() const { return M_FrameHeight; }
	private:

		void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName, WNDPROC InWndProc);

		void CreateHWND(const wchar_t* InWindowClassName, HINSTANCE InHInstance,
			const wchar_t* InWindowTitle, uint32_t width, uint32_t height);

		void CreateSwapChain(ComPtr<ID3D12CommandQueue> InCmdQueue, uint32_t InBufWidth, uint32_t InBufHeight);

		void UpdateDepthStencil();
		void UpdateRenderTargetViews();


		// Default number of buffers handled by the swapchain
		static const uint32_t m_DefaultBufferCount = 3;

		ComPtr<ID3D12Device2> m_CurrentDevice;
		D3D12GEPUtils::D3D12CommandQueue& m_CmdQueue;
		HWND m_HWND;
		RECT m_WindowModeRect;
		UINT m_FrameWidth = 1, M_FrameHeight = 1;
		ComPtr<IDXGISwapChain4> m_SwapChain;
		ComPtr<ID3D12Resource> m_BackBuffers[m_DefaultBufferCount];
		uint64_t m_FrameFenceValues[m_DefaultBufferCount] = { 0 }; // Note: important to initialize every member variable, otherwise it could contain garbage!
		UINT m_CurrentBackBufferIndex = 0;
		ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
		ComPtr<ID3D12Resource> m_DSBuffer;
		// DS buffer views need to be contained in a heap even if we use just one
		ComPtr<ID3D12DescriptorHeap> m_DSVHeap;
		UINT m_RTVDescIncrementSize = 0;
		bool m_IsFullScreen = false;
		bool m_VSync = false;
		bool m_TearingSupported = false;

		bool m_IsInitialized = false;
	};
}
#endif // D3D12Window_h__
