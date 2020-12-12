/*!
 * Project: First D3D12 Renderer - https://github.com/logins/FirstDX12Renderer
 *
 * File: D3D12GraphicsAllocator.cpp
 *
 * Author: Riccardo Loggini
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
#include "D3D12GpuDescHeap.h"

namespace GEPUtils { namespace Graphics {

	D3D12GraphicsAllocator::D3D12GraphicsAllocator()
	{
		m_DummySRV = std::make_unique<D3D12GEPUtils::D3D12ShaderResourceView>(); 
		m_DummyCBV = std::make_unique<D3D12GEPUtils::D3D12ConstantBufferView>();
		m_DummyVBV = std::make_unique<D3D12GEPUtils::D3D12VertexBufferView>();
		m_DummyIBV = std::make_unique<D3D12GEPUtils::D3D12IndexBufferView>();
		m_DummyBuffer = std::make_unique<D3D12GEPUtils::D3D12Resource>();
		m_DummyTexture = std::make_unique<D3D12GEPUtils::D3D12Texture>();
		m_DummyPSO = std::make_unique<GEPUtils::Graphics::D3D12PipelineState>();
	}

	GEPUtils::Graphics::Resource& D3D12GraphicsAllocator::AllocateEmptyResource()
	{
		m_ResourceArray.push(std::make_unique<D3D12GEPUtils::D3D12Resource>(nullptr)); //TODO possibly checking to not pass a certain number of allocations

		return *m_ResourceArray.back();
	}

	GEPUtils::Graphics::Buffer& D3D12GraphicsAllocator::AllocateBuffer(size_t InSize, GEPUtils::Graphics::RESOURCE_HEAP_TYPE InHeapType, GEPUtils::Graphics::RESOURCE_STATE InState, GEPUtils::Graphics::RESOURCE_FLAGS InFlags /*= RESOURCE_FLAGS::NONE*/)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource;
		D3D12GEPUtils::CreateCommittedResource(
			static_cast<GEPUtils::Graphics::D3D12Device&>(GEPUtils::Graphics::GetDevice()).GetInner(), d3d12Resource.GetAddressOf(),
			D3D12GEPUtils::HeapTypeToD3D12(InHeapType), InSize, D3D12GEPUtils::ResFlagsToD3D12(InFlags), D3D12GEPUtils::ResourceStateTypeToD3D12(InState));

		m_ResourceArray.push(std::make_unique<D3D12GEPUtils::D3D12Resource>(d3d12Resource));

		d3d12Resource.Reset();

		return static_cast<GEPUtils::Graphics::Buffer&>(*m_ResourceArray.back());
	}

	GEPUtils::Graphics::DynamicBuffer& D3D12GraphicsAllocator::AllocateDynamicBuffer()
	{
		m_ResourceArray.push(std::make_unique<D3D12GEPUtils::D3D12DynamicBuffer>());

		return static_cast<GEPUtils::Graphics::DynamicBuffer&>(*m_ResourceArray.back());
	}

	GEPUtils::Graphics::Texture& D3D12GraphicsAllocator::AllocateTextureFromFile(wchar_t const* InTexturePath, GEPUtils::Graphics::TEXTURE_FILE_FORMAT InFileFormat)
	{
		m_ResourceArray.push(std::make_unique<D3D12GEPUtils::D3D12Texture>(InTexturePath, InFileFormat));

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

	void D3D12GraphicsAllocator::UploadViewToGPU(GEPUtils::Graphics::ConstantBufferView& InView) // TODO continue here: Upload "Static" descriptor
	{
		GEPUtils::Graphics::AllocatedDescRange& allocDescRange = *static_cast<D3D12GEPUtils::D3D12ConstantBufferView&>(InView).m_AllocatedDescRange.get();

		allocDescRange.SetGpuDescHandle(GEPUtils::Graphics::D3D12GpuDescHeap::Get(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).UploadSingleStaticDescriptor(allocDescRange.GetDescHandleAt(0)));
	}

	void D3D12GraphicsAllocator::ResetGPUResourceDescriptorHeap()
	{
		GEPUtils::Graphics::D3D12GpuDescHeap::Get(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).Reset();
	}

	GEPUtils::Graphics::VertexBufferView& D3D12GraphicsAllocator::AllocateVertexBufferView()
	{
		m_VertexViewArray.push(std::make_unique<D3D12GEPUtils::D3D12VertexBufferView>()); //TODO possibly checking to not pass a certain number of allocations
		return *m_VertexViewArray.back();
	}

	GEPUtils::Graphics::IndexBufferView& D3D12GraphicsAllocator::AllocateIndexBufferView()
	{
		m_IndexViewArray.push(std::make_unique <D3D12GEPUtils::D3D12IndexBufferView>());
		return *m_IndexViewArray.back();
	}

	GEPUtils::Graphics::ConstantBufferView& D3D12GraphicsAllocator::AllocateConstantBufferView(GEPUtils::Graphics::Buffer& InResource)
	{
		// Allocate the view
		// Note: the constructor will allocate a corresponding descriptor in a CPU desc heap
		m_ResourceViewArray.push(std::make_unique<D3D12GEPUtils::D3D12ConstantBufferView>(InResource));

		return static_cast<GEPUtils::Graphics::ConstantBufferView&>(*m_ResourceViewArray.back());
	}

	GEPUtils::Graphics::ConstantBufferView& D3D12GraphicsAllocator::AllocateConstantBufferView()
	{
		m_ResourceViewArray.push(std::make_unique<D3D12GEPUtils::D3D12ConstantBufferView>());

		return static_cast<GEPUtils::Graphics::ConstantBufferView&>(*m_ResourceViewArray.back());
	}

	GEPUtils::Graphics::ShaderResourceView& D3D12GraphicsAllocator::AllocateShaderResourceView(GEPUtils::Graphics::Texture& InTexture)
	{
		// Allocate the view
		// Note: the constructor will allocate a corresponding descriptor in a CPU desc heap
		m_ResourceViewArray.push(std::make_unique<D3D12GEPUtils::D3D12ShaderResourceView>(InTexture));

		return static_cast<GEPUtils::Graphics::ShaderResourceView&>(*m_ResourceViewArray.back());
	}

	GEPUtils::Graphics::Shader& D3D12GraphicsAllocator::AllocateShader(wchar_t const* InShaderPath)
	{
		Microsoft::WRL::ComPtr<ID3DBlob> OutFileBlob;
		D3D12GEPUtils::ThrowIfFailed(::D3DReadFileToBlob(InShaderPath, &OutFileBlob));
		m_ShaderArray.push(std::make_unique<D3D12GEPUtils::D3D12Shader>(OutFileBlob));

		return *m_ShaderArray.back();
	}

	GEPUtils::Graphics::PipelineState& D3D12GraphicsAllocator::AllocatePipelineState()
{
		m_PipelineStateArray.push(std::make_unique<GEPUtils::Graphics::D3D12PipelineState>());

		return *m_PipelineStateArray.back();
	}

} }
