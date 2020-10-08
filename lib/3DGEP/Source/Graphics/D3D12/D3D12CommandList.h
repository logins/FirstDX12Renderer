#ifndef D3D12CommandList_h__
#define D3D12CommandList_h__
#include "CommandList.h"
#include <wrl.h>
#include <d3d12.h>

namespace GEPUtils { namespace Graphics {

	class D3D12Device;

	class D3D12CommandList : public GEPUtils::Graphics::CommandList
	{
	public:
		D3D12CommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> InCmdList, GEPUtils::Graphics::Device& InOwningDevice);
	
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>& GetInner() { return m_D3D12CmdList; }

		virtual void ResourceBarrier(GEPUtils::Graphics::Resource& InResource, GEPUtils::Graphics::RESOURCE_STATE InPrevState, GEPUtils::Graphics::RESOURCE_STATE InAfterState) override;


		virtual void ClearRTV(GEPUtils::Graphics::CpuDescHandle& InDescHandle, float* InColor) override;


		virtual void ClearDepth(GEPUtils::Graphics::CpuDescHandle& InDescHandle) override;


		virtual void Close() override;


		virtual void UpdateBufferResource(GEPUtils::Graphics::Resource& InDestResource, GEPUtils::Graphics::Resource& InIntermediateResource, size_t InNunElements, size_t InElementSize, const void* InBufferData, GEPUtils::Graphics::RESOURCE_FLAGS InFlags = GEPUtils::Graphics::RESOURCE_FLAGS::NONE) override;


		virtual void SetPipelineStateAndResourceBinder(Graphics::PipelineState& InPipelineState) override;


		virtual void SetInputAssemblerData(GEPUtils::Graphics::PRIMITIVE_TOPOLOGY InPrimTopology, GEPUtils::Graphics::VertexBufferView& InVertexBufView, GEPUtils::Graphics::IndexBufferView& InIndexBufView) override;


		virtual void SetViewportAndScissorRect(GEPUtils::Graphics::ViewPort& InViewport, GEPUtils::Graphics::Rect& InScissorRect) override;


		virtual void SetRenderTargetFromWindow(GEPUtils::Graphics::Window& InWindow) override;


		virtual void SetGraphicsRootConstants(uint64_t InRootParameterIndex, uint64_t InNum32BitValuesToSet, const void* InSrcData, uint64_t InDestOffsetIn32BitValues) override;


		virtual void DrawIndexed(uint64_t InIndexCountPerInstance) override;

	private:
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> m_D3D12CmdList;

	};

	} }
#endif // D3D12CommandList_h__
