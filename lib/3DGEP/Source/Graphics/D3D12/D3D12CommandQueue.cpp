/*
 D3D12CommandQueue.cpp

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/
 
#include "D3D12CommandQueue.h"
#include <D3D12GEPUtils.h>
#include "D3D12Device.h"
#include "D3D12UtilsInternal.h"
#include "D3D12CommandList.h"
#include "../Public/CommandList.h"

namespace D3D12GEPUtils {

	using namespace Microsoft::WRL;

	D3D12CommandQueue::D3D12CommandQueue(GEPUtils::Graphics::Device& InDevice, GEPUtils::Graphics::COMMAND_LIST_TYPE InCmdListType)
		: GEPUtils::Graphics::CommandQueue(InDevice)
	{
		// Note: static cast reference conversion because at this point we should be certain that the device is a D3D12 one
		Init(static_cast<GEPUtils::Graphics::D3D12Device&>(InDevice).GetInner(), D3D12GEPUtils::CmdListTypeToD3D12(InCmdListType));
	}

	D3D12CommandQueue::~D3D12CommandQueue()
	{
		GEPUtils::Graphics::D3D12BufferAllocator::ShutDown(); // TODO this class needs to be refractored inside the graphics allocator

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

	// Platform-agnostic version
	GEPUtils::Graphics::CommandList& D3D12CommandQueue::GetAvailableCommandList()
	{
		// Get an available command allocator first
		ComPtr<ID3D12CommandAllocator> cmdAllocator;
		// Check first if we have an available allocator in the queue (each allocator uniquely corresponds to a different list)
		// Note: an allocator is available if the relative commands have been fully executed, 
		// so if the relative fence value has been reached by the command queue
		if (!m_CmdAllocators.empty() && IsFenceComplete(m_CmdAllocators.front().FenceValue))
		{
			cmdAllocator = m_CmdAllocators.front().CmdAllocator;
			GEPUtils::Graphics::D3D12BufferAllocator::Get().ReleasePage(m_CmdAllocators.front().DynamicBufAllocatorPageIdx); // When the command allocator finishes executing, we can release memory of the relative dynamic buffer allocator
			m_CmdAllocators.pop();

			D3D12GEPUtils::ThrowIfFailed(cmdAllocator->Reset());
		}
		else
		{
			cmdAllocator = D3D12GEPUtils::CreateCommandAllocator(m_Device, m_CmdListType);
		}

		// Then get an available command list
		if (!m_CmdListsAvailable.empty())
		{
			GEPUtils::Graphics::D3D12CommandList* outObj = static_cast<GEPUtils::Graphics::D3D12CommandList*>(m_CmdListsAvailable.front());
			outObj->SetDynamicBufAllocatorPage(GEPUtils::Graphics::D3D12BufferAllocator::Get().ReservePage()); // We need to assign an available dynamic buffer allocator because the previous one can be in flight

			auto cmdList = outObj->GetInner();
			m_CmdListsAvailable.pop();
			// Resetting the command list with the previously selected command allocator (so binding the two together)
			cmdList->Reset(cmdAllocator.Get(), nullptr);
			// Reference the chosen command allocator in the command list's private data, so we can retrieve it on the fly when we need it
			D3D12GEPUtils::ThrowIfFailed(outObj->GetInner()->SetPrivateDataInterface(__uuidof(cmdAllocator), cmdAllocator.Get()));
			// Note: setting a ComPtr as private data Does increment the reference count of that ComPtr !!
			return *outObj;
		}
		
		// If here, we need to create a new command list
		m_CmdListPool.emplace(std::make_unique<GEPUtils::Graphics::D3D12CommandList>(D3D12GEPUtils::CreateCommandList(m_Device, cmdAllocator, m_CmdListType, false), m_GraphicsDevice));
		GEPUtils::Graphics::D3D12CommandList& outObj = *static_cast<GEPUtils::Graphics::D3D12CommandList*>(m_CmdListPool.back().get());
		
		D3D12GEPUtils::ThrowIfFailed(outObj.GetInner()->SetPrivateDataInterface(__uuidof(cmdAllocator), cmdAllocator.Get()));
		
		return outObj;
	}

	uint64_t D3D12CommandQueue::ExecuteCmdList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> InCmdList)
	{
		InCmdList->Close();

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator;
		UINT dataSize = sizeof(cmdAllocator);
		D3D12GEPUtils::ThrowIfFailed(InCmdList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, cmdAllocator.GetAddressOf()));

		ID3D12CommandList* const ppCmdLists[] = { InCmdList.Get() };

		m_CmdQueue->ExecuteCommandLists(1, ppCmdLists);
		uint64_t fenceValue = Signal();

		m_CmdAllocators.emplace( CmdAllocatorEntry{fenceValue, cmdAllocator, 0 } ); // Note: implicit creation of a ComPtr from a raw pointer to create CmdAllocatorEntry
		m_CmdLists.push(InCmdList);

		// We do not need the local command allocator Pointer anymore, we can release it because the allocator reference is stored in the queue


		return fenceValue;
	}

	// Platform-agnostic version
	uint64_t D3D12CommandQueue::ExecuteCmdList(GEPUtils::Graphics::CommandList& InCmdList)
	{
		InCmdList.Close();

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator;
		UINT dataSize = sizeof(cmdAllocator);

		auto d3d12CmdList = static_cast<GEPUtils::Graphics::D3D12CommandList&>(InCmdList).GetInner();

		D3D12GEPUtils::ThrowIfFailed(d3d12CmdList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, cmdAllocator.GetAddressOf()));

		ID3D12CommandList* const ppCmdLists[] = { d3d12CmdList.Get() };

		m_CmdQueue->ExecuteCommandLists(1, ppCmdLists);
		uint64_t fenceValue = Signal();

		m_CmdAllocators.emplace(CmdAllocatorEntry{ fenceValue, cmdAllocator, static_cast<GEPUtils::Graphics::D3D12CommandList&>(InCmdList).GetDynamicBufAllocatorPage() }); // Note: implicit creation of a ComPtr from a raw pointer to create CmdAllocatorEntry
		
		m_CmdListsAvailable.push(&InCmdList);

		// We do not need the local command allocator Pointer anymore, we can release it because the allocator reference is stored in the queue


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
