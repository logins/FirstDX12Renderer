/*!
 * Project: First D3D12 Renderer - https://github.com/logins/FirstDX12Renderer
 *
 * File: D3D12DescAllocator.cpp
 *
 * Author: Riccardo Loggini
 */
 
 #include "D3D12DescAllocator.h"

#ifdef max
#undef max
#undef min
#endif

namespace GEPUtils{ namespace Graphics {

	D3D12DescAllocator::D3D12DescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE InType, uint32_t InNumDescriptors /*= 256*/)
		: m_HeapType(InType), m_NumDescriptorsPerHeap(InNumDescriptors)
	{
	}

	bool D3D12DescAllocator::AllocateIfPossible(D3D12DescAllocatorPage::AllocatedDescRange& OutDescRange, uint32_t InNumDescriptors /*= 1*/)
	{
		std::lock_guard<std::mutex> mutexLock(m_AllocationMutex);

		bool allocationMade = false;

		for (auto iter = m_AvailableHeapIds.begin(); iter != m_AvailableHeapIds.end(); ++iter)
		{
			D3D12DescAllocatorPage& allocatorPage = *m_PagePool[*iter];

			if (allocatorPage.AllocateRangeIfPossible(InNumDescriptors, OutDescRange))
			{
				allocationMade = true;

				if (allocatorPage.GetNumFreeHandles() == 0) // If no more free handles after the allocation, remove the current page from the free pages indices
					m_AvailableHeapIds.erase(iter);

				break;
			}
		}

		if (!allocationMade) // If not enough space was available for the allocation, create a new page and make the allocation there
		{
			// Enlarge the max number of descriptors per page to the requested size 
			// so we are sure that the created page will be able to contain the requested range
			m_NumDescriptorsPerHeap = std::max(m_NumDescriptorsPerHeap, InNumDescriptors);

			D3D12DescAllocatorPage& newPage = CreateAllocatorPage();

			if (!newPage.AllocateRangeIfPossible(InNumDescriptors, OutDescRange))
				return false;
		}

		return true;
	}

	void D3D12DescAllocator::ReleaseStaleDescriptors(uint64_t InFrameNumber)
	{
		for (size_t i = 0; i < m_PagePool.size(); i++)
		{
			D3D12DescAllocatorPage& page = *m_PagePool[i];

			page.ReleaseStaleDescriptors(InFrameNumber);

			if (page.GetNumFreeHandles() > 0)
			{
				m_AvailableHeapIds.insert(i);
			}
		}
	}

	D3D12DescAllocatorPage& D3D12DescAllocator::CreateAllocatorPage()
	{
		m_PagePool.emplace_back(std::make_unique<D3D12DescAllocatorPage>(m_HeapType, m_NumDescriptorsPerHeap));

		m_AvailableHeapIds.insert(m_PagePool.size() - 1);

		return *m_PagePool.back();
	}

} }
