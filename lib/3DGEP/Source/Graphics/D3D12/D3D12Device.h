#ifndef D3D12Device_h__
#define D3D12Device_h__

#include <wrl.h>
#include <d3d12.h>
#include "Device.h"


namespace GEPUtils { namespace Graphics {

class D3D12Device : public Device
{
public:
	// Parameterless constructor will create the device from the default main adapter
	D3D12Device();

	Microsoft::WRL::ComPtr<ID3D12Device2> GetInner() const { return m_D3D12Device; };
private:
	Microsoft::WRL::ComPtr<ID3D12Device2> m_D3D12Device;
};

} }
#endif // D3D12Device_h__
