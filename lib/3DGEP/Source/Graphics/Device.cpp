/*
 Device.cpp

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/
 
#include "Device.h"

#ifdef GRAPHICS_SDK_D3D12
#include "D3D12Device.h"
#endif
#include "Public/GraphicsUtils.h"

namespace GEPUtils { namespace Graphics {

GEPUtils::Graphics::Device& GetDevice()
{
	static std::unique_ptr<GEPUtils::Graphics::Device> graphicsDevice = nullptr;


#ifdef GRAPHICS_SDK_D3D12
	if (!graphicsDevice) 
	{
		// Note: Debug Layer needs to be created before creating the Device
		Graphics::EnableDebugLayer();

		graphicsDevice = std::make_unique<GEPUtils::Graphics::D3D12Device>();
	}
#endif

	return *graphicsDevice;
}


} }
