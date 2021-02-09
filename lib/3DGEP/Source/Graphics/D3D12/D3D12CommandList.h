/*
 D3D12CommandList.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef D3D12CommandList_h__
#define D3D12CommandList_h__

#include "CommandList.h"
#include <wrl.h>
#include <functional>
#include "d3dx12.h"

namespace GEPUtils { namespace Graphics {

	class D3D12Device;

	class D3D12CommandList : public GEPUtils::Graphics::CommandList
	{
	public:
		D3D12CommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> InCmdList, GEPUtils::Graphics::Device& InOwningDevice);
		
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>& GetInner() { return m_D3D12CmdList; }

		virtual void ResourceBarrier(GEPUtils::Graphics::Resource& InResource, GEPUtils::Graphics::RESOURCE_STATE InPrevState, GEPUtils::Graphics::RESOURCE_STATE InAfterState) override;


		virtual void ClearRTV(GEPUtils::Graphics::CpuDescHandle& InDescHandle, float* InColor) override;


		virtual void ClearDepth(GEPUtils::Graphics::CpuDescHandle& InDescHandle) override;


		virtual void Close() override;

		virtual void SetPipelineStateAndResourceBinder(Graphics::PipelineState& InPipelineState) override;


		virtual void SetInputAssemblerData(GEPUtils::Graphics::PRIMITIVE_TOPOLOGY InPrimTopology, GEPUtils::Graphics::VertexBufferView& InVertexBufView, GEPUtils::Graphics::IndexBufferView& InIndexBufView) override;


		virtual void SetViewportAndScissorRect(GEPUtils::Graphics::ViewPort& InViewport, GEPUtils::Graphics::Rect& InScissorRect) override;


		virtual void SetRenderTargetFromWindow(GEPUtils::Graphics::Window& InWindow) override;


		virtual void SetGraphicsRootConstants(uint64_t InRootParameterIndex, uint64_t InNum32BitValuesToSet, const void* InSrcData, uint64_t InDestOffsetIn32BitValues) override;

		virtual void SetComputeRootConstants(uint64_t InRootParameterIndex, uint64_t InNum32BitValuesToSet, const void* InSrcData, uint64_t InDestOffsetIn32BitValues) override;


		virtual void DrawIndexed(uint64_t InIndexCountPerInstance) override;

		virtual void Dispatch(uint32_t InGroupsNumX, uint32_t InGroupsNumY, uint32_t InGroupsNumZ) override;

		virtual void SetGraphicsRootTable(uint32_t InRootIndex, GEPUtils::Graphics::ConstantBufferView& InView) override;

		virtual void StoreAndReferenceDynamicBuffer(uint32_t InRootIdx, GEPUtils::Graphics::DynamicBuffer& InDynBuffer, GEPUtils::Graphics::ConstantBufferView& InResourceView) override;


		virtual void UploadBufferData(GEPUtils::Graphics::Buffer& DestinationBuffer, GEPUtils::Graphics::Buffer& IntermediateBuffer, const void* InBufferData, size_t InDataSize) override;


		virtual void UploadViewToGPU(GEPUtils::Graphics::ShaderResourceView& InSRV) override;

		virtual void UploadUavToGpu(GEPUtils::Graphics::UnorderedAccessView& InUav) override;

		virtual void ReferenceSRV(uint32_t InRootIdx, GEPUtils::Graphics::ShaderResourceView& InSRV) override;

		virtual void ReferenceComputeTable(uint32_t InRootIdx, GEPUtils::Graphics::ShaderResourceView& InUav) override;

		virtual void ReferenceComputeTable(uint32_t InRootIdx, GEPUtils::Graphics::UnorderedAccessView& InUav) override;


		void SetGraphicsRootDescriptorTable(uint32_t InRootIdx, D3D12_GPU_DESCRIPTOR_HANDLE InGpuDescHandle);

		CD3DX12_GPU_DESCRIPTOR_HANDLE CopyDynamicDescriptorsToBoundHeap(uint32_t InTablesNum, D3D12_CPU_DESCRIPTOR_HANDLE* InDescHandleArray, uint32_t* InRageSizeArray);

	private:
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> m_D3D12CmdList;

		class D3D12StagedDescriptorManager {
		public:
			// Dynamic entries will be first uploaded to the desc heap bound to the root signature, and then bound to the command list as root table when the next draw/dispatch command is executed
			void StageDynamicDescriptors(uint32_t InRootParamIndex, D3D12_CPU_DESCRIPTOR_HANDLE InFirstCpuDescHandle, uint32_t InRangeSize);

			// Static entries are already allocated in GPU and they will be directly bound to the command list as root table when the next draw/dispatch command is executed
			void StageStaticDescriptors(uint32_t InRootParamIndex, D3D12_GPU_DESCRIPTOR_HANDLE InFirstGpuDescHandle);

			void CommitStagedDescriptorsForDraw(D3D12CommandList& InCmdList);

		private:
			void CommitStagedDescriptors_Internal(D3D12CommandList& InCmdList, std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> InSetFn);

			uint32_t m_DynamicTableRootIdx[32];
			D3D12_CPU_DESCRIPTOR_HANDLE m_DynamicTableFirstHandle[32];
			uint32_t m_DynamicRangeSizes[32];
			uint32_t m_CurrentStagedDynamicTablesNum = 0;

			uint32_t m_StaticTableRootIdx[32];
			D3D12_GPU_DESCRIPTOR_HANDLE m_StaticTableFirstHandle[32];
			uint32_t m_CurrentStagedStaticTablesNum = 0;
		};
		D3D12StagedDescriptorManager m_StagedDescriptorManager;

	};

	} }
#endif // D3D12CommandList_h__
