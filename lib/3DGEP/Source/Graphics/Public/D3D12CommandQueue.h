#ifndef D3D12CommandQueue_h__
#define D3D12CommandQueue_h__

#include <wrl.h>
#include <d3d12.h>
#include <queue> // For std::queue
#include "CommandQueue.h"

namespace D3D12GEPUtils {

	class D3D12CommandQueue : public GEPUtils::Graphics::CommandQueue
	{
	public:

		D3D12CommandQueue(class GEPUtils::Graphics::Device& InDevice, GEPUtils::Graphics::COMMAND_LIST_TYPE InCmdListType);

		~D3D12CommandQueue();

		void Init(Microsoft::WRL::ComPtr <ID3D12Device2> InDevice, D3D12_COMMAND_LIST_TYPE InCmdListType);

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetAvailableCmdList();

		// Platform-agnostic version
		virtual GEPUtils::Graphics::CommandList& GetAvailableCommandList() override;

		uint64_t ExecuteCmdList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> InCmdList);

		// Platform-agnostic version
		virtual uint64_t ExecuteCmdList(GEPUtils::Graphics::CommandList& InCmdList) override;


		uint64_t Signal();
		bool IsFenceComplete(uint64_t InFenceValue);
		void WaitForFenceValue(uint64_t InFenceValue);
		// Signals the fence and stalls the thread it is invoked on to wait for the just signaled fence value
		virtual void Flush();

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CmdQueue() const { return m_CmdQueue; }

		Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12Device() const { return m_Device; };


	private:
		// Keep track of command allocators in current execution
		struct CmdAllocatorEntry
		{
			uint64_t FenceValue;
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdAllocator;
		};
		using D3D12CmdAllocatorQueue = std::queue<CmdAllocatorEntry>;

		using D3D12CmdListQueue = std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>>;

		D3D12_COMMAND_LIST_TYPE m_CmdListType;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CmdQueue;
		Microsoft::WRL::ComPtr<ID3D12Device2> m_Device;

		Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
		HANDLE m_FenceEvent;
		uint64_t m_LastSeenFenceValue = 0;

		D3D12CmdAllocatorQueue m_CmdAllocators;
		D3D12CmdListQueue m_CmdLists;

		bool IsInitialized = false;
	};

}
#endif // D3D12CommandQueue_h__
