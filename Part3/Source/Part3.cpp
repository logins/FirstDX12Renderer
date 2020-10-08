
#include "Part3.h"
#include <iostream>
#include "GraphicsUtils.h"
#include "../../lib/3DGEP/Source/Graphics/Public/PipelineState.h"
#include "../../lib/3DGEP/Source/Public/GEPUtilsGeometry.h"

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
	: m_VertexBuffer(Graphics::AllocateEmptyResource()), m_IndexBuffer(Graphics::AllocateEmptyResource()),
		m_VertexBufferView(Graphics::AllocateVertexBufferView()), 
		m_IndexBufferView(Graphics::AllocateIndexBufferView()),
		m_PipelineState(Graphics::AllocatePipelineState(*m_GraphicsDevice))
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

	auto& loadContentCmdList = m_CmdQueue->GetAvailableCommandList();  // TODO continue translating here
	// Upload vertex buffer data
	//ComPtr<ID3D12Resource> intermediateVertexBuffer;
	//D3D12GEPUtils::UpdateBufferResource(m_GraphicsDevice, loadContentCmdList, m_VertexBuffer.GetAddressOf(), intermediateVertexBuffer.GetAddressOf(),
	//	_countof(m_VertexData), sizeof(VertexPosColor), m_VertexData);
	Graphics::Resource& intermediateVertexBuffer = Graphics::AllocateEmptyResource();
	loadContentCmdList.UpdateBufferResource(m_VertexBuffer, intermediateVertexBuffer, _countof(m_VertexData), sizeof(VertexPosColor), m_VertexData);

	// Create the Vertex Buffer View associated to m_VertexBuffer
	//m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress(); 
	//m_VertexBufferView.SizeInBytes = sizeof(m_VertexData); // Size in bytes of the whole buffer
	//m_VertexBufferView.StrideInBytes = sizeof(VertexPosColor); // Stride Size = Size of a single element
	m_VertexBufferView.ReferenceResource(m_VertexBuffer, sizeof(m_VertexData), sizeof(VertexPosColor));

	// Upload index buffer data
	//ComPtr<ID3D12Resource> intermediateIndexBuffer;
	Graphics::Resource& intermediateIndexBuffer = Graphics::AllocateEmptyResource();
	//D3D12GEPUtils::UpdateBufferResource(m_GraphicsDevice, loadContentCmdList, m_IndexBuffer.GetAddressOf(), intermediateIndexBuffer.GetAddressOf(),
	//	_countof(m_IndexData), sizeof(WORD), m_IndexData);
	loadContentCmdList.UpdateBufferResource(m_IndexBuffer, intermediateIndexBuffer, _countof(m_IndexData), sizeof(unsigned short), m_IndexData);

	// Create the Index Buffer View associated to m_IndexBuffer
	//m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
	//m_IndexBufferView.SizeInBytes = sizeof(m_IndexData);
	//m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT; // Single channel 16 bits, because WORD = unsigned short = 2 bytes = 16 bits
	m_IndexBufferView.ReferenceResource(m_IndexBuffer, sizeof(m_IndexData), Graphics::BUFFER_FORMAT::R16_UINT);

	// --- Shader Loading ---
	// Note: to generate the .cso file I will be using the offline method, using fxc.exe integrated in visual studio (but downloadable separately).
	// fxc command can be used by opening a developer command console in the hlsl shader folder.
	// To generate VertexShader.cso I will be using: fxc /Zi /T vs_5_1 /Fo VertexShader.cso VertexShader.hlsl
	// To generate PixelShader.cso I will be using: fxc /Zi /T ps_5_1 /Fo PixelShader.cso PixelShader.hlsl
	// Load the Vertex Shader
	//ComPtr<ID3DBlob> vertexShaderBlob;
	//D3D12GEPUtils::ReadFileToBlob(PART2_SHADERS_PATH(VertexShader.cso), &vertexShaderBlob);
	Graphics::Shader& vertexShader = GEPUtils::Graphics::AllocateShader(PART3_SHADERS_PATH(VertexShader.cso));
	// Load the Pixel Shader
	//ComPtr<ID3DBlob> pixelShaderBlob;
	//D3D12GEPUtils::ReadFileToBlob(PART2_SHADERS_PATH(PixelShader.cso), &pixelShaderBlob);
	Graphics::Shader& pixelShader = GEPUtils::Graphics::AllocateShader(PART3_SHADERS_PATH(PixelShader.cso));

	// Create the Vertex Input Layout
	//LPCSTR SemanticName; UINT SemanticIndex; DXGI_FORMAT Format; UINT InputSlot;
	//UINT AlignedByteOffset; D3D12_INPUT_CLASSIFICATION InputSlotClass; UINT InstanceDataStepRate;
	/*D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};*/
	Graphics::PipelineState::INPUT_LAYOUT_DESC inputLayout { {
		{"POSITION",GEPUtils::Graphics::BUFFER_FORMAT::R32G32B32_FLOAT},
		{"COLOR",GEPUtils::Graphics::BUFFER_FORMAT::R32G32B32_FLOAT}
	}
	};

	//Create Root Signature
	/*D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(m_GraphicsDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(D3D12_FEATURE_DATA_ROOT_SIGNATURE))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}*/
	// Allow Input layout access to shader resources (in out case, the MVP matrix) 
	// and deny it to other stages (small optimization)
	/*D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;*/

	Graphics::PipelineState::RESOURCE_BINDER_DESC resourceBinderDesc;
	resourceBinderDesc.Flags = 
		Graphics::PipelineState::RESOURCE_BINDER_FLAGS::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		Graphics::PipelineState::RESOURCE_BINDER_FLAGS::DENY_HULL_SHADER_ACCESS |
		Graphics::PipelineState::RESOURCE_BINDER_FLAGS::DENY_DOMAIN_SHADER_ACCESS |
		Graphics::PipelineState::RESOURCE_BINDER_FLAGS::DENY_GEOMETRY_SHADER_ACCESS |
		Graphics::PipelineState::RESOURCE_BINDER_FLAGS::DENY_PIXEL_SHADER_ACCESS;

	// Using a single 32-bit constant root parameter (MVP matrix) that is used by the vertex shader
	//CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	//rootParameters[0].InitAsConstants(sizeof(Eigen::Matrix4f) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	
	Graphics::PipelineState::RESOURCE_BINDER_PARAM mvpMatrix;
	mvpMatrix.InitAsConstants(sizeof(Eigen::Matrix4f) / 4, 0, 0, Graphics::SHADER_VISIBILITY::SV_VERTEX);

	resourceBinderDesc.Params.emplace(resourceBinderDesc.Params.end(),std::move(mvpMatrix));

	// Init Root Signature Desc
	//CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	//rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0U, nullptr, rootSignatureFlags);
	//// Create Root Signature serialized blob and then the object from it
	//m_RootSignature = D3D12GEPUtils::SerializeAndCreateRootSignature(m_GraphicsDevice, &rootSignatureDesc, featureData.HighestVersion);

	////RTV Formats
	//D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	//rtvFormats.NumRenderTargets = 1;
	//rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // Note: texel data of the render target is 4x 8bit channels of range [0,1]
	//// Pipeline State Stream definition and fill
	//struct PipelineStateStreamType {
	//	CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
	//	CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopology;
	//	CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
	//	CD3DX12_PIPELINE_STATE_STREAM_VS VertexShader;
	//	CD3DX12_PIPELINE_STATE_STREAM_PS PixelShader;
	//	CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
	//	CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	//} pipelineStateStream;
	//pipelineStateStream.pRootSignature = m_RootSignature.Get();
	//pipelineStateStream.PrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
	//pipelineStateStream.VertexShader = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	//pipelineStateStream.PixelShader = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	//pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	//pipelineStateStream.RTVFormats = rtvFormats;

	//D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
	//	sizeof(pipelineStateStream), &pipelineStateStream
	//};
	//D3D12GEPUtils::ThrowIfFailed(m_GraphicsDevice->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));

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
}

void Part3Application::RenderContent(Graphics::CommandList& InCmdList)
{
	// Fill Command List Pipeline-related Data
	{
		InCmdList.SetPipelineStateAndResourceBinder(m_PipelineState);
		//InCmdList.SetGraphicsRootSignature(m_RootSignature.Get());

		/*InCmdList.IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		InCmdList.IASetVertexBuffers(0, 1, &m_VertexBufferView);
		InCmdList.IASetIndexBuffer(&m_IndexBufferView);*/
		InCmdList.SetInputAssemblerData(Graphics::PRIMITIVE_TOPOLOGY::PT_TRIANGLELIST, m_VertexBufferView, m_IndexBufferView);

		/*InCmdList.RSSetViewports(1, &m_Viewport);
		InCmdList.RSSetScissorRects(1, &m_ScissorRect);*/
		InCmdList.SetViewportAndScissorRect(*m_Viewport, *m_ScissorRect);

		//InCmdList.OMSetRenderTargets(1, &m_MainWindow->GetCurrentRTVDescHandle(), FALSE, &m_MainWindow->GetCuttentDSVDescHandle());
		InCmdList.SetRenderTargetFromWindow(*m_MainWindow);
	}

	// Fill Command List Buffer Data and Draw Command
	{
		Eigen::Matrix4f mvpMatrix = m_ProjMatrix * m_ViewMatrix * m_ModelMatrix;

		//InCmdList->SetGraphicsRoot32BitConstants(0, sizeof(Eigen::Matrix4f) / 4, mvpMatrix.data(), 0);
		InCmdList.SetGraphicsRootConstants(0, sizeof(Eigen::Matrix4f) / 4, mvpMatrix.data(), 0);

		//InCmdList->DrawIndexedInstanced(_countof(m_IndexData), 1, 0, 0, 0);
		InCmdList.DrawIndexed(_countof(m_IndexData));
	}
}
