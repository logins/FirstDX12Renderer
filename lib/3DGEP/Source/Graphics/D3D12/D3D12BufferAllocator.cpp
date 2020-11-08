#include "D3D12BufferAllocator.h"
#include "../../Public/GEPUtilsMath.h"
#include "../Public/D3D12GEPUtils.h"
#include "D3D12Device.h"

namespace GEPUtils{ namespace Graphics {

	D3D12BufferAllocator::D3D12BufferAllocator(size_t InPageSizeBytes /*= m_DefaultPageSize*/)
			: m_PageSize(InPageSizeBytes), m_CurrentPage(RequestPage())
	{

	}

	bool D3D12BufferAllocator::AllocateIfPossible(size_t InSizeBytes, size_t InAlignmentBytes, D3D12BufferAllocator::Allocation& OutAllocation)
	{
		if (InSizeBytes > m_PageSize)
			false; // We cannot allocate a size greater than the page size

		if (!m_CurrentPage.HasSpace(InSizeBytes, InAlignmentBytes))
		{
			m_CurrentPage = RequestPage();
		}

		if (m_CurrentPage.AllocateIfPossible(InSizeBytes, InAlignmentBytes, OutAllocation))
			return true;

		return false;
	}

	GEPUtils::Graphics::D3D12BufferAllocator::Page& D3D12BufferAllocator::RequestPage()
	{
		if (!m_FreePagesNum)
		{
			m_PagesPool.push_back(std::make_unique<Page>(m_PageSize));
			return *m_PagesPool.back();
		}
		else
		{
			return *m_PagesPool[m_PagesPool.size() - m_FreePagesNum--]; // m_FreePagesNum will be decreased after resolving the index
		}

		return *m_PagesPool.back();
	}

	void D3D12BufferAllocator::Reset()
	{
		for (auto& page : m_PagesPool)
		{
			page->Reset();
		}

		m_FreePagesNum = m_PagesPool.size();
	}

	D3D12BufferAllocator::Page::Page(size_t InSizeBytes)
	{
		ID3D12Device2* d3d12Device = static_cast<Graphics::D3D12Device&>(Graphics::GetDevice()).GetInner().Get();

		// Allocates a resource in shared memory
		D3D12GEPUtils::ThrowIfFailed(d3d12Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(InSizeBytes),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_D3d12Resource)
		));

		m_GpuBasePtr = m_D3d12Resource->GetGPUVirtualAddress();
		// Opens a mapping channel between the resource on the GPU and the resource pointed by m_CpuBasePtr, 
		// meaning that all the changes made in this last one will be reflected on the GPU conterpart!
		m_D3d12Resource->Map(0, nullptr, &m_CpuBasePtr);
	}

	D3D12BufferAllocator::Page::~Page()
	{
		// Closes the mappping channel with the CPU side
		m_D3d12Resource->Unmap(0, nullptr);

		m_CpuBasePtr = nullptr;
		m_GpuBasePtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
	}

	bool D3D12BufferAllocator::Page::HasSpace(size_t InSizeBytes, size_t InAlignmentBytes)
	{
		size_t alignedInSize = GEPUtils::Math::Align(InSizeBytes, InAlignmentBytes);
		size_t alignedTakenSpaceSize = GEPUtils::Math::Align(m_TakenSpaceOffset, InAlignmentBytes);

		return alignedInSize + alignedTakenSpaceSize <= m_PageSize;
	}

	// Note: This is NOT thread-safe !
	bool D3D12BufferAllocator::Page::AllocateIfPossible(size_t InSizeBytes, size_t InAlignmentBytes, D3D12BufferAllocator::Allocation& OutAllocation)
	{
		size_t alignedInSize = GEPUtils::Math::Align(InSizeBytes, InAlignmentBytes);

		m_TakenSpaceOffset = GEPUtils::Math::Align(m_TakenSpaceOffset, InAlignmentBytes); // taken space offset is now aligned with this allocation alignment

		if(alignedInSize + m_TakenSpaceOffset > m_PageSize)
			return false; // Not enough space, before attempting allocation we should check to have enough space

		OutAllocation.CPU = static_cast<uint8_t*>(m_CpuBasePtr) + m_TakenSpaceOffset; // CPU pointer for the start of the allocation
		
		OutAllocation.GPU = m_GpuBasePtr + m_TakenSpaceOffset; // GPU pointer for the start of the allocation

		m_TakenSpaceOffset += alignedInSize; // Update the taken space offset

		return true;
	}

	void D3D12BufferAllocator::Page::Reset()
	{
		m_TakenSpaceOffset = 0;
	}

}
}