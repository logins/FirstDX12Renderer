/*!
 * Project: First D3D12 Renderer - https://github.com/logins/FirstDX12Renderer
 *
 * File: D3D12GpuDescHeap.cpp
 *
 * Author: Riccardo Loggini
 */
 
 #include "D3D12GpuDescHeap.h"
#include "d3dx12.h"
#include "D3D12Device.h"
#include "D3D12GEPUtils.h"
#include "GEPUtils.h"
#include "D3D12CommandList.h"
#include "D3D12PipelineState.h"

namespace GEPUtils { namespace Graphics {

	D3D12GpuDescHeap D3D12GpuDescHeap::m_CbvSrvUavDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12GpuDescHeap D3D12GpuDescHeap::m_SamplerDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	D3D12GpuDescHeap::D3D12GpuDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE InDescHeapType, uint32_t InDescPerHeapNum /*= 1024*/)
		: m_DescHeapType(InDescHeapType), m_DescPerHeapNum(InDescPerHeapNum)
	{
		auto d3d12Device = static_cast<Graphics::D3D12Device&>(Graphics::GetDevice()).GetInner().Get();

		m_DescHandleIncrementSize = d3d12Device->GetDescriptorHandleIncrementSize(InDescHeapType);

		m_DescHandleCache = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(InDescPerHeapNum);

		// Creating descriptor heap (assuming we are using a single one for the current object)
		CreateD3d12DescHeap();

	}

	void D3D12GpuDescHeap::CreateD3d12DescHeap()
	{
		auto d3d12Device = static_cast<Graphics::D3D12Device&>(Graphics::GetDevice()).GetInner().Get();

		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
		descHeapDesc.Type = m_DescHeapType;
		descHeapDesc.NumDescriptors = m_DescPerHeapNum;
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // Allocating it on GPU!

		D3D12GEPUtils::ThrowIfFailed(d3d12Device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&m_D3d12DescHeap)));

		// Static Descriptors will take the first half of the GPU desc heap
		m_CurrentStaticCPUDescHandle = m_D3d12DescHeap->GetCPUDescriptorHandleForHeapStart();
		m_CurrentStaticGPUDescHandle = m_D3d12DescHeap->GetGPUDescriptorHandleForHeapStart();
		m_NumFreeStaticHandles = m_DescPerHeapNum / 2;
		// Dynamic Descriptors will take the second half
		ResetDynamicDescPointers();

	}

	void D3D12GpuDescHeap::ResetDynamicDescPointers()
	{
		// Dynamic descriptors start from the second half of the descriptor heap
		m_CurrentDynamicCPUDescHandle.InitOffsetted(m_D3d12DescHeap->GetCPUDescriptorHandleForHeapStart(), m_DescPerHeapNum / 2, m_DescHandleIncrementSize);
		m_CurrentDynamicGPUDescHandle.InitOffsetted(m_D3d12DescHeap->GetGPUDescriptorHandleForHeapStart(), m_DescPerHeapNum / 2, m_DescHandleIncrementSize);
		m_NumFreeDynamicHandles = m_DescPerHeapNum/2;
	}

	D3D12GpuDescHeap::~D3D12GpuDescHeap()
	{
		Reset();
	}

	void D3D12GpuDescHeap::StageDynamicDescriptors(uint32_t InRootParamIndex, uint32_t InOffset, uint32_t InDescriptorsNum, const D3D12_CPU_DESCRIPTOR_HANDLE InCPUDescHandle)
	{
		// Cannot stage descriptors more than maximum per heap or at greater index than maximum
		Check(InDescriptorsNum <= m_DescPerHeapNum && InRootParamIndex < m_MaxDescTablesNum)

		RootTableEntry& tableEntry = m_RootTableCache[InRootParamIndex];

		// Check to not exceed the number of descriptors referenced by the descriptor table
		Check(InOffset + InDescriptorsNum <= tableEntry.m_DescriptorsNum)

		// Filling staged descriptor for the selected table entry
		D3D12_CPU_DESCRIPTOR_HANDLE* dstDescriptor = tableEntry.m_BaseCPUDescHandle + InOffset;
		for (uint32_t i = 0; i < InDescriptorsNum; i++)
		{
			dstDescriptor[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(InCPUDescHandle, i, m_DescHandleIncrementSize);
		}

		// Notify stale descriptors bitmask that the index of the selected table needs to be considered
		m_StaleDescTableBitMask |= (1 << InRootParamIndex);
	}

	void D3D12GpuDescHeap::CommitStagedDescriptors_Internal(GEPUtils::Graphics::D3D12CommandList& InCmdList, std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> InSetFn)
	{
		uint32_t numDescriptorsToUpload = ComputeStaleDescriptorCount();

		// If we don't have enough free descriptors, we reset the pointers and free handles and start from the beginning of the descriptor heap (used as a ring buffer)
		// We cannot override the same descriptors because they might be currently used by a command allocator in flight !!
		if (numDescriptorsToUpload > m_NumFreeDynamicHandles)
		{
			ResetDynamicDescPointers();
		}

		if (numDescriptorsToUpload)
		{
			auto device = static_cast<Graphics::D3D12Device&>(Graphics::GetDevice()).GetInner();

			auto d3d12GraphicsCmdList = InCmdList.GetInner().Get();

			DWORD currentRootIndex;
			uint32_t staleDescTablesMask = m_StaleDescTableBitMask;

			while (_BitScanForward(&currentRootIndex, staleDescTablesMask))
			{
				UINT descRangeSize = m_RootTableCache[currentRootIndex].m_DescriptorsNum;
				D3D12_CPU_DESCRIPTOR_HANDLE* baseCPUDescHandle = m_RootTableCache[currentRootIndex].m_BaseCPUDescHandle;

				device->CopyDescriptorsSimple(descRangeSize, m_CurrentDynamicCPUDescHandle, *baseCPUDescHandle, m_DescHeapType);

				// Set descriptors on the command list using the passed-in function
				// Note: this function will set the root table in the command list, 
				// so it is gonna be either ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable or ID3D12GraphicsCommandList::SetComputeRootDescriptorTable
				InSetFn(d3d12GraphicsCmdList, currentRootIndex, m_CurrentDynamicGPUDescHandle);

				// Offset current GPU and CPU desc heap handles
				m_CurrentDynamicCPUDescHandle.Offset(descRangeSize, m_DescHandleIncrementSize);
				m_CurrentDynamicGPUDescHandle.Offset(descRangeSize, m_DescHandleIncrementSize);
				m_NumFreeDynamicHandles -= descRangeSize;

				// Flip current root index in the stale root tables mask so we do not iterate over it again
				staleDescTablesMask ^= (1 << currentRootIndex);
			}
		}

		// TODO we can copy descriptor ranges altogether with a single call rather than 1 range at a time
	}

	void D3D12GpuDescHeap::CommitStagedDescriptorsForDraw(GEPUtils::Graphics::D3D12CommandList& InCmdList)
	{
		CommitStagedDescriptors_Internal(InCmdList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
	}

	void D3D12GpuDescHeap::CommitStagedDescriptorsForDispatch(GEPUtils::Graphics::D3D12CommandList& InCmdList)
	{
		CommitStagedDescriptors_Internal(InCmdList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE D3D12GpuDescHeap::UploadSingleStaticDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE InCPUDescHandle)
	{
		Check(m_NumFreeStaticHandles > 0)
		// Note: we are assuming a single desc heap will be used for the whole instance, and so assuming it will be big enough to contain all the descriptors we need

		auto device = static_cast<Graphics::D3D12Device&>(Graphics::GetDevice()).GetInner();
		D3D12_GPU_DESCRIPTOR_HANDLE allocatedGPUDescHandle = m_CurrentDynamicGPUDescHandle;

		device->CopyDescriptorsSimple(1, m_CurrentDynamicCPUDescHandle, InCPUDescHandle, m_DescHeapType);

		// Offset current CPU and GPU desc handles by 1
		m_CurrentStaticCPUDescHandle.Offset(m_DescHandleIncrementSize);
		m_CurrentStaticGPUDescHandle.Offset(m_DescHandleIncrementSize);

		m_NumFreeStaticHandles -= 1;

		return allocatedGPUDescHandle;
	}

	void D3D12GpuDescHeap::ParseRootSignature(GEPUtils::Graphics::D3D12PipelineState& InPipelineState)
	{
		// If the root signature changes, all descriptors must be rebound to the command list
		m_StaleDescTableBitMask = 0;

		const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC& rootSignatureDesc = InPipelineState.GetInnerRootSignatureDesc();

		m_DescTableBitMask = InPipelineState.GenerateRootTableBitMask();

		uint32_t numParameters = rootSignatureDesc.Version == D3D_ROOT_SIGNATURE_VERSION_1_1 ? rootSignatureDesc.Desc_1_1.NumParameters : rootSignatureDesc.Desc_1_0.NumParameters;
		uint32_t descTableMask = m_DescTableBitMask;
		uint32_t currentOffset = 0;
		DWORD rootIndex = 0;
		while (_BitScanForward(&rootIndex, descTableMask) && rootIndex < numParameters)
		{
			uint32_t DescriptorsNum = InPipelineState.GetRootDescriptorsNumAtIndex(rootIndex);

			RootTableEntry& rootTable = m_RootTableCache[rootIndex];
			rootTable.m_DescriptorsNum = DescriptorsNum;
			rootTable.m_BaseCPUDescHandle = m_DescHandleCache.get() + currentOffset;

			currentOffset += DescriptorsNum;

			// Flip current descriptor table bit so it is not scanned again
			descTableMask ^= (1 << rootIndex);
		}

		Check(currentOffset <= m_DescPerHeapNum); //The root signature requires more than the current maximum number of descriptors in the descriptor heap

	}

	void D3D12GpuDescHeap::Reset()
	{
		m_D3d12DescHeap.Reset(); // Note: we are resetting the ComPtr: it releases the interface associated with this ComPtr and returns a new reference count

		m_CurrentDynamicCPUDescHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
		m_CurrentDynamicGPUDescHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
		m_NumFreeDynamicHandles = 0;
		m_DescTableBitMask = 0;
		m_StaleDescTableBitMask = 0;

		// Reset table cache
		for (int32_t i = 0; i < m_MaxDescTablesNum; i++)
			m_RootTableCache[i].Reset();
	}

	GEPUtils::Graphics::D3D12GpuDescHeap& D3D12GpuDescHeap::Get(D3D12_DESCRIPTOR_HEAP_TYPE InType)
	{
		switch (InType)
		{
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
			return m_CbvSrvUavDescHeap;
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
			return m_SamplerDescHeap;
		default:
			StopForFail("Getting D3D12DynamicDescHeap of unsupported type.")
			break;
		}

		return m_CbvSrvUavDescHeap;
	}

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> D3D12GpuDescHeap::GetInner()
	{
		return m_D3d12DescHeap;
	}

	uint32_t D3D12GpuDescHeap::ComputeStaleDescriptorCount() const
	{
		uint32_t staleDescriptorsNum = 0;
		uint32_t staleDescMask = m_StaleDescTableBitMask;
		DWORD i = 0;

		// _BitScanForward from the lest significant bit, returns in i the fist non-zero bit index
		while (_BitScanForward(&i, staleDescMask))
		{
			staleDescriptorsNum++;
			staleDescMask ^= (1 << i); // swapping bit at current index (that will become 0) so it will not get considered by _BitScanForward on the next iteration
		}

		return staleDescriptorsNum;
	}

} }