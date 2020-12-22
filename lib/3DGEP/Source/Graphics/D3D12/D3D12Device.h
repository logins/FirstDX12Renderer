
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

	inline uint32_t GetCbvSrvUavDescHandleSize() { return m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); }

	inline void CopyDescriptors(uint32_t NumDestDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE* pDestDescriptorRangeStarts, const uint32_t* pDestDescriptorRangeSizes,	
		uint32_t NumSrcDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorRangeStarts, const uint32_t* pSrcDescriptorRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType)
	{
		m_D3D12Device->CopyDescriptors(NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes, NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes, DescriptorHeapsType);
	}

	Microsoft::WRL::ComPtr<ID3D12Device2> GetInner() const { return m_D3D12Device; };
private:
	Microsoft::WRL::ComPtr<ID3D12Device2> m_D3D12Device;
};

} }
#endif // D3D12Device_h__
