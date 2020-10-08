#include "GraphicsAllocator.h"
#ifdef GRAPHICS_SDK_D3D12
#include "D3D12GEPUtils.h"
#include "D3D12PipelineState.h"
#include "d3dcompiler.h"
#include <wrl.h>
#endif
#include "Public/Device.h"

namespace GEPUtils { namespace Graphics {

	class PipelineState;

	// Static queues definitions
	std::queue<std::unique_ptr<GEPUtils::Graphics::Resource>> GraphicsAllocator::m_ResourceArray;
	std::queue<std::unique_ptr<GEPUtils::Graphics::ResourceView>> GraphicsAllocator::m_ResourceViewArray;
	std::queue<std::unique_ptr<GEPUtils::Graphics::VertexBufferView>> GraphicsAllocator::m_VertexViewArray;
	std::queue<std::unique_ptr<GEPUtils::Graphics::IndexBufferView>> GraphicsAllocator::m_IndexViewArray;
	std::queue<std::unique_ptr<GEPUtils::Graphics::Shader>> GraphicsAllocator::m_ShaderArray;
	std::queue<std::unique_ptr<GEPUtils::Graphics::PipelineState>> GraphicsAllocator::m_PipelineStateArray;


#ifdef GRAPHICS_SDK_D3D12

	GEPUtils::Graphics::Resource& GraphicsAllocator::AllocateResource()
	{
		m_ResourceArray.push(std::make_unique<D3D12GEPUtils::D3D12Resource>(nullptr)); //TODO possibly checking to not pass a certain number of allocations

		return *m_ResourceArray.back();
	}

	GEPUtils::Graphics::VertexBufferView& GraphicsAllocator::AllocateVertexBufferView()
	{
		m_VertexViewArray.push(std::make_unique<D3D12GEPUtils::D3D12VertexBufferView>()); //TODO possibly checking to not pass a certain number of allocations
		return *m_VertexViewArray.back();
	}

	GEPUtils::Graphics::IndexBufferView& GraphicsAllocator::AllocateIndexBufferView()
	{
		m_IndexViewArray.push(std::make_unique <D3D12GEPUtils::D3D12IndexBufferView>());
		return *m_IndexViewArray.back();
	}

	GEPUtils::Graphics::ResourceView& GraphicsAllocator::AllocateResourceView(GEPUtils::Graphics::RESOURCE_VIEW_TYPE InType)
{
		switch (InType)
		{
		case GEPUtils::Graphics::RESOURCE_VIEW_TYPE::CONSTANT_BUFFER: // TODO
			std::exception("We probably need to allocate a view here");
			break;
		case GEPUtils::Graphics::RESOURCE_VIEW_TYPE::SHADER_RESOURCE: // TODO
			break;
		case GEPUtils::Graphics::RESOURCE_VIEW_TYPE::UNORDERED_ACCESS: // TODO
			break;
		default:
			std::exception("Resource View Type undefined.");
			break;
		}

		return *m_ResourceViewArray.back();
	}

	GEPUtils::Graphics::Shader& GraphicsAllocator::AllocateShader(wchar_t const* InShaderPath)
	{
		Microsoft::WRL::ComPtr<ID3DBlob> OutFileBlob;
		D3D12GEPUtils::ThrowIfFailed(::D3DReadFileToBlob(InShaderPath, &OutFileBlob));
		m_ShaderArray.push(std::make_unique<D3D12GEPUtils::D3D12Shader>(OutFileBlob));

		return *m_ShaderArray.back();
	}

	GEPUtils::Graphics::PipelineState& GraphicsAllocator::AllocatePipelineState(GEPUtils::Graphics::Device& InGraphicsDevice)
	{
		m_PipelineStateArray.push(std::make_unique<GEPUtils::Graphics::D3D12PipelineState>(InGraphicsDevice));

		return *m_PipelineStateArray.back();
	}


#endif

}
}
