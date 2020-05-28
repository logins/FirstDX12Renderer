#pragma once

#include <iostream>
#include <Windows.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>


namespace D3D12GEPUtils {

	using namespace Microsoft::WRL;

	void PrintHello();

	ComPtr<IDXGIAdapter4> GetMainAdapter(bool InUseWarp);

	ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> InAdapter);

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw std::exception();
		}
	}

	class D3D12Window
	{
		D3D12Window() = delete;

	public:
		D3D12Window(const wchar_t* InWindowClassName, const wchar_t* InWindowTitle, 
			uint32_t InWinWidth, uint32_t InWinHeight, 
			uint32_t InBufWidth, uint32_t InBufHeight,
			ComPtr<ID3D12CommandQueue> InCmdQueue
			);
	private:

		void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName);

		void CreateHWND(const wchar_t* InWindowClassName, HINSTANCE InHInstance,
			const wchar_t* InWindowTitle, uint32_t width, uint32_t height);

		void CreateSwapChain(ComPtr<ID3D12CommandQueue> InCmdQueue, uint32_t InBufWidth, uint32_t InBufHeight);

		
		// Default number of buffers handled by the swapchain
		static const uint32_t m_DefaultBufferCount = 3;

		HWND m_HWND;
		ComPtr<IDXGISwapChain4> m_SwapChain;
		ComPtr<ID3D12Resource> m_BackBuffers[m_DefaultBufferCount];
		bool m_IsInitialized = false;

	};
}
