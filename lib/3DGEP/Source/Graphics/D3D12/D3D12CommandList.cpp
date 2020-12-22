#include "D3D12CommandList.h"
#include "GraphicsTypes.h"
#include "d3dx12.h"
#include "D3D12GEPUtils.h"
#include "D3D12UtilsInternal.h"
#include "D3D12Device.h"
#include "D3D12PipelineState.h"
#include "D3D12Window.h"
#include "D3D12GpuDescHeap.h"
#include "GEPUtils.h"

namespace GEPUtils { namespace Graphics {

	D3D12CommandList::D3D12CommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> InCmdList, GEPUtils::Graphics::Device& InOwningDevice) 
		: CommandList(InOwningDevice), m_D3D12CmdList(InCmdList), m_DynamicBufferPageIdx(GEPUtils::Graphics::D3D12BufferAllocator::Get().ReservePage())
	{
	}

	void D3D12CommandList::ResourceBarrier(GEPUtils::Graphics::Resource& InResource, GEPUtils::Graphics::RESOURCE_STATE InPrevState, GEPUtils::Graphics::RESOURCE_STATE InAfterState)
	{
		CD3DX12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			static_cast<D3D12GEPUtils::D3D12Resource&>(InResource).GetInner().Get(),
			// We can be sure that the previous state was present because in this application all the render targets
			// are first filled and then presented to the main window repetitevely.
			D3D12GEPUtils::ResourceStateTypeToD3D12(InPrevState), D3D12GEPUtils::ResourceStateTypeToD3D12(InAfterState));

		m_D3D12CmdList->ResourceBarrier(1, &transitionBarrier);
	}

	void D3D12CommandList::ClearRTV(GEPUtils::Graphics::CpuDescHandle& InDescHandle, float* InColor)
	{
		m_D3D12CmdList->ClearRenderTargetView(static_cast<D3D12GEPUtils::D3D12CpuDescriptorHandle&>(InDescHandle).GetInner(), InColor, 0, nullptr);
	}

	void D3D12CommandList::ClearDepth(GEPUtils::Graphics::CpuDescHandle& InDescHandle)
	{
		m_D3D12CmdList->ClearDepthStencilView(static_cast<D3D12GEPUtils::D3D12CpuDescriptorHandle&>(InDescHandle).GetInner(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	}

	void D3D12CommandList::Close()
	{
		m_D3D12CmdList->Close();
	}

	void D3D12CommandList::SetPipelineStateAndResourceBinder(Graphics::PipelineState& InPipelineState)
	{
		Graphics::D3D12PipelineState& d3d12PSO = static_cast<Graphics::D3D12PipelineState&>(InPipelineState);

		// Set PSO
		m_D3D12CmdList->SetPipelineState(d3d12PSO.GetInnerPSO().Get());

		// Set root signature
		m_D3D12CmdList->SetGraphicsRootSignature(d3d12PSO.GetInnerRootSignature().Get());

		// Bind descriptor heap(s)
		m_D3D12CmdList->SetDescriptorHeaps(1, GEPUtils::Graphics::D3D12DescHeapFactory::GetGPUHeap().GetInner().GetAddressOf());

		// GPU desc heap parse root signature
		Graphics::D3D12GpuDescHeap::Get(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).ParseRootSignature(d3d12PSO);
	}

	void D3D12CommandList::SetInputAssemblerData(GEPUtils::Graphics::PRIMITIVE_TOPOLOGY InPrimTopology, GEPUtils::Graphics::VertexBufferView& InVertexBufView, GEPUtils::Graphics::IndexBufferView& InIndexBufView)
	{
		m_D3D12CmdList->IASetPrimitiveTopology(D3D12GEPUtils::PrimitiveTopoToD3D12(InPrimTopology));
		m_D3D12CmdList->IASetVertexBuffers(0, 1, &static_cast<D3D12GEPUtils::D3D12VertexBufferView&>(InVertexBufView).m_VertexBufferView);
		m_D3D12CmdList->IASetIndexBuffer(&static_cast<D3D12GEPUtils::D3D12IndexBufferView&>(InIndexBufView).m_IndexBufferView);
	}

	void D3D12CommandList::SetViewportAndScissorRect(GEPUtils::Graphics::ViewPort& InViewport, GEPUtils::Graphics::Rect& InScissorRect)
	{
		// Creating a viewport and scissor rect on the fly
		m_D3D12CmdList->RSSetViewports(1, &static_cast<D3D12GEPUtils::D3D12ViewPort&>(InViewport).D3d12Viewport);
		m_D3D12CmdList->RSSetScissorRects(1, &static_cast<D3D12GEPUtils::D3D12Rect&>(InScissorRect).D3d12Rect);
	}

	void D3D12CommandList::SetRenderTargetFromWindow(GEPUtils::Graphics::Window& InWindow)
	{
		D3D12GEPUtils::D3D12Window& d3d12Window = static_cast<D3D12GEPUtils::D3D12Window&>(InWindow);
		m_D3D12CmdList->OMSetRenderTargets(1, &d3d12Window.GetCurrentRTVDescHandle(), FALSE, &d3d12Window.GetCuttentDSVDescHandle());
	}

	void D3D12CommandList::SetGraphicsRootConstants(uint64_t InRootParameterIndex, uint64_t InNum32BitValuesToSet, const void* InSrcData, uint64_t InDestOffsetIn32BitValues)
	{
		m_D3D12CmdList->SetGraphicsRoot32BitConstants(InRootParameterIndex, InNum32BitValuesToSet, InSrcData, InDestOffsetIn32BitValues);
	}

	void D3D12CommandList::DrawIndexed(uint64_t InIndexCountPerInstance)
	{
		// Note: this will upload descriptors relative to descriptor tables on GPU and then reference them in the pipeline!
		m_StagedDescriptorManager.CommitStagedDescriptorsForDraw(*this);

		// Now that the descriptors are in GPU we can reference the relative views in the pipeline
		m_D3D12CmdList->DrawIndexedInstanced(InIndexCountPerInstance, 1, 0, 0, 0);
	}

	void D3D12CommandList::SetGraphicsRootTable(uint32_t InRootIndex, GEPUtils::Graphics::ConstantBufferView& InView)
	{
		m_D3D12CmdList->SetGraphicsRootDescriptorTable(InRootIndex, static_cast<D3D12GEPUtils::D3D12ConstantBufferView&>(InView).m_GpuAllocatedRange->m_FirstGpuHandle);
	}


	void D3D12CommandList::StoreAndReferenceDynamicBuffer(uint32_t InRootIndex, GEPUtils::Graphics::DynamicBuffer& InDynBuffer, GEPUtils::Graphics::ConstantBufferView& InResourceView)
	{
		// Create space for current Dynamic Buffer value
		D3D12BufferAllocator::Allocation dynamicAllocation = D3D12BufferAllocator::Get().Allocate(m_DynamicBufferPageIdx, InDynBuffer.GetBufferSize(), InDynBuffer.GetAlignSize()); // TODO here Allocation object will be lost.. we could probably need it in the future..

		// Copy buffer content in the allocation
		memcpy(dynamicAllocation.CPU, InDynBuffer.GetData(), InDynBuffer.GetDataSize());

		// Reference it in the view object
		static_cast<D3D12GEPUtils::D3D12ConstantBufferView&>(InResourceView).ReferenceBuffer(dynamicAllocation.GPU, InDynBuffer.GetBufferSize());

		// Stage View's descriptor for GPU heap insertion
		D3D12GEPUtils::D3D12ConstantBufferView& bufferView = static_cast<D3D12GEPUtils::D3D12ConstantBufferView&>(InResourceView);
			
		m_StagedDescriptorManager.StageDynamicDescriptors(InRootIndex, bufferView.GetCPUDescHandle(), bufferView.GetRangeSize());
	}

	void D3D12CommandList::UploadBufferData(GEPUtils::Graphics::Buffer& DestinationBuffer, GEPUtils::Graphics::Buffer& IntermediateBuffer, const void* InBufferData, size_t InDataSize)
	{
		// Now that both copy and dest resource are created on CPU, we can use them to update the corresponding GPU SubResource
		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = InBufferData; // Pointer to the memory block that contains the subresource data on CPU
		subresourceData.RowPitch = InDataSize; // Physical size in Bytes of the subresource data
		subresourceData.SlicePitch = subresourceData.RowPitch; // Size of each slice for the resource, since we assume only 1 slice, this corresponds to the size of the entire resource

		// Note: UpdateSubresources first uploads data in the intermediate resource, which is expected to be in shared memory (upload heap) 
		// and then transfers the content to the destination resource, most of the time in upload heap
		::UpdateSubresources(m_D3D12CmdList.Get(), static_cast<D3D12GEPUtils::D3D12Resource&>(DestinationBuffer).GetInner().Get(), static_cast<D3D12GEPUtils::D3D12Resource&>(IntermediateBuffer).GetInner().Get(), 0, 0, 1, &subresourceData);

	}

	void D3D12CommandList::UploadViewToGPU(GEPUtils::Graphics::ShaderResourceView& InSRV)
	{
		D3D12GEPUtils::D3D12ShaderResourceView& d3d12SRV = static_cast<D3D12GEPUtils::D3D12ShaderResourceView&>(InSRV);

		d3d12SRV.m_GpuAllocatedRange = GEPUtils::Graphics::D3D12DescHeapFactory::GetGPUHeap().AllocateStaticRange(1, d3d12SRV.GetCPUDescHandle()); // Note: we are assuming SRV too always reference a range of 1 descriptors
	}

	void D3D12CommandList::ReferenceSRV(uint32_t InRootIdx, GEPUtils::Graphics::ShaderResourceView& InSRV)
	{
		// TODO this will work for graphics command list only, it would also need to work for compute... so we would need to know the type of operation we are executing...

		m_D3D12CmdList->SetGraphicsRootDescriptorTable(InRootIdx, static_cast<D3D12GEPUtils::D3D12ShaderResourceView&>(InSRV).GetGPUDescHandle());
	}

	void D3D12CommandList::SetGraphicsRootDescriptorTable(uint32_t InRootIdx, D3D12_GPU_DESCRIPTOR_HANDLE InGpuDescHandle) { m_D3D12CmdList->SetGraphicsRootDescriptorTable(InRootIdx, InGpuDescHandle); }

	CD3DX12_GPU_DESCRIPTOR_HANDLE D3D12CommandList::CopyDynamicDescriptorsToBoundHeap(uint32_t InRangesNum, D3D12_CPU_DESCRIPTOR_HANDLE* InDescHandleArray, uint32_t* InRageSizeArray)
	{
		return GEPUtils::Graphics::D3D12DescHeapFactory::GetGPUHeap().CopyDynamicDescriptors(InRangesNum, InDescHandleArray, InRageSizeArray);
	}

	void D3D12CommandList::D3D12StagedDescriptorManager::StageDynamicDescriptors(uint32_t InRootParamIndex, D3D12_CPU_DESCRIPTOR_HANDLE InFirstCpuDescHandle, uint32_t InRangeSize)
	{
		m_DynamicTableRootIdx[m_CurrentStagedDynamicTablesNum] = InRootParamIndex;
		m_DynamicTableFirstHandle[m_CurrentStagedDynamicTablesNum] = InFirstCpuDescHandle;
		m_DynamicRangeSizes[m_CurrentStagedDynamicTablesNum] = InRangeSize;
		
		m_CurrentStagedDynamicTablesNum++;
	}

	void D3D12CommandList::D3D12StagedDescriptorManager::StageStaticDescriptors(uint32_t InRootParamIndex, D3D12_GPU_DESCRIPTOR_HANDLE InFirstGpuDescHandle)
	{
		m_StaticTableRootIdx[m_CurrentStagedStaticTablesNum] = InRootParamIndex;
		m_StaticTableFirstHandle[m_CurrentStagedStaticTablesNum] = InFirstGpuDescHandle;

		m_CurrentStagedStaticTablesNum++;
	}

	void D3D12CommandList::D3D12StagedDescriptorManager::CommitStagedDescriptorsForDraw(D3D12CommandList& InCmdList)
	{
		CommitStagedDescriptors_Internal(InCmdList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
	}

	void D3D12CommandList::D3D12StagedDescriptorManager::CommitStagedDescriptors_Internal(D3D12CommandList& InCmdList, std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> InSetFn)
	{
		GEPUtils::Graphics::D3D12Device& d3d12Device = static_cast<GEPUtils::Graphics::D3D12Device&>(Graphics::GetDevice());
		uint32_t descSize = d3d12Device.GetCbvSrvUavDescHandleSize();

		// Uploading dynamic descriptors to GPU and adding them to the current static table entries so they can be uploaded altogether to GPU
		CD3DX12_GPU_DESCRIPTOR_HANDLE newGpuDescFirstHandle = InCmdList.CopyDynamicDescriptorsToBoundHeap(m_CurrentStagedDynamicTablesNum, &m_DynamicTableFirstHandle[0], &m_DynamicRangeSizes[0]);

		while (m_CurrentStagedDynamicTablesNum)
		{
			m_CurrentStagedDynamicTablesNum--;
			InCmdList.SetGraphicsRootDescriptorTable(m_DynamicTableRootIdx[m_CurrentStagedDynamicTablesNum], CD3DX12_GPU_DESCRIPTOR_HANDLE(newGpuDescFirstHandle, m_CurrentStagedDynamicTablesNum, descSize));
		}

		// Setting static descriptors (the ones already uploaded to GPU)
		while (m_CurrentStagedStaticTablesNum)
		{
			m_CurrentStagedStaticTablesNum--;
			InCmdList.SetGraphicsRootDescriptorTable(m_StaticTableRootIdx[m_CurrentStagedStaticTablesNum], m_StaticTableFirstHandle[m_CurrentStagedStaticTablesNum]);
		}

		// Note: m_CurrentStagedDynamicTablesNum and m_CurrentStagedStaticTablesNum are already 0 at this point, so the array are effectively reset
	}

}
}