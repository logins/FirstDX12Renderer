#ifndef GraphicsAllocator_h__
#define GraphicsAllocator_h__
#include <queue>
#include <memory> // for std::unique_ptr
#include "Public/GraphicsTypes.h"

namespace GEPUtils { namespace Graphics {

	class PipelineState;
	class Device;

class GraphicsAllocator
{
public:
	static GEPUtils::Graphics::Resource& AllocateResource();
	static GEPUtils::Graphics::VertexBufferView& AllocateVertexBufferView();
	static GEPUtils::Graphics::IndexBufferView& AllocateIndexBufferView();
	static GEPUtils::Graphics::ResourceView& AllocateResourceView(GEPUtils::Graphics::RESOURCE_VIEW_TYPE InType);
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
