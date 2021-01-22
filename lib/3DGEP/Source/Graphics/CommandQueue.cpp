/*
 CommandQueue.cpp

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/
 
#include "CommandQueue.h"

#ifdef GRAPHICS_SDK_D3D12
#include "D3D12CommandQueue.h"
#endif
#include "Public/CommandList.h"
namespace GEPUtils { namespace Graphics {

	std::unique_ptr<GEPUtils::Graphics::CommandQueue> CreateCommandQueue(Device& InDevice, COMMAND_LIST_TYPE InCmdListType)
	{
#ifdef GRAPHICS_SDK_D3D12
		return std::make_unique<D3D12GEPUtils::D3D12CommandQueue>(InDevice, InCmdListType);
#endif
	}


	CommandQueue::CommandQueue(GEPUtils::Graphics::Device& InDevice) : m_GraphicsDevice(InDevice)
	{

	}

} }