/*
 Application.cpp

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/
 
#include "Application.h"
#include <chrono>
#include <Windows.h>
#include "GraphicsUtils.h"
#include "GraphicsTypes.h"
#include "GEPUtilsGeometry.h"
#include "CommandList.h"
#include "GraphicsAllocator.h"

using namespace GEPUtils::Graphics;

namespace GEPUtils
{
	Application* Application::m_Instance; // Necessary (as standard 9.4.2.2 specifies) definition of the singleton instance

	uint64_t Application::m_CpuFrameNumber = 1;

	bool Application::CanComputeFrame()
	{
		return m_PaintStarted;
	}

	void Application::SetFov(float InFov)
	{
		m_Fov = InFov;
		char buffer[256];
		::sprintf_s(buffer, "Fov: %f\n", m_Fov);
		::OutputDebugStringA(buffer);
		// Projection Matrix needs updating
		m_ProjMatrix = GEPUtils::Geometry::Perspective(m_ZMin, m_ZMax, m_AspectRatio, m_Fov);
	}

	void Application::OnMainWindowClose()
	{
		::PostQuitMessage(0); // Next message from winapi will be WM_QUIT
	}

	void Application::OnWindowPaint()
	{
		m_PaintStarted = true;
	}

	void Application::OnWindowResize(uint32_t InNewWidth, uint32_t InNewHeight)
	{
		m_MainWindow->Resize(InNewWidth, InNewHeight);
		// Update ViewPort here since the application acts as a "frame director" here
		m_Viewport->SetWidthAndHeight(static_cast<FLOAT>(InNewWidth), static_cast<FLOAT>(InNewHeight));
		SetAspectRatio(InNewWidth / static_cast<float>(InNewHeight));
	}


	void Application::SetAspectRatio(float InAspectRatio)
	{
		m_AspectRatio = InAspectRatio;
		// Projection Matrix needs updating
		m_ProjMatrix = GEPUtils::Geometry::Perspective(m_ZMin, m_ZMax, m_AspectRatio, m_Fov);
	}


	void Application::OnCpuFrameStarted()
	{
		// Checking if we are too far in frame computation compared to the GPU work.
		// If it is the case, wait for completion
		const int64_t framesToWaitNum = m_CmdQueue->ComputeFramesInFlightNum() - GetMaxConcurrentFramesNum() + 1; // +1 because we need space for the current frame
		if (framesToWaitNum > 0)
		{
			m_CmdQueue->WaitForQueuedFramesOnGpu(framesToWaitNum);
		}

		// Trigger all the begin CPU frame mechanics
		m_CmdQueue->OnCpuFrameStarted();

		GEPUtils::Graphics::GraphicsAllocator::Get()->OnNewFrameStarted();
	}

	void Application::OnCpuFrameFinished()
	{
		m_CmdQueue->OnCpuFrameFinished();
	}

	Application::Application()
		: m_GraphicsDevice(Graphics::GetDevice())
	{

	}

	void Application::Initialize()
	{
		GEPUtils::Graphics::GraphicsAllocator::Get()->Initialize();

		// Create Command Queue
		m_CmdQueue = Graphics::CreateCommandQueue(m_GraphicsDevice, Graphics::COMMAND_LIST_TYPE::COMMAND_LIST_TYPE_DIRECT);

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
		false // vsync disabled to test max fps, but you can set it here if the used monitor allows tearing to happen
		};
		m_MainWindow = Graphics::CreateGraphicsWindow(mainWindowInput);

		// Wiring Window events
		m_MainWindow->OnPaintDelegate.Add<Application, &Application::OnWindowPaint>(this);

		m_MainWindow->OnResizeDelegate.Add<Application, &Application::OnWindowResize>(this);

		m_MainWindow->OnDestroyDelegate.Add<Application, &Application::OnMainWindowClose>(this);

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

			if (CanComputeFrame())
			{

				Update();

			}
		}

		OnQuitApplication();
	}

	void Application::OnQuitApplication()
	{
		// Finish all the render commands currently in flight
		m_CmdQueue->Flush();

		m_MainWindow.reset();

		m_CmdQueue.reset();

		// Release all the allocated graphics resources
		GEPUtils::Graphics::GraphicsAllocator::ShutDown();
	}

	void Application::Update()
	{
		static double elapsedSeconds = 0;
		static uint64_t frameNumberPerSecond = 0;
		static std::chrono::high_resolution_clock clock;
		auto t0 = clock.now();

		OnCpuFrameStarted();

		UpdateContent(m_DeltaTime);

		Render();

		// Frame on CPU side finished computing, send the notice
		OnCpuFrameFinished();

		m_CpuFrameNumber++;
		frameNumberPerSecond++;

		auto t1 = clock.now();
		auto deltaTime = t1 - t0;

		m_DeltaTime = std::chrono::duration_cast<std::chrono::microseconds>(deltaTime).count() / 1000.f; // Delta Time expressed in Milliseconds: 10^-3 seconds

		t0 = t1;
		elapsedSeconds += deltaTime.count() * 1e-9; // Conversion from nanoseconds into seconds

		if (elapsedSeconds > 1.0)
		{
			char buffer[500]; auto fps = frameNumberPerSecond / elapsedSeconds;
			sprintf_s(buffer, 500, "Average FPS: %f\n", fps);
			OutputDebugStringA(buffer);

			frameNumberPerSecond = 0;
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
