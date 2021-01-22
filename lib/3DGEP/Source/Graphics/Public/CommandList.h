#ifndef CommandList_h__
#define CommandList_h__
#include "GraphicsTypes.h"

namespace GEPUtils { namespace Graphics {

	class Device;
	class PipelineState;
	class Window;

	class CommandList
	{
	public:

		virtual void Close() = 0;

		virtual void ResourceBarrier(GEPUtils::Graphics::Resource& InResource,
			GEPUtils::Graphics::RESOURCE_STATE InPrevState, GEPUtils::Graphics::RESOURCE_STATE InAfterState) = 0;

		virtual void ClearRTV(GEPUtils::Graphics::CpuDescHandle& InDescHandle, float* InColor) = 0;
		
		virtual void ClearDepth(GEPUtils::Graphics::CpuDescHandle& InDescHandle) = 0;

		virtual void SetPipelineStateAndResourceBinder(GEPUtils::Graphics::PipelineState& InPipelineState) = 0;

		virtual void SetInputAssemblerData(GEPUtils::Graphics::PRIMITIVE_TOPOLOGY InPrimTopology, GEPUtils::Graphics::VertexBufferView& InVertexBufView, GEPUtils::Graphics::IndexBufferView& InIndexBufView) = 0;

		virtual void SetViewportAndScissorRect(GEPUtils::Graphics::ViewPort& InViewport, GEPUtils::Graphics::Rect& InScissorRect) = 0;

		virtual void SetRenderTargetFromWindow(GEPUtils::Graphics::Window& InWindow) = 0;

		virtual void SetGraphicsRootConstants(uint64_t InRootParameterIndex, uint64_t InNum32BitValuesToSet, const void* InSrcData, uint64_t InDestOffsetIn32BitValues) = 0;

		virtual void SetComputeRootConstants(uint64_t InRootParameterIndex, uint64_t InNum32BitValuesToSet, const void* InSrcData, uint64_t InDestOffsetIn32BitValues) = 0;

		virtual void SetGraphicsRootTable(uint32_t InRootIndex, GEPUtils::Graphics::ConstantBufferView& InView) = 0;

		virtual void DrawIndexed(uint64_t InIndexCountPerInstance) = 0;

		virtual void Dispatch(uint32_t InGroupsNumX, uint32_t InGroupsNumY, uint32_t InGroupsNumZ) = 0;

		virtual void UploadViewToGPU(GEPUtils::Graphics::ShaderResourceView& InSRV) = 0;

		virtual void UploadUavToGpu(GEPUtils::Graphics::UnorderedAccessView& InUav) = 0;

		virtual void StoreAndReferenceDynamicBuffer(uint32_t InRootIdx, GEPUtils::Graphics::DynamicBuffer& InDynBuffer, GEPUtils::Graphics::ConstantBufferView& InResourceView) = 0;

		virtual void ReferenceSRV(uint32_t InRootIdx, GEPUtils::Graphics::ShaderResourceView& InSRV) = 0;

		virtual void ReferenceComputeTable(uint32_t InRootIdx, GEPUtils::Graphics::ShaderResourceView& InUav) = 0;

		virtual void ReferenceComputeTable(uint32_t InRootIdx, GEPUtils::Graphics::UnorderedAccessView& InUav) = 0;

		// Internally calls ::UpdateSubresources(..) where IntermediateBuffer is expected to be allocated in upload heap
		virtual void UploadBufferData(GEPUtils::Graphics::Buffer& DestinationBuffer, GEPUtils::Graphics::Buffer& IntermediateBuffer, const void* InBufferData, size_t InDataSize) = 0;

	protected:
		CommandList(GEPUtils::Graphics::Device& InDevice);

		GEPUtils::Graphics::Device& m_Device;
	};

} }

#endif // CommandList_h__
