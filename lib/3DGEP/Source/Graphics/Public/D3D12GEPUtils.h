#ifndef D3D12GEPUtils_h__
#define D3D12GEPUtils_h__

#include <iostream>
#include <chrono>
#include <Windows.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3dx12.h>
#include "GraphicsTypes.h"

#ifdef max
#undef max // This is needed to avoid conflicts with functions called max(), like chrono::milliseconds::max()
#endif

#define Q(x) L#x
#define LQUOTE(x) Q(x)

namespace D3D12GEPUtils {

	using namespace Microsoft::WRL;

	void PrintHello();

	ComPtr<IDXGIAdapter4> GetMainAdapter(bool InUseWarp);

	ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> InAdapter);

	ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> InDevice, D3D12_COMMAND_LIST_TYPE InType);

	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType, UINT InNumDescriptors);

	ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> InDevice, D3D12_COMMAND_LIST_TYPE InCmdListType);

	ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(ComPtr<ID3D12Device2> InDevice, ComPtr<ID3D12CommandAllocator> InCmdAllocator, D3D12_COMMAND_LIST_TYPE InCmdListType, bool InInitClosed = true);

	ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> InDevice);

	HANDLE CreateFenceEventHandle();

	void TransitionResource(ComPtr<ID3D12GraphicsCommandList2> InCmdList, ComPtr<ID3D12Resource> InResource, D3D12_RESOURCE_STATES InBeforeStates, D3D12_RESOURCE_STATES InAfterStates);

	void ClearRTV(ID3D12GraphicsCommandList2* InCmdList, D3D12_CPU_DESCRIPTOR_HANDLE InRTVCPUDescHandle, FLOAT* InClearColor);

	void ClearDepth(ID3D12GraphicsCommandList2* InCmdList, D3D12_CPU_DESCRIPTOR_HANDLE InDepthCPUDescHandle, FLOAT InDepth = 1.0f);

	void UpdateBufferResource(ComPtr<ID3D12Device2> InDevice, ComPtr<ID3D12GraphicsCommandList2> InCmdList, ID3D12Resource** InDestResource, ID3D12Resource** InIntermediateResource,
		size_t InNunElements, size_t InElementSize, const void* InBufferData, D3D12_RESOURCE_FLAGS InFlags = D3D12_RESOURCE_FLAG_NONE
		);

	void CreateCommittedResource(ComPtr<ID3D12Device2> InDevice, ID3D12Resource** InResource, D3D12_HEAP_TYPE InHeapType, uint64_t InBufferSize, D3D12_RESOURCE_FLAGS InFlags, D3D12_RESOURCE_STATES InInitialStates);

	void CreateDepthStencilCommittedResource(ComPtr<ID3D12Device2> InDevice, ID3D12Resource** InResource, uint64_t InWidth, uint64_t InHeight, D3D12_RESOURCE_STATES InInitialStates, D3D12_CLEAR_VALUE* InClearValue);

	void CreateDepthStencilView(ComPtr<ID3D12Device2> InDevice, ID3D12Resource* InResource, D3D12_CPU_DESCRIPTOR_HANDLE& InDSVCPUDescHandle);

	void SignalCmdQueue(ComPtr<ID3D12CommandQueue> InCmdQueue, ComPtr<ID3D12Fence> InFence, uint64_t& OutFenceValue);

	// Stalls the thread up until the InFenceEvent is signaled with InFenceValue, or when optional InMaxDuration has passed
	void WaitForFenceValue(ComPtr<ID3D12Fence> InFence, uint64_t InFenceValue, HANDLE InFenceEvent, std::chrono::milliseconds InMaxDuration = std::chrono::milliseconds::max());

	void FlushCmdQueue(ComPtr<ID3D12CommandQueue> InCmdQueue, ComPtr<ID3D12Fence> InFence, HANDLE InFenceEvent, uint64_t& OutFenceValue);

	void EnableDebugLayer();

	void ReadFileToBlob(LPCWSTR InFilePath, ID3DBlob** OutFileBlob);

	ComPtr<ID3D12RootSignature> SerializeAndCreateRootSignature(ComPtr<ID3D12Device2> InDevice, CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC* InRootSigDesc, D3D_ROOT_SIGNATURE_VERSION InVersion);

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw std::exception();
		}
	}

	struct D3D12CpuDescriptorHandle : public GEPUtils::Graphics::CpuDescHandle{
		D3D12CpuDescriptorHandle() {}
		D3D12CpuDescriptorHandle(CD3DX12_CPU_DESCRIPTOR_HANDLE InDescHandle)
			: m_DescHandle(InDescHandle)
		{
				IsBound = true;
		}
		CD3DX12_CPU_DESCRIPTOR_HANDLE& GetInner() { return m_DescHandle; }
	private:
		CD3DX12_CPU_DESCRIPTOR_HANDLE m_DescHandle;
	};

	struct D3D12Resource : public GEPUtils::Graphics::Resource {
		D3D12Resource(Microsoft::WRL::ComPtr<ID3D12Resource> InResource)
			: m_D3D12Resource(InResource)
		{ }
		Microsoft::WRL::ComPtr<ID3D12Resource>& GetInner() { return m_D3D12Resource; }
		void SetInner(Microsoft::WRL::ComPtr<ID3D12Resource> InResource) { m_D3D12Resource = InResource; }
	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_D3D12Resource;
	};

	struct D3D12VertexBufferView : public GEPUtils::Graphics::VertexBufferView {
		virtual void ReferenceResource(GEPUtils::Graphics::Resource& InResource, size_t DataSize, size_t StrideSize)
		{
			m_VertexBufferView.BufferLocation = static_cast<D3D12Resource&>(InResource).GetInner()->GetGPUVirtualAddress();
			m_VertexBufferView.SizeInBytes = DataSize;
			m_VertexBufferView.StrideInBytes = StrideSize;
		}
		D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	};

	struct D3D12IndexBufferView : public GEPUtils::Graphics::IndexBufferView {
		virtual void ReferenceResource(GEPUtils::Graphics::Resource& InResource, size_t InDataSize, GEPUtils::Graphics::BUFFER_FORMAT InFormat);
		D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
	};

	
	struct D3D12Rect : public GEPUtils::Graphics::Rect {
		D3D12Rect(int32_t InLeft, int32_t InTop, int32_t InRight, int32_t InBottom)
			: D3d12Rect(InLeft, InTop, InRight, InBottom)
		{ }
		CD3DX12_RECT D3d12Rect;
	};

	struct D3D12ViewPort : public GEPUtils::Graphics::ViewPort {
		D3D12ViewPort(float InTopLeftX, float InTopLeftY, float InWidth, float InHeight)
			: D3d12Viewport(InTopLeftX, InTopLeftY, InWidth, InHeight)
		{ }
		CD3DX12_VIEWPORT D3d12Viewport;
	};

	struct D3D12Shader : public GEPUtils::Graphics::Shader {
		D3D12Shader(ComPtr<ID3DBlob> InShaderBlob) : m_ShaderBlob(InShaderBlob) { }
		ComPtr<ID3DBlob> m_ShaderBlob;
	};

}


#endif // D3D12GEPUtils_h__
