/*
 D3D12BufferAllocator.cpp

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/
 
#include "D3D12BufferAllocator.h"
#include "GEPUtilsMath.h"
#include "D3D12GEPUtils.h"
#include "D3D12Device.h"
#include "GEPUtils.h"
#include "RangeAllocators.h"

namespace GEPUtils{ namespace Graphics {

	D3D12LinearBufferAllocator::D3D12LinearBufferAllocator(D3D12GEPUtils::D3D12Resource& InResource)
		: m_Resource(InResource)
	{
		m_ResourceGpuPtr = m_Resource.GetInner()->GetGPUVirtualAddress();

		// Opening mapping channel with CPU
		m_Resource.Map(&m_ResourceCpuPtr);

		m_AllocationLimit = m_Resource.GetSizeInBytes();
	}

	// Necessary to forward declare class type in unique_ptr member variable,
	// it prevents destructors to be inlined in which case breaks compilation
	D3D12LinearBufferAllocator::~D3D12LinearBufferAllocator()
	{
		// Closes the mappping channel with the CPU side
		m_Resource.UnMap();
	}


	void D3D12LinearBufferAllocator::Allocate(size_t InSizeBytes, void*& OutCpuPtr, D3D12_GPU_VIRTUAL_ADDRESS& OutGpuPtr)
	{
		// Align size to allocate with the default constant buffer suballocation alignment
		size_t alignedAllocationSize = GEPUtils::Math::Align(InSizeBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT); // taken space offset is now aligned with this allocation alignment

		m_TakenSize += alignedAllocationSize;

		if (m_TakenSize >= m_AllocationLimit || m_TakenSize >= m_Resource.GetSizeInBytes())
		{
			StopForFail("[D3D12BufferAllocator] Not enough space, resource is possibly too small");
		}
		
		OutCpuPtr = static_cast<uint8_t*>(m_ResourceCpuPtr) + m_TakenSize; // CPU pointer for the start of the allocation

		OutGpuPtr = m_ResourceGpuPtr + m_TakenSize; // GPU pointer for the start of the allocation
		
	}

	void D3D12LinearBufferAllocator::SetAdmittedAllocationRegion(float InStartPercentage, float InEndPercentage)
	{
		// Setting taken size to match start percentage
		m_TakenSize = GEPUtils::Math::Align(std::ceil(m_Resource.GetSizeInBytes() * InStartPercentage), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		// End percentage will be used for allocation limit
		m_AllocationLimit = GEPUtils::Math::Align(std::trunc(m_Resource.GetSizeInBytes() * InEndPercentage), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	}

	void D3D12LinearBufferAllocator::Reset()
	{
		m_TakenSize = 0;
	}

} }