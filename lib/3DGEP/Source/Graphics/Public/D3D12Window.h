/*
 D3D12Window.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef D3D12Window_h__
#define D3D12Window_h__

#include <iostream>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12.h>
#include <D3D12CommandQueue.h>
#include "Window.h"
#include <unordered_map>
#include "D3D12GEPUtils.h"

namespace D3D12GEPUtils {

	using namespace Microsoft::WRL;

	class D3D12Window : public GEPUtils::Graphics::Window
	{

	public:
		D3D12Window(const GEPUtils::Graphics::WindowInitInput& InWindowInitInput);

		D3D12Window() = delete;

		virtual ~D3D12Window() override;

		// Struct containing the many params used to initialize a D3D12Window  (old version, used only in Part2)
		struct D3D12WindowInitInput
		{
			const wchar_t* WindowClassName; const wchar_t* WindowTitle;
			D3D12CommandQueue& CmdQueue;
			uint32_t WinWidth; uint32_t WinHeight;
			uint32_t BufWidth; uint32_t BufHeight; WNDPROC WndProc;
			bool VSyncEnabled;
		};
		// Constructor left for compatibility with Part2. For the other examples use the platform-agnostic constructor instead
		D3D12Window(const D3D12WindowInitInput& InInitParams);


		virtual void ShowWindow() override;

		virtual void Close() override;

		virtual void Present() override;

		uint32_t GetCurrentBackbufferIndex() const { return m_CurrentBackBufferIndex; }

		ComPtr<ID3D12Resource> GetCurrentBackbuffer();
		ComPtr<ID3D12Resource> GetBackbufferAtIndex(uint32_t InIdx);

		// Returns the CPU descriptor handle of the render target view of the backbuffer at the current index
		CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVDescHandle();

		D3D12_CPU_DESCRIPTOR_HANDLE GetCuttentDSVDescHandle();

		inline bool IsFullScreen() { return m_IsFullScreen; };
		void SetFullscreenState(bool InNowFullScreen);

		virtual bool IsVSyncEnabled() const { return m_VSyncEnabled; }
		virtual void SetVSyncEnabled(bool InNowEnabled);

		virtual void Resize(uint32_t InNewWidth, uint32_t InNewHeight) override;


		virtual GEPUtils::Graphics::CpuDescHandle& GetCurrentRTVDescriptorHandle() override;

		virtual GEPUtils::Graphics::CpuDescHandle& GetCurrentDSVDescriptorHandle() override;

	private:

		void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName, WNDPROC InWndProc);

		void CreateHWND(const wchar_t* InWindowClassName, HINSTANCE InHInstance,
			const wchar_t* InWindowTitle, uint32_t width, uint32_t height);

		void UpdatePresentFlags();

		void CreateSwapChain(ComPtr<ID3D12CommandQueue> InCmdQueue, uint32_t InBufWidth, uint32_t InBufHeight);

		void UpdateDepthStencil();
		void UpdateRenderTargetViews();

		static LRESULT GlobalWndProc(HWND InHwnd, UINT InMsg, WPARAM InWParam, LPARAM InLParam);

		ComPtr<ID3D12Device2> m_CurrentDevice;
		D3D12GEPUtils::D3D12CommandQueue& m_CmdQueue;
		HWND m_HWND;
		RECT m_WindowModeRect;
		ComPtr<IDXGISwapChain4> m_SwapChain;
		// Replaced with the platform-agnostic version in Window
		uint64_t m_FrameFenceValues[m_DefaultBufferCount] = { 0 }; // Note: important to initialize every member variable, otherwise it could contain garbage!
		ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
		ComPtr<ID3D12Resource> m_DSBuffer;
		// DS buffer views need to be contained in a heap even if we use just one
		ComPtr<ID3D12DescriptorHeap> m_DSVHeap;
		UINT m_RTVDescIncrementSize = 0;
		bool m_IsFullScreen = false;
		bool m_VSyncEnabled = false;
		bool m_TearingSupported = false;
		UINT m_PresentFlags = 0;


		D3D12GEPUtils::D3D12CpuDescriptorHandle CurrentRtvDescHandle;
		D3D12GEPUtils::D3D12CpuDescriptorHandle CurrentDsvDescHandle;
	};
}
#endif // D3D12Window_h__
