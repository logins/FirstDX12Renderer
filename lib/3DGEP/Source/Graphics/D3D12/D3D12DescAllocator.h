#ifndef D3D12DescriptorAllocator_h__
#define D3D12DescriptorAllocator_h__

#include "d3d12.h"
#include <set>
#include "D3D12DescAllocatorPage.h"

namespace GEPUtils{ namespace Graphics {

/*!
 * \class D3D12DescriptorAllocator
 *
 * \brief Used to allocate descriptors on CPU when loading new resources (such as textures).
 * 
 * - DescriptorAllocatorPage corresponds to a descriptor heap (ID3D12DescriptorHeap).
 *
 * - When allocating descriptors, it finds the first heap (AllocatorPage) with enough free spaces and performs the allocation.
 * 
 * - At the end of the frame we can call ReleaseStaleDescriptors that will get rid of all the unused descriptors in every page.
 * 
 * \author Riccardo Loggini
 * \date October 2020
 */
	class D3D12DescAllocator
	{
	public:
		D3D12DescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE InType, uint32_t InNumDescriptors = 256);

		// When a frame has completed, the stale descriptors can be released
		void ReleaseStaleDescriptors(uint64_t InFrameNumber);

		static D3D12DescAllocator& Get(D3D12_DESCRIPTOR_HEAP_TYPE InType);

		
		// Allocates a number of contiguous descriptors from a CPU-visible descriptor heap
		bool Allocate(AllocatedDescRange& OutAllocatedRange, uint32_t InNumDescriptors = 1);

	private:
		using PagePool = std::vector<std::unique_ptr<Graphics::D3D12DescAllocatorPage>>;

		Graphics::D3D12DescAllocatorPage& CreateAllocatorPage();

		D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;

		uint32_t m_NumDescriptorsPerHeap;

		PagePool m_PagePool;
		// Indices of available heaps in the heap pool
		std::set<size_t> m_AvailableHeapIds;

		// Static instances of desc heap allocator to use as singletons. Retrieved with D3D12DescAllocator::Get(D3D12_DESCRIPTOR_HEAP_TYPE) function
		static D3D12DescAllocator m_CbvSrvUavAllocator;
		static D3D12DescAllocator m_SamplerAllocator;
	};

} }

#endif // D3D12DescriptorAllocator_h__