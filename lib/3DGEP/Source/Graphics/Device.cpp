#include "Device.h"

#ifdef GRAPHICS_SDK_D3D12
#include "D3D12Device.h"
#endif
#include "Public/GraphicsUtils.h"

namespace GEPUtils { namespace Graphics {

Device::Device()
{

}

GEPUtils::Graphics::Device& GetDevice()
{
	static std::unique_ptr<GEPUtils::Graphics::Device> graphicsDevice = nullptr;

	// Note: Debug Layer needs to be created before creating the Device
	Graphics::EnableDebugLayer();

#ifdef GRAPHICS_SDK_D3D12
	if (!graphicsDevice)
		graphicsDevice = std::make_unique<GEPUtils::Graphics::D3D12Device>();
#endif

	return *graphicsDevice;
}


} }