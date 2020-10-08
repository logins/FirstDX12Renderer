#include "D3D12Device.h"
#include "d3dx12.h"
#include <dxgi1_6.h>
#include "D3D12GEPUtils.h"

using namespace Microsoft::WRL;

namespace GEPUtils { namespace Graphics {

D3D12Device::D3D12Device()
{
	ComPtr<IDXGIAdapter4> adapter = D3D12GEPUtils::GetMainAdapter(false);

	m_D3D12Device = D3D12GEPUtils::CreateDevice(adapter);
}

} }