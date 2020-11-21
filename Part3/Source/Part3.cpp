
#include "Part3.h"
#include <iostream>
#include <algorithm>
#include "GraphicsUtils.h"
#include "PipelineState.h"
#include "GEPUtilsGeometry.h"
#include "GraphicsAllocator.h"

#define Q(x) L#x
#define LQUOTE(x) Q(x) // TODO these are re-defined .. find a way to link the original defines
#define PART3_SHADERS_PATH(NAME) LQUOTE(PART3_PROJ_ROOT_PATH/shaders/NAME)


using namespace GEPUtils;

int main()
{
	std::cout << "Hello from Part 3!" << std::endl;

	Part3Application::Get()->Initialize();

	Part3Application::Get()->Run();
}

Part3Application::Part3Application()
	: m_VertexBuffer(Graphics::AllocateEmptyResource()), m_IndexBuffer(Graphics::AllocateEmptyResource()), m_ColorModBuffer(Graphics::AllocateDynamicBuffer()),
		m_VertexBufferView(Graphics::AllocateVertexBufferView()), 
		m_IndexBufferView(Graphics::AllocateIndexBufferView()),
		m_ColorModBufferView(Graphics::AllocateResourceView(m_ColorModBuffer, GEPUtils::Graphics::RESOURCE_VIEW_TYPE::CONSTANT_BUFFER)), // TODO possibly better changing this to Allocate ConstantBufferView and create the relative platform agnostic type...
		m_PipelineState(Graphics::AllocatePipelineState(m_GraphicsDevice))
{
}

GEPUtils::Application* Part3Application::Get()
{
	if (m_Instance == nullptr)
		m_Instance = new Part3Application();
	return m_Instance;
}

void Part3Application::Initialize()
{
	Application::Initialize();

	// Load Content
	Graphics::CommandList& loadContentCmdList = m_CmdQueue->GetAvailableCommandList();

	// Upload vertex buffer data
	Graphics::Resource& intermediateVertexBuffer = Graphics::AllocateEmptyResource(); // Note: we are allocating intermediate buffer that will not be used anymore later on but will stay in memory (leak)
	Graphics::GraphicsAllocator::AllocateBufferCommittedResource(loadContentCmdList, m_VertexBuffer, intermediateVertexBuffer, _countof(m_VertexData), sizeof(VertexPosColor), m_VertexData);

	// Create the Vertex Buffer View associated to m_VertexBuffer
	m_VertexBufferView.ReferenceResource(m_VertexBuffer, sizeof(m_VertexData), sizeof(VertexPosColor));

	// Upload index buffer data
	Graphics::Resource& intermediateIndexBuffer = Graphics::AllocateEmptyResource(); // Note: we are allocating intermediate buffer that will not be used anymore later on but will stay in memory (leak)

	Graphics::GraphicsAllocator::AllocateBufferCommittedResource(loadContentCmdList, m_IndexBuffer, intermediateIndexBuffer, _countof(m_IndexData), sizeof(unsigned short), m_IndexData);

	// Create the Index Buffer View associated to m_IndexBuffer
	m_IndexBufferView.ReferenceResource(m_IndexBuffer, sizeof(m_IndexData), Graphics::BUFFER_FORMAT::R16_UINT); // Single channel 16 bits, because WORD = unsigned short = 2 bytes = 16 bits

	// --- Shader Loading ---
	// Note: to generate the .cso file I will be using the offline method, using fxc.exe integrated in visual studio (but downloadable separately).
	// fxc command can be used by opening a developer command console in the hlsl shader folder.
	// To generate VertexShader.cso I will be using: fxc /Zi /T vs_5_1 /Fo VertexShader.cso VertexShader.hlsl
	// To generate PixelShader.cso I will be using: fxc /Zi /T ps_5_1 /Fo PixelShader.cso PixelShader.hlsl

	// Load the Vertex Shader
	Graphics::Shader& vertexShader = GEPUtils::Graphics::AllocateShader(PART3_SHADERS_PATH(VertexShader.cso));
	// Load the Pixel Shader
	Graphics::Shader& pixelShader = GEPUtils::Graphics::AllocateShader(PART3_SHADERS_PATH(PixelShader.cso));

	// Create the Vertex Input Layout
	Graphics::PipelineState::INPUT_LAYOUT_DESC inputLayout { {
		{"POSITION",GEPUtils::Graphics::BUFFER_FORMAT::R32G32B32_FLOAT},
		{"COLOR",GEPUtils::Graphics::BUFFER_FORMAT::R32G32B32_FLOAT}
	}
	};

	//Create Root Signature
	// Allow Input layout access to shader resources (in out case, the MVP matrix) 
	// and deny it to other stages (small optimization)
	Graphics::PipelineState::RESOURCE_BINDER_DESC resourceBinderDesc;
	resourceBinderDesc.Flags = 
		Graphics::PipelineState::RESOURCE_BINDER_FLAGS::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		Graphics::PipelineState::RESOURCE_BINDER_FLAGS::DENY_HULL_SHADER_ACCESS |
		Graphics::PipelineState::RESOURCE_BINDER_FLAGS::DENY_DOMAIN_SHADER_ACCESS |
		Graphics::PipelineState::RESOURCE_BINDER_FLAGS::DENY_GEOMETRY_SHADER_ACCESS;

	// Using a single 32-bit constant root parameter (MVP matrix) that is used by the vertex shader	
	Graphics::PipelineState::RESOURCE_BINDER_PARAM mvpMatrix;
	mvpMatrix.InitAsConstants(sizeof(Eigen::Matrix4f) / 4, 0, 0, Graphics::SHADER_VISIBILITY::SV_VERTEX);

	resourceBinderDesc.Params.emplace(resourceBinderDesc.Params.end(),std::move(mvpMatrix));

	// Constant buffer ColorModifierCB
	Graphics::PipelineState::RESOURCE_BINDER_PARAM colorModifierParam;
	colorModifierParam.InitAsTableCBVRange(1, 0, 1, Graphics::SHADER_VISIBILITY::SV_PIXEL);

	resourceBinderDesc.Params.emplace(resourceBinderDesc.Params.end(), std::move(colorModifierParam));

	// Init Root Signature Desc
	// Create Root Signature serialized blob and then the object from it
	// RTV Formats
	// Pipeline State Stream definition and fill
	Graphics::PipelineState::PIPELINE_STATE_DESC pipelineStateDesc{
		inputLayout,
		resourceBinderDesc,
		Graphics::PRIMITIVE_TOPOLOGY_TYPE::PTT_TRIANGLE,
		vertexShader,
		pixelShader,
		Graphics::BUFFER_FORMAT::D32_FLOAT,
		Graphics::BUFFER_FORMAT::R8G8B8A8_UNORM
	};

	//Init the Pipeline State Object
	m_PipelineState.Init(pipelineStateDesc);

	// Executing command list and waiting for full execution
	m_CmdQueue->ExecuteCmdList(loadContentCmdList);

	m_CmdQueue->Flush();

	// Initialize the Model Matrix
	m_ModelMatrix = Eigen::Matrix4f::Identity();

	// Initialize the View Matrix
	const Eigen::Vector3f eyePosition = Eigen::Vector3f(0, 0, -10);
	const Eigen::Vector3f focusPoint = Eigen::Vector3f(0, 0, 0);
	const Eigen::Vector3f upDirection = Eigen::Vector3f(0, 1, 0);
	m_ViewMatrix = GEPUtils::Geometry::LookAt(eyePosition, focusPoint, upDirection);

	// Initialize the Projection Matrix
	m_ProjMatrix = GEPUtils::Geometry::Perspective(m_ZMin, m_ZMax, m_AspectRatio, m_Fov);

	// Window events delegates
	m_MainWindow->OnMouseMoveDelegate.Add<Part3Application, &Part3Application::OnMouseMove>(this);
	m_MainWindow->OnMouseWheelDelegate.Add<Part3Application, &Part3Application::OnMouseWheel>(this);
}

void Part3Application::OnMouseWheel(float InDeltaRot)
{
	m_Fov -= InDeltaRot / 1200.f;
	SetFov(std::max(0.2094395102f, std::min(m_Fov, 1.570796327f))); // clamping
}

void Part3Application::OnMouseMove(int32_t InX, int32_t InY)
{
	static int32_t prevX = 0, prevY = 0;
	if (m_MainWindow->IsMouseRightHold())
		OnRightMouseDrag(InX - prevX, InY - prevY);
	if (m_MainWindow->IsMouseLeftHold())
		OnLeftMouseDrag(InX - prevX, InY - prevY);

	prevX = InX; prevY = InY;
}

void Part3Application::OnLeftMouseDrag(int32_t InDeltaX, int32_t InDeltaY)
{
	Eigen::Transform<float, 3, Eigen::Affine> tr;
	tr.setIdentity();
	tr.rotate(Eigen::AngleAxisf(-InDeltaX / static_cast<float>(m_MainWindow->GetFrameWidth()), Eigen::Vector3f::UnitY()))
		.rotate(Eigen::AngleAxisf(-InDeltaY / static_cast<float>(m_MainWindow->GetFrameHeight()), Eigen::Vector3f::UnitX()));

	m_ModelMatrix = tr.matrix() * m_ModelMatrix;
}

void Part3Application::OnRightMouseDrag(int32_t InDeltaX, int32_t InDeltaY)
{
	Eigen::Transform<float, 3, Eigen::Affine> tr;
	tr.setIdentity();
	tr.translate(Eigen::Vector3f(InDeltaX / static_cast<float>(m_MainWindow->GetFrameWidth()), -InDeltaY / static_cast<float>(m_MainWindow->GetFrameHeight()), 0));

	m_ModelMatrix = tr.matrix() * m_ModelMatrix;
}

void Part3Application::UpdateContent(float InDeltaTime)
{
	// Updating MVP matrix
	m_MvpMatrix = m_ProjMatrix * m_ViewMatrix * m_ModelMatrix;

	// Updating color modifier
	float counter = 1.f;
	static float progress = 0.f;
	counter = 0.5f + std::sin(progress)/2.f;
	progress += 0.002f * InDeltaTime;

	m_ColorModBuffer.SetData(&counter, sizeof(counter), sizeof(float));
}

void Part3Application::RenderContent(Graphics::CommandList& InCmdList)
{
	// Fill Command List Pipeline-related Data
	{
		InCmdList.SetPipelineStateAndResourceBinder(m_PipelineState);

		InCmdList.SetInputAssemblerData(Graphics::PRIMITIVE_TOPOLOGY::PT_TRIANGLELIST, m_VertexBufferView, m_IndexBufferView);

		InCmdList.SetViewportAndScissorRect(*m_Viewport, *m_ScissorRect);

		InCmdList.SetRenderTargetFromWindow(*m_MainWindow);
	}

	// Fill Command List Buffer Data and Draw Command
	{
		InCmdList.SetGraphicsRootConstants(0, sizeof(Eigen::Matrix4f) / 4, m_MvpMatrix.data(), 0);

		InCmdList.StoreAndReferenceDynamicBuffer(1, m_ColorModBuffer, m_ColorModBufferView);

		InCmdList.DrawIndexed(_countof(m_IndexData));
	}

}
