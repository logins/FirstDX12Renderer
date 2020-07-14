#ifndef D3D12CommandQueue_h__
#define D3D12CommandQueue_h__

#include <wrl.h>
#include <d3d12.h>
#include <queue> // For std::queue

namespace D3D12GEPUtils {

	class D3D12CommandQueue
	{
	public:

		~D3D12CommandQueue();

		void Init(Microsoft::WRL::ComPtr <ID3D12Device2> InDevice, D3D12_COMMAND_LIST_TYPE InCmdListType);

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetAvailableCmdList();

		uint64_t ExecuteCmdList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> InCmdList);

		uint64_t Signal();
		bool IsFenceComplete(uint64_t InFenceValue);
		void WaitForFenceValue(uint64_t InFenceValue);
		// Signals the fence and stalls the thread it is invoked on to wait for the just signaled fence value
		void Flush();

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CmdQueue() const { return m_CmdQueue; }

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
