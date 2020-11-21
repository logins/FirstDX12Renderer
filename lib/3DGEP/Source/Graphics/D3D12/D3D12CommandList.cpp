#include "D3D12CommandList.h"
#include "GraphicsTypes.h"
#include "d3dx12.h"
#include "D3D12GEPUtils.h"
#include "D3D12UtilsInternal.h"
#include "D3D12Device.h"
#include "D3D12PipelineState.h"
#include "../Public/D3D12Window.h"
#include "D3D12DynamicDescHeap.h"
#include "../../Public/GEPUtils.h"

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
			D3D12GEPUtils::ResStateTypeToD3D12(InPrevState), D3D12GEPUtils::ResStateTypeToD3D12(InAfterState));

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
		m_D3D12CmdList->SetDescriptorHeaps(1, GEPUtils::Graphics::D3D12DynamicDescHeap::Get(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).GetInner().GetAddressOf());

		// GPU desc heap parse root signature
		Graphics::D3D12DynamicDescHeap::Get(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).ParseRootSignature(d3d12PSO);
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
		Graphics::D3D12DynamicDescHeap::Get(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).CommitStagedDescriptorsForDraw(*this);

		// Now that the descriptors are in GPU we can reference the relative views in the pipeline
		m_D3D12CmdList->DrawIndexedInstanced(InIndexCountPerInstance, 1, 0, 0, 0);
	}

	void D3D12CommandList::SetGraphicsRootTable(uint32_t InRootIndex, GEPUtils::Graphics::ResourceView& InView)
	{
		// Now descriptor is staged for upload but not uploaded yet..!

		m_D3D12CmdList->SetGraphicsRootDescriptorTable(InRootIndex, static_cast<D3D12GEPUtils::D3D12ResourceView&>(InView).m_AllocatedDescRange->GetGPUDescHandle());
	}


	void D3D12CommandList::StoreAndReferenceDynamicBuffer(uint32_t InRootIndex, GEPUtils::Graphics::DynamicBuffer& InDynBuffer, GEPUtils::Graphics::ResourceView& InResourceView)
	{
		// Store Dynamic Buffer
		D3D12BufferAllocator::Allocation dynamicAllocation = D3D12BufferAllocator::Get().Allocate(m_DynamicBufferPageIdx, InDynBuffer.GetBufferSize(), InDynBuffer.GetAlignSize()); // TODO here Allocation object will be lost.. we could probably need it in the future..

		// Copy buffer content in the allocation
		memcpy(dynamicAllocation.CPU, InDynBuffer.GetData(), InDynBuffer.GetDataSize());

		// Reference it in the view object
		static_cast<D3D12GEPUtils::D3D12ConstantBufferView&>(InResourceView).ReferenceBuffer(dynamicAllocation.GPU, InDynBuffer.GetBufferSize());

		// Stage View's descriptor for GPU heap insertion
		if (InResourceView.GetType() == RESOURCE_VIEW_TYPE::CONSTANT_BUFFER)
		{
			D3D12GEPUtils::D3D12ConstantBufferView& bufferView = static_cast<D3D12GEPUtils::D3D12ConstantBufferView&>(InResourceView);
			
			GEPUtils::Graphics::D3D12DynamicDescHeap::Get(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).StageDescriptors(InRootIndex, 0, 1, bufferView.GetCPUDescHandle());
		}
		else
		{
			StopForFail("Other types of resource views not implemented")
		}
	}

}
}