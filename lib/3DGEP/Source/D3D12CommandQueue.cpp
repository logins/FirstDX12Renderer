#include "D3D12CommandQueue.h"
#include <D3D12GEPUtils.h>

namespace D3D12GEPUtils {

	using namespace Microsoft::WRL;

	D3D12CommandQueue::~D3D12CommandQueue()
	{
		::CloseHandle(m_FenceEvent);
	}

	void D3D12CommandQueue::Init(Microsoft::WRL::ComPtr <ID3D12Device2> InDevice, D3D12_COMMAND_LIST_TYPE InCmdListType)
	{
		m_LastSeenFenceValue = 0;
		m_CmdListType = InCmdListType;
		m_Device = InDevice;

		m_CmdQueue = D3D12GEPUtils::CreateCommandQueue(InDevice, InCmdListType);

		m_Fence = D3D12GEPUtils::CreateFence(InDevice);
		m_FenceEvent = D3D12GEPUtils::CreateFenceEventHandle();

		IsInitialized = true;
	}

	ComPtr<ID3D12GraphicsCommandList2> D3D12CommandQueue::GetAvailableCmdList()
	{
		// Get an available command allocator first
		ComPtr<ID3D12CommandAllocator> cmdAllocator;
		// Check first if we have an available allocator in the queue (each allocator uniquely corresponds to a different list)
		// Note: an allocator is available if the relative commands have been fully executed, 
		// so if the relative fence value has been reached by the command queue
		if (!m_CmdAllocators.empty() && IsFenceComplete(m_CmdAllocators.front().FenceValue))
		{
			cmdAllocator = m_CmdAllocators.front().CmdAllocator;
			m_CmdAllocators.pop();

			D3D12GEPUtils::ThrowIfFailed(cmdAllocator->Reset());
		}
		else
		{
			cmdAllocator = D3D12GEPUtils::CreateCommandAllocator(m_Device, m_CmdListType);
		}

		// Then get an available command list
		ComPtr<ID3D12GraphicsCommandList2> cmdList;
		if (!m_CmdLists.empty())
		{
			cmdList = m_CmdLists.front();
			m_CmdLists.pop();
			// Resetting the command list with the previously selected command allocator (so binding the two together)
			cmdList->Reset(cmdAllocator.Get(), nullptr);
		}
		else
		{
			cmdList = D3D12GEPUtils::CreateCommandList(m_Device, cmdAllocator, m_CmdListType, false);
		}

		// Reference the chosen command allocator in the command list's private data, so we can retrieve it on the fly when we need it
		D3D12GEPUtils::ThrowIfFailed(cmdList->SetPrivateDataInterface(__uuidof(cmdAllocator), cmdAllocator.Get()));
		// Note: setting a ComPtr as private data Does increment the reference count of that ComPtr !!

		return cmdList;
	}

	uint64_t D3D12CommandQueue::ExecuteCmdList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> InCmdList)
	{
		InCmdList->Close();

		ID3D12CommandAllocator* cmdAllocator;
		UINT dataSize = sizeof(cmdAllocator);
		D3D12GEPUtils::ThrowIfFailed(InCmdList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &cmdAllocator));

		ID3D12CommandList* const ppCmdLists[] = { InCmdList.Get() };

		m_CmdQueue->ExecuteCommandLists(1, ppCmdLists);
		uint64_t fenceValue = Signal();

		m_CmdAllocators.emplace( CmdAllocatorEntry{fenceValue, cmdAllocator} ); // Note: implicit creation of a ComPtr from a raw pointer to create CmdAllocatorEntry
		m_CmdLists.push(InCmdList);

		// We do not need the local command allocator Pointer anymore, we can release it because the allocator reference is stored in the queue
		cmdAllocator->Release();

		return fenceValue;
	}

	uint64_t D3D12CommandQueue::Signal()
	{
		D3D12GEPUtils::SignalCmdQueue(m_CmdQueue, m_Fence, m_LastSeenFenceValue);
		return m_LastSeenFenceValue;
	}

	bool D3D12CommandQueue::IsFenceComplete(uint64_t InFenceValue)
	{
		return m_Fence->GetCompletedValue() >= InFenceValue;
	}

	void D3D12CommandQueue::WaitForFenceValue(uint64_t InFenceValue)
	{
		D3D12GEPUtils::WaitForFenceValue(m_Fence, InFenceValue, m_FenceEvent);
	}

	void D3D12CommandQueue::Flush()
	{
		D3D12GEPUtils::FlushCmdQueue(m_CmdQueue, m_Fence, m_FenceEvent, m_LastSeenFenceValue);
	}

}
