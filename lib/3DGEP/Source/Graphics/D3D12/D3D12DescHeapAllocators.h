/*!
 * Project: First D3D12 Renderer - https://github.com/logins/FirstDX12Renderer
 *
 * File: D3D12DescHeapAllocators.h
 *
 * Author: Riccardo Loggini
 */
#ifndef D3D12DescHeapAllocators_h__
#define D3D12DescHeapAllocators_h__

#include "D3D12DescHeapFactory.h"
#include <map>

namespace GEPUtils { namespace Graphics {

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

	class DynamicRangeAllocator : public GEPUtils::Graphics::RangeAllocator
	{
	public:
		DynamicRangeAllocator(uint32_t InStartingOffset, uint32_t InPoolSize);

		virtual uint32_t AllocateRange(uint32_t InRangeSize);

		// Trying to free a range in a DynamicRangeAllocator will do nothing at the moment, since this is a circular allocator
		// and all the allocations are executed in a linear manner, so it would not make sense to free a specific range...
		virtual void FreeAllocatedRange(uint32_t InRangeOffset, uint32_t InRangeSize) { } 
	protected:
		DynamicRangeAllocator() = default;
		// No copies, only moves are allowed
		DynamicRangeAllocator(const DynamicRangeAllocator&) = delete;
		DynamicRangeAllocator& operator= (const DynamicRangeAllocator&) = delete;
	private:
		uint32_t m_CurrentOffset;
	};

} }
#endif // D3D12DescHeapAllocators_h__