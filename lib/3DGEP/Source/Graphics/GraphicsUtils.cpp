#include "GraphicsUtils.h"

#ifdef GRAPHICS_SDK_D3D12
#include "D3D12GEPUtils.h"

#endif
#include "GraphicsAllocator.h"
#include "Public/PipelineState.h"
namespace GEPUtils { namespace Graphics {



#ifdef GRAPHICS_SDK_D3D12

	void EnableDebugLayer()
	{
		return D3D12GEPUtils::EnableDebugLayer();
	}

	Resource& AllocateEmptyResource() // TODO this needs to allocate a resource in a resource array on the heap and return a reference to the just instantiated resource, possibly with a check for the number of resources already allocated
	{
		return GraphicsAllocator::AllocateResource();
	}

	GEPUtils::Graphics::VertexBufferView& AllocateVertexBufferView()
{
		return GraphicsAllocator::AllocateVertexBufferView();
	}

	GEPUtils::Graphics::IndexBufferView& AllocateIndexBufferView()
{
		return GraphicsAllocator::AllocateIndexBufferView();
	}

	GEPUtils::Graphics::ResourceView& AllocateEmptyBufferView(GEPUtils::Graphics::RESOURCE_VIEW_TYPE InType)
{
		return GraphicsAllocator::AllocateResourceView(InType);
	}

	GEPUtils::Graphics::Shader& AllocateShader(wchar_t const* InShaderPath)
	{
		return GraphicsAllocator::AllocateShader(InShaderPath);
	}

	GEPUtils::Graphics::PipelineState& AllocatePipelineState(GEPUtils::Graphics::Device& InGraphicsDevice)
{
		return GraphicsAllocator::AllocatePipelineState(InGraphicsDevice);
	}

	std::unique_ptr<GEPUtils::Graphics::Rect> AllocateRect(int32_t InLeft, int32_t InTop, int32_t InRight, int32_t InBottom)
	{
		return std::make_unique<D3D12GEPUtils::D3D12Rect>(InLeft, InTop, InRight, InBottom);
	}

	std::unique_ptr<GEPUtils::Graphics::ViewPort> AllocateViewport(float InTopLeftX, float InTopLeftY, float InWidth, float InHeight)
	{
		return std::make_unique<D3D12GEPUtils::D3D12ViewPort>(InTopLeftX, InTopLeftY, InWidth, InHeight);
	}

#endif
}
}