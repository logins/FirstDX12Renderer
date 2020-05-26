#pragma once

#include <iostream>
#include <Windows.h>
#include <D3D12UtilsInternal.h>


namespace D3D12GEPUtils {

	void PrintHello()
	{
		std::cout << "Hello From D3D12GEPUtils Library!" << std::endl;

		ThisIsMyInternalFunction();
	}

	class D3D12Window
	{
		D3D12Window() = delete;

	public:
		D3D12Window(const wchar_t* InWindowClassName, HINSTANCE InHInstance,
			const wchar_t* InWindowTitle, uint32_t width, uint32_t height);
	private:

		void CreateHWND(const wchar_t* InWindowClassName, HINSTANCE InHInstance,
			const wchar_t* InWindowTitle, uint32_t width, uint32_t height);

		void CreateSwapChain();

		HWND m_HWND;

		bool m_IsInitialized = false;
	};
}
