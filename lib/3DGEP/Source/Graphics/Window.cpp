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
#include "Application.h"

namespace GEPUtils { namespace Graphics {

	// We cannot return instances to virtual classes! But we can return pointers or references,
	// so we instantiate an object of a class derived from Window and we return the smart pointer.



	Window::~Window() = default;

} }