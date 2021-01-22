/*
 GraphicsAllocator.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef GraphicsAllocator_h__
#define GraphicsAllocator_h__

#include <queue>
#include <memory> // for std::unique_ptr
#include "GraphicsTypes.h"
#include "PipelineState.h"

namespace GEPUtils { namespace Graphics {

	class Device;
	class CommandList;

// This pure virtual class serves as interface for any Graphics Allocator we want to implement.
// In Application code we are going to use GraphicsAllocator with a factory pattern class, so we can call GraphicsAllocator::Get()
// to retrieve the real allocator (in our case a D3D12GraphicsAllocator object).
// Using this pattern is better compared to a singleton, because we are then able to extend the graphics allocator class
// to different platform-agnostic implementations and possibly different versions for a specific platform (e.g. a specific implementation for testing).
class GraphicsAllocatorBase
{
public:
	GraphicsAllocatorBase() = default;

	virtual GEPUtils::Graphics::Resource& AllocateEmptyResource() = 0;

	virtual GEPUtils::Graphics::DynamicBuffer& AllocateDynamicBuffer() = 0;

	virtual GEPUtils::Graphics::Texture& AllocateTextureFromFile(wchar_t const* InTexturePath, GEPUtils::Graphics::TEXTURE_FILE_FORMAT InFileFormat, int32_t InMipsNum = 0, GEPUtils::Graphics::RESOURCE_FLAGS InCreationFlags = RESOURCE_FLAGS::NONE) = 0;
	
	virtual GEPUtils::Graphics::Texture& AllocateEmptyTexture(uint32_t InWidth, uint32_t InHeight, GEPUtils::Graphics::TEXTURE_TYPE InType, GEPUtils::Graphics::BUFFER_FORMAT InFormat, uint32_t InArraySize, uint32_t InMipLevels) = 0;

	// Preferable for Static Buffers such as Vertex and Index Buffers.
	// First creates an intermediary buffer in shared memory (upload heap), then the same buffer in reserved memory (default heap)
	// and then calls UpdateSubresources that will copy the content of the first buffer in the second one.
	// The type of allocation is Committed Resource, meaning that a resource heap will be created specifically to contain the allocated resource each time.
	virtual void AllocateBufferCommittedResource(GEPUtils::Graphics::CommandList& InCmdList, GEPUtils::Graphics::Resource& InDestResource, GEPUtils::Graphics::Resource& InIntermediateResource,
		size_t InNunElements, size_t InElementSize, const void* InBufferData, GEPUtils::Graphics::RESOURCE_FLAGS InFlags = GEPUtils::Graphics::RESOURCE_FLAGS::NONE) = 0;

	virtual GEPUtils::Graphics::Buffer& AllocateBuffer(size_t InSize, GEPUtils::Graphics::RESOURCE_HEAP_TYPE InHeapType, GEPUtils::Graphics::RESOURCE_STATE InState, GEPUtils::Graphics::RESOURCE_FLAGS InFlags = RESOURCE_FLAGS::NONE) = 0;



	virtual GEPUtils::Graphics::VertexBufferView& AllocateVertexBufferView() = 0;
	virtual GEPUtils::Graphics::IndexBufferView& AllocateIndexBufferView() = 0;
	virtual GEPUtils::Graphics::ConstantBufferView& AllocateConstantBufferView(GEPUtils::Graphics::Buffer& InResource) = 0;
	virtual GEPUtils::Graphics::ConstantBufferView& AllocateConstantBufferView() = 0;

	virtual GEPUtils::Graphics::ShaderResourceView& AllocateShaderResourceView(GEPUtils::Graphics::Texture& InTexture) = 0;
	// SRV referencing a Tex2D Array
	virtual GEPUtils::Graphics::ShaderResourceView& AllocateSrvTex2DArray(GEPUtils::Graphics::Texture& InTexture, uint32_t InArraySize, uint32_t InMostDetailedMip = 0, int32_t InMipLevels = -1, uint32_t InFirstArraySlice = 0, uint32_t InPlaceSlice = 0) = 0;

	// UAV referencing a Tex2D Array
	virtual GEPUtils::Graphics::UnorderedAccessView& AllocateUavTex2DArray(GEPUtils::Graphics::Texture& InTexture, uint32_t InArraySize, int32_t InMipSlice = -1, uint32_t InFirstArraySlice = 0, uint32_t InPlaceSlice = 0) = 0;
	
	virtual GEPUtils::Graphics::Shader& AllocateShader(wchar_t const* InShaderPath) = 0;
	virtual GEPUtils::Graphics::PipelineState& AllocatePipelineState() = 0;

	GEPUtils::Graphics::ShaderResourceView& GetDummySRV() const { return *m_DummySRV; }
	GEPUtils::Graphics::ConstantBufferView& GetDummyCBV() const { return *m_DummyCBV; }
	GEPUtils::Graphics::VertexBufferView& GetDummyVBV() const { return *m_DummyVBV; }
	GEPUtils::Graphics::IndexBufferView& GetDummyIBV() const { return *m_DummyIBV; }

	GEPUtils::Graphics::Texture& GetDummyTexture() const { return *m_DummyTexture; }
	GEPUtils::Graphics::Buffer& GetDummyBuffer() const { return *m_DummyBuffer; }

	GEPUtils::Graphics::PipelineState& GetDummyPSO() const { return *m_DummyPSO; }

	// Deleting copy constructor, assignment operator, move constructor and move assignment
	GraphicsAllocatorBase(const GraphicsAllocatorBase&) = delete;
	GraphicsAllocatorBase& operator=(const GraphicsAllocatorBase&) = delete;
	GraphicsAllocatorBase(GraphicsAllocatorBase&&) = delete;
	GraphicsAllocatorBase& operator=(GraphicsAllocatorBase&&) = delete;

protected:
	std::queue<std::unique_ptr<GEPUtils::Graphics::Resource>> m_ResourceArray;
	std::queue<std::unique_ptr<GEPUtils::Graphics::ResourceView>> m_ResourceViewArray;
	std::queue<std::unique_ptr<GEPUtils::Graphics::VertexBufferView>> m_VertexViewArray;
	std::queue<std::unique_ptr<GEPUtils::Graphics::IndexBufferView>> m_IndexViewArray;
	std::queue<std::unique_ptr<GEPUtils::Graphics::Shader>> m_ShaderArray;
	std::queue<std::unique_ptr<GEPUtils::Graphics::PipelineState>> m_PipelineStateArray;

	std::unique_ptr<GEPUtils::Graphics::ConstantBufferView> m_DummyCBV;
	std::unique_ptr<GEPUtils::Graphics::ShaderResourceView> m_DummySRV;
	std::unique_ptr<GEPUtils::Graphics::VertexBufferView> m_DummyVBV;
	std::unique_ptr<GEPUtils::Graphics::IndexBufferView> m_DummyIBV;

	std::unique_ptr<GEPUtils::Graphics::Buffer> m_DummyBuffer;
	std::unique_ptr<GEPUtils::Graphics::Texture> m_DummyTexture;



	std::unique_ptr<GEPUtils::Graphics::PipelineState> m_DummyPSO;
};



class GraphicsAllocator
{
public:
	// Will return the instance of the chosen graphics allocator for the current configuration (for now only the DX12 one)
	static GraphicsAllocatorBase* Get();

	static GEPUtils::Graphics::ShaderResourceView& DummySRV() { return Get()->GetDummySRV(); }
	static GEPUtils::Graphics::ConstantBufferView& DummyCBV() { return Get()->GetDummyCBV(); }
	static GEPUtils::Graphics::VertexBufferView& DummyVBV() { return Get()->GetDummyVBV(); }
	static GEPUtils::Graphics::IndexBufferView& DummyIBV() { return Get()->GetDummyIBV(); }

	static GEPUtils::Graphics::Buffer& DummyBuffer() { return Get()->GetDummyBuffer(); }
	static GEPUtils::Graphics::Texture& DummyTexture() { return Get()->GetDummyTexture(); }

	static GEPUtils::Graphics::PipelineState& DummyPSO() { return Get()->GetDummyPSO(); }

	// Deleting copy constructor, assignment operator, move constructor and move assignment
	GraphicsAllocator(const GraphicsAllocator&) = delete;
	GraphicsAllocator& operator=(const GraphicsAllocator&) = delete;
	GraphicsAllocator(GraphicsAllocator&&) = delete;
	GraphicsAllocator& operator=(GraphicsAllocator&&) = delete;

private:
	static std::unique_ptr<GraphicsAllocatorBase> m_Instance;
};



} }
#endif // GraphicsAllocator_h__
