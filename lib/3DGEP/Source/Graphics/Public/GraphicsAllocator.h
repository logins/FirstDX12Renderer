#ifndef GraphicsAllocator_h__
#define GraphicsAllocator_h__
#include <queue>
#include <memory> // for std::unique_ptr
#include "GraphicsTypes.h"

namespace GEPUtils { namespace Graphics {

	class PipelineState;
	class Device;
	class CommandList;

class GraphicsAllocator
{
public:
	static GEPUtils::Graphics::Resource& AllocateEmptyResource();

	static GEPUtils::Graphics::DynamicBuffer& AllocateDynamicBuffer();

	// Preferable for Static Buffers such as Vertex and Index Buffers.
	// First creates an intermediary buffer in shared memory (upload heap), then the same buffer in reserved memory (default heap)
	// and then calls UpdateSubresources that will copy the content of the first buffer in the second one.
	// The type of allocation is Committed Resource, meaning that a resource heap will be created specifically to contain the allocated resource each time.
	static void AllocateBufferCommittedResource(GEPUtils::Graphics::CommandList& InCmdList, GEPUtils::Graphics::Resource& InDestResource, GEPUtils::Graphics::Resource& InIntermediateResource,
		size_t InNunElements, size_t InElementSize, const void* InBufferData, GEPUtils::Graphics::RESOURCE_FLAGS InFlags = GEPUtils::Graphics::RESOURCE_FLAGS::NONE);

	static void StageViewForGPU(uint32_t InRootIdx, GEPUtils::Graphics::ResourceView& InView);

	static void UploadViewToGPU(GEPUtils::Graphics::ResourceView& InView);

	static void ResetGPUResourceDescriptorHeap();

	static GEPUtils::Graphics::VertexBufferView& AllocateVertexBufferView();
	static GEPUtils::Graphics::IndexBufferView& AllocateIndexBufferView();
	static GEPUtils::Graphics::ResourceView& AllocateResourceView(GEPUtils::Graphics::Resource& InResource, GEPUtils::Graphics::RESOURCE_VIEW_TYPE InType);
	static GEPUtils::Graphics::Shader& AllocateShader(wchar_t const* InShaderPath);
	static GEPUtils::Graphics::PipelineState& AllocatePipelineState(GEPUtils::Graphics::Device& InGraphicsDevice);

private:
	// Note: since these are static, here they are only declared, so they also need to be defined in source code!
	static std::queue<std::unique_ptr<GEPUtils::Graphics::Resource>> m_ResourceArray;
	static std::queue<std::unique_ptr<GEPUtils::Graphics::ResourceView>> m_ResourceViewArray;
	static std::queue<std::unique_ptr<GEPUtils::Graphics::VertexBufferView>> m_VertexViewArray;
	static std::queue<std::unique_ptr<GEPUtils::Graphics::IndexBufferView>> m_IndexViewArray;
	static std::queue<std::unique_ptr<GEPUtils::Graphics::Shader>> m_ShaderArray;
	static std::queue<std::unique_ptr<GEPUtils::Graphics::PipelineState>> m_PipelineStateArray;
};

} }
#endif // GraphicsAllocator_h__
