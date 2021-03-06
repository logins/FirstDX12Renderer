/*
 D3D12GEPUtils.cpp

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/
 
#include "D3D12GEPUtils.h"
#include "D3D12UtilsInternal.h"
#include "GEPUtils.h"
#include "D3D12Device.h"
#include "GEPUtilsMath.h"
#include "D3D12CommandList.h"
#include "D3D12GraphicsAllocator.h"

#ifdef max
#undef max // This is needed to avoid conflicts with functions called max(), like chrono::milliseconds::max()
#undef min
#endif

namespace D3D12GEPUtils
{


	void PrintHello()
	{
		std::cout << "Hello From D3D12GEPUtils Library!" << std::endl;

		D3D12GEPUtils::ThisIsMyInternalFunction();
	}

	ComPtr<IDXGIAdapter4> GetMainAdapter(bool InUseWarp)
	{
		// Note: warp is a virtual platform that provides backward compatibility to DX12 for older graphics devices.
		// More info at: https://docs.microsoft.com/en-us/windows/win32/direct3darticles/directx-warp#what-is-warp 
		ComPtr<IDXGIFactory4> dxgiFactory;
		UINT createFactoryFlags = 0;

#ifdef _DEBUG
		createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
		ThrowIfFailed(::CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

		ComPtr<IDXGIAdapter1> dxgiAdapter1;
		ComPtr<IDXGIAdapter4> dxgiAdapter4;

		if (InUseWarp)
		{
			// Retrieve the first adapter found from the factory
			ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
			// Try casting it to adapter4
			ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
		}
		else
		{
			SIZE_T maxDedicatedVideoMemory = 0;
			// Inspecting all the devices found and taking the one with largest video memory
			for (UINT i =0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
			{
				DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
				dxgiAdapter1->GetDesc1(&dxgiAdapterDesc);
				if ((dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 // Avoid the adapter called the "Microsoft Basic Render Driver" adapter, which is a default adapter without display outputs
					&& SUCCEEDED(::D3D12CreateDevice(dxgiAdapter1.Get(),
						D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)) // Check if current adapter can create a D3D12 device without actually creating it
					&& dxgiAdapterDesc.DedicatedVideoMemory > maxDedicatedVideoMemory
					)
				{
					maxDedicatedVideoMemory = dxgiAdapterDesc.DedicatedVideoMemory;
					ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
				}
			}
		}
		return dxgiAdapter4;
	}

	ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> InAdapter)
	{
		ComPtr<ID3D12Device2> d3dDevice2;
		ThrowIfFailed(::D3D12CreateDevice(InAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&d3dDevice2)));
#if _DEBUG // Adding Info Queue filters
		ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(d3dDevice2.As(&infoQueue)))
		{
			// When a message with the following severities passes trough the Storage Filter, the program will break.
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
		}
		else
		{
			assert("We were not able to get the InfoQueue from device... Maybe the Debug Layer is disabled..");
		}
		// Following message severities will be suppressed
		D3D12_MESSAGE_SEVERITY suppressedSeverities[] = { D3D12_MESSAGE_SEVERITY_INFO };

		// Following messages will be suppressed based on their id
		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
		};

		D3D12_INFO_QUEUE_FILTER infoQueueFilter = {};
		infoQueueFilter.DenyList.NumSeverities = _countof(suppressedSeverities);
		infoQueueFilter.DenyList.pSeverityList = suppressedSeverities;
		infoQueueFilter.DenyList.NumIDs = _countof(denyIds);
		infoQueueFilter.DenyList.pIDList = denyIds;
		// Note: we can also deny entire message categories
		ThrowIfFailed(infoQueue->PushStorageFilter(&infoQueueFilter));
#endif
		return d3dDevice2;
	}

	ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> InDevice, D3D12_COMMAND_LIST_TYPE InType)
	{
		ComPtr<ID3D12CommandQueue> d3d12CmdQueue;

		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		cmdQueueDesc.Type = InType;
		cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		cmdQueueDesc.NodeMask = 0; // Assuming using 1 GPU
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

		ThrowIfFailed(InDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&d3d12CmdQueue)));

		return d3d12CmdQueue;
	}

	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType, UINT InNumDescriptors)
	{
		ComPtr<ID3D12DescriptorHeap> descrHeap;

		D3D12_DESCRIPTOR_HEAP_DESC descrHeapDesc = {};
		descrHeapDesc.Type = InType;
		descrHeapDesc.NumDescriptors = InNumDescriptors;
		descrHeapDesc.NodeMask = 0;
		descrHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		ThrowIfFailed(InDevice->CreateDescriptorHeap(&descrHeapDesc, IID_PPV_ARGS(&descrHeap)));

		return descrHeap;
	}

	ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> InDevice, D3D12_COMMAND_LIST_TYPE InCmdListType)
	{
		ComPtr<ID3D12CommandAllocator> cmdAllocator;
		ThrowIfFailed(InDevice->CreateCommandAllocator(InCmdListType, IID_PPV_ARGS(&cmdAllocator)));
		return cmdAllocator;
	}

	ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(ComPtr<ID3D12Device2> InDevice, ComPtr<ID3D12CommandAllocator> InCmdAllocator, D3D12_COMMAND_LIST_TYPE InCmdListType, bool InInitClosed /*= true*/)
	{
		ComPtr<ID3D12GraphicsCommandList2> cmdList;
		ThrowIfFailed(InDevice->CreateCommandList(0, InCmdListType, InCmdAllocator.Get(), nullptr, IID_PPV_ARGS(&cmdList)));
		// Note: PSO parameter is optional and we can initialize the command list with it
		// Note: the initial state of a newly created command list is Open, so we manually need to close it.
		if(InInitClosed)
			ThrowIfFailed(cmdList->Close()); // Close it because the beginning of the render method will execute reset on the cmdList
	
		return cmdList;
	}	

	ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> InDevice)
	{
		ComPtr<ID3D12Fence> fence;
		ThrowIfFailed(InDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
		return fence;
	}

	HANDLE CreateFenceEventHandle()
	{
		HANDLE eventHandle;
		eventHandle = ::CreateEvent(
			NULL,	// Handle cannot be inherited by child processes
			FALSE,	// This is going to be an auto-reset event object, after being signaled, it will automatically return in non-signaled state
			FALSE,	// Initial event state is non-signaled 
			NULL);	// Name of the event object, for events comparison
		assert(eventHandle && "[D3D12GEPUtils] Failed to create fence event.");

		return eventHandle;
	}

	void WaitForFenceValue(ComPtr<ID3D12Fence> InFence, uint64_t InFenceValue, HANDLE InFenceEvent, std::chrono::milliseconds InMaxDuration /*= std::chrono::milliseconds::max()*/)
	{
		if (InFence->GetCompletedValue() < InFenceValue)
		{
			ThrowIfFailed(InFence->SetEventOnCompletion(InFenceValue, InFenceEvent));

			::WaitForSingleObject(InFenceEvent, static_cast<DWORD>(InMaxDuration.count())); // count() returns the number of ticks
		}
	}

	void ClearRTV(ID3D12GraphicsCommandList2* InCmdList, D3D12_CPU_DESCRIPTOR_HANDLE InRTVCPUDescHandle, FLOAT* InClearColor)
	{
		InCmdList->ClearRenderTargetView(InRTVCPUDescHandle, InClearColor, 0, nullptr);
	}

	void ClearDepth(ID3D12GraphicsCommandList2* InCmdList, D3D12_CPU_DESCRIPTOR_HANDLE InDepthCPUDescHandle, FLOAT InDepth /*= 1.0f*/)
	{
		InCmdList->ClearDepthStencilView(InDepthCPUDescHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}

	void UpdateBufferResource(ComPtr<ID3D12Device2> InDevice, ComPtr<ID3D12GraphicsCommandList2> InCmdList, ID3D12Resource** InDestResource, ID3D12Resource** InIntermediateResource, size_t InNunElements, size_t InElementSize, const void* InBufferData, D3D12_RESOURCE_FLAGS InFlags)
	{
		// Note: ID3D12Resource** InDestResource, ID3D12Resource** InIntermediateResource are CPU Buffer Data !!!
		// We create them on CPU, then we use them to update the corresponding SubResouce on the GPU!
		size_t bufferSize = InNunElements * InElementSize;
		// Create a committed resource for the GPU resource in a default heap
		// Note: CreateCommittedResource will allocate a resource heap and a resource in it in GPU memory, then it will return the corresponding GPUVirtualAddress that will be stored inside the ID3D12Resource object,
		// so that we can reference that GPU memory address from CPU side.
		CreateCommittedResource(InDevice, InDestResource, D3D12_HEAP_TYPE_DEFAULT, bufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
		if (InBufferData)
		{ // Create a committed resource in an upload heap to upload content to the first resource
			CreateCommittedResource(InDevice, InIntermediateResource, D3D12_HEAP_TYPE_UPLOAD, bufferSize, InFlags, D3D12_RESOURCE_STATE_GENERIC_READ);

			// Now that both copy and dest resource are created on CPU, we can use them to update the corresponding GPU SubResource
			D3D12_SUBRESOURCE_DATA subresourceData = {};
			subresourceData.pData = InBufferData; // Pointer to the memory block that contains the subresource data on CPU
			subresourceData.RowPitch = bufferSize; // Physical size in Bytes of the subresource data
			subresourceData.SlicePitch = subresourceData.RowPitch; // Size of each slice for the resource, since we assume only 1 slice, this corresponds to the size of the entire resource
			::UpdateSubresources(InCmdList.Get(), *InDestResource, *InIntermediateResource, 0, 0, 1, &subresourceData);
		}
	}

	void CreateCommittedResource(ComPtr<ID3D12Device2> InDevice, ID3D12Resource** InResource, D3D12_HEAP_TYPE InHeapType, uint64_t InBufferSize, D3D12_RESOURCE_FLAGS InFlags, D3D12_RESOURCE_STATES InInitialStates)
	{
		ThrowIfFailed(InDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(InHeapType),
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(InBufferSize, InFlags),
			InInitialStates,
			nullptr, IID_PPV_ARGS(InResource)
		));
	}

	void CreateDepthStencilCommittedResource(ComPtr<ID3D12Device2> InDevice, ID3D12Resource** InResource, uint64_t InWidth, uint64_t InHeight, D3D12_RESOURCE_STATES InInitialStates, D3D12_CLEAR_VALUE* InClearValue)
	{
		ThrowIfFailed(InDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, InWidth, InHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			InInitialStates, InClearValue, IID_PPV_ARGS(InResource)
		));
	}

	void CreateDepthStencilView(ComPtr<ID3D12Device2> InDevice, ID3D12Resource* InResource, D3D12_CPU_DESCRIPTOR_HANDLE& InDSVCPUDescHandle)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D = { 0 };
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		InDevice->CreateDepthStencilView(InResource, &dsvDesc, InDSVCPUDescHandle);
	}

	void SignalCmdQueue(ComPtr<ID3D12CommandQueue> InCmdQueue, ComPtr<ID3D12Fence> InFence, uint64_t& OutFenceValue)
	{
		++OutFenceValue;

		ThrowIfFailed(InCmdQueue->Signal(InFence.Get(), OutFenceValue));
	}

	void FlushCmdQueue(ComPtr<ID3D12CommandQueue> InCmdQueue, ComPtr<ID3D12Fence> InFence, HANDLE InFenceEvent, uint64_t& OutFenceValue)
	{
		SignalCmdQueue(InCmdQueue, InFence, OutFenceValue);

		WaitForFenceValue(InFence, OutFenceValue, InFenceEvent);
	}

	// Note: enabling debug layer after creating a device will cause the runtime to remove the device.
	// Always enable debug layer before the device to be created.
	void EnableDebugLayer()
	{
#ifdef _DEBUG
		ComPtr<ID3D12Debug> debugInterface;
		ThrowIfFailed(::D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));

		debugInterface->EnableDebugLayer();
#endif
	}

	void ReadFileToBlob(LPCWSTR InFilePath, ID3DBlob** OutFileBlob)
	{
		return ThrowIfFailed(::D3DReadFileToBlob(InFilePath, OutFileBlob));
	}

	ComPtr<ID3D12RootSignature> SerializeAndCreateRootSignature(ComPtr<ID3D12Device2> InDevice, CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC* InRootSigDesc, D3D_ROOT_SIGNATURE_VERSION InVersion)
	{
		// Create Root Signature Blob
		ComPtr<ID3DBlob> rootSignatureBlob;
		ComPtr<ID3DBlob> errorBlob;
		ThrowIfFailed(::D3DX12SerializeVersionedRootSignature(InRootSigDesc, InVersion, 
			rootSignatureBlob.GetAddressOf(), errorBlob.GetAddressOf()));

		// Create the Root Signature object from the blob
		ComPtr<ID3D12RootSignature> rootSignature;
		ThrowIfFailed(InDevice->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

		return rootSignature;
	}

	void D3D12VertexBufferView::ReferenceResource(GEPUtils::Graphics::Resource& InResource, size_t DataSize, size_t StrideSize)
	{
		// Note: A Vertex Buffer View does not use any descriptor from a desc heap
		m_VertexBufferView.BufferLocation = static_cast<D3D12Resource&>(InResource).GetInner()->GetGPUVirtualAddress();
		m_VertexBufferView.SizeInBytes = DataSize;
		m_VertexBufferView.StrideInBytes = StrideSize;
	}

	void D3D12IndexBufferView::ReferenceResource(GEPUtils::Graphics::Resource& InResource, size_t InDataSize, GEPUtils::Graphics::BUFFER_FORMAT InFormat)
	{
		// Note: An Index Buffer View does not use any descriptor from a desc heap
		m_IndexBufferView.BufferLocation = static_cast<D3D12Resource&>(InResource).GetInner()->GetGPUVirtualAddress();
		m_IndexBufferView.SizeInBytes = InDataSize;
		m_IndexBufferView.Format = D3D12GEPUtils::BufferFormatToD3D12(InFormat);
	}

	D3D12ShaderResourceView::D3D12ShaderResourceView(GEPUtils::Graphics::Texture& InTextureToReference)
	{
		InitAsTex2DOrCubemap(InTextureToReference);
	}

	void D3D12ShaderResourceView::InitAsTex2DOrCubemap(GEPUtils::Graphics::Texture& InTexture)
{
		if (!m_CpuAllocatedRange)
		{
			// Allocate descriptor in CPU descriptor heap
			m_CpuAllocatedRange = static_cast<GEPUtils::Graphics::D3D12GraphicsAllocator*>(GEPUtils::Graphics::GraphicsAllocator::Get())->GetCpuHeap().AllocateStaticRange(1);
		}
		// TODO Note: we are currently assuming we only handle a single descriptor and never a range... also in D3D12ConstantBufferView::ReferenceBuffer

		// Generate View Desc
		D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
		viewDesc.Format = D3D12GEPUtils::BufferFormatToD3D12(InTexture.GetFormat());
		viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		if (InTexture.GetType() == GEPUtils::Graphics::TEXTURE_TYPE::TEX_CUBE)
		{
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			viewDesc.TextureCube.MipLevels = InTexture.GetMipLevelsNum();
		}
		else
		{
			StopForFail("SRV referenced texture type not handled yet!")
		}

		// Instantiate View
		GEPUtils::Graphics::D3D12Device& d3d12Device = static_cast<GEPUtils::Graphics::D3D12Device&>(GEPUtils::Graphics::GetDevice());

		d3d12Device.GetInner()->CreateShaderResourceView(static_cast<D3D12Texture&>(InTexture).GetInner().Get(), &viewDesc, m_CpuAllocatedRange->m_FirstCpuHandle);

	}

	void D3D12ShaderResourceView::InitAsTex2DArray(GEPUtils::Graphics::Texture& InTexture, uint32_t InArraySize, uint32_t InMostDetailedMip, uint32_t InMipLevels, uint32_t InFirstArraySlice, uint32_t InPlaceSlice)
	{
		// Allocate static descriptor in the CPU-only desc heap
		if (!m_CpuAllocatedRange)
		{
			m_CpuAllocatedRange = static_cast<GEPUtils::Graphics::D3D12GraphicsAllocator*>(GEPUtils::Graphics::GraphicsAllocator::Get())->GetCpuHeap().AllocateStaticRange(1);
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Format = D3D12GEPUtils::BufferFormatToD3D12(InTexture.GetFormat());;
		srvDesc.Texture2DArray.ArraySize = InArraySize;
		srvDesc.Texture2DArray.MostDetailedMip = InMostDetailedMip;
		srvDesc.Texture2DArray.FirstArraySlice = InFirstArraySlice;
		srvDesc.Texture2DArray.MipLevels = InMipLevels;
		srvDesc.Texture2DArray.PlaneSlice = InPlaceSlice;

		// Instantiate View
		GEPUtils::Graphics::D3D12Device& d3d12Device = static_cast<GEPUtils::Graphics::D3D12Device&>(GEPUtils::Graphics::GetDevice());

		d3d12Device.GetInner()->CreateShaderResourceView(static_cast<D3D12Texture&>(InTexture).GetInner().Get(), &srvDesc, m_CpuAllocatedRange->m_FirstCpuHandle);
	}

	void D3D12UnorderedAccessView::InitAsTex2DArray(GEPUtils::Graphics::Texture& InTexture, uint32_t InArraySize, uint32_t InMipSlice, uint32_t InFirstArraySlice, uint32_t InPlaneSlice)
	{
		// Allocate static descriptor in the CPU-only desc heap
		if (!m_CpuAllocatedRange)
		{
			m_CpuAllocatedRange = static_cast<GEPUtils::Graphics::D3D12GraphicsAllocator*>(GEPUtils::Graphics::GraphicsAllocator::Get())->GetCpuHeap().AllocateStaticRange(1);
		}

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Format = D3D12GEPUtils::BufferFormatToD3D12(InTexture.GetFormat());;
		uavDesc.Texture2DArray.ArraySize = InArraySize;
		uavDesc.Texture2DArray.FirstArraySlice = InFirstArraySlice;
		uavDesc.Texture2DArray.MipSlice = InMipSlice;
		uavDesc.Texture2DArray.PlaneSlice = InPlaneSlice;

		// Instantiate View
		GEPUtils::Graphics::D3D12Device& d3d12Device = static_cast<GEPUtils::Graphics::D3D12Device&>(GEPUtils::Graphics::GetDevice());

		d3d12Device.GetInner()->CreateUnorderedAccessView(static_cast<D3D12Texture&>(InTexture).GetInner().Get(), nullptr, &uavDesc, m_CpuAllocatedRange->m_FirstCpuHandle);
	}

	D3D12ConstantBufferView::D3D12ConstantBufferView(GEPUtils::Graphics::Buffer& InResource)
	{
		D3D12GEPUtils::D3D12Resource& buffer = static_cast<D3D12GEPUtils::D3D12Resource&>(InResource);

		ReferenceBuffer(buffer.GetInner()->GetGPUVirtualAddress(), buffer.GetSizeInBytes());
	}

	void D3D12ConstantBufferView::ReferenceBuffer(D3D12_GPU_VIRTUAL_ADDRESS InBufferGPUAddress, size_t InBufferSize)
	{
		if (!m_CpuAllocatedRange)
		{
			// Allocate descriptor in CPU descriptor heap

			m_CpuAllocatedRange = static_cast<GEPUtils::Graphics::D3D12GraphicsAllocator*>(GEPUtils::Graphics::GraphicsAllocator::Get())->GetCpuHeap().AllocateStaticRange(1); // Even if we have a dynamic buffer, the descriptor on the CPU staging desc heap will be allocated statically, and change value frequently
		}

		// Generate View Desc
		D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
		viewDesc.BufferLocation = InBufferGPUAddress;
		viewDesc.SizeInBytes = InBufferSize;

		// Instantiate View
		GEPUtils::Graphics::D3D12Device& d3d12Device = static_cast<GEPUtils::Graphics::D3D12Device&>(GEPUtils::Graphics::GetDevice());

		d3d12Device.GetInner()->CreateConstantBufferView(&viewDesc, m_CpuAllocatedRange->m_FirstCpuHandle);
	}

	void D3D12ConstantBufferView::ReferenceBuffer(GEPUtils::Graphics::Buffer& InResource, size_t InDataSize, size_t InStrideSize)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void D3D12DynamicBuffer::SetData(void* InData, size_t InSize, size_t InAlignmentSize)
	{
		// Note: A buffer needs to have an alignment multiple of D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT which is now 256
		// Assuming D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT is a power of 2, we can align the input value with this formula
		m_AlignmentSize = GEPUtils::Math::Align(InAlignmentSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		m_DataSize = InSize;
		
		m_BufferSize = GEPUtils::Math::Align(InSize, m_AlignmentSize);

		if (m_Data.size() < InSize)
			m_Data.resize(InSize);
		
		memcpy(m_Data.data(), InData, InSize);
	}

	void D3D12Texture::SetGeneralTextureParams(uint32_t InWidth, uint32_t InHeight, GEPUtils::Graphics::TEXTURE_TYPE InType, GEPUtils::Graphics::BUFFER_FORMAT InFormat, uint32_t InArraySize, uint32_t InMipLevels, GEPUtils::Graphics::RESOURCE_FLAGS InCreationFlags)
	{
		m_Width = InWidth; m_Height = InHeight;
		m_ArraySize = InArraySize; m_MipLevelsNum = InMipLevels;
		m_TexelFormat = InFormat;
		m_Type = InType;

		if (InType == GEPUtils::Graphics::TEXTURE_TYPE::TEX_2D || InType == GEPUtils::Graphics::TEXTURE_TYPE::TEX_CUBE)
		{
			m_TextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
				D3D12GEPUtils::BufferFormatToD3D12(InFormat),
				static_cast<UINT>(InWidth),
				static_cast<UINT>(InHeight),
				static_cast<UINT16>(InArraySize),
				static_cast<UINT16>(InMipLevels),	// Note: there is an ORRIBLE bug that comes if we do not specify mip levels here: it will display only one cube face, and the other 5 faces will be black !!!
													// The reason is the default mip levels will become 9, so 9 mips will be generated for the first face all subsequent in memory,
													// so when we later copy subresources, copying the first 6 subresources, we are going to copy the first face plus its first 5 mips instead of the other cube faces!!!
				1U, 0U, D3D12GEPUtils::ResFlagsToD3D12(InCreationFlags)
			);
		}
		else {
			StopForFail("[D3D12Texture::SetGeneralTextureParams] Texture Type not handled yet.")
		}
	}

	D3D12Texture::D3D12Texture(uint32_t InWidth, uint32_t InHeight, GEPUtils::Graphics::TEXTURE_TYPE InType, GEPUtils::Graphics::BUFFER_FORMAT InFormat, uint32_t InArraySize, uint32_t InMipLevels)
	{
		SetGeneralTextureParams(InWidth, InHeight, InType, InFormat, InArraySize, InMipLevels, GEPUtils::Graphics::RESOURCE_FLAGS::NONE);
	}

	D3D12Texture::D3D12Texture(const wchar_t* InTexturePath, GEPUtils::Graphics::TEXTURE_FILE_FORMAT InFileFormat, int32_t InMipsNum, GEPUtils::Graphics::RESOURCE_FLAGS InCreationFlags)
	{
		// Informations about the texture resource
		DirectX::TexMetadata metadata;

		// Content of the texture resource
		DirectX::ScratchImage loadedScratchImage;

		switch (InFileFormat)
		{
		case GEPUtils::Graphics::TEXTURE_FILE_FORMAT::DDS:
		{
			D3D12GEPUtils::ThrowIfFailed(DirectX::LoadFromDDSFile(
				InTexturePath,
				DirectX::DDS_FLAGS_FORCE_RGB,
				&metadata,
				loadedScratchImage));
			break;
		}
		default:
			StopForFail("Trying to load an unhandled texture format!")
				break;
		}

		// If Mips number is not specified, the texture will be created with a number of mips equal to the ones found in the loaded file
		SetGeneralTextureParams(metadata.width, metadata.height, metadata.IsCubemap() ? GEPUtils::Graphics::TEXTURE_TYPE::TEX_CUBE : D3D12GEPUtils::TextureTypeToEngine(metadata.dimension), 
			D3D12GEPUtils::BufferFormatToEngine(metadata.format), metadata.arraySize, InMipsNum == 0? metadata.mipLevels : InMipsNum, InCreationFlags);

		// Fetch data found from file
		const DirectX::Image* foundImages = loadedScratchImage.GetImages();

		// We create a new scratch image with the selected mip levels to compute
		DirectX::ScratchImage newScratchImage;
		switch (metadata.dimension)
		{
		case DirectX::TEX_DIMENSION_TEXTURE2D:
			newScratchImage.Initialize2D(metadata.format, m_Width, m_Height, m_ArraySize, m_MipLevelsNum); // In a Tex2D array, every different texture of the array needs to have the same size (and the same applies for their relative mip levels)
			break;
		default:
			StopForFail("[D3D12Texture] Texture Dimension Not Handled.")
			break;
		}

		// Copying loaded texture data to internal buffer
		m_Data.resize(newScratchImage.GetPixelsSize()); 

		m_SubresourceDesc.resize(m_ArraySize * m_MipLevelsNum);// The number of subresources in a texture is equal to the number of textures multiplied by the number of their mips //scratchImage.GetImageCount());

		const DirectX::Image* newImages = newScratchImage.GetImages();
		size_t imgIdx = newScratchImage.GetImageCount();
		while(imgIdx)
		{
			imgIdx--;
			D3D12_SUBRESOURCE_DATA& currentSubresource = m_SubresourceDesc[imgIdx];
			currentSubresource.RowPitch = newImages[imgIdx].rowPitch;
			currentSubresource.SlicePitch = newImages[imgIdx].slicePitch;
			currentSubresource.pData = m_Data.data() + (newImages[imgIdx].pixels - newScratchImage.GetPixels()); // Storing pointer to the subresource, referring now to the internal buffer region
		}
		// Copy and reference loaded images
		imgIdx = loadedScratchImage.GetImageCount();
		while (imgIdx)
		{
			imgIdx--;
			uint32_t currentMip = imgIdx % metadata.mipLevels;
			uint32_t currentSlice = imgIdx / metadata.mipLevels;
			// Copy loaded mip in the new location
			void* ptrSrc = foundImages[imgIdx].pixels;
			void* ptrDest = m_Data.data() + (newImages[currentSlice * m_MipLevelsNum + currentMip].pixels - newImages->pixels);//(foundImages[imgIdx].pixels - loadedScratchImage.GetPixels());
			memcpy(ptrDest, ptrSrc, foundImages[imgIdx].slicePitch);
			// Assign pointer to the copied data
			//m_SubresourceDesc[(currentSlice * m_MipLevelsNum) + currentMip].pData = ptrDest;//m_Data.data() + (foundImages[imgIdx].pixels - loadedScratchImage.GetPixels());

		}


	}

	void D3D12Texture::UploadToGPU(GEPUtils::Graphics::CommandList& InCommandList, GEPUtils::Graphics::Buffer& InIntermediateBuffer)
	{
		InstantiateOnGPU();

		ID3D12GraphicsCommandList2* d3d12CmdList = static_cast<GEPUtils::Graphics::D3D12CommandList&>(InCommandList).GetInner().Get();

		::UpdateSubresources(d3d12CmdList, m_D3D12Resource.Get(), static_cast<D3D12GEPUtils::D3D12Resource&>(InIntermediateBuffer).GetInner().Get(), 0, 0, m_SubresourceDesc.size(), m_SubresourceDesc.data());

		// Transition texture state to GENERIC_READ to be read by shaders
		// Note: this is not optimal, usually we should transition the resource depending on the situation in which we want to use it, e.g. D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE in the case of pixel shader usage
		CD3DX12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_D3D12Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
		d3d12CmdList->ResourceBarrier(1, &transitionBarrier);
	}

	void D3D12Texture::InstantiateOnGPU()
	{
		ID3D12Device2* d3d12Device = static_cast<GEPUtils::Graphics::D3D12Device&>(GEPUtils::Graphics::GetDevice()).GetInner().Get();

		if (!m_D3D12Resource)
		{
			// Allocate a committed resource in GPU dedicated memory (default heap)
			d3d12Device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&m_TextureDesc,
				D3D12_RESOURCE_STATE_COPY_DEST, // We can create the resource directly in copy destination state since we want to fill it with content
				nullptr,
				IID_PPV_ARGS(&m_D3D12Resource));
		}
		else
		{
			StopForFail("Texture GPU resource already allocated")
		}
	}

	size_t D3D12Texture::GetGPUSize()
	{
		size_t requiredIntermediateSize = 0;
		static_cast<GEPUtils::Graphics::D3D12Device&>(GEPUtils::Graphics::GetDevice()).GetInner()->GetCopyableFootprints(&m_TextureDesc, 0, m_SubresourceDesc.size(), 0, nullptr, nullptr, nullptr, &requiredIntermediateSize);
		return requiredIntermediateSize;
	}

}

