/*!
 * Project: First D3D12 Renderer - https://github.com/logins/FirstDX12Renderer
 *
 * File: D3D12DescAllocatorPage.h
 *
 * Author: Riccardo Loggini
 */
#ifndef D3D12DescAllocatorPage_h__
#define D3D12DescAllocatorPage_h__

#include <map>
#include <queue>
#include <mutex>
#include <wrl.h>
#include <d3dx12.h>

namespace GEPUtils { namespace Graphics {

	class D3D12DescAllocatorPage;

	struct AllocatedDescRange
	{
		friend D3D12DescAllocatorPage; // D3D12DescAllocatorPage has full control over DescRangeAllocation

		AllocatedDescRange() = default;

		// Creates an empty
		AllocatedDescRange(D3D12DescAllocatorPage& InDescAllocPage);

		// Destructor will automatically free the allocation
		~AllocatedDescRange();

		void SetGpuDescHandle(D3D12_GPU_DESCRIPTOR_HANDLE InHandle) { m_StartingGPUDescHandle = InHandle; }

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescHandle() const { return m_StartingGPUDescHandle; }

		// Gets a specific descriptor of the range at the given offset
		D3D12_CPU_DESCRIPTOR_HANDLE GetDescHandleAt(uint32_t InDescRangeOffset = 0);

		uint32_t GetNumHandles() const { return m_NumDescriptors; }

		D3D12DescAllocatorPage& GetRelativeAllocatorPage() const { return *m_DescAllocatorPage; }

		bool IsValid() const { return m_StartingCPUDescHandle.ptr != 0; }

	private:
		void Init(D3D12_CPU_DESCRIPTOR_HANDLE InCPUDescHandle, uint32_t InNumHandles, uint32_t InDescSize, D3D12DescAllocatorPage& InDescAllocPage);

		D3D12DescAllocatorPage* m_DescAllocatorPage = nullptr; // Note: this needs to be a pointer because copy assignment is deleted for this class and with a reference we could not assign it
		D3D12_CPU_DESCRIPTOR_HANDLE m_StartingCPUDescHandle = { 0 };
		uint32_t m_NumDescriptors = 0;
		uint32_t m_DescSize = 0;

		D3D12_GPU_DESCRIPTOR_HANDLE m_StartingGPUDescHandle = { 0 };
	};

/*!
 * \class D3D12DescAllocatorPage
 *
 * \brief Wrapper for a single ID3D12DescriptorHeap, it handles the allocation and removal of descriptor ranges in it.
 *
 * - This allocator works based on the available free spaces, to achieve logarithmic time for insertion, removal and random access of descriptors.
 * - To achieve such features, it relies on two maps, here a std::map and a std::multimap. More details about this later.
 * - Used by D3D12DescAllocator.
 * - Heavily inspired to https://www.3dgep.com/learning-directx-12-3/#DescriptorAllocatorPage_Class
 */
	class D3D12DescAllocatorPage
	{

	public:
		D3D12DescAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE InHeapType, uint32_t InNumDescriptors);

		inline D3D12_DESCRIPTOR_HEAP_TYPE GetDescHeapType() const { return m_D3D12DescHeapType; }

		bool HasSpace(uint32_t InNumDescriptors) const;

		inline uint32_t GetNumFreeHandles() const { return m_NumFreeHandles; }

		// Returns true if the allocation succeeded
		bool AllocateRangeIfPossible(uint32_t InNumDescriptors, AllocatedDescRange& OutDescRange);

		void DeclareStale(AllocatedDescRange& InDescHandle);

		// Should be called at the end of the frame
		void ReleaseStaleDescriptors(uint64_t InFrameNumber);

	protected:
		// Computes the number of descriptors offset from the start of the heap to the input
		uint32_t ComputeNumDescriptorsOffset(D3D12_CPU_DESCRIPTOR_HANDLE InDescHandle);

		// Adds a new range of descriptors in the heap, starting from the specified offset
		void AddNewRange(uint32_t InStartingOffset, uint32_t InNumDescriptors);

		void FreeRange(uint32_t InStartingOffset, uint32_t InNumDescriptors);

	private:
		using DescOffsetType = uint32_t;
		using DescRangeSizeType = uint32_t;

		struct FreeDescRange;
		/**
		 * FreeListByOffset and FreeListBySize map types (corresponding to a map each) are the main point of D3D12DescAllocatorPage.
		 * 
		 * FreeListByOffset takes account of the free descriptor ranges on the descriptor heap, ordered by their starting offset (Note: std::map is ordered! Insert, delete and access is O(log(n))). With this map will be easy to retrieve a range and to merge two consecutive free ranges.
		 * 
		 * FreeListBySize groups descriptor ranges by their size (Note: std::multimap is ordered! Insert, delete and access is O(log(n))) so to retrieve the first range big enough for an allocation will take O(log(n)).
		 */
		using FreeListByOffset = std::map<DescOffsetType, FreeDescRange>;

		using FreeListBySize = std::multimap<DescRangeSizeType, FreeListByOffset::iterator>;

		struct FreeDescRange //We do not need to store the page offset for a FreeDescRange because stored in FreeListByOffset
		{
			FreeDescRange(uint32_t InSize) : m_Size(InSize) {}

			uint32_t m_Size;

			FreeListBySize::iterator m_FreeListBySizeIt;
		};

		FreeListByOffset m_FreeListByOffset;
		FreeListBySize m_FreeListBySize;

		struct StaleDescRange
		{
			StaleDescRange(DescOffsetType InOffset, DescRangeSizeType InNumDescriptors, uint64_t InFrame)
				: m_Offset(InOffset), m_DescSize(InNumDescriptors), m_FrameNumber(InFrame)
			{}

			DescOffsetType m_Offset; DescRangeSizeType m_DescSize; uint64_t m_FrameNumber;
		};

		std::queue<StaleDescRange> m_StaleDescriptors;

		// D3D12 Member Vars
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_D3D12DescHeap;
		D3D12_DESCRIPTOR_HEAP_TYPE m_D3D12DescHeapType;

		CD3DX12_CPU_DESCRIPTOR_HANDLE m_BaseCPUDescriptor;

		uint32_t m_DescHandleIncrementSize;
		uint32_t m_NumTotalDescriptors;
		uint32_t m_NumFreeHandles;
	
		std::mutex m_AllocationMutex;
	};

} }
#endif // D3D12DescAllocatorPage_h__