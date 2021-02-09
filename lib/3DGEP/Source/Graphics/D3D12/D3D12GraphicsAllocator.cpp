/*
 D3D12GraphicsAllocator.cpp

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/
 
#include "D3D12GraphicsAllocator.h"
#include "D3D12GEPUtils.h"
#include "D3D12PipelineState.h"
#include "d3dcompiler.h"
#include <wrl.h>
#include "D3D12CommandList.h"
#include "D3D12Device.h"
#include "D3D12UtilsInternal.h"
#include "GEPUtils.h"
#include "D3D12BufferAllocator.h"
#include "Application.h"
#include "D3D12DescHeapFactory.h"

namespace GEPUtils { namespace Graphics {

	// Note: The non-inline definition of the constructor is necessary to forward classes used in member smart pointers!!
	// More info in this thread: https://stackoverflow.com/questions/27336779/unique-ptr-and-forward-declaration
	D3D12GraphicsAllocator::D3D12GraphicsAllocator() = default;

	D3D12GraphicsAllocator::~D3D12GraphicsAllocator()
	{
		m_DynamicBufferAllocator.reset();

		m_DescHeapFactory.reset();

	}

	GEPUtils::Graphics::Resource& D3D12GraphicsAllocator::AllocateEmptyResource()
	{
		m_ResourceArray.push_back(std::make_unique<D3D12GEPUtils::D3D12Resource>(nullptr)); //TODO possibly checking to not pass a certain number of allocations

		return *m_ResourceArray.back();
	}

	GEPUtils::Graphics::Buffer& D3D12GraphicsAllocator::AllocateBufferResource(size_t InSize, GEPUtils::Graphics::RESOURCE_HEAP_TYPE InHeapType, GEPUtils::Graphics::RESOURCE_STATE InState, GEPUtils::Graphics::RESOURCE_FLAGS InFlags /*= RESOURCE_FLAGS::NONE*/)
	{
		// If passed size is 0, set it to minimum resource size
		if (InSize == 0)
			InSize = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

		Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource;
		D3D12GEPUtils::CreateCommittedResource(
			static_cast<GEPUtils::Graphics::D3D12Device&>(GEPUtils::Graphics::GetDevice()).GetInner(), d3d12Resource.GetAddressOf(),
			D3D12GEPUtils::HeapTypeToD3D12(InHeapType), InSize, D3D12GEPUtils::ResFlagsToD3D12(InFlags), D3D12GEPUtils::ResourceStateTypeToD3D12(InState));

		m_ResourceArray.push_back(std::make_unique<D3D12GEPUtils::D3D12Resource>(d3d12Resource));

		d3d12Resource.Reset();

		return static_cast<GEPUtils::Graphics::Buffer&>(*m_ResourceArray.back());
	}

	GEPUtils::Graphics::DynamicBuffer& D3D12GraphicsAllocator::AllocateDynamicBuffer()
	{
		m_ResourceArray.push_back(std::make_unique<D3D12GEPUtils::D3D12DynamicBuffer>());

		return static_cast<GEPUtils::Graphics::DynamicBuffer&>(*m_ResourceArray.back());
	}

	GEPUtils::Graphics::Texture& D3D12GraphicsAllocator::AllocateTextureFromFile(wchar_t const* InTexturePath, GEPUtils::Graphics::TEXTURE_FILE_FORMAT InFileFormat, int32_t InMipsNum /*= 0*/, GEPUtils::Graphics::RESOURCE_FLAGS InCreationFlags /*= RESOURCE_FLAGS::NONE*/)
	{
		m_ResourceArray.push_back(std::make_unique<D3D12GEPUtils::D3D12Texture>(InTexturePath, InFileFormat, InMipsNum, InCreationFlags));

		return static_cast<GEPUtils::Graphics::Texture&>(*m_ResourceArray.back());
	}

	GEPUtils::Graphics::Texture& D3D12GraphicsAllocator::AllocateEmptyTexture(uint32_t InWidth, uint32_t InHeight, GEPUtils::Graphics::TEXTURE_TYPE InType, GEPUtils::Graphics::BUFFER_FORMAT InFormat, uint32_t InArraySize, uint32_t InMipLevels)
	{
		std::unique_ptr<D3D12GEPUtils::D3D12Texture> outputTexture = std::make_unique<D3D12GEPUtils::D3D12Texture>(InWidth, InHeight, InType, InFormat, InArraySize, InMipLevels);
		outputTexture->InstantiateOnGPU(); // Allocate empty space on GPU

		m_ResourceArray.push_back(std::move(outputTexture));

		return static_cast<GEPUtils::Graphics::Texture&>(*m_ResourceArray.back());
	}

	void D3D12GraphicsAllocator::AllocateBufferCommittedResource(GEPUtils::Graphics::CommandList& InCmdList, GEPUtils::Graphics::Resource& InDestResource, GEPUtils::Graphics::Resource& InIntermediateResource, size_t InNunElements, size_t InElementSize, const void* InBufferData, GEPUtils::Graphics::RESOURCE_FLAGS InFlags /*= GEPUtils::Graphics::RESOURCE_FLAGS::NONE*/)
	{
		// Note: ID3D12Resource** InDestResource, ID3D12Resource** InIntermediateResource are CPU Buffer Data !!!
		// We create them on CPU, then we use them to update the corresponding SubResouce on the GPU!
		size_t bufferSize = InNunElements * InElementSize;
		// Create a committed resource for the GPU resource in a default heap
		// Note: CreateCommittedResource will allocate a resource heap and a resource in it in GPU memory, then it will return the corresponding GPUVirtualAddress that will be stored inside the ID3D12Resource object,
		// so that we can reference that GPU memory address from CPU side.
		D3D12GEPUtils::CreateCommittedResource(static_cast<GEPUtils::Graphics::D3D12Device&>(GEPUtils::Graphics::GetDevice()).GetInner(),
			&static_cast<D3D12GEPUtils::D3D12Resource&>(InDestResource).GetInner(), D3D12_HEAP_TYPE_DEFAULT, bufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
		if (InBufferData)
		{ // Create a committed resource in an upload heap to upload content to the first resource
			D3D12GEPUtils::CreateCommittedResource(static_cast<GEPUtils::Graphics::D3D12Device&>(GEPUtils::Graphics::GetDevice()).GetInner(), &static_cast<D3D12GEPUtils::D3D12Resource&>(InIntermediateResource).GetInner(), D3D12_HEAP_TYPE_UPLOAD, bufferSize, D3D12GEPUtils::ResFlagsToD3D12(InFlags), D3D12_RESOURCE_STATE_GENERIC_READ);

			// Now that both copy and dest resource are created on CPU, we can use them to update the corresponding GPU SubResource
			D3D12_SUBRESOURCE_DATA subresourceData = {};
			subresourceData.pData = InBufferData; // Pointer to the memory block that contains the subresource data on CPU
			subresourceData.RowPitch = bufferSize; // Physical size in Bytes of the subresource data
			subresourceData.SlicePitch = subresourceData.RowPitch; // Size of each slice for the resource, since we assume only 1 slice, this corresponds to the size of the entire resource
			// Note: UpdateSubresources first uploads data in the intermediate resource, which is expected to be in shared memory (upload heap) 
			// and then transfers the content to the destination resource, expected to be in dedicated memory (default heap) probably trough a mapping mechanism
			::UpdateSubresources(static_cast<GEPUtils::Graphics::D3D12CommandList&>(InCmdList).GetInner().Get(), static_cast<D3D12GEPUtils::D3D12Resource&>(InDestResource).GetInner().Get(), static_cast<D3D12GEPUtils::D3D12Resource&>(InIntermediateResource).GetInner().Get(), 0, 0, 1, &subresourceData);
		}
	}


	GEPUtils::Graphics::VertexBufferView& D3D12GraphicsAllocator::AllocateVertexBufferView()
	{
		m_VertexViewArray.push_back(std::make_unique<D3D12GEPUtils::D3D12VertexBufferView>()); //TODO possibly checking to not pass a certain number of allocations
		return *m_VertexViewArray.back();
	}

	GEPUtils::Graphics::IndexBufferView& D3D12GraphicsAllocator::AllocateIndexBufferView()
	{
		m_IndexViewArray.push_back(std::make_unique <D3D12GEPUtils::D3D12IndexBufferView>());
		return *m_IndexViewArray.back();
	}

	GEPUtils::Graphics::ConstantBufferView& D3D12GraphicsAllocator::AllocateConstantBufferView(GEPUtils::Graphics::Buffer& InResource)
	{
		// Allocate the view
		// Note: the constructor will allocate a corresponding descriptor in a CPU desc heap
		return static_cast<GEPUtils::Graphics::ConstantBufferView&>(
			m_DescHeapFactory->AddViewObject(std::make_unique<D3D12GEPUtils::D3D12ConstantBufferView>(InResource))
			);
	}

	GEPUtils::Graphics::ConstantBufferView& D3D12GraphicsAllocator::AllocateConstantBufferView()
	{
		return static_cast<GEPUtils::Graphics::ConstantBufferView&>(
			m_DescHeapFactory->AddViewObject(std::make_unique<D3D12GEPUtils::D3D12ConstantBufferView>())
			);
	}

	GEPUtils::Graphics::ShaderResourceView& D3D12GraphicsAllocator::AllocateShaderResourceView(GEPUtils::Graphics::Texture& InTexture)
	{
		// Allocate the view
		// Note: the constructor will allocate a corresponding descriptor in a CPU desc heap
		return static_cast<GEPUtils::Graphics::ShaderResourceView&>(
			m_DescHeapFactory->AddViewObject(std::make_unique<D3D12GEPUtils::D3D12ShaderResourceView>(InTexture))
			);
	}

	GEPUtils::Graphics::ShaderResourceView& D3D12GraphicsAllocator::AllocateSrvTex2DArray(GEPUtils::Graphics::Texture& InTexture, uint32_t InArraySize, uint32_t InMostDetailedMip /*= 0*/, int32_t InMipLevels /*= -1*/, uint32_t InFirstArraySlice /*= 0*/, uint32_t InPlaneSlice /*= 0*/)
	{
		std::unique_ptr<D3D12GEPUtils::D3D12ShaderResourceView> outSrv = std::make_unique<D3D12GEPUtils::D3D12ShaderResourceView>();

		outSrv->InitAsTex2DArray(InTexture, InArraySize, InMostDetailedMip, InMipLevels, InFirstArraySlice, InPlaneSlice);

		return static_cast<GEPUtils::Graphics::ShaderResourceView&>(
			m_DescHeapFactory->AddViewObject(std::move(outSrv))
			);
	}

	GEPUtils::Graphics::UnorderedAccessView& D3D12GraphicsAllocator::AllocateUavTex2DArray(GEPUtils::Graphics::Texture& InTexture, uint32_t InArraySize, int32_t InMipSlice /*= -1*/, uint32_t InFirstArraySlice /*= 0*/, uint32_t InPlaceSlice /*= 0*/)
	{
		std::unique_ptr<D3D12GEPUtils::D3D12UnorderedAccessView> outUav = std::make_unique<D3D12GEPUtils::D3D12UnorderedAccessView>();

		outUav->InitAsTex2DArray(InTexture, InArraySize, InMipSlice, InFirstArraySlice, InPlaceSlice);

		return static_cast<GEPUtils::Graphics::UnorderedAccessView&>(
			m_DescHeapFactory->AddViewObject(std::move(outUav))
			);
	}

	GEPUtils::Graphics::Shader& D3D12GraphicsAllocator::AllocateShader(wchar_t const* InShaderPath)
	{
		Microsoft::WRL::ComPtr<ID3DBlob> OutFileBlob;
		D3D12GEPUtils::ThrowIfFailed(::D3DReadFileToBlob(InShaderPath, &OutFileBlob));
		m_ShaderArray.push_back(std::make_unique<D3D12GEPUtils::D3D12Shader>(OutFileBlob));

		return *m_ShaderArray.back();
	}

	GEPUtils::Graphics::PipelineState& D3D12GraphicsAllocator::AllocatePipelineState()
{
		m_PipelineStateArray.push_back(std::make_unique<GEPUtils::Graphics::D3D12PipelineState>());

		return *m_PipelineStateArray.back();
	}

	void D3D12GraphicsAllocator::ReserveDynamicBufferMemory(size_t InSize, void*& OutCpuPtr, D3D12_GPU_VIRTUAL_ADDRESS& OutGpuPtr)
	{
		m_DynamicBufferAllocator->Allocate(InSize, OutCpuPtr, OutGpuPtr);
	}

	GEPUtils::Graphics::D3D12DescriptorHeap& D3D12GraphicsAllocator::GetCpuHeap()
	{
		return m_DescHeapFactory->GetCPUHeap();
	}

	GEPUtils::Graphics::D3D12DescriptorHeap& D3D12GraphicsAllocator::GetGpuHeap()
	{
		return m_DescHeapFactory->GetGPUHeap();
	}

	void D3D12GraphicsAllocator::OnNewFrameStarted()
	{
		// When a new frame starts, we are sure by the application that there are a total of Application::GetMaxConcurrentFramesNum() active,
		// so we can use that number to divide the dynamic allocator circular memory pool in equal parts, and move the maximum allocation index each time we have a new frame.
		// The rest of memory is considered off limits since their relative frames are still in flight.
		auto currentFrameNum = Application::GetCurrentFrameNumber();
		float currentFramePartition = (Application::GetCurrentFrameNumber() % Application::GetMaxConcurrentFramesNum() ) / static_cast<float>(Application::GetMaxConcurrentFramesNum());
		static const float fractionSize = 1.0f / Application::GetMaxConcurrentFramesNum();

		m_DynamicBufferAllocator->SetAdmittedAllocationRegion(currentFramePartition, currentFramePartition + fractionSize);

		GetGpuHeap().SetAllowedDynamicAllocationRegion(currentFramePartition, currentFramePartition + fractionSize);
	}

	void D3D12GraphicsAllocator::Initialize()
	{
		m_DescHeapFactory = std::make_unique<GEPUtils::Graphics::D3D12DescHeapFactory>();

		// Allocate an empty resource and create the dynamic buffer allocator on it
		GEPUtils::Graphics::Buffer& dynamicBufferResource = AllocateBufferResource(0, RESOURCE_HEAP_TYPE::UPLOAD, RESOURCE_STATE::GEN_READ);

		m_DynamicBufferAllocator = std::make_unique<GEPUtils::Graphics::D3D12LinearBufferAllocator>(static_cast<D3D12GEPUtils::D3D12Resource&>(dynamicBufferResource));

	}

} }
