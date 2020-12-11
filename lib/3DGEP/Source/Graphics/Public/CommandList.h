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

		virtual void Close() { };

		virtual void ResourceBarrier(GEPUtils::Graphics::Resource& InResource,
			GEPUtils::Graphics::RESOURCE_STATE InPrevState, GEPUtils::Graphics::RESOURCE_STATE InAfterState) { }

		virtual void ClearRTV(GEPUtils::Graphics::CpuDescHandle& InDescHandle, float* InColor) { }
		
		virtual void ClearDepth(GEPUtils::Graphics::CpuDescHandle& InDescHandle) { }

		virtual void SetPipelineStateAndResourceBinder(GEPUtils::Graphics::PipelineState& InPipelineState) { }

		virtual void SetInputAssemblerData(GEPUtils::Graphics::PRIMITIVE_TOPOLOGY InPrimTopology, GEPUtils::Graphics::VertexBufferView& InVertexBufView, GEPUtils::Graphics::IndexBufferView& InIndexBufView) { }

		virtual void SetViewportAndScissorRect(GEPUtils::Graphics::ViewPort& InViewport, GEPUtils::Graphics::Rect& InScissorRect) { }

		virtual void SetRenderTargetFromWindow(GEPUtils::Graphics::Window& InWindow) { }

		virtual void SetGraphicsRootConstants(uint64_t InRootParameterIndex, uint64_t InNum32BitValuesToSet, const void* InSrcData, uint64_t InDestOffsetIn32BitValues) { }

		virtual void SetGraphicsRootTable(uint32_t InRootIndex, GEPUtils::Graphics::ConstantBufferView& InView) { }

		virtual void DrawIndexed(uint64_t InIndexCountPerInstance) { }

		virtual void UploadViewToGPU(GEPUtils::Graphics::ShaderResourceView& InSRV) { }

		virtual void StoreAndReferenceDynamicBuffer(uint32_t InRootIdx, GEPUtils::Graphics::DynamicBuffer& InDynBuffer, GEPUtils::Graphics::ConstantBufferView& InResourceView) { }

		virtual void ReferenceSRV(uint32_t InRootIdx, GEPUtils::Graphics::ShaderResourceView& InSRV) { }

		// Internally calls ::UpdateSubresources(..) where IntermediateBuffer is expected to be allocated in upload heap
		virtual void UploadBufferData(GEPUtils::Graphics::Buffer& DestinationBuffer, GEPUtils::Graphics::Buffer& IntermediateBuffer, const void* InBufferData, size_t InDataSize) { }

	protected:
		CommandList(GEPUtils::Graphics::Device& InDevice);

		GEPUtils::Graphics::Device& m_Device;
	};

} }

#endif // CommandList_h__
