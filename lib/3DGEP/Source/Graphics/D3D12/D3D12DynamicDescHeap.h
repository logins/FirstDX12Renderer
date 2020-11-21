/*!
 * Project: First D3D12 Renderer - https://github.com/logins/FirstDX12Renderer
 *
 * File: D3D12DynamicDescHeap.h
 *
 * Author: Riccardo Loggini
 */
#ifndef D3D12DynamicDescHeap_h__
#define D3D12DynamicDescHeap_h__

#include <queue>
#include <d3dx12.h>
#include <functional>

namespace GEPUtils { namespace Graphics { class D3D12PipelineState; } }

namespace GEPUtils { namespace Graphics {

	class D3D12CommandList;

// The purpose of D3D12DynamicDescHeap is to allocate descriptors on GPU (shader visible)
// that will store CBV, SRV, UAV and Samplers to the rendering pipeline.
// This is needed since D3D12DescAllocator class generate descriptors on CPU side only.
// TODO This class is designed without considering views' dynamic indexing functionality that started from shader model 5.1
// so it would be better to consider it as well.. more info at https://docs.microsoft.com/en-us/windows/desktop/direct3d12/dynamic-indexing-using-hlsl-5-1
class D3D12DynamicDescHeap
{
public:
	D3D12DynamicDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE InDescHeapType, uint32_t InNumDescPerHeap = 1024);

	void CreateD3d12DescHeap();

	~D3D12DynamicDescHeap();

	// Stage Descriptors: Stage CPU side descriptors to the table cache, that later will get uploaded to the GPU by calling CommitStagedDescriptors.
	void StageDescriptors(uint32_t InRootParamIndex, uint32_t InOffset, uint32_t InNumDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE InCPUDescHandle);

	// Commit Staged Descriptors: Commit the staged descriptors to a descriptor heap in GPU (shader visible)
	// It will then also set such descriptors in the pipeline!
	void CommitStagedDescriptorsForDraw(GEPUtils::Graphics::D3D12CommandList& InCmdList);
	void CommitStagedDescriptorsForDispatch(GEPUtils::Graphics::D3D12CommandList& InCmdList);

	// Upload Single Descriptor: Immediately order the copy of a descriptor from CPU to a shader visible descriptor heap
	// (useful for cases of clearing views)
	D3D12_GPU_DESCRIPTOR_HANDLE UploadSingleDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE InCPUDescHandle);

	// Scans the root signature to find the bound descriptor tables and the number of descriptors referenced by each one of them
	void ParseRootSignature(GEPUtils::Graphics::D3D12PipelineState& InPipelineState);

	// Releases resources and resets parameters
	void Reset();

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetInner();

	static D3D12DynamicDescHeap& Get(D3D12_DESCRIPTOR_HEAP_TYPE InType);

private:

	void CommitStagedDescriptors_Internal(GEPUtils::Graphics::D3D12CommandList& InCmdList, std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> InSetFn);

	uint32_t ComputeStaleDescriptorCount() const;

	static const uint32_t m_MaxDescTablesNum = 32;

	struct RootTableEntry
	{
		void Reset() { m_DescriptorsNum = 0; m_BaseCPUDescHandle = nullptr;	}
		uint32_t m_DescriptorsNum = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE* m_BaseCPUDescHandle = nullptr;
	};

	D3D12_DESCRIPTOR_HEAP_TYPE m_DescHeapType;

	uint32_t m_DescPerHeapNum;

	uint32_t m_DescHandleIncrementSize;

	std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> m_DescHandleCache;

	RootTableEntry m_RootTableCache[m_MaxDescTablesNum];

	// Every bit represents an index for the descriptor table. 
	// If the n-th bit stores a value of 1, it means that the current root signature has a root table at index n
	uint32_t m_DescTableBitMask = 0;

	// Indices of the desc tables that have changed and need to be copied to GPU
	uint32_t m_StaleDescTableBitMask = 0;

	std::queue<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> m_DescHeapPool; // We are using a single desc heap per each type for simplicity

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_D3d12DescHeap;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_CurrentCPUDescHandle = D3D12_DEFAULT;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_CurrentGPUDescHandle = D3D12_DEFAULT;

	uint32_t m_NumFreeHandles = 0;

	static D3D12DynamicDescHeap m_CbvSrvUavDescHeap;
	static D3D12DynamicDescHeap m_SamplerDescHeap;
};

} }

#endif // D3D12DynamicDescHeap_h__