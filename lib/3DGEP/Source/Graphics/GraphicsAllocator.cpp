#include "GraphicsAllocator.h"
#/*include "Public/Device.h"

#include "D3D12/D3D12BufferAllocator.h"
#include "D3D12/D3D12DynamicDescHeap.h"
#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12UtilsInternal.h"
#include "GEPUtils.h"*/

#ifdef GRAPHICS_SDK_D3D12

#include "D3D12GraphicsAllocator.h"

#endif

namespace GEPUtils { namespace Graphics {


	std::unique_ptr<GEPUtils::Graphics::GraphicsAllocatorBase> GraphicsAllocator::m_Instance;




	//GEPUtils::Graphics::Resource& GraphicsAllocator::AllocateEmptyResource()
	//{
	//	m_ResourceArray.push(std::make_unique<D3D12GEPUtils::D3D12Resource>(nullptr)); //TODO possibly checking to not pass a certain number of allocations

	//	return *m_ResourceArray.back();
	//}

	//void GraphicsAllocator::AllocateBufferCommittedResource(GEPUtils::Graphics::CommandList& InCmdList, GEPUtils::Graphics::Resource& InDestResource, GEPUtils::Graphics::Resource& InIntermediateResource, size_t InNunElements, size_t InElementSize, const void* InBufferData, GEPUtils::Graphics::RESOURCE_FLAGS InFlags /*= GEPUtils::Graphics::RESOURCE_FLAGS::NONE*/)
	//{
	//	// Note: ID3D12Resource** InDestResource, ID3D12Resource** InIntermediateResource are CPU Buffer Data !!!
	//	// We create them on CPU, then we use them to update the corresponding SubResouce on the GPU!
	//	size_t bufferSize = InNunElements * InElementSize;
	//	// Create a committed resource for the GPU resource in a default heap
	//	// Note: CreateCommittedResource will allocate a resource heap and a resource in it in GPU memory, then it will return the corresponding GPUVirtualAddress that will be stored inside the ID3D12Resource object,
	//	// so that we can reference that GPU memory address from CPU side.
	//	D3D12GEPUtils::CreateCommittedResource(static_cast<GEPUtils::Graphics::D3D12Device&>(GEPUtils::Graphics::GetDevice()).GetInner(),
	//		&static_cast<D3D12GEPUtils::D3D12Resource&>(InDestResource).GetInner(), D3D12_HEAP_TYPE_DEFAULT, bufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
	//	if (InBufferData)
	//	{ // Create a committed resource in an upload heap to upload content to the first resource
	//		D3D12GEPUtils::CreateCommittedResource(static_cast<GEPUtils::Graphics::D3D12Device&>(GEPUtils::Graphics::GetDevice()).GetInner(), &static_cast<D3D12GEPUtils::D3D12Resource&>(InIntermediateResource).GetInner(), D3D12_HEAP_TYPE_UPLOAD, bufferSize, D3D12GEPUtils::ResFlagsToD3D12(InFlags), D3D12_RESOURCE_STATE_GENERIC_READ);

	//		// Now that both copy and dest resource are created on CPU, we can use them to update the corresponding GPU SubResource
	//		D3D12_SUBRESOURCE_DATA subresourceData = {};
	//		subresourceData.pData = InBufferData; // Pointer to the memory block that contains the subresource data on CPU
	//		subresourceData.RowPitch = bufferSize; // Physical size in Bytes of the subresource data
	//		subresourceData.SlicePitch = subresourceData.RowPitch; // Size of each slice for the resource, since we assume only 1 slice, this corresponds to the size of the entire resource
	//		// Note: UpdateSubresources first uploads data in the intermediate resource, which is expected to be in shared memory (upload heap) 
	//		// and then transfers the content to the destination resource, expected to be in dedicated memory (default heap) probably trough a mapping mechanism
	//		::UpdateSubresources(static_cast<GEPUtils::Graphics::D3D12CommandList&>(InCmdList).GetInner().Get(), static_cast<D3D12GEPUtils::D3D12Resource&>(InDestResource).GetInner().Get(), static_cast<D3D12GEPUtils::D3D12Resource&>(InIntermediateResource).GetInner().Get(), 0, 0, 1, &subresourceData);
	//	}
	//}

	//GEPUtils::Graphics::DynamicBuffer& GraphicsAllocator::AllocateDynamicBuffer()
	//{
	//	m_ResourceArray.push(std::make_unique<D3D12GEPUtils::D3D12DynamicBuffer>());

	//	return static_cast<GEPUtils::Graphics::DynamicBuffer&>(*m_ResourceArray.back());
	//}

	//GEPUtils::Graphics::Texture& GraphicsAllocator::LoadAndAllocateTextureFromFile(GEPUtils::Graphics::CommandList& InCmdList, wchar_t const* InTexturePath, GEPUtils::Graphics::TEXTURE_FILE_FORMAT InFileFormat)
	//{
	//	// Informations about the texture resource
	//	DirectX::TexMetadata metadata;

	//	// Content of the texture resource
	//	DirectX::ScratchImage scratchImage;

	//	switch (InFileFormat)
	//	{
	//	case GEPUtils::Graphics::TEXTURE_FILE_FORMAT::DDS:
	//	{
	//		D3D12GEPUtils::ThrowIfFailed(DirectX::LoadFromDDSFile(
	//			InTexturePath,
	//			DirectX::DDS_FLAGS_FORCE_RGB,
	//			&metadata,
	//			scratchImage));
	//		break;
	//	}
	//	default:
	//		StopForFail("Trying to load an unhandled texture format!")
	//		break;
	//	}
	//	
	//	D3D12_RESOURCE_DESC textureDesc = {};

	//	textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
	//		metadata.format,
	//		static_cast<UINT64>(metadata.width),
	//		static_cast<UINT>(metadata.height),
	//		static_cast<UINT16>(metadata.arraySize));

	//	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;

	//	ID3D12Device2* d3d12Device = static_cast<GEPUtils::Graphics::D3D12Device&>(GEPUtils::Graphics::GetDevice()).GetInner().Get();

	//	// Allocate a committed resource in GPU dedicated memory (default heap)
	//	d3d12Device->CreateCommittedResource(
	//		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
	//		D3D12_HEAP_FLAG_NONE,
	//		&textureDesc,
	//		D3D12_RESOURCE_STATE_COMMON, // Note: Texture must be in COMMON state to be used as a target for a copy operation
	//		nullptr,
	//		IID_PPV_ARGS(&textureResource));

	//	// Fetch data found from file
	//	std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());

	//	const DirectX::Image* foundImages = scratchImage.GetImages();

	//	for (int i = 0; i < scratchImage.GetImageCount(); ++i)
	//	{
	//		D3D12_SUBRESOURCE_DATA& currentSubresource = subresources[i];
	//		currentSubresource.RowPitch = foundImages[i].rowPitch;
	//		currentSubresource.SlicePitch = foundImages[i].slicePitch;
	//		currentSubresource.pData = foundImages[i].pixels;
	//	}

	//	// Resource must be in copy destination state
	//	CD3DX12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(textureResource.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST );

	//	ID3D12GraphicsCommandList2* d3d12CmdList = static_cast<GEPUtils::Graphics::D3D12CommandList&>(InCmdList).GetInner().Get();
	//	d3d12CmdList->ResourceBarrier(1, &transitionBarrier);

	//	// Create the intermediate resource to upload the found data in textureResource 
	//	// (we need this because textureResource is in a default heap, and the only way to access it 
	//	// is having copying content from an intermediate resource form a copy heap).
	//	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;

	//	UINT64 requiredIntermediateSize = 0;
	//	// The intermediate size can be also obtained with the wrapper function GetRequiredIntermediateSize from d3dx12.h but directly calling d3d12Device->GetCopyableFootprints will be faster
	//	d3d12Device->GetCopyableFootprints(&textureResource->GetDesc(), 0, subresources.size(), 0, nullptr, nullptr, nullptr, &requiredIntermediateSize);

	//	d3d12Device->CreateCommittedResource(
	//		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
	//		D3D12_HEAP_FLAG_NONE,
	//		&CD3DX12_RESOURCE_DESC::Buffer(requiredIntermediateSize),
	//		D3D12_RESOURCE_STATE_COPY_SOURCE,
	//		nullptr,
	//		IID_PPV_ARGS(&intermediateResource)
	//		);

	//	// UpdateSubresources will push commands on the d3d12CmdList so that it will 
	//	// first copy the surbresources data to the intermediate resource through mapping memcopy operations, 
	//	// then execute a resource copy from the intermediateResource content to the textureResource.
	//	::UpdateSubresources(d3d12CmdList, textureResource.Get(), intermediateResource.Get(), 0, 0, subresources.size(), subresources.data());

	//	// Now that the command list got stored the instructions to update the texture resource, we can create the texture object for the engine

	//	m_ResourceArray.push(std::make_unique<D3D12GEPUtils::D3D12Texture>(textureResource, metadata.width, metadata.height, metadata.arraySize, 
	//		metadata.mipLevels, D3D12GEPUtils::BufferFormatToEngine(metadata.format), D3D12GEPUtils::TextureTypeToEngine(metadata.dimension)));

	//	// Note: Before using this texture we would have to wait for it to be uploaded to memory. 
	//	// That corresponds to the command list finishing executing, which means the corresponding command allocator finishes execution on the command queue.

	//	return static_cast<GEPUtils::Graphics::Texture&>(*m_ResourceArray.back());
	//}

	//void GraphicsAllocator::StageViewForGPU(uint32_t InRootIdx, GEPUtils::Graphics::ConstantBufferView& InView)
	//{
	//	GEPUtils::Graphics::AllocatedDescRange& allocDescRange = *static_cast<D3D12GEPUtils::D3D12ConstantBufferView&>(InView).m_AllocatedDescRange.get();
	//	
	//	GEPUtils::Graphics::D3D12DynamicDescHeap::Get(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).StageDescriptors(InRootIdx, 0, allocDescRange.GetNumHandles(), allocDescRange.GetDescHandleAt(0));
	//}

	//void GraphicsAllocator::UploadViewToGPU(GEPUtils::Graphics::ConstantBufferView& InView)
	//{
	//	GEPUtils::Graphics::AllocatedDescRange& allocDescRange = *static_cast<D3D12GEPUtils::D3D12ConstantBufferView&>(InView).m_AllocatedDescRange.get();

	//	allocDescRange.SetGpuDescHandle(GEPUtils::Graphics::D3D12DynamicDescHeap::Get(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).UploadSingleDescriptor(allocDescRange.GetDescHandleAt(0)));
	//}

	//void GraphicsAllocator::ResetGPUResourceDescriptorHeap()
	//{
	//	GEPUtils::Graphics::D3D12DynamicDescHeap::Get(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).Reset();
	//}

	//GEPUtils::Graphics::VertexBufferView& GraphicsAllocator::AllocateVertexBufferView()
	//{
	//	m_VertexViewArray.push(std::make_unique<D3D12GEPUtils::D3D12VertexBufferView>()); //TODO possibly checking to not pass a certain number of allocations
	//	return *m_VertexViewArray.back();
	//}

	//GEPUtils::Graphics::IndexBufferView& GraphicsAllocator::AllocateIndexBufferView()
	//{
	//	m_IndexViewArray.push(std::make_unique <D3D12GEPUtils::D3D12IndexBufferView>());
	//	return *m_IndexViewArray.back();
	//}

	//GEPUtils::Graphics::ConstantBufferView& GraphicsAllocator::AllocateConstantBufferView(GEPUtils::Graphics::Resource& InResource)
	//{
	//	// Allocate the view
	//	// Note: the constructor will allocate a corresponding descriptor in a CPU desc heap
	//	m_ResourceViewArray.push(std::make_unique<D3D12GEPUtils::D3D12ConstantBufferView>(InResource));
	//		
	//	return static_cast<GEPUtils::Graphics::ConstantBufferView&>(*m_ResourceViewArray.back());
	//}

	//GEPUtils::Graphics::ShaderResourceView& GraphicsAllocator::AllocateShaderResourceView(GEPUtils::Graphics::Texture& InTexture)
	//{
	//	// Allocate the view
	//	// Note: the constructor will allocate a corresponding descriptor in a CPU desc heap
	//	m_ResourceViewArray.push(std::make_unique<D3D12GEPUtils::D3D12ShaderResourceView>(InTexture));

	//	return static_cast<GEPUtils::Graphics::ShaderResourceView&>(*m_ResourceViewArray.back());
	//}

	//GEPUtils::Graphics::Shader& GraphicsAllocator::AllocateShader(wchar_t const* InShaderPath)
	//{
	//	Microsoft::WRL::ComPtr<ID3DBlob> OutFileBlob;
	//	D3D12GEPUtils::ThrowIfFailed(::D3DReadFileToBlob(InShaderPath, &OutFileBlob));
	//	m_ShaderArray.push(std::make_unique<D3D12GEPUtils::D3D12Shader>(OutFileBlob));

	//	return *m_ShaderArray.back();
	//}

	//GEPUtils::Graphics::PipelineState& GraphicsAllocator::AllocatePipelineState(GEPUtils::Graphics::Device& InGraphicsDevice)
	//{
	//	m_PipelineStateArray.push(std::make_unique<GEPUtils::Graphics::D3D12PipelineState>(InGraphicsDevice));

	//	return *m_PipelineStateArray.back();
	//}



	GEPUtils::Graphics::GraphicsAllocatorBase* GraphicsAllocator::Get()
	{
		if (!m_Instance)
		{
#ifdef GRAPHICS_SDK_D3D12
			m_Instance = std::make_unique<GEPUtils::Graphics::D3D12GraphicsAllocator>();
#endif
		}
		return m_Instance.get();
	}


}
}
