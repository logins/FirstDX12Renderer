#ifndef Window_h__
#define Window_h__


#include <memory>
#include "CommandQueue.h"
#include "Delegate.h"
#include "GraphicsTypes.h"

namespace GEPUtils { namespace Graphics {
/*!
 * \class Window
 *
 * \brief Virtual class to be used as a platform-agnostic representation of a graphic window.
 *
 * \author Riccardo Loggini
 * \date July 2020
 */
	class Window
	{
	public:
		// Struct containing the many params used to initialize a Window
		struct WindowInitInput
		{
			const wchar_t* WindowClassName; const wchar_t* WindowTitle;
			CommandQueue& CmdQueue;
			uint32_t WinWidth; uint32_t WinHeight;
			uint32_t BufWidth; uint32_t BufHeight;
		};


		virtual void Init() = 0; // TODO change required virtual method to destructor

		virtual void ShowWindow() = 0;

		virtual void Present() = 0;

		virtual void Resize(uint32_t InNewWidth, uint32_t InNewHeight) = 0;

		virtual GEPUtils::Graphics::CpuDescHandle& GetCurrentRTVDescriptorHandle() = 0;

		virtual GEPUtils::Graphics::CpuDescHandle& GetCurrentDSVDescriptorHandle() = 0;

		uint32_t GetFrameWidth() const { return m_FrameWidth; }
		uint32_t GetFrameHeight() const { return M_FrameHeight; }

		GEPUtils::Graphics::Resource& GetCurrentBackBuffer() { return *m_BackBuffers[m_CurrentBackBufferIndex]; }

		GEPUtils::MulticastDelegate<> OnPaintDelegate;
		GEPUtils::MulticastDelegate<> OnCreateDelegate;
		GEPUtils::MulticastDelegate<> OnDestroyDelegate;
		GEPUtils::MulticastDelegate<uint32_t, int32_t, int32_t> OnMouseButtonUpDelegate;
		GEPUtils::MulticastDelegate<uint32_t, int32_t, int32_t> OnMouseButtonDownDelegate;
		GEPUtils::MulticastDelegate<float> OnMouseWheelDelegate;
		GEPUtils::MulticastDelegate<int32_t, int32_t> OnMouseMoveDelegate;
		GEPUtils::MulticastDelegate<uint64_t> OnKeyDownDelegate;
		GEPUtils::MulticastDelegate<uint64_t> OnSysKeyDownDelegate;
		GEPUtils::MulticastDelegate<uint32_t, uint32_t> OnResizeDelegate;

		bool IsMouseRightHold() const { return m_IsMouseRightHold; }
		bool IsMouseLeftHold() const { return m_IsMouseLeftHold; }

	protected:

		uint32_t m_FrameWidth = 1, M_FrameHeight = 1;

		// Default number of buffers handled by the swapchain
		static const uint32_t m_DefaultBufferCount = 3;

		uint32_t m_CurrentBackBufferIndex = 0;

		std::vector<std::unique_ptr<GEPUtils::Graphics::Resource>> m_BackBuffers;

		bool m_IsMouseLeftHold = false, m_IsMouseRightHold = false;

	};

	std::unique_ptr<Window> CreateGraphicsWindow(const Window::WindowInitInput& InWindowInitInput);

} }

#endif // Window_h__
