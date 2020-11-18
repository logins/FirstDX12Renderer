#include "GraphicsAllocator.h"
#include "Public/Device.h"

#ifdef GRAPHICS_SDK_D3D12
#include "D3D12GEPUtils.h"
#include "D3D12PipelineState.h"
#include "d3dcompiler.h"
#include <wrl.h>
#include "D3D12CommandList.h"
#endif
#include "D3D12/D3D12BufferAllocator.h"
#include "D3D12/D3D12DynamicDescHeap.h"
#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12UtilsInternal.h"

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

	GEPUtils::Graphics::Resource& GraphicsAllocator::AllocateEmptyResource()
	{
		m_ResourceArray.push(std::make_unique<D3D12GEPUtils::D3D12Resource>(nullptr)); //TODO possibly checking to not pass a certain number of allocations

		return *m_ResourceArray.back();
	}

	void GraphicsAllocator::AllocateBufferCommittedResource(GEPUtils::Graphics::CommandList& InCmdList, GEPUtils::Graphics::Resource& InDestResource, GEPUtils::Graphics::Resource& InIntermediateResource, size_t InNunElements, size_t InElementSize, const void* InBufferData, GEPUtils::Graphics::RESOURCE_FLAGS InFlags /*= GEPUtils::Graphics::RESOURCE_FLAGS::NONE*/)
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

	//void GraphicsAllocator::AllocateDynamicBuffer(GEPUtils::Graphics::Resource& InResource, size_t InSize, size_t InAlignmentSize)
	//{
	//	// Note: The D3D12BufferAllocator is responsible to allocate resources and here we are sub-allocating from one of them
	//	GEPUtils::Graphics::D3D12BufferAllocator::Allocation bufferAllocation = GEPUtils::Graphics::D3D12BufferAllocator::Get().Allocate(InSize, InAlignmentSize);

	//	static_cast<D3D12GEPUtils::D3D12Resource&>(InResource).InitAsDynamicBuffer(bufferAllocation, InSize, InAlignmentSize);
	//}

	GEPUtils::Graphics::DynamicBuffer& GraphicsAllocator::AllocateDynamicBuffer()
	{
		m_ResourceArray.push(std::make_unique<D3D12GEPUtils::D3D12DynamicBuffer>());

		return static_cast<GEPUtils::Graphics::DynamicBuffer&>(*m_ResourceArray.back());
	}

	void GraphicsAllocator::StageViewForGPU(uint32_t InRootIdx, GEPUtils::Graphics::ResourceView& InView)
	{
		GEPUtils::Graphics::AllocatedDescRange& allocDescRange = *static_cast<D3D12GEPUtils::D3D12ResourceView&>(InView).m_AllocatedDescRange.get();
		
		GEPUtils::Graphics::D3D12DynamicDescHeap::Get(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).StageDescriptors(InRootIdx, 0, allocDescRange.GetNumHandles(), allocDescRange.GetDescHandleAt(0));
	}

	void GraphicsAllocator::UploadViewToGPU(GEPUtils::Graphics::ResourceView& InView)
	{
		GEPUtils::Graphics::AllocatedDescRange& allocDescRange = *static_cast<D3D12GEPUtils::D3D12ResourceView&>(InView).m_AllocatedDescRange.get();

		allocDescRange.SetGpuDescHandle(GEPUtils::Graphics::D3D12DynamicDescHeap::Get(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).UploadSingleDescriptor(allocDescRange.GetDescHandleAt(0)));
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

	GEPUtils::Graphics::ResourceView& GraphicsAllocator::AllocateResourceView(GEPUtils::Graphics::Resource& InResource, GEPUtils::Graphics::RESOURCE_VIEW_TYPE InType)
{
		switch (InType)
		{
		case GEPUtils::Graphics::RESOURCE_VIEW_TYPE::CONSTANT_BUFFER:
		{
			// Allocate the view
			// Note: the constructor will allocate a corresponding descriptor in a CPU desc heap
			m_ResourceViewArray.push(std::make_unique<D3D12GEPUtils::D3D12ConstantBufferView>(InResource));
			
			// TODO continue here.. how to De-allocate the allocatedDescRange later on ??
			break;
		}
			
		case GEPUtils::Graphics::RESOURCE_VIEW_TYPE::SHADER_RESOURCE: // TODO
			std::exception("We probably need to allocate a view here");
			break;
		case GEPUtils::Graphics::RESOURCE_VIEW_TYPE::UNORDERED_ACCESS: // TODO
			std::exception("We probably need to allocate a view here");
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
