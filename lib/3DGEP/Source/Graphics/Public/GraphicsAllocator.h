/*
 GraphicsAllocator.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef GraphicsAllocator_h__
#define GraphicsAllocator_h__

#include "GraphicsTypes.h"
#include "PipelineState.h"


namespace GEPUtils { namespace Graphics {

	class Device;
	class CommandList;
	class Window;
	class WindowInitInput;
	class CommandQueue;

// This pure virtual class serves as interface for any Graphics Allocator we want to implement.
// In Application code we are going to use GraphicsAllocator with a factory pattern class, so we can call GraphicsAllocator::Get()
// to retrieve the real allocator (in our case a D3D12GraphicsAllocator object).
// Using this pattern is better compared to a singleton, because we are then able to extend the graphics allocator class
// to different platform-agnostic implementations and possibly different versions for a specific platform (e.g. a specific implementation for testing).
class GraphicsAllocatorBase
{
public:
	GraphicsAllocatorBase();
	
	virtual ~GraphicsAllocatorBase();

	// Creates default resources
	virtual void Initialize() = 0;

	virtual void OnNewFrameStarted() = 0;

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

	virtual GEPUtils::Graphics::Buffer& AllocateBufferResource(size_t InSize, GEPUtils::Graphics::RESOURCE_HEAP_TYPE InHeapType, GEPUtils::Graphics::RESOURCE_STATE InState, GEPUtils::Graphics::RESOURCE_FLAGS InFlags = RESOURCE_FLAGS::NONE) = 0;

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

	virtual GEPUtils::Graphics::Window& AllocateWindow(GEPUtils::Graphics::WindowInitInput& InWindowInitInput) = 0;

	virtual GEPUtils::Graphics::CommandQueue& AllocateCommandQueue(class Device& InDevice, COMMAND_LIST_TYPE InCmdListType) = 0;

	// Deleting copy constructor, assignment operator, move constructor and move assignment
	GraphicsAllocatorBase(const GraphicsAllocatorBase&) = delete;
	GraphicsAllocatorBase& operator=(const GraphicsAllocatorBase&) = delete;
	GraphicsAllocatorBase(GraphicsAllocatorBase&&) = delete;
	GraphicsAllocatorBase& operator=(GraphicsAllocatorBase&&) = delete;
};



class GraphicsAllocator
{
public:
	// Will return the instance of the chosen graphics allocator for the current configuration (for now only the DX12 one)
	static GraphicsAllocatorBase* Get() { return m_DefaultInstance; };

	static std::unique_ptr<GraphicsAllocatorBase> CreateInstance();

	// Sets the default reference to be called with the Get() method.
	// This allows other systems (such as Application) to control the lifetime and take ownership of the default graphics allocator
	static void SetDefaultInstance(GraphicsAllocatorBase* InGraphicsAllocator);

	// Deleting copy constructor, assignment operator, move constructor and move assignment
	GraphicsAllocator(const GraphicsAllocator&) = delete;
	GraphicsAllocator& operator=(const GraphicsAllocator&) = delete;
	GraphicsAllocator(GraphicsAllocator&&) = delete;
	GraphicsAllocator& operator=(GraphicsAllocator&&) = delete;

private:
	// This class is not intended to be instantiated
	GraphicsAllocator() = default;
	static GraphicsAllocatorBase* m_DefaultInstance;
};



} }
#endif // GraphicsAllocator_h__
