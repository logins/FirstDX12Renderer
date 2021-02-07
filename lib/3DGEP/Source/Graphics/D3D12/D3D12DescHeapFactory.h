/*
 D3D12DescHeapFactory.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef D3D12DescHeapFactory_h__
#define D3D12DescHeapFactory_h__

#include "d3dx12.h"
#include "GraphicsTypes.h"
#include <deque>

namespace GEPUtils { namespace Graphics {

	//This set of classes has been inspired by the implementations of 
	// 3DGEP: https://www.3dgep.com/learning-directx-12-3/
	// DiligentGraphics: http://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-descriptor-heaps/
	// with the difference that there is going to be a bit less abstraction and smaller classes.

	class D3D12DescriptorHeap;
	class RangeAllocator;

	struct DescAllocation {
		DescAllocation(D3D12_CPU_DESCRIPTOR_HANDLE InCPUHandle, uint32_t InRangeSize) : m_FirstCpuHandle(InCPUHandle), m_RangeSize(InRangeSize) { }
		DescAllocation(D3D12_CPU_DESCRIPTOR_HANDLE InCPUHandle, uint32_t InRangeSize, D3D12_GPU_DESCRIPTOR_HANDLE InGPUHandle)
			: m_FirstGpuHandle(InGPUHandle), m_FirstCpuHandle(InCPUHandle), m_RangeSize(InRangeSize) { }
		// First CPU descriptor handle in this allocation
		D3D12_CPU_DESCRIPTOR_HANDLE m_FirstCpuHandle = { 0 };
		// First GPU descriptor handle in this allocation
		D3D12_GPU_DESCRIPTOR_HANDLE m_FirstGpuHandle = { 0 };
		// Number of descriptors in the allocation
		uint32_t m_RangeSize = 0;
	};
	// A static desc allocation will free itself in the originating allocator upon destroy
	struct StaticDescAllocation : public DescAllocation {
		StaticDescAllocation(D3D12DescriptorHeap& InDescHeap, const DescAllocation& InDescAllocation)
			: m_DescHeap(InDescHeap), DescAllocation(InDescAllocation) { }
		~StaticDescAllocation();

		D3D12DescriptorHeap& m_DescHeap;
	};

	// A descriptor heap is fundamentally used to call Allocate Descriptor Range. It contains 2 allocators (dynamic and static) that will handle a portion of descriptors in the way they want.
	// If it is shader visible, the heap is also responsible to open and close mappings with GPU.
	class D3D12DescriptorHeap {
	public:
		D3D12DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE InType, bool IsShaderVisible, uint32_t InDescSize, uint32_t InDescriptorsNum = 256, float InStaticDescPercentage = 1.0f);
		
		~D3D12DescriptorHeap(); //  Define a destructor is needed to forward declare unique_ptr members that use forward declared object types (e.g. std::unique_ptr<RangeAllocator> )

		void SetAllowedDynamicAllocationRegion(float InStartPercentage, float InEndPercentage);

		std::unique_ptr<StaticDescAllocation> AllocateStaticRange(uint32_t InRangeSize);
		// This version allocates a range and directly copies descriptors from an input cpu handle
		std::unique_ptr<StaticDescAllocation> AllocateStaticRange(uint32_t InRangeSize, D3D12_CPU_DESCRIPTOR_HANDLE InStartingCpuHandleToCopyFrom);
		
		void FreeAllocatedStaticRange(const D3D12_CPU_DESCRIPTOR_HANDLE& InFirstCpuHandle, uint32_t InRangeSize);

		DescAllocation AllocateDynamicRange(uint32_t InRangeSize);

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetInner() const { return m_D3D12DescHeap; }

		CD3DX12_GPU_DESCRIPTOR_HANDLE CopyDynamicDescriptors(uint32_t InRangesNum, D3D12_CPU_DESCRIPTOR_HANDLE* InDescHandleArray, uint32_t InRageSizeArray[]);

	private:
		uint32_t CpuDescToAllocatorOffset(const D3D12_CPU_DESCRIPTOR_HANDLE& InCpuHandle);
		D3D12_CPU_DESCRIPTOR_HANDLE AllocatorOffsetToCpuDesc(uint32_t InIndex);

		D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
		bool m_IsShaderVisible;
		D3D12_CPU_DESCRIPTOR_HANDLE m_FirstCpuDesc;
		D3D12_GPU_DESCRIPTOR_HANDLE m_FirstGpuDesc;
		uint32_t m_DescSize;
		uint32_t m_DescriptorsNum;
		std::unique_ptr<RangeAllocator> m_StaticDescAllocator;
		std::unique_ptr<RangeAllocator> m_DynamicDescAllocator;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_D3D12DescHeap;
	};

	// Heap factory purpose is to statically return CPU and GPU heaps.
	// For simplicity and in respect to our use cases, we only need one CPU and one GPU descriptor heaps.
	class D3D12DescHeapFactory 
	{
	public:
		D3D12DescHeapFactory();

		~D3D12DescHeapFactory();

		static D3D12DescHeapFactory* Get();

		static void ShutDown() { m_Instance.reset(); }

		// Caches descriptors on CPU side
		static D3D12DescriptorHeap& GetCPUHeap();
		// Shader visible CBV_SRV_UAV descriptor heap used by the command lists
		static D3D12DescriptorHeap& GetGPUHeap();

		GEPUtils::Graphics::ResourceView& AddViewObject(std::unique_ptr<GEPUtils::Graphics::ResourceView> InResourceView);

	private:

		// The desc heap factory owns view objects since views are stored in desc heaps
		std::deque<std::unique_ptr<GEPUtils::Graphics::ResourceView>> m_ResourceViewArray;

		// TODO delete copy construct and assignment op
		static std::unique_ptr<D3D12DescHeapFactory> m_Instance;
		std::unique_ptr<D3D12DescriptorHeap> m_CPUDescHeap;
		std::unique_ptr<D3D12DescriptorHeap> m_GPUDescHeap;
	};

} }
#endif // D3D12DescHeap_h__