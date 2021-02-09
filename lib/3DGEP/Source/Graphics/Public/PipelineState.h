/*
 PipelineState.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef PipelineState_h__
#define PipelineState_h__

#include <vector>
#include "GraphicsTypes.h"
#include "GraphicsUtils.h"

namespace GEPUtils{ namespace Graphics {

class PipelineState {

public:

	PipelineState();

	// Note: Virtual base destructor is necessary to call the destructor of the derived class first! 
	// We need it to release references to graphics resources (with ComPtr)!!
	virtual ~PipelineState(); 

	struct INPUT_LAYOUT_DESC {
		struct LayoutElement {
			std::string m_Name;
			BUFFER_FORMAT m_Format;
		};
		std::vector<LayoutElement> LayoutElements;
	};
	// Abstraction of root signature parameter desc
	struct RESOURCE_BINDER_PARAM {
		// TODO create support for root descriptors
		void InitAsConstants(uint32_t InNum32BitValues, uint32_t InShaderRegister, uint32_t InRegisterSpace = 0, GEPUtils::Graphics::SHADER_VISIBILITY InShaderVisibility = SHADER_VISIBILITY::SV_ALL)
		{
			Num32BitValues = InNum32BitValues;
			ShaderRegister = InShaderRegister;
			RegisterSpace = InRegisterSpace;
			ResourceType = RESOURCE_TYPE::CONSTANTS;
			shaderVisibility = InShaderVisibility;
		}

		void InitAsTableCBVRange(uint32_t InNumDescriptors, uint32_t InShaderRegister, uint32_t InRegisterSpace = 0, GEPUtils::Graphics::SHADER_VISIBILITY InShaderVisibility = SHADER_VISIBILITY::SV_ALL)
		{
			NumDescriptors = InNumDescriptors;
			ShaderRegister = InShaderRegister;
			RegisterSpace = InRegisterSpace;
			ResourceType = RESOURCE_TYPE::CBV_RANGE;
			shaderVisibility = InShaderVisibility;
		}

		void InitAsTableSRVRange(uint32_t InNumDescriptors, uint32_t InShaderRegister, uint32_t InRegisterSpace = 0, GEPUtils::Graphics::SHADER_VISIBILITY InShaderVisibility = SHADER_VISIBILITY::SV_ALL)
		{
			NumDescriptors = InNumDescriptors;
			ShaderRegister = InShaderRegister;
			RegisterSpace = InRegisterSpace;
			ResourceType = RESOURCE_TYPE::SRV_RANGE;
			shaderVisibility = InShaderVisibility;
		}

		void InitAsTableUAVRange(uint32_t InNumDescriptors, uint32_t InShaderRegister, uint32_t InRegisterSpace = 0, GEPUtils::Graphics::SHADER_VISIBILITY InShaderVisibility = SHADER_VISIBILITY::SV_ALL)
		{
			NumDescriptors = InNumDescriptors;
			ShaderRegister = InShaderRegister;
			RegisterSpace = InRegisterSpace;
			ResourceType = RESOURCE_TYPE::UAV_RANGE;
			shaderVisibility = InShaderVisibility;
		}

		uint32_t Num32BitValues = 0; uint32_t ShaderRegister = 0; uint32_t RegisterSpace = 0; uint32_t NumDescriptors = 0;

		enum class RESOURCE_TYPE {
			CONSTANTS,
			CBV_RANGE,
			SRV_RANGE,
			UAV_RANGE
		} ResourceType = RESOURCE_TYPE::CONSTANTS;
		
		SHADER_VISIBILITY shaderVisibility;
	};

	enum RESOURCE_BINDER_FLAGS : uint32_t {
		NONE = 0,
		ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT = 0x1,
		DENY_VERTEX_SHADER_ACCESS = 0x2,
		DENY_HULL_SHADER_ACCESS = 0x4,
		DENY_DOMAIN_SHADER_ACCESS = 0x8,
		DENY_GEOMETRY_SHADER_ACCESS = 0x10,
		DENY_PIXEL_SHADER_ACCESS = 0x20,
		ALLOW_STREAM_OUTPUT = 0x40
	};
	// Remember that this enum needs to generate its operators, this is done outside the PipelineState class

	// Abstraction of root signature desc
	struct RESOURCE_BINDER_DESC {
		RESOURCE_BINDER_FLAGS Flags;
		std::vector<RESOURCE_BINDER_PARAM> Params;
		std::vector<StaticSampler> StaticSamplers;
	};


	struct GRAPHICS_PSO_DESC {
		INPUT_LAYOUT_DESC& InputLayoutDesc;
		RESOURCE_BINDER_DESC& ResourceBinderDesc;
		GEPUtils::Graphics::PRIMITIVE_TOPOLOGY_TYPE TopologyType;
		Graphics::Shader& VertexShader;
		Graphics::Shader& PixelShader;
		Graphics::BUFFER_FORMAT DSFormat;
		Graphics::BUFFER_FORMAT RTFormat; //Note: considering a single render target even though platforms support many (there is a maximum of 8 render targets in D3D12)
	};

	struct COMPUTE_PSO_DESC {
		RESOURCE_BINDER_DESC& ResourceBinderDesc;
		Graphics::Shader& ComputeShader;

	};

	virtual void Init(GRAPHICS_PSO_DESC& InPipelineStateDesc) = 0;

	virtual void Init(COMPUTE_PSO_DESC& InPipelineStateDesc) = 0;

	bool IsGraphics() const  {return m_IsGraphicsPSO; }

protected:
	bool m_IsGraphicsPSO = false;
};


// Generate needed operators to use enum as flag type
DEFINE_OPERATORS_FOR_ENUM(PipelineState::RESOURCE_BINDER_FLAGS)


} }

#endif // PipelineState_h__
