#ifndef D3D12PipelineState_h__
#define D3D12PipelineState_h__

#include "PipelineState.h"
#include <d3dx12.h>


namespace GEPUtils{ namespace Graphics {

class Device;

class D3D12PipelineState : public PipelineState{

public:
	D3D12PipelineState() = default;

	virtual void Init(GRAPHICS_PSO_DESC& InPipelineStateDesc) override;

	virtual void Init(COMPUTE_PSO_DESC& InPipelineStateDesc) override;


	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetInnerPSO() { return m_PipelineState; }
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetInnerRootSignature() { return m_RootSignature; }
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC& GetInnerRootSignatureDesc() { return m_RootSignatureInfo.rootSignatureDesc; }

	uint32_t GenerateRootTableBitMask();

	uint32_t GetRootDescriptorsNumAtIndex(uint32_t InRootIndex);

private:

	void GenerateRootSignature(Microsoft::WRL::ComPtr<ID3D12Device2> d3d12GraphicsDevice, RESOURCE_BINDER_DESC& InPipelineStateDesc);

	bool m_IsInitialized = false;

	static D3D12_ROOT_SIGNATURE_FLAGS TransformResourceBinderFlags(RESOURCE_BINDER_FLAGS InResourceBinderFlags); 
	static D3D12_INPUT_ELEMENT_DESC TransformInputLayoutElement(INPUT_LAYOUT_DESC::LayoutElement& InLayoutElementDesc);
	static CD3DX12_STATIC_SAMPLER_DESC TransformStaticSamplerElement(Graphics::StaticSampler& InLayoutElementDesc);
	static CD3DX12_ROOT_PARAMETER1 TransformResourceBinderParams(RESOURCE_BINDER_PARAM& InResourceBinderParam, std::vector<D3D12_DESCRIPTOR_RANGE1>& OutDescRanges);

	static D3D12_SHADER_VISIBILITY TransformShaderVisibility(SHADER_VISIBILITY shaderVisibility);
	
	// Root Signature
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
	// Pipeline State Object
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState = nullptr;


	struct RootSignatureInfo {
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters;
		std::vector<D3D12_DESCRIPTOR_RANGE1> m_DescRanges;
		std::vector<CD3DX12_STATIC_SAMPLER_DESC> m_StaticSamplers;
	} m_RootSignatureInfo;

};


} }

#endif // D3D12PipelineState_h__
