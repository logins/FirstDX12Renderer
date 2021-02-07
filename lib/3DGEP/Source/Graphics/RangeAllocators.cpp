/*
 RangeAllocators.cpp

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/
 
#include "RangeAllocators.h"
 
#include "GEPUtils.h"

namespace GEPUtils { namespace Graphics {

	StaticRangeAllocator::StaticRangeAllocator(uint32_t InStartingOffset, uint32_t InPoolSize)
	{
		m_StartingOffset = InStartingOffset;
		m_PoolSize = InPoolSize;
		// Adding the initial free allocated range
		FreeAllocatedRange(m_StartingOffset, InPoolSize);
	}

	uint32_t StaticRangeAllocator::AllocateRange(uint32_t InRangeSize)
	{
		// Find a range big enough to contain the range
		auto freeRangesIt = m_FreeRangesBySize.lower_bound(InRangeSize); //lower_bound returns an iterator with the first element Not less than the given key
		if (freeRangesIt == m_FreeRangesBySize.end()) {
			StopForFail("[StaticRangeAllocator] Not enough free spaces.")
			return 0;
		}

		RangeSize freeRangeSize = freeRangesIt->first;

		DescOffset freeRangeOffset = freeRangesIt->second->first;

		// Remove the chosen free range to use
		m_FreeRangesByOffset.erase(freeRangesIt->second);
		m_FreeRangesBySize.erase(freeRangesIt);

		// Compute new free range as the leftover from the allocation
		if (RangeSize newFreeSize = freeRangeSize - InRangeSize)
		{
			DescOffset newFreeOffset = freeRangeOffset + InRangeSize;
			FreeAllocatedRange(newFreeOffset, newFreeSize);
		}

		return freeRangeOffset;
	}

	void StaticRangeAllocator::FreeAllocatedRange(uint32_t InRangeOffset, uint32_t InRangeSize)
	{
		// Get next and previous free spaces to the declared offset, so that we can merge them
		FreeRangesByOffset::iterator nextFreeRangeIt = m_FreeRangesByOffset.upper_bound(InRangeOffset);

		FreeRangesByOffset::iterator prevFreeRangeIt = nextFreeRangeIt;

		if (nextFreeRangeIt != m_FreeRangesByOffset.begin()) // If the next free range is not the beginning of the list
			prevFreeRangeIt--;
		else
			prevFreeRangeIt = m_FreeRangesByOffset.end();

		// Now we have 4 possible cases: 
		// 1) The previous range finishes where the new free range starts.
		// 2) The next range starts where the new range ends.
		// 3) Cases 1) and 2) happen simultaneously.
		// 4) Both 1) and 2) cases do not happen.

		// 1) The previous range finishes where the new free range starts.
		if (prevFreeRangeIt != m_FreeRangesByOffset.end() && prevFreeRangeIt->first + InRangeSize == prevFreeRangeIt->second.m_Size) // Note: we are not checking for any validity on the input parameters
		{
			// Merging the previous free range with the current one: create a free range to contain both, and delete the previous free block
			InRangeSize += prevFreeRangeIt->second.m_Size;
			InRangeOffset = prevFreeRangeIt->first;
			// Remove previous free range from the two maps
			m_FreeRangesBySize.erase(prevFreeRangeIt->second.m_FreeRangeBySizeIt);
			m_FreeRangesByOffset.erase(prevFreeRangeIt);
		}
		// 2) The next range starts where the new range ends.
		if (nextFreeRangeIt != m_FreeRangesByOffset.end() && nextFreeRangeIt->first == InRangeOffset + InRangeSize)
		{
			// Merging the next free range with the current one
			InRangeSize += nextFreeRangeIt->second.m_Size;
			// Removing next free range
			m_FreeRangesBySize.erase(nextFreeRangeIt->second.m_FreeRangeBySizeIt);
			m_FreeRangesByOffset.erase(nextFreeRangeIt);
		}
		// Both 3) and 4) cases are automatically handled within the previous if statements!
		
		// Adding the new resulting free range
		std::pair<FreeRangesByOffset::iterator, bool> freeRangeOffsetIt = m_FreeRangesByOffset.emplace(InRangeOffset, InRangeSize); // Note: map.emplace(..) here returns a std::pair<FreeRangesByOffset::iterator, bool> !!
		FreeRangesBySize::iterator freeRangeSizeIt = m_FreeRangesBySize.emplace(InRangeSize, freeRangeOffsetIt.first); // Note: multimap.emplace(..) returns the iterator directly, and not a pair like the map !!
		freeRangeOffsetIt.first->second.m_FreeRangeBySizeIt = freeRangeSizeIt;
	}

	LinearRangeAllocator::LinearRangeAllocator(uint32_t InStartingOffset, uint32_t InPoolSize)
	{
		m_StartingOffset = InStartingOffset;
		m_CurrentOffset = InStartingOffset;
		m_AllocationLimit = InStartingOffset + InPoolSize;
		m_PoolSize = InPoolSize;
	}

	uint32_t LinearRangeAllocator::AllocateRange(uint32_t InRangeSize)
	{
		m_CurrentOffset += InRangeSize;
		
		if (m_CurrentOffset > m_AllocationLimit)
		{
			StopForFail("[LinearRangeAllocator] Not enough space");
		}
		
		return m_CurrentOffset - InRangeSize;
	}

	void LinearRangeAllocator::SetAdmittedAllocationRegion(float InStartPercentage, float InEndPercentage)
	{
		m_CurrentOffset = m_StartingOffset + std::ceil(m_PoolSize * InStartPercentage);

		m_AllocationLimit = m_StartingOffset + std::trunc(m_PoolSize * InEndPercentage);
	}

} }