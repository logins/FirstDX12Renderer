/*!
 * Project: First D3D12 Renderer - https://github.com/logins/FirstDX12Renderer
 *
 * File: D3D12DescAllocatorPage.cpp
 *
 * Author: Riccardo Loggini
 */
 
#include "D3D12DescAllocatorPage.h"
#include "Public/D3D12GEPUtils.h"
#include "D3D12Device.h"
#include "../../Public/GEPUtils.h"
#include "../../Public/Application.h"

namespace GEPUtils { namespace Graphics {


	void AllocatedDescRange::Init(D3D12_CPU_DESCRIPTOR_HANDLE InCPUDescHandle, uint32_t InNumHandles, uint32_t InDescSize, D3D12DescAllocatorPage& InDescAllocPage)
	{
		m_StartingCPUDescHandle = InCPUDescHandle; m_DescAllocatorPage = &InDescAllocPage; m_NumDescriptors = InNumHandles; m_DescSize = InDescSize;
	}

	AllocatedDescRange::AllocatedDescRange(D3D12DescAllocatorPage& InDescAllocPage)
		: m_StartingCPUDescHandle({ 0 }), m_DescAllocatorPage(&InDescAllocPage), m_NumDescriptors(0), m_DescSize(0)
	{
	}

	D3D12_CPU_DESCRIPTOR_HANDLE AllocatedDescRange::GetDescHandleAt(uint32_t InDescRangeOffset /*= 0*/)
	{
		Check(InDescRangeOffset < m_NumDescriptors)
		return D3D12_CPU_DESCRIPTOR_HANDLE{ m_StartingCPUDescHandle.ptr + (InDescRangeOffset * m_DescSize) };
	}

	AllocatedDescRange::~AllocatedDescRange()
	{
		m_DescAllocatorPage->DeclareStale(*this);
	}

	D3D12DescAllocatorPage::D3D12DescAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE InHeapType, uint32_t InNumDescriptors)
		: m_D3D12DescHeapType(InHeapType), m_NumTotalDescriptors(InNumDescriptors)
	{
		ID3D12Device2* d3d12Device = static_cast<GEPUtils::Graphics::D3D12Device&>(Graphics::GetDevice()).GetInner().Get();

		// Allocate desc heap for the current page
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = InHeapType;
		heapDesc.NumDescriptors = InNumDescriptors;

		D3D12GEPUtils::ThrowIfFailed(d3d12Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_D3D12DescHeap)));

		m_BaseCPUDescriptor = m_D3D12DescHeap->GetCPUDescriptorHandleForHeapStart();

		m_DescHandleIncrementSize = d3d12Device->GetDescriptorHandleIncrementSize(m_D3D12DescHeapType);

		m_NumFreeHandles = m_NumTotalDescriptors;

		// Initialize the free lists with a single unique free range
		AddNewRange(0, m_NumTotalDescriptors);
	}

	bool D3D12DescAllocatorPage::HasSpace(uint32_t InNumDescriptors) const
	{
		return m_FreeListBySize.lower_bound(InNumDescriptors) != m_FreeListBySize.end();
	}

	bool D3D12DescAllocatorPage::AllocateRangeIfPossible(uint32_t InNumDescriptors, AllocatedDescRange& OutDescRange)
	{
		if (InNumDescriptors > m_NumFreeHandles)
		{
			StopForFail("Trying to allocate descriptor range without enough space on page.")
			return false; // Allocation not possible

		}

		// Get the one of the smallest range big enough to contain the requested allocation
		auto smallestFreeRangeListIt = m_FreeListBySize.lower_bound(InNumDescriptors); // Note: std::multimap::lower_bound returns an iterator to the first element not less than the given key !
		if (smallestFreeRangeListIt == m_FreeListBySize.end())
		{
			StopForFail("No free range big enough for the requested allocation.")
			return false; // Allocation not possible
		}

		DescRangeSizeType rangeSize = smallestFreeRangeListIt->first;
			
		FreeListByOffset::iterator offsetListIt = smallestFreeRangeListIt->second; // Note: storing the iterator of the offset list into the free list entry will allow to retrieve the offset element in constant time O(1) instead of having to use std::map::find which is O(log(n))

		DescOffsetType descOffset = offsetListIt->first;

		// Remove the free range we are going to use for the allocation
		m_FreeListBySize.erase(smallestFreeRangeListIt);
		m_FreeListByOffset.erase(offsetListIt);

		// Compute the new free range as leftover from the allocation
		DescOffsetType newFreeOffset = descOffset + InNumDescriptors;

		DescRangeSizeType newFreeSize = rangeSize - InNumDescriptors;

		if (newFreeSize > 0) // If there are some leftover descriptors, create the new free range
		{
			AddNewRange(newFreeOffset, newFreeSize);
		}

		m_NumFreeHandles -= InNumDescriptors;

		OutDescRange.Init(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_BaseCPUDescriptor, descOffset, m_DescHandleIncrementSize), InNumDescriptors, m_DescHandleIncrementSize,*this);

		return true;
	}

	uint32_t D3D12DescAllocatorPage::ComputeNumDescriptorsOffset(D3D12_CPU_DESCRIPTOR_HANDLE InDescHandle)
	{
		return static_cast<uint32_t>(InDescHandle.ptr - m_BaseCPUDescriptor.ptr) / m_DescHandleIncrementSize;
	}

	void D3D12DescAllocatorPage::DeclareStale(AllocatedDescRange& InDescAllocation)
	{
		if (&InDescAllocation.GetRelativeAllocatorPage() != this)
			std::invalid_argument("Trying to declare stale a range from another page");

		uint32_t numDescOffset = ComputeNumDescriptorsOffset(InDescAllocation.GetDescHandleAt());

		m_StaleDescriptors.emplace(numDescOffset, InDescAllocation.GetNumHandles(), Application::Get()->GetCurrentFrameNumber());

		// Removing references from the stale allocation object
		InDescAllocation.m_DescSize = 0;
		InDescAllocation.m_NumDescriptors = 0;
		InDescAllocation.m_StartingCPUDescHandle.ptr = 0;
	}

	void D3D12DescAllocatorPage::ReleaseStaleDescriptors(uint64_t InFrameNumber)
	{
		while (!m_StaleDescriptors.empty() && m_StaleDescriptors.front().m_FrameNumber <= InFrameNumber) // Release all descriptors allocated in a frame older or equal to the input one
		{
			StaleDescRange& staleDescRange = m_StaleDescriptors.front();

			auto descRangeStartingOffset = staleDescRange.m_Offset;

			auto numDescriptors = staleDescRange.m_DescSize;

			FreeRange(descRangeStartingOffset, numDescriptors);

			m_StaleDescriptors.pop();
		}
	}

	void D3D12DescAllocatorPage::AddNewRange(uint32_t InStartingOffset, uint32_t InNumDescriptors)
	{
		// Adding to the offset list
		auto offsetIt = m_FreeListByOffset.emplace(InStartingOffset, InNumDescriptors);
		// Adding correspondent to the size list
		auto sizeIt = m_FreeListBySize.emplace(InNumDescriptors, offsetIt.first);

		// Adding reference of the size list into the allocation object from the offset list
		offsetIt.first->second.m_FreeListBySizeIt = sizeIt;
	}

	void D3D12DescAllocatorPage::FreeRange(uint32_t InStartingOffset, uint32_t InNumDescriptors)
	{
		// Get reference for the previous and next free range to the input one, so that if they are contiguous we can merge them

		auto nextRangeIt = m_FreeListByOffset.upper_bound(InStartingOffset);

		auto prevRangeIt = nextRangeIt;

		if (nextRangeIt != m_FreeListByOffset.begin()) // If the next free range is not the beginning of the free rages list
			prevRangeIt--; // prev range will be the previous of the next
		else
			prevRangeIt = m_FreeListByOffset.end(); // otherwise we are not going to use prevRangeIt

		m_NumFreeHandles += InNumDescriptors; // Increment free descriptor counter

		// Now we have 4 possible cases: 
		// 1) The previous range finishes where the new free range starts.
		// 2) The next range starts where the new range ends.
		// 3) Cases 1) and 2) happen simultaneously.
		// 4) Both 1) and 2) cases do not happen.

		// 1) The previous range finishes where the new free range starts.
		if (prevRangeIt != m_FreeListByOffset.end() && InStartingOffset == prevRangeIt->first + prevRangeIt->second.m_Size) // TODO what if the input range to free overlaps?
		{
			// Merging the previous free range with the input one
			InStartingOffset = prevRangeIt->first;
			InNumDescriptors += prevRangeIt->second.m_Size;

			// Remove previous free range from the list
			m_FreeListBySize.erase(prevRangeIt->second.m_FreeListBySizeIt);
			m_FreeListByOffset.erase(prevRangeIt);
		}

		// 2) The next range starts where the new range ends.
		if (nextRangeIt != m_FreeListByOffset.end() && InStartingOffset + InNumDescriptors == nextRangeIt->first)
		{
			// Merging the next free range with the input one
			InNumDescriptors += nextRangeIt->second.m_Size;

			// Removing nect free range from list
			m_FreeListBySize.erase(nextRangeIt->second.m_FreeListBySizeIt);
			m_FreeListByOffset.erase(nextRangeIt);
		}

		// At this point Case 3) and 4) have been handled automatically

		// Add the new free range to the list
		AddNewRange(InStartingOffset, InNumDescriptors);
	}


} }
