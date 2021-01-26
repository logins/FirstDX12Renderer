/*
 CommandQueue.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef CommandQueue_h__
#define CommandQueue_h__

#include <memory>
#include <queue>
#include "CommandList.h"

namespace GEPUtils { namespace Graphics {

enum class COMMAND_LIST_TYPE : int;

class Device;

/*!
 * \class CommandQueue
 *
 * \brief Platform agnostic representation of a graphics command queue.
 *
 * \author Riccardo Loggini
 * \date July 2020
 */
	class CommandQueue
	{
	public:
		virtual ~CommandQueue() = default;

		CommandQueue(GEPUtils::Graphics::Device& InDevice);

		virtual GEPUtils::Graphics::CommandList& GetAvailableCommandList() = 0;

		virtual uint64_t ExecuteCmdList(GEPUtils::Graphics::CommandList& InCmdList) = 0;

		virtual void Flush() = 0;

	protected:

		using CmdListQueue = std::queue<std::unique_ptr<GEPUtils::Graphics::CommandList>>;
		// Note: references are objects that are not copyable hence we cannot use them for containers and need to store pointers
		using CmdListQueueRefs = std::queue<GEPUtils::Graphics::CommandList*>;

		CmdListQueue m_CmdListPool;
		CmdListQueueRefs m_CmdListsAvailable;

		// Platform-agnostic reference to the device that holds this command queue
		GEPUtils::Graphics::Device& m_GraphicsDevice;
	};

	std::unique_ptr<CommandQueue> CreateCommandQueue(class Device& InDevice, COMMAND_LIST_TYPE InCmdListType);


} }

#endif // CommandQueue_h__
