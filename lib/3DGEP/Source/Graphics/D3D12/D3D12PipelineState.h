#ifndef D3D12PipelineState_h__
#define D3D12PipelineState_h__

#include "PipelineState.h"
#include <d3dx12.h>


namespace GEPUtils{ namespace Graphics {

class Device;

class D3D12PipelineState : public PipelineState{

public:
	D3D12PipelineState(GEPUtils::Graphics::Device& InGraphicsDevice) : PipelineState(InGraphicsDevice) { }

	virtual void Init(PIPELINE_STATE_DESC& InPipelineStateDesc) override;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetInnerPSO() { return m_PipelineState; }
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetInnerRootSignature() { return m_RootSignature; }

private:
	bool m_IsInitialized = false;

	static D3D12_ROOT_SIGNATURE_FLAGS TransformResourceBinderFlags(RESOURCE_BINDER_FLAGS InResourceBinderFlags);
	static D3D12_INPUT_ELEMENT_DESC TransformInputLayoutElement(INPUT_LAYOUT_DESC::LayoutElement& InLayoutElementDesc);
	static CD3DX12_ROOT_PARAMETER1 TransformResourceBinderParams(RESOURCE_BINDER_PARAM& InResourceBinderParam);

	static D3D12_SHADER_VISIBILITY TransformShaderVisibility(SHADER_VISIBILITY shaderVisibility);
	
	// Root Signature
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
	// Pipeline State Object
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState = nullptr;

};


} }

#endif // D3D12PipelineState_h__
