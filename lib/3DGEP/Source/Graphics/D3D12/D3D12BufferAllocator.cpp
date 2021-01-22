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

namespace GEPUtils{ namespace Graphics {

	std::unique_ptr<GEPUtils::Graphics::D3D12BufferAllocator> D3D12BufferAllocator::m_Instance;

	D3D12BufferAllocator::D3D12BufferAllocator(size_t InPageSizeBytes /*= m_DefaultPageSize*/)
			: m_PageSize(InPageSizeBytes)/*, m_CurrentPage(RequestPage())*/
	{

	}

	GEPUtils::Graphics::D3D12BufferAllocator::D3D12BufferAllocator::Allocation D3D12BufferAllocator::Allocate(uint32_t InPageIdx, size_t InSizeBytes, size_t InAlignmentBytes)
	{
		// Note: We are assuming size and alignment already aligned with the platform requirements
		
		Allocation outAllocation = {};
		
		Check(InPageIdx < m_PagesPool.size())

		m_PagesPool[InPageIdx]->AllocateIfPossible(InSizeBytes, InAlignmentBytes, outAllocation);

		Check(outAllocation.CPU != nullptr)

		return outAllocation;
	}

	uint32_t D3D12BufferAllocator::ReservePage()
	{
		if (!m_FreePagesIdxPool.size())
		{
			m_PagesPool.push_back(std::make_unique<Page>(m_PageSize));
			return m_PagesPool.size()-1;
		}

		uint32_t freePageIdx = m_FreePagesIdxPool.back();
		m_FreePagesIdxPool.pop_back();
		return freePageIdx;
	}

	void D3D12BufferAllocator::ReleasePage(uint32_t InPageIdx)
	{
		// Index has to be valid and page not found in the available ones
		Check(InPageIdx < m_PagesPool.size() && std::find(m_FreePagesIdxPool.begin(), m_FreePagesIdxPool.end(), InPageIdx) == m_FreePagesIdxPool.end())

		m_PagesPool[InPageIdx]->Reset();

		m_FreePagesIdxPool.push_back(InPageIdx);
	}

	GEPUtils::Graphics::D3D12BufferAllocator& D3D12BufferAllocator::Get()
	{
		if (!m_Instance)
			m_Instance = std::make_unique<D3D12BufferAllocator>();


		return *m_Instance;
	}

	void D3D12BufferAllocator::Reset()
	{
		uint32_t tempIdx = 0;
		m_FreePagesIdxPool.clear(); // removes all the elements from a vector container, thus making its size 0

		for (auto& page : m_PagesPool)
		{
			m_FreePagesIdxPool.push_back(tempIdx);

			page->Reset();

			tempIdx++;
		}
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

	bool D3D12BufferAllocator::Page::AllocateIfPossible(size_t InAlignedSizeBytes, size_t InAlignmentBytes, D3D12BufferAllocator::Allocation& OutAllocation)
	{
		m_TakenSpaceOffset = GEPUtils::Math::Align(m_TakenSpaceOffset, InAlignmentBytes); // taken space offset is now aligned with this allocation alignment

		if(InAlignedSizeBytes + m_TakenSpaceOffset > m_PageSize)
			return false; // Not enough space, before attempting allocation we should check to have enough space

		OutAllocation.CPU = static_cast<uint8_t*>(m_CpuBasePtr) + m_TakenSpaceOffset; // CPU pointer for the start of the allocation
		
		OutAllocation.GPU = m_GpuBasePtr + m_TakenSpaceOffset; // GPU pointer for the start of the allocation

		m_TakenSpaceOffset += InAlignedSizeBytes; // Update the taken space offset

		return true;
	}

	void D3D12BufferAllocator::Page::Reset()
	{
		m_TakenSpaceOffset = 0;
	}

}
}