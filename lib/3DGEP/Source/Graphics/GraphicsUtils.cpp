/*
 GraphicsUtils.cpp

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/
 
#include "GraphicsUtils.h"

#ifdef GRAPHICS_SDK_D3D12
#include "D3D12GEPUtils.h"

#endif
#include "GraphicsAllocator.h"
#include "Public/PipelineState.h"
#include "../../../DirectXTex/DirectXTex/DirectXTex.h"
#include "../Public/GEPUtils.h"
namespace GEPUtils { namespace Graphics {



#ifdef GRAPHICS_SDK_D3D12

	void EnableDebugLayer()
	{
		return D3D12GEPUtils::EnableDebugLayer();
	}

	GEPUtils::Graphics::DynamicBuffer& AllocateDynamicBuffer()
	{
		return GraphicsAllocator::Get()->AllocateDynamicBuffer();
	}

	GEPUtils::Graphics::VertexBufferView& AllocateVertexBufferView()
{
		return GraphicsAllocator::Get()->AllocateVertexBufferView();
	}

	GEPUtils::Graphics::IndexBufferView& AllocateIndexBufferView()
{
		return GraphicsAllocator::Get()->AllocateIndexBufferView();
	}

	GEPUtils::Graphics::ConstantBufferView& AllocateConstantBufferView(GEPUtils::Graphics::Buffer& InResource, GEPUtils::Graphics::RESOURCE_VIEW_TYPE InType)
{
		return GraphicsAllocator::Get()->AllocateConstantBufferView(InResource);
	}

	GEPUtils::Graphics::Shader& AllocateShader(wchar_t const* InShaderPath)
	{
		return GraphicsAllocator::Get()->AllocateShader(InShaderPath);
	}

	GEPUtils::Graphics::PipelineState& AllocatePipelineState()
{
		return GraphicsAllocator::Get()->AllocatePipelineState();
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