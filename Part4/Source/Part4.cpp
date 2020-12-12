
#include "Part4.h"
#include <iostream>
#include <algorithm>
#include "GraphicsUtils.h"
#include "PipelineState.h"
#include "GEPUtilsGeometry.h"
#include "GraphicsAllocator.h"

#define Q(x) L#x
#define LQUOTE(x) Q(x) // TODO these are re-defined .. find a way to link the original defines
#define Part4_SHADERS_PATH(NAME) LQUOTE(PART4_PROJ_ROOT_PATH/shaders/NAME)
#define Part4_CONTENT_PATH(NAME) LQUOTE(PART4_PROJ_ROOT_PATH/Content/NAME)


using namespace GEPUtils;

int main()
{
	std::cout << "Hello from Part 4: Texture Usage!" << std::endl;

	Part4Application::Get()->Initialize();

	Part4Application::Get()->Run();
}

GEPUtils::Application* Part4Application::Get()
{
	if (m_Instance == nullptr)
		m_Instance = new Part4Application();
	return m_Instance;
}

void Part4Application::Initialize()
{
	Application::Initialize();

	// Load Content
	Graphics::CommandList& loadContentCmdList = m_CmdQueue->GetAvailableCommandList();

	// --- Vertex Buffer ---
	size_t vertexDataSize = sizeof(VertexPosColor) * _countof(m_VertexData);

	m_VertexBuffer = &Graphics::GraphicsAllocator::Get()->AllocateBuffer(vertexDataSize, Graphics::RESOURCE_HEAP_TYPE::DEFAULT, Graphics::RESOURCE_STATE::COPY_DEST); // Note: this is this supposed to be created as COPY_DEST and later changing state to read
	// Note: we are allocating intermediate buffer that will not be used anymore later on but will stay in memory (leak)
	Graphics::Buffer& intermediateVertexBuffer = Graphics::GraphicsAllocator::Get()->AllocateBuffer(vertexDataSize, Graphics::RESOURCE_HEAP_TYPE::UPLOAD, Graphics::RESOURCE_STATE::GEN_READ); 
	
	loadContentCmdList.UploadBufferData(*m_VertexBuffer, intermediateVertexBuffer, m_VertexData, vertexDataSize);

	// Create the Vertex Buffer View associated to m_VertexBuffer
	m_VertexBufferView = &Graphics::GraphicsAllocator::Get()->AllocateVertexBufferView();
	m_VertexBufferView->ReferenceResource(*m_VertexBuffer, sizeof(m_VertexData), sizeof(VertexPosColor));

	// --- Index Buffer ---
	size_t indexDataSize = sizeof(unsigned short) * _countof(m_IndexData);

	m_IndexBuffer = &Graphics::GraphicsAllocator::Get()->AllocateBuffer(indexDataSize, Graphics::RESOURCE_HEAP_TYPE::DEFAULT, Graphics::RESOURCE_STATE::COPY_DEST);
	// Note: we are allocating intermediate buffer that will not be used anymore later on but will stay in memory (leak)
	Graphics::Buffer& intermediateIndexBuffer = Graphics::GraphicsAllocator::Get()->AllocateBuffer(indexDataSize, Graphics::RESOURCE_HEAP_TYPE::UPLOAD, Graphics::RESOURCE_STATE::GEN_READ);

	loadContentCmdList.UploadBufferData(*m_IndexBuffer, intermediateIndexBuffer, m_IndexData, indexDataSize);

	// Create the Index Buffer View associated to m_IndexBuffer
	m_IndexBufferView = &Graphics::GraphicsAllocator::Get()->AllocateIndexBufferView();
	m_IndexBufferView->ReferenceResource(*m_IndexBuffer, sizeof(m_IndexData), Graphics::BUFFER_FORMAT::R16_UINT); // Single channel 16 bits, because WORD = unsigned short = 2 bytes = 16 bits

	// --- Shader Loading ---
	// Note: to generate the .cso file I will be using the offline method, using fxc.exe integrated in visual studio (but downloadable separately).
	// fxc command can be used by opening a developer command console in the hlsl shader folder.
	// To generate VertexShader.cso I will be using: fxc /Zi /T vs_5_1 /Fo VertexShader.cso VertexShader.hlsl
	// To generate PixelShader.cso I will be using: fxc /Zi /T ps_5_1 /Fo PixelShader.cso PixelShader.hlsl

	// Load the Vertex Shader
	Graphics::Shader& vertexShader = GEPUtils::Graphics::AllocateShader(Part4_SHADERS_PATH(VertexShader.cso));
	// Load the Pixel Shader
	Graphics::Shader& pixelShader = GEPUtils::Graphics::AllocateShader(Part4_SHADERS_PATH(PixelShader.cso));

	// --- Pipeline State ---
	// Create the Vertex Input Layout
	Graphics::PipelineState::INPUT_LAYOUT_DESC inputLayout { {
		{"POSITION",GEPUtils::Graphics::BUFFER_FORMAT::R32G32B32_FLOAT},
		{"TEX_COORDS",GEPUtils::Graphics::BUFFER_FORMAT::R32G32B32_FLOAT}
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

	// Cubemap loading
	// The file was made using the handy ATI CubeMapGen (discontinued) available at https://gpuopen.com/archived/cubemapgen/

	m_Cubemap = &Graphics::GraphicsAllocator::Get()->AllocateTextureFromFile(Part4_CONTENT_PATH(CubeMap.dds), GEPUtils::Graphics::TEXTURE_FILE_FORMAT::DDS);
	// Uploading cubemap data in GPU
	// Note: we are allocating intermediate buffer that will not be used anymore later on but will stay in memory (leak)
	Graphics::Buffer& intermediateCubemapBuffer = Graphics::GraphicsAllocator::Get()->AllocateBuffer(m_Cubemap->GetGPUSize(), Graphics::RESOURCE_HEAP_TYPE::UPLOAD, Graphics::RESOURCE_STATE::GEN_READ);

	m_Cubemap->UploadToGPU(loadContentCmdList, intermediateCubemapBuffer);

	// SRV referencing the cubemap
	m_CubemapView = &Graphics::GraphicsAllocator::Get()->AllocateShaderResourceView(*m_Cubemap);

	loadContentCmdList.UploadViewToGPU(*m_CubemapView);
	
	// Root parameter for the cubemap
	Graphics::PipelineState::RESOURCE_BINDER_PARAM cubemapParam;
	cubemapParam.InitAsTableSRVRange(1, 0, 1, Graphics::SHADER_VISIBILITY::SV_PIXEL);

	resourceBinderDesc.Params.emplace_back(std::move(cubemapParam));

	// Sampler for the cubemap
	resourceBinderDesc.StaticSamplers.emplace_back(0, Graphics::SAMPLE_FILTER_TYPE::LINEAR);

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

	m_PipelineState = &Graphics::AllocatePipelineState();

	//Init the Pipeline State Object
	m_PipelineState->Init(pipelineStateDesc);

	// Executing command list and waiting for full execution
	m_CmdQueue->ExecuteCmdList(loadContentCmdList);

	m_CmdQueue->Flush(); // Note: Flushing operations on the command queue here will ensure that all the operations made on resources by the loadContentCmdList finished executing!

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
	m_MainWindow->OnMouseMoveDelegate.Add<Part4Application, &Part4Application::OnMouseMove>(this);
	m_MainWindow->OnMouseWheelDelegate.Add<Part4Application, &Part4Application::OnMouseWheel>(this);
}

void Part4Application::OnMouseWheel(float InDeltaRot)
{
	m_Fov -= InDeltaRot / 1200.f;
	SetFov(std::max(0.2094395102f, std::min(m_Fov, 1.570796327f))); // clamping
}

void Part4Application::OnMouseMove(int32_t InX, int32_t InY)
{
	static int32_t prevX = 0, prevY = 0;
	if (m_MainWindow->IsMouseRightHold())
		OnRightMouseDrag(InX - prevX, InY - prevY);
	if (m_MainWindow->IsMouseLeftHold())
		OnLeftMouseDrag(InX - prevX, InY - prevY);

	prevX = InX; prevY = InY;
}

void Part4Application::OnLeftMouseDrag(int32_t InDeltaX, int32_t InDeltaY)
{
	Eigen::Transform<float, 3, Eigen::Affine> tr;
	tr.setIdentity();
	tr.rotate(Eigen::AngleAxisf(-InDeltaX / static_cast<float>(m_MainWindow->GetFrameWidth()), Eigen::Vector3f::UnitY()))
		.rotate(Eigen::AngleAxisf(-InDeltaY / static_cast<float>(m_MainWindow->GetFrameHeight()), Eigen::Vector3f::UnitX()));

	m_ModelMatrix = tr.matrix() * m_ModelMatrix;
}

void Part4Application::OnRightMouseDrag(int32_t InDeltaX, int32_t InDeltaY)
{
	Eigen::Transform<float, 3, Eigen::Affine> tr;
	tr.setIdentity();
	tr.translate(Eigen::Vector3f(InDeltaX / static_cast<float>(m_MainWindow->GetFrameWidth()), -InDeltaY / static_cast<float>(m_MainWindow->GetFrameHeight()), 0));

	m_ModelMatrix = tr.matrix() * m_ModelMatrix;
}

void Part4Application::UpdateContent(float InDeltaTime)
{

	// Updating MVP matrix
	m_MvpMatrix = m_ProjMatrix * m_ViewMatrix * m_ModelMatrix;

}

void Part4Application::RenderContent(Graphics::CommandList& InCmdList)
{
	// Fill Command List Pipeline-related Data
	{
		InCmdList.SetPipelineStateAndResourceBinder(*m_PipelineState);

		InCmdList.SetInputAssemblerData(Graphics::PRIMITIVE_TOPOLOGY::PT_TRIANGLELIST, *m_VertexBufferView, *m_IndexBufferView);

		InCmdList.SetViewportAndScissorRect(*m_Viewport, *m_ScissorRect);

		InCmdList.SetRenderTargetFromWindow(*m_MainWindow);
	}

	// Fill Command List Buffer Data and Draw Command
	{
		InCmdList.SetGraphicsRootConstants(0, sizeof(Eigen::Matrix4f) / 4, m_MvpMatrix.data(), 0);

		InCmdList.ReferenceSRV(1, *m_CubemapView); // Note: the SRV is already uploaded to GPU and at render time it just need to be referenced in the pipeline at the given root index

		InCmdList.DrawIndexed(_countof(m_IndexData));
	}

}
