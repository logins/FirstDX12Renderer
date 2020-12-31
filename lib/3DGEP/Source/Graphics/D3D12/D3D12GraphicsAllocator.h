/*!
 * Project: First D3D12 Renderer - https://github.com/logins/FirstDX12Renderer
 *
 * File: D3D12GraphicsAllocator.h
 *
 * Author: Riccardo Loggini
 */
#ifndef D3D12GraphicsAllocator_h__
#define D3D12GraphicsAllocator_h__

#include "GraphicsAllocator.h"

namespace GEPUtils { namespace Graphics {

class D3D12GraphicsAllocator : public GEPUtils::Graphics::GraphicsAllocatorBase
{
public:
	D3D12GraphicsAllocator();

	virtual GEPUtils::Graphics::Resource& AllocateEmptyResource() override;

	virtual GEPUtils::Graphics::Buffer& AllocateBuffer(size_t InSize, GEPUtils::Graphics::RESOURCE_HEAP_TYPE InHeapType, GEPUtils::Graphics::RESOURCE_STATE InState, GEPUtils::Graphics::RESOURCE_FLAGS InFlags = RESOURCE_FLAGS::NONE) override;

	virtual GEPUtils::Graphics::DynamicBuffer& AllocateDynamicBuffer() override;

	virtual GEPUtils::Graphics::Texture& AllocateTextureFromFile(wchar_t const* InTexturePath, GEPUtils::Graphics::TEXTURE_FILE_FORMAT InFileFormat, int32_t InMipsNum = 0, GEPUtils::Graphics::RESOURCE_FLAGS InCreationFlags = RESOURCE_FLAGS::NONE) override;

	virtual GEPUtils::Graphics::Texture& AllocateEmptyTexture(uint32_t InWidth, uint32_t InHeight, GEPUtils::Graphics::TEXTURE_TYPE InType, GEPUtils::Graphics::BUFFER_FORMAT InFormat, uint32_t InArraySize, uint32_t InMipLevels) override;

	virtual void AllocateBufferCommittedResource(GEPUtils::Graphics::CommandList& InCmdList, GEPUtils::Graphics::Resource& InDestResource, GEPUtils::Graphics::Resource& InIntermediateResource, size_t InNunElements, size_t InElementSize, const void* InBufferData, GEPUtils::Graphics::RESOURCE_FLAGS InFlags = GEPUtils::Graphics::RESOURCE_FLAGS::NONE) override;

	virtual GEPUtils::Graphics::VertexBufferView& AllocateVertexBufferView() override;

	virtual GEPUtils::Graphics::IndexBufferView& AllocateIndexBufferView() override;

	virtual GEPUtils::Graphics::ConstantBufferView& AllocateConstantBufferView(GEPUtils::Graphics::Buffer& InResource) override;

	virtual GEPUtils::Graphics::ConstantBufferView& AllocateConstantBufferView() override;

	virtual GEPUtils::Graphics::ShaderResourceView& AllocateShaderResourceView(GEPUtils::Graphics::Texture& InTexture) override;

	virtual GEPUtils::Graphics::ShaderResourceView& AllocateSrvTex2DArray(GEPUtils::Graphics::Texture& InTexture, uint32_t InArraySize, uint32_t InMostDetailedMip = 0, int32_t InMipLevels = -1, uint32_t InFirstArraySlice = 0, uint32_t InPlaceSlice = 0) override;

	virtual GEPUtils::Graphics::UnorderedAccessView& AllocateUavTex2DArray(GEPUtils::Graphics::Texture& InTexture, uint32_t InArraySize, int32_t InMipSlice = -1, uint32_t InFirstArraySlice = 0, uint32_t InPlaceSlice = 0) override;

	virtual GEPUtils::Graphics::Shader& AllocateShader(wchar_t const* InShaderPath) override;

	virtual GEPUtils::Graphics::PipelineState& AllocatePipelineState() override;


};
	

} }

#endif // D3D12GraphicsAllocator_h__