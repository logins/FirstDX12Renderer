#include "D3D12GEPUtils.h"
#include <assert.h>
#include <algorithm>

namespace D3D12GEPUtils
{

	D3D12Window::D3D12Window(const wchar_t* InWindowClassName, HINSTANCE InHInstance, const wchar_t* InWindowTitle, uint32_t InWidth, uint32_t InHeight)
	{
		CreateHWND(InWindowClassName, InHInstance, InWindowTitle, InWidth, InHeight);

		m_IsInitialized = true;
	}

	void D3D12Window::CreateHWND(const wchar_t* InWindowClassName, HINSTANCE InHInstance, const wchar_t* InWindowTitle, uint32_t width, uint32_t height)
	{
		int32_t screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int32_t screenHeight = GetSystemMetrics(SM_CYSCREEN);

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

}

