#include <Part2.h>
#include <iostream>
#include <algorithm>
#include <D3D12GEPUtils.h>
#include <D3D12Window.h>
#include <wrl.h> // For WRL::ComPtr
#include <dxgi1_6.h>
#include <d3dx12.h>
#include <GEPUtilsGeometry.h>
#include <Windowsx.h> // For mouse macros
#include <Delegate.h>

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#define PART2_SHADERS_PATH(NAME) LQUOTE(PART2_PROJ_ROOT_PATH/shaders/NAME)

using namespace Microsoft::WRL;

int main()
{
	std::cout << "Hello from Part 2!" << std::endl;

	D3D12GEPUtils::PrintHello();

	Part2::Get()->Initialize();

	Part2::Get()->Run();

}

Part2* Part2::m_Instance; // Necessary (as standard 9.4.2.2 specifies) definition of the singleton instance

Part2* Part2::Get()
{
	if (m_Instance == nullptr)
		m_Instance = new Part2();
	return m_Instance;
}

void Part2::Initialize()
{
	// Create all the D3D objects requited for rendering and create the main window
	ComPtr<IDXGIAdapter4> adapter = D3D12GEPUtils::GetMainAdapter(false);
	// Note: Debug Layer needs to be created before creating the Device
	D3D12GEPUtils::EnableDebugLayer();
	m_GraphicsDevice = D3D12GEPUtils::CreateDevice(adapter);
	m_CmdQueue = D3D12GEPUtils::CreateCommandQueue(m_GraphicsDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_Fence = D3D12GEPUtils::CreateFence(m_GraphicsDevice);
	m_FenceEvent = D3D12GEPUtils::CreateFenceEventHandle();

	m_ScissorRect = CD3DX12_RECT(0l, 0l, LONG_MAX, LONG_MAX);

	uint32_t mainWindowWidth = 1024, mainWindowHeight = 768;

	m_Viewport = CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(mainWindowWidth), static_cast<float>(mainWindowHeight));
	m_FoV = 0.7853981634f;
	m_ZMin = 0.1f;
	m_ZMax = 100.f;
	m_AspectRatio = 1024 / 768.f; // TODO change this to dynamic

	// Initialize main render window
	D3D12GEPUtils::D3D12Window::D3D12WindowInitInput windowInitInput = {
		L"DX12WindowClass", L"Part2 Main Window",
		m_GraphicsDevice,
		mainWindowWidth, mainWindowHeight, // Window sizes
		mainWindowWidth, mainWindowHeight, // BackBuffer sizes
		m_CmdQueue, MainWndProc, m_Fence
	};
	m_MainWindow.Initialize(windowInitInput);

	LoadContent();
	
	m_IsInitialized = true;
}

void Part2::OnLeftMouseDrag(int32_t InDeltaX, int32_t InDeltaY)
{
	Eigen::Transform<float, 3, Eigen::Affine> tr; 
	tr.setIdentity();
	tr.rotate(Eigen::AngleAxisf(-InDeltaX / 1024.f, Eigen::Vector3f::UnitY())) //TODO set variable window size
		.rotate(Eigen::AngleAxisf(-InDeltaY / 768.f, Eigen::Vector3f::UnitX()));
	
	m_ModelMatrix = tr.matrix() * m_ModelMatrix;
}

void Part2::OnRightMouseDrag(int32_t InDeltaX, int32_t InDeltaY)
{
	Eigen::Transform<float, 3, Eigen::Affine> tr; 
	tr.setIdentity();
	tr.translate(Eigen::Vector3f(InDeltaX/1024.f, -InDeltaY/768.f, 0)); //TODO store window sizes
	
	m_ModelMatrix = tr.matrix() * m_ModelMatrix;
}

void Part2::LoadContent()
{
	ComPtr<ID3D12CommandAllocator> loadContentCmdAllocator = D3D12GEPUtils::CreateCommandAllocator(m_GraphicsDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
	ComPtr<ID3D12GraphicsCommandList2> loadContentCmdList = D3D12GEPUtils::CreateCommandList(m_GraphicsDevice, loadContentCmdAllocator, D3D12_COMMAND_LIST_TYPE_DIRECT, false);
	// Upload vertex buffer data
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	D3D12GEPUtils::UpdateBufferResource(m_GraphicsDevice, loadContentCmdList, m_VertexBuffer.GetAddressOf(), intermediateVertexBuffer.GetAddressOf(),
		_countof(m_VertexData), sizeof(VertexPosColor), m_VertexData);
	// Create the Vertex Buffer View associated to m_VertexBuffer
	m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = sizeof(m_VertexData); // Size in bytes of the whole buffer
	m_VertexBufferView.StrideInBytes = sizeof(VertexPosColor); // Stride Size = Size of a single element

	// Upload index buffer data
	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	D3D12GEPUtils::UpdateBufferResource(m_GraphicsDevice, loadContentCmdList, m_IndexBuffer.GetAddressOf(), intermediateIndexBuffer.GetAddressOf(),
		_countof(m_IndexData), sizeof(WORD), m_IndexData);
	// Create the Index Buffer View associated to m_IndexBuffer
	m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
	m_IndexBufferView.SizeInBytes = sizeof(m_IndexData);
	m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT; // Single channel 16 bits, because WORD = unsigned short = 2 bytes = 16 bits

	// Create Descriptor Heap for the DepthStencil View
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {}; // Note: DepthStencil View requires storage in a heap even if we are going to use only 1 view
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	D3D12GEPUtils::ThrowIfFailed(m_GraphicsDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap)));

	// --- Shader Loading ---
	// Note: to generate the .cso file I will be using the offline method, using fxc.exe integrated in visual studio (but downloadable separately).
	// fxc command can be used by opening a developer command console in the hlsl shader folder.
	// To generate VertexShader.cso I will be using: fxc /Zi /T vs_5_1 /Fo VertexShader.cso VertexShader.hlsl
	// To generate PixelShader.cso I will be using: fxc /Zi /T ps_5_1 /Fo PixelShader.cso PixelShader.hlsl
	// Load the Vertex Shader
	ComPtr<ID3DBlob> vertexShaderBlob; 
	D3D12GEPUtils::ReadFileToBlob(PART2_SHADERS_PATH(VertexShader.cso), &vertexShaderBlob);
	// Load the Pixel Shader
	ComPtr<ID3DBlob> pixelShaderBlob;
	D3D12GEPUtils::ReadFileToBlob(PART2_SHADERS_PATH(PixelShader.cso), &pixelShaderBlob);
	
	// Create the Vertex Input Layout
	//LPCSTR SemanticName; UINT SemanticIndex; DXGI_FORMAT Format; UINT InputSlot;
	//UINT AlignedByteOffset; D3D12_INPUT_CLASSIFICATION InputSlotClass; UINT InstanceDataStepRate;
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	//Create Root Signature
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(m_GraphicsDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(D3D12_FEATURE_DATA_ROOT_SIGNATURE))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}
	// Allow Input layout access to shader resources (in out case, the MVP matrix) 
	// and deny it to other stages (small optimization)
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
	// Using a single 32-bit constant root parameter (MVP matrix) that is used by the vertex shader
	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstants(sizeof(Eigen::Matrix4f) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	// Init Root Signature Desc
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0U, nullptr, rootSignatureFlags);
	// Create Root Signature serialized blob and then the object from it
	m_RootSignature = D3D12GEPUtils::SerializeAndCreateRootSignature(m_GraphicsDevice, &rootSignatureDesc, featureData.HighestVersion);

	//RTV Formats
	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // Note: texel data of the render target is 4x 8bit channels of range [0,1]
	// Pipeline State Stream definition and fill
	struct PipelineStateStreamType {
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_VS VertexShader;
			CD3DX12_PIPELINE_STATE_STREAM_PS PixelShader;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineStateStream;
	pipelineStateStream.pRootSignature = m_RootSignature.Get();
	pipelineStateStream.PrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
	pipelineStateStream.VertexShader = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	pipelineStateStream.PixelShader = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.RTVFormats = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(pipelineStateStream), &pipelineStateStream
	};
	D3D12GEPUtils::ThrowIfFailed(m_GraphicsDevice->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));
	
	// Executing command list
	loadContentCmdList->Close();
	ID3D12CommandList* const ppCommandLists[] = { loadContentCmdList.Get() };
	m_CmdQueue->ExecuteCommandLists(1, ppCommandLists);
	uint64_t loadContentFenceValue;
	D3D12GEPUtils::SignalCmdQueue(m_CmdQueue, m_Fence, loadContentFenceValue);
	D3D12GEPUtils::WaitForFenceValue(m_Fence, loadContentFenceValue, m_FenceEvent);

	// Allocate DepthStencil resource in GPU
	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	optimizedClearValue.DepthStencil = { 1.f, 0 };
	D3D12GEPUtils::CreateDepthStencilCommittedResource(m_GraphicsDevice, m_DSBuffer.GetAddressOf(), 1024, 768,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearValue);
	// Update DepthStencil View
	D3D12GEPUtils::CreateDepthStencilView(m_GraphicsDevice, m_DSBuffer.Get(), m_DSVHeap->GetCPUDescriptorHandleForHeapStart());


	// Initialize the Model Matrix
	m_ModelMatrix = Eigen::Matrix4f::Identity();
	// Initialize the View Matrix
	const Eigen::Vector4f eyePosition = Eigen::Vector4f(0, 0, -10, 1);
	const Eigen::Vector4f focusPoint = Eigen::Vector4f(0, 0, 0, 1);
	const Eigen::Vector4f upDirection = Eigen::Vector4f(0, 1, 0, 0);
	m_ViewMatrix = GEPUtils::Geometry::LookAt(eyePosition, focusPoint, upDirection);
	// Initialize the Projection Matrix
	m_ProjMatrix = GEPUtils::Geometry::Perspective(m_ZMin, m_ZMax, m_AspectRatio, m_FoV); //TODO make this dynamic
}

void Part2::Run()
{
	if (!m_IsInitialized)
		return;

	m_MainWindow.ShowWindow();

	// Application's main loop is based on received window messages, specifically WM_PAINT will trigger Update() and Render()
	MSG windowMessage = {};
	while (windowMessage.message != WM_QUIT)
	{
		if (::PeekMessage(&windowMessage, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&windowMessage);
			::DispatchMessage(&windowMessage);
		}

		OnMainWindowPaint();
	}
}

void Part2::OnMainWindowPaint()
{
	if (!m_PaintStarted)
		return;
	// Window paint event will trigger game thread update and render methods (as we are in a simple single threaded example)
	Update();

	Render();
}

void Part2::Update()
{
	static uint64_t frameCounter = 0;
	static double elapsedSeconds = 0;
	static std::chrono::high_resolution_clock clock;
	auto t0 = clock.now();


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

void Part2::Render()
{
	auto backBuffer = m_MainWindow.GetCurrentBackbuffer();

	ID3D12GraphicsCommandList* cmdList = m_MainWindow.ResetCmdListWithCurrentAllocator().Get();

	// Clear render target and depth stencil
	{
		// Transitioning current backbuffer resource to render target state
		CD3DX12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			// We can be sure that the previous state was present because in this application all the render targets
			// are first filled and then presented to the main window repetitevely.
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		cmdList->ResourceBarrier(1, &transitionBarrier);

		FLOAT clearColor[] = { .4f, .6f, .9f, 1.f };
		cmdList->ClearRenderTargetView(m_MainWindow.GetCurrentRTVDescHandle(), clearColor, 0, nullptr);
		// Note: Clearing Render Target and Depth Stencil is a good practice, but in this case is also essential.
		// Without clearing the DepthStencilView, the rasterizer would not be able to use it!!
		cmdList->ClearDepthStencilView(m_DSVHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}

	// Fill Command List Pipeline-related Data
	{
		cmdList->SetPipelineState(m_PipelineState.Get());
		cmdList->SetGraphicsRootSignature(m_RootSignature.Get());
		
		cmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
		cmdList->IASetIndexBuffer(&m_IndexBufferView);

		cmdList->RSSetViewports(1, &m_Viewport);
		cmdList->RSSetScissorRects(1, &m_ScissorRect);

		cmdList->OMSetRenderTargets(1, &m_MainWindow.GetCurrentRTVDescHandle(), FALSE, &m_DSVHeap->GetCPUDescriptorHandleForHeapStart());
		
	}

	// Fill Command List Buffer Data and Draw Command
	{
		Eigen::Matrix4f mvpMatrix = m_ProjMatrix * m_ViewMatrix * m_ModelMatrix;
		
		cmdList->SetGraphicsRoot32BitConstants(0, sizeof(Eigen::Matrix4f) / 4, mvpMatrix.data(), 0);

		cmdList->DrawIndexedInstanced(_countof(m_IndexData), 1, 0, 0, 0);
	}

	// Execute command list and present current render target from the main window
	{
		CD3DX12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		cmdList->ResourceBarrier(1, &transitionBarrier);

		// Mandatory for the command list to close before getting executed by the command queue
		D3D12GEPUtils::ThrowIfFailed(cmdList->Close());

		ID3D12CommandList* const cmdLists[] = { cmdList };
		m_CmdQueue->ExecuteCommandLists(1, cmdLists);

		m_MainWindow.Present();
	}
}

void Part2::QuitApplication()
{
	// TODO make sure all the instantiated objects are cleaned up (see output log when closing application if there were some forgotten live objects)
	m_MainWindow.Close();
	uint64_t fenceValue;
	D3D12GEPUtils::FlushCmdQueue(m_CmdQueue, m_Fence, m_FenceEvent, fenceValue);
	::CloseHandle(m_FenceEvent);

	::PostQuitMessage(0); // Causes the application to terminate
}

void Part2::OnMouseWheel(MouseWheelEventArgs& e)
{
	m_FoV -= e.WheelDelta / 1200.f;
	m_FoV = std::max(0.2094395102f, std::min(m_FoV, 1.570796327f)); // clamping
	char buffer[256];
	::sprintf_s(buffer, "FoV: %f\n", m_FoV);
	::OutputDebugStringA(buffer);

	m_ProjMatrix = GEPUtils::Geometry::Perspective( m_ZMin, m_ZMax, m_AspectRatio, m_FoV); // TODO make this dynamic
}

void Part2::OnMousePressed(MouseButtonEventArgs& InEvent)
{
	switch (InEvent.Button)
	{
	case 0:
		std::cout << "Pressed Left Mouse Button" << std::endl;
		if(InEvent.DragDetected)
			m_MouseLeftDrag = true;
		break;
	case 2:
		std::cout << "Pressed Right Mouse Button" << std::endl;
		m_MouseRightDrag = true;
	}
}

void Part2::OnMouseReleased(MouseButtonEventArgs& InEvent)
{
	switch (InEvent.Button)
	{
	case 0:
		std::cout << "Released Left Mouse Button" << std::endl;
		m_MouseLeftDrag = false;
		break;
	case 2:
		std::cout << "Released Right Mouse Button" << std::endl;
		m_MouseRightDrag = false;
		break;
	}
}

LRESULT CALLBACK Part2::MainWndProc_Internal(HWND InHWND, UINT InMsg, WPARAM InWParam, LPARAM InLParam)
{
	// Preventing application to process events when all the necessery D3D12 objects are not initialized yet
	// Note: Do Not Return 0, otherwise HWND window creation will fail with no message!!
	if (!m_IsInitialized)
		return ::DefWindowProcW(InHWND, InMsg, InWParam, InLParam);

	bool altKeyDown = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
	switch (InMsg)
	{
		case WM_PAINT:
			m_PaintStarted = true;

			break;
	case WM_SYSKEYDOWN: // When a system key is pressed (e.g. Alt key)
	case  WM_KEYDOWN: // When a non-system key is pressed (e.g. v key)
	{
		switch (InWParam)
		{
		case 'V':
			m_MainWindow.SetVSync(!m_MainWindow.IsVSyncEnabled());
			break;
		case VK_ESCAPE:
			QuitApplication();
			break;
		case VK_RETURN:
			if (altKeyDown)
			{
			case VK_F11:
				m_MainWindow.SetFullscreenState(!m_MainWindow.IsFullScreen());
			}
			break;
		default:
			break;
		}
	}
	case WM_SYSCHAR:
		break; // Preventing Alt+Enter hotkey to try switching the window to fullscreen since we are manually handling it with Alt+F11
	case WM_MOUSEMOVE:
	{
		POINT newCoords{ GET_X_LPARAM(InLParam), GET_Y_LPARAM(InLParam) };
		if(m_MouseLeftDrag)
			OnLeftMouseDrag(newCoords.x - m_PrevMouseCoords.x, newCoords.y - m_PrevMouseCoords.y);
		if(m_MouseRightDrag)
			OnRightMouseDrag(newCoords.x - m_PrevMouseCoords.x, newCoords.y - m_PrevMouseCoords.y);

		m_PrevMouseCoords.x = newCoords.x;
		m_PrevMouseCoords.y = newCoords.y;
		break;
	}
	case WM_MOUSEWHEEL:
		OnMouseWheel(MouseWheelEventArgs{  static_cast<float>(GET_WHEEL_DELTA_WPARAM(InWParam))  });
		break;
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		POINT dragPoint = { GET_X_LPARAM(InLParam), GET_Y_LPARAM(InLParam) };	
		m_PrevMouseCoords.x = dragPoint.x;
		m_PrevMouseCoords.y = dragPoint.y;

		uint8_t button = InMsg == WM_LBUTTONDOWN ? 0 : InMsg == WM_MBUTTONDOWN ? 1 : InMsg == WM_RBUTTONDOWN ? 2 : 3;
		OnMousePressed(MouseButtonEventArgs{ static_cast<int32_t>(dragPoint.x), static_cast<int32_t>(dragPoint.y), button, static_cast<bool>(::DragDetect(InHWND, dragPoint)) });
		break;
	}
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	{
		uint8_t button = (InMsg == WM_LBUTTONUP) ? 0 : (InMsg == WM_MBUTTONUP) ? 1 : (InMsg == WM_RBUTTONUP) ? 2 : 3;
		OnMouseReleased(MouseButtonEventArgs{ GET_X_LPARAM(InLParam), GET_Y_LPARAM(InLParam), button });
		break;
	}	
	case WM_SIZE:
	{
		RECT clientRect = {};
		::GetClientRect(InHWND, &clientRect);
		int32_t newWidth = clientRect.right - clientRect.left;
		int32_t newHeight = clientRect.bottom - clientRect.top;
		m_MainWindow.Resize(newWidth, newHeight);
		break;
	}
	case WM_DESTROY: // Called when X is pressed at the top right
		QuitApplication();
		break;
	}

	return ::DefWindowProcW(InHWND, InMsg, InWParam, InLParam); // Message will be handled by the OS' System Procedure
}
