#ifndef PipelineState_h__
#define PipelineState_h__

#include <vector>
#include "GraphicsTypes.h"
#include "GraphicsUtils.h"

namespace GEPUtils{ namespace Graphics {

class Device;

class PipelineState {

public:

	PipelineState(GEPUtils::Graphics::Device& InGraphicsDevice) : m_GraphicsDevice(InGraphicsDevice) {};

	PipelineState() = delete; // Do not allow to instatiate this object without passing a GraphicsDevice

	struct INPUT_LAYOUT_DESC {
		struct LayoutElement {
			std::string m_Name;
			BUFFER_FORMAT m_Format;
		};
		std::vector<LayoutElement> LayoutElements;
	};
	// Abstraction of root signature parameter desc
	struct RESOURCE_BINDER_PARAM {
		// TODO create support for root descriptors and root tables
		void InitAsConstants(uint32_t InNum32BitValues, uint32_t InShaderRegister, uint32_t InRegisterSpace = 0, GEPUtils::Graphics::SHADER_VISIBILITY InShaderVisibility = SHADER_VISIBILITY::SV_ALL)
		{
			Num32BitValues = InNum32BitValues;
			ShaderRegister = InShaderRegister;
			RegisterSpace = InRegisterSpace;
			resourceType = RESOURCE_TYPE::CONSTANTS;
			shaderVisibility = InShaderVisibility;
		}

		void InitAsTableCBVRange(uint32_t InNumDescriptors, uint32_t InShaderRegister, uint32_t InRegisterSpace = 0, GEPUtils::Graphics::SHADER_VISIBILITY InShaderVisibility = SHADER_VISIBILITY::SV_ALL)
		{
			NumDescriptors = InNumDescriptors;
			ShaderRegister = InShaderRegister;
			RegisterSpace = InRegisterSpace;
			resourceType = RESOURCE_TYPE::CBV_RANGE;
			shaderVisibility = InShaderVisibility;
		}

		uint32_t Num32BitValues = 0; uint32_t ShaderRegister = 0; uint32_t RegisterSpace = 0; uint32_t NumDescriptors = 0;

		enum class RESOURCE_TYPE {
			CONSTANTS,
			CBV_RANGE
		} resourceType = RESOURCE_TYPE::CONSTANTS;
		
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
	};


	struct PIPELINE_STATE_DESC {
		INPUT_LAYOUT_DESC& InputLayoutDesc;
		RESOURCE_BINDER_DESC& ResourceBinderDesc;
		GEPUtils::Graphics::PRIMITIVE_TOPOLOGY_TYPE TopologyType;
		Graphics::Shader& VertexShader;
		Graphics::Shader& PixelShader; //TODO later support for more shaders
		Graphics::BUFFER_FORMAT DSFormat;
		Graphics::BUFFER_FORMAT RTFormat; //TODO later on support for different RT formats (there is a maximum of 8 render targets in D3D12)
	};

	virtual void Init(PIPELINE_STATE_DESC& InPipelineStateDesc) = 0;

protected:

	GEPUtils::Graphics::Device& m_GraphicsDevice;

};


// Generate needed operators to use enum as flag type
DEFINE_OPERATORS_FOR_ENUM(PipelineState::RESOURCE_BINDER_FLAGS)


} }

#endif // PipelineState_h__
