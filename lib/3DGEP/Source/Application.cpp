
#include "Application.h"
#include <Windows.h>
#include "GraphicsUtils.h"
#include "GraphicsTypes.h"
#include "Public/GEPUtilsGeometry.h"
#include "Graphics/Public/CommandList.h"
#include <chrono>

using namespace GEPUtils::Graphics;

namespace GEPUtils
{
	Application* Application::m_Instance; // Necessary (as standard 9.4.2.2 specifies) definition of the singleton instance

	void Application::SetFov(float InFov)
	{
		m_Fov = InFov;
		char buffer[256];
		::sprintf_s(buffer, "Fov: %f\n", m_Fov);
		::OutputDebugStringA(buffer);
		// Projection Matrix needs updating
		m_ProjMatrix = GEPUtils::Geometry::Perspective(m_ZMin, m_ZMax, m_AspectRatio, m_Fov);
	}

	void Application::OnMainWindowUpdate()
	{
		if (!m_PaintStarted)
			return;
		// Window paint event will trigger game thread update and render methods (as we are in a simple single threaded example)
		Update();

	}

	void Application::OnWindowPaint()
	{
		m_PaintStarted = true;
	}

	void Application::SetAspectRatio(float InAspectRatio)
	{
		m_AspectRatio = InAspectRatio;
		// Projection Matrix needs updating
		m_ProjMatrix = GEPUtils::Geometry::Perspective(m_ZMin, m_ZMax, m_AspectRatio, m_Fov);
	}

	Application* Application::Get()
	{
		if (m_Instance == nullptr)
			m_Instance = new Application();
		return m_Instance;
	}

	Application::Application()
	{
		// Note: Debug Layer needs to be created before creating the Device
		Graphics::EnableDebugLayer();

		// Create Device
		m_GraphicsDevice = Graphics::CreateDevice();
	}

	void Application::Initialize()
	{

		// Create Command Queue
		m_CmdQueue = Graphics::CreateCommandQueue(*m_GraphicsDevice, Graphics::COMMAND_LIST_TYPE::COMMAND_LIST_TYPE_DIRECT);


		uint32_t mainWindowWidth = 1024, mainWindowHeight = 768;

		m_ScissorRect = Graphics::AllocateRect(0l, 0l, LONG_MAX, LONG_MAX);

		m_Viewport = Graphics::AllocateViewport(0.f, 0.f, static_cast<float>(mainWindowWidth), static_cast<float>(mainWindowHeight));
		m_ZMin = 0.1f;
		m_ZMax = 100.f;
		SetAspectRatio(mainWindowWidth / static_cast<float>(mainWindowHeight));
		SetFov(0.7853981634f);

		Graphics::Window::WindowInitInput mainWindowInput = {
		L"DX12WindowClass", L"Part3 Main Window",
		*m_CmdQueue,
		mainWindowWidth, mainWindowHeight, // Window sizes
		mainWindowWidth, mainWindowHeight, // BackBuffer sizes
		};
		m_MainWindow = Graphics::CreateGraphicsWindow(mainWindowInput);

		// Wiring Window events
		m_MainWindow->OnPaintDelegate.Add<Application, &Application::OnWindowPaint>(this);

		m_IsInitialized = true;

	}

	void Application::Run()
	{
		if (!m_IsInitialized)
			return;

		m_MainWindow->ShowWindow();

		// Application's main loop is based on received window messages, specifically WM_PAINT will trigger Update() and Render()
		MSG windowMessage = {};
		while (windowMessage.message != WM_QUIT)
		{
			if (::PeekMessage(&windowMessage, NULL, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&windowMessage);
				::DispatchMessage(&windowMessage);
			}

			OnMainWindowUpdate();
		}
	}

	void Application::QuitApplication()
	{

	}

	void Application::Update()
	{
		static uint64_t frameCounter = 0;
		static double elapsedSeconds = 0;
		static std::chrono::high_resolution_clock clock;
		auto t0 = clock.now();

		Render();

		frameCounter++;
		auto t1 = clock.now();
		auto deltaTime = t1 - t0;
		t0 = t1;
		elapsedSeconds += deltaTime.count() * 1e-9; // Conversion from nanoseconds into seconds

		if (elapsedSeconds > 1.0)
		{
			char buffer[500]; auto fps = frameCounter / elapsedSeconds;
			sprintf_s(buffer, 500, "Average FPS: %f\n", fps);
			OutputDebugStringA(buffer);

			frameCounter = 0;
			elapsedSeconds = .0f;
		}
	}

	void Application::Render()
	{
		Graphics::Resource& backBuffer = m_MainWindow->GetCurrentBackBuffer();

		Graphics::CommandList& cmdList = m_CmdQueue->GetAvailableCommandList();

		// Clear render target and depth stencil
		{
			// Transitioning current backbuffer resource to render target state
			// We can be sure that the previous state was present because in this application all the render targets
			// are first filled and then presented to the main window repetitevely.
			cmdList.ResourceBarrier(backBuffer,RESOURCE_STATE::PRESENT, RESOURCE_STATE::RENDER_TARGET);

			FLOAT clearColor[] = { .4f, .6f, .9f, 1.f };
			cmdList.ClearRTV(m_MainWindow->GetCurrentRTVDescriptorHandle(), clearColor);

			// Note: Clearing Render Target and Depth Stencil is a good practice, but in this case is also essential.
			// Without clearing the DepthStencilView, the rasterizer would not be able to use it!!
			cmdList.ClearDepth(m_MainWindow->GetCurrentDSVDescriptorHandle());
		}

		RenderContent(cmdList); // Derived classes will call this to render their application content

		// Execute command list and present current render target from the main window
		{
			cmdList.ResourceBarrier(backBuffer, RESOURCE_STATE::RENDER_TARGET, RESOURCE_STATE::PRESENT);

			// Mandatory for the command list to close before getting executed by the command queue
			m_CmdQueue->ExecuteCmdList(cmdList);

			m_MainWindow->Present();

		}
	}

}
