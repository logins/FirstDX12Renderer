/*
 Window.cpp

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#include "Window.h"

#ifdef GRAPHICS_SDK_D3D12
#include "D3D12Window.h"
#include "D3D12CommandQueue.h"
#endif

namespace GEPUtils { namespace Graphics {


	// We cannot return instances to virtual classes! But we can return pointers or references,
	// so we instantiate an object of a class derived from Window and we return the smart pointer.

	std::unique_ptr<GEPUtils::Graphics::Window> CreateGraphicsWindow(const Window::WindowInitInput& InWindowInitInput)
{
#ifdef GRAPHICS_SDK_D3D12
		D3D12GEPUtils::D3D12Window::D3D12WindowInitInput d3d12WinInput{
		InWindowInitInput.WindowClassName, InWindowInitInput.WindowTitle, 
			static_cast<D3D12GEPUtils::D3D12CommandQueue&>(InWindowInitInput.CmdQueue), // Static cast on the reference since here we should be convinced that it is a D3D12 CmdQueue
			InWindowInitInput.WinWidth, InWindowInitInput.WinHeight,
			InWindowInitInput.BufWidth, InWindowInitInput.BufHeight,
			nullptr
		};

		return std::make_unique<D3D12GEPUtils::D3D12Window>(d3d12WinInput);
#endif
	}

} }