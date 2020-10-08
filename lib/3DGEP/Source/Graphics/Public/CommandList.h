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


		virtual void UpdateBufferResource(GEPUtils::Graphics::Resource& InDestResource, GEPUtils::Graphics::Resource& InIntermediateResource,
			size_t InNunElements, size_t InElementSize, const void* InBufferData, GEPUtils::Graphics::RESOURCE_FLAGS InFlags = GEPUtils::Graphics::RESOURCE_FLAGS::NONE
		) { }

		virtual void SetPipelineStateAndResourceBinder(GEPUtils::Graphics::PipelineState& InPipelineState) { }

		virtual void SetInputAssemblerData(GEPUtils::Graphics::PRIMITIVE_TOPOLOGY InPrimTopology, GEPUtils::Graphics::VertexBufferView& InVertexBufView, GEPUtils::Graphics::IndexBufferView& InIndexBufView) { }

		virtual void SetViewportAndScissorRect(GEPUtils::Graphics::ViewPort& InViewport, GEPUtils::Graphics::Rect& InScissorRect) { }

		virtual void SetRenderTargetFromWindow(GEPUtils::Graphics::Window& InWindow) { }

		virtual void SetGraphicsRootConstants(uint64_t InRootParameterIndex, uint64_t InNum32BitValuesToSet, const void* InSrcData, uint64_t InDestOffsetIn32BitValues) { }

		virtual void DrawIndexed(uint64_t InIndexCountPerInstance) { }

	protected:
		CommandList(GEPUtils::Graphics::Device& InDevice);

		GEPUtils::Graphics::Device& m_Device;
	};

} }

#endif // CommandList_h__
