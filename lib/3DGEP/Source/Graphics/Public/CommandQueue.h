/*
 CommandQueue.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef CommandQueue_h__
#define CommandQueue_h__

namespace GEPUtils { namespace Graphics {

enum class COMMAND_LIST_TYPE : int;

class Device;
class CommandList;

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


		virtual GEPUtils::Graphics::CommandList& GetAvailableCommandList() = 0;

		virtual uint64_t ExecuteCmdList(GEPUtils::Graphics::CommandList& InCmdList) = 0;

		virtual void Flush() = 0;

		virtual void OnCpuFrameStarted() = 0;

		virtual void OnCpuFrameFinished() = 0;

		virtual uint64_t ComputeFramesInFlightNum() = 0;

		virtual void WaitForQueuedFramesOnGpu(uint64_t InFramesToWaitNum) = 0;

	};



} }

#endif // CommandQueue_h__
