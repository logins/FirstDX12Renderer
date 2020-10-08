#include "Device.h"

#ifdef GRAPHICS_SDK_D3D12
#include "D3D12Device.h"
#endif

namespace GEPUtils { namespace Graphics {

Device::Device()
{

}

std::unique_ptr<GEPUtils::Graphics::Device> CreateDevice()
{
#ifdef GRAPHICS_SDK_D3D12
	return std::make_unique<GEPUtils::Graphics::D3D12Device>();
#endif
}


} }
