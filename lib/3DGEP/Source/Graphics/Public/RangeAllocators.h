/*
 RangeAllocators.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef RangeAllocators_h__
#define RangeAllocators_h__

#include <map>

namespace GEPUtils { namespace Graphics {

	// Note: a range allocator is a generic class, and it's purpose relies on working with indices. It just knows that there are a pool of indices, and we can request ranges of them.
	class RangeAllocator {
	public:
		RangeAllocator(uint32_t InStartingOffset, uint32_t InPoolSize);

		// Returns the offset of the range
		virtual uint32_t AllocateRange(uint32_t InRangeSize) = 0;
		virtual void FreeAllocatedRange(uint32_t InStartingIndex, uint32_t InRangeSize) = 0;
	protected:
		RangeAllocator() = default;

		uint32_t m_StartingOffset, m_PoolSize;
	};

	class StaticRangeAllocator : public GEPUtils::Graphics::RangeAllocator
	{
	public:
		StaticRangeAllocator(uint32_t InStartingOffset, uint32_t InPoolSize);

		virtual uint32_t AllocateRange(uint32_t InRangeSize);

		virtual void FreeAllocatedRange(uint32_t InRangeOffset, uint32_t InRangeSize);
protected:
		StaticRangeAllocator() = default;
		// No copies, only moves are allowed
		StaticRangeAllocator(const StaticRangeAllocator&) = delete;
		StaticRangeAllocator& operator= (const StaticRangeAllocator&) = delete;
private:
		using DescOffset = uint32_t;
		using RangeSize = uint32_t;
		struct FreeRange;

		using FreeRangesByOffset = std::map<DescOffset, FreeRange>;
		using FreeRangesBySize = std::multimap<RangeSize, FreeRangesByOffset::iterator>;

		using FreeRageBySizeIt = FreeRangesBySize::iterator;
		struct FreeRange {
			FreeRange(uint32_t InSize) : m_Size(InSize) { }
			FreeRangesBySize::iterator m_FreeRangeBySizeIt;
			// Note: Free rage size will be also used as a key value for the map m_FreeRangesBySize down this class, 
			// so this parameter is effectively redundant, but we are keeping it here for convenience.
			uint32_t m_Size;
		};
		// This allocator uses 2 ordered maps: 
		// - FreeRangesByOffset: ordered map with unique key
		// - FreeRangesBySize: ordered multimap
		FreeRangesByOffset m_FreeRangesByOffset;
		
		FreeRangesBySize m_FreeRangesBySize;

	};

	class LinearRangeAllocator : public GEPUtils::Graphics::RangeAllocator
	{
	public:
		LinearRangeAllocator(uint32_t InStartingOffset, uint32_t InPoolSize);

		virtual uint32_t AllocateRange(uint32_t InRangeSize);

		// Trying to free a range in a DynamicRangeAllocator will do nothing at the moment, since this is a circular allocator
		// and all the allocations are executed in a linear manner, so it would not make sense to free a specific range...
		virtual void FreeAllocatedRange(uint32_t InRangeOffset, uint32_t InRangeSize) { } 

		// Sets the possible allocation region to a fraction of the whole pool size. Useful when we are allocating for multiple different frames.
		// If something tries to allocate more than the assigned buffer region, an assert will be called.
		// Current offset will be shifted at the start of the admitted allocation region
		void SetAdmittedAllocationRegion(float InStartPercentage, float InEndPercentage);

	protected:
		LinearRangeAllocator() = default;
		// No copies, only moves are allowed
		LinearRangeAllocator(const LinearRangeAllocator&) = delete;
		LinearRangeAllocator& operator= (const LinearRangeAllocator&) = delete;
	private:
		uint32_t m_CurrentOffset;
		uint32_t m_AllocationLimit;
	};

} }

#endif // RangeAllocators_h__
