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
		virtual ~CommandQueue() = 0;// Do not allow this class to be instantiated by itself: it needs to be derived from a platform specific implementation

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

	// Note: This definition is necessary to compile, and still having CommandQueue as abstract class
	inline CommandQueue::~CommandQueue() { }

	std::unique_ptr<CommandQueue> CreateCommandQueue(class Device& InDevice, COMMAND_LIST_TYPE InCmdListType);


} }

#endif // CommandQueue_h__
