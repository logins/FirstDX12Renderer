/*
 D3D12BufferAllocator.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef D3D12BufferAllocator_h__
#define D3D12BufferAllocator_h__

#include <deque>
#include "d3dx12.h"

namespace D3D12GEPUtils { struct D3D12Resource; }

namespace GEPUtils{ namespace Graphics {

	class RangeAllocator;

	/*
	D3D12LinearBufferAllocator performs constant buffer sub-allocations in a single buffer resource.
	*/
	class D3D12LinearBufferAllocator {

	public:
		D3D12LinearBufferAllocator(D3D12GEPUtils::D3D12Resource& InResource);

		~D3D12LinearBufferAllocator();

		// It will first align the alignment with the hardware constraints, then align the buffer size and then allocate a buffer inside a resource
		// At the moment the way to check if the allocation was successful is to check if the CPU pointer field is non-zero
		void Allocate(size_t InSizeBytes, void*& OutCpuPtr, D3D12_GPU_VIRTUAL_ADDRESS& OutGpuPtr);

		// Sets the possible allocation region to a fraction of the whole pool size. Useful when we are allocating for multiple different frames.
		// If the buffer tries to allocate more than the assigned buffer region, an assert will be called.
		void SetAdmittedAllocationRegion(float InStartPercentage, float InEndPercentage);

		void Reset();

		// Do not allow copy construct
		D3D12LinearBufferAllocator(const D3D12LinearBufferAllocator& ) = delete;
		// Do not allow copy assignment
		D3D12LinearBufferAllocator& operator=(const D3D12LinearBufferAllocator&) = delete;

	private:
		D3D12GEPUtils::D3D12Resource& m_Resource;

		size_t m_TakenSize = 0;

		size_t m_AllocationLimit = 0;

		std::unique_ptr<RangeAllocator> m_RangeAllocator;

		void* m_ResourceCpuPtr;
		D3D12_GPU_VIRTUAL_ADDRESS m_ResourceGpuPtr;
	};


} }

#endif // D3D12BufferAllocator_h__

