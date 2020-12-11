#include "D3D12UtilsInternal.h"
#include "GEPUtils.h"

namespace D3D12GEPUtils {


void ThisIsMyInternalFunction()
{
	std::cout << "This is My D3D12GEP Internal Function" << std::endl;
}

bool CheckTearingSupport()
{
	BOOL allowTearing = FALSE;
	ComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		ComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory4.As(&factory5))) // Try casting factory 4 to 5 (5 has support for tearing check)
		{
			if (FAILED(factory5->CheckFeatureSupport(
				DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&allowTearing, sizeof(allowTearing)))
				)
			{
				return false;
			}
			else
			{
				return true;
			}
		}
	}
	return false;
}

D3D12_COMMAND_LIST_TYPE CmdListTypeToD3D12(GEPUtils::Graphics::COMMAND_LIST_TYPE InCmdListType)
{
	switch (InCmdListType)
	{
	case GEPUtils::Graphics::COMMAND_LIST_TYPE::COMMAND_LIST_TYPE_DIRECT: return D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
	case GEPUtils::Graphics::COMMAND_LIST_TYPE::COMMAND_LIST_TYPE_BUNDLE: return D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_BUNDLE;
	case GEPUtils::Graphics::COMMAND_LIST_TYPE::COMMAND_LIST_TYPE_COMPUTE: return D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;
	case GEPUtils::Graphics::COMMAND_LIST_TYPE::COMMAND_LIST_TYPE_COPY: return D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;
	default: std::exception("D3D12 Command List Type undefined.");
	}
	return D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
}

D3D12_RESOURCE_STATES ResourceStateTypeToD3D12(GEPUtils::Graphics::RESOURCE_STATE InResState)
{
	switch (InResState)
	{
	case GEPUtils::Graphics::RESOURCE_STATE::PRESENT: return D3D12_RESOURCE_STATE_PRESENT;
	case GEPUtils::Graphics::RESOURCE_STATE::RENDER_TARGET: return D3D12_RESOURCE_STATE_RENDER_TARGET;
	case GEPUtils::Graphics::RESOURCE_STATE::COPY_SOURCE: return D3D12_RESOURCE_STATE_COPY_SOURCE;
	case GEPUtils::Graphics::RESOURCE_STATE::COPY_DEST: return D3D12_RESOURCE_STATE_COPY_DEST;
	case GEPUtils::Graphics::RESOURCE_STATE::GEN_READ: return D3D12_RESOURCE_STATE_GENERIC_READ;
	default: StopForFail("Resource State Type not handled");
	}
	return D3D12_RESOURCE_STATE_GENERIC_READ;
}

D3D12_RESOURCE_FLAGS ResFlagsToD3D12(GEPUtils::Graphics::RESOURCE_FLAGS InResFlags)
{
	D3D12_RESOURCE_FLAGS returnFlags = D3D12_RESOURCE_FLAG_NONE;
	if (InResFlags == GEPUtils::Graphics::RESOURCE_FLAGS::NONE)
		return returnFlags;
	if(InResFlags & GEPUtils::Graphics::RESOURCE_FLAGS::ALLOW_RENDER_TARGET)
		returnFlags |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (InResFlags & GEPUtils::Graphics::RESOURCE_FLAGS::ALLOW_DEPTH_STENCIL)
		returnFlags |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (InResFlags & GEPUtils::Graphics::RESOURCE_FLAGS::ALLOW_UNORDERED_ACCESS)
		returnFlags |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	if (InResFlags & GEPUtils::Graphics::RESOURCE_FLAGS::DENY_SHADER_RESOURCE)
		returnFlags |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	if (InResFlags & GEPUtils::Graphics::RESOURCE_FLAGS::ALLOW_CROSS_ADAPTER)
		returnFlags |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
	if (InResFlags & GEPUtils::Graphics::RESOURCE_FLAGS::ALLOW_SIMULTANEOUS_ACCESS)
		returnFlags |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
	if (InResFlags & GEPUtils::Graphics::RESOURCE_FLAGS::VIDEO_DECODE_REFERENCE_ONLY)
		returnFlags |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY;
		
	return returnFlags;
}

D3D12_HEAP_TYPE HeapTypeToD3D12(GEPUtils::Graphics::RESOURCE_HEAP_TYPE InHeapType)
{
	switch (InHeapType)
	{
	case GEPUtils::Graphics::RESOURCE_HEAP_TYPE::DEFAULT: return D3D12_HEAP_TYPE_DEFAULT;
	case GEPUtils::Graphics::RESOURCE_HEAP_TYPE::UPLOAD: return D3D12_HEAP_TYPE_UPLOAD;
	}
	return D3D12_HEAP_TYPE_DEFAULT;
}

DXGI_FORMAT BufferFormatToD3D12(GEPUtils::Graphics::BUFFER_FORMAT InFormat)
{
	switch (InFormat)
	{
	case GEPUtils::Graphics::BUFFER_FORMAT::R16_UINT : return DXGI_FORMAT_R16_UINT;
	case GEPUtils::Graphics::BUFFER_FORMAT::R32G32B32_FLOAT: return DXGI_FORMAT_R32G32B32_FLOAT;
	case GEPUtils::Graphics::BUFFER_FORMAT::R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case GEPUtils::Graphics::BUFFER_FORMAT::D32_FLOAT: return DXGI_FORMAT_D32_FLOAT;
	case GEPUtils::Graphics::BUFFER_FORMAT::BC1_UNORM: return DXGI_FORMAT_BC1_UNORM;
	default: StopForFail("DXGI Buffer Format undefined.") break;
	}
	return DXGI_FORMAT_R16_UINT;
}

GEPUtils::Graphics::BUFFER_FORMAT BufferFormatToEngine(DXGI_FORMAT InFormat)
{
	switch (InFormat)
	{
	case DXGI_FORMAT_R16_UINT: return GEPUtils::Graphics::BUFFER_FORMAT::R16_UINT;
	case DXGI_FORMAT_R32G32B32_FLOAT: return GEPUtils::Graphics::BUFFER_FORMAT::R32G32B32_FLOAT;
	case DXGI_FORMAT_R8G8B8A8_UNORM: return GEPUtils::Graphics::BUFFER_FORMAT::R8G8B8A8_UNORM;
	case DXGI_FORMAT_D32_FLOAT: return GEPUtils::Graphics::BUFFER_FORMAT::D32_FLOAT;
	case DXGI_FORMAT_BC1_UNORM: return GEPUtils::Graphics::BUFFER_FORMAT::BC1_UNORM;
	default: StopForFail("DXGI Buffer Format undefined.") break;
	}
	return GEPUtils::Graphics::BUFFER_FORMAT::R16_UINT;
}

DirectX::TEX_DIMENSION TextureTypeToD3D12(GEPUtils::Graphics::TEXTURE_TYPE InFormat)
{
	switch (InFormat)
	{
	case GEPUtils::Graphics::TEXTURE_TYPE::TEX_1D: return DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE1D;
	case GEPUtils::Graphics::TEXTURE_TYPE::TEX_2D: return DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE2D;
	case GEPUtils::Graphics::TEXTURE_TYPE::TEX_3D: return DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE3D;
	default: StopForFail("Texture Format undefined.") break;
	}
	return DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE1D;
}

GEPUtils::Graphics::TEXTURE_TYPE TextureTypeToEngine(DirectX::TEX_DIMENSION InFormat)
{
	switch (InFormat)
	{
		case DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE1D: return GEPUtils::Graphics::TEXTURE_TYPE::TEX_1D;
		case DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE2D: return GEPUtils::Graphics::TEXTURE_TYPE::TEX_2D;
		case DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE3D: return GEPUtils::Graphics::TEXTURE_TYPE::TEX_3D;
	default: StopForFail("Texture Format undefined.") break;
	}
	return GEPUtils::Graphics::TEXTURE_TYPE::TEX_1D;
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyTypeToD3D12(GEPUtils::Graphics::PRIMITIVE_TOPOLOGY_TYPE InPrimitiveTopologyType)
{
	switch (InPrimitiveTopologyType)
	{
	case GEPUtils::Graphics::PRIMITIVE_TOPOLOGY_TYPE::PTT_UNDEFINED: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	case GEPUtils::Graphics::PRIMITIVE_TOPOLOGY_TYPE::PTT_POINT: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case GEPUtils::Graphics::PRIMITIVE_TOPOLOGY_TYPE::PTT_LINE: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case GEPUtils::Graphics::PRIMITIVE_TOPOLOGY_TYPE::PTT_TRIANGLE: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case GEPUtils::Graphics::PRIMITIVE_TOPOLOGY_TYPE::PTT_PATCH: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	default: std::exception("Primitive Topology Type undefined.");
	}
	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopoToD3D12(GEPUtils::Graphics::PRIMITIVE_TOPOLOGY InPrimitiveTopo)
{
	switch (InPrimitiveTopo)
	{
	case GEPUtils::Graphics::PRIMITIVE_TOPOLOGY::PT_UNDEFINED: return D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	case GEPUtils::Graphics::PRIMITIVE_TOPOLOGY::PT_POINTLIST: return D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	case GEPUtils::Graphics::PRIMITIVE_TOPOLOGY::PT_LINELIST: return D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	case GEPUtils::Graphics::PRIMITIVE_TOPOLOGY::PT_LINESTRIP: return D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case GEPUtils::Graphics::PRIMITIVE_TOPOLOGY::PT_TRIANGLELIST: return D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case GEPUtils::Graphics::PRIMITIVE_TOPOLOGY::PT_TRIANGLESTRIP: return D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	}
	StopForFail("Primitive Topology undefined.");
	return D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

D3D12_FILTER SampleFilterToD3D12(GEPUtils::Graphics::SAMPLE_FILTER_TYPE InFilterType)
{
	switch (InFilterType)
	{
	case GEPUtils::Graphics::SAMPLE_FILTER_TYPE::POINT: return D3D12_FILTER_MIN_MAG_MIP_POINT;
	case GEPUtils::Graphics::SAMPLE_FILTER_TYPE::LINEAR: return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	case GEPUtils::Graphics::SAMPLE_FILTER_TYPE::ANISOTROPIC: return D3D12_FILTER_ANISOTROPIC;
	}
	StopForFail("Sampler Type Conversion not handled.")
	return D3D12_FILTER_MIN_MAG_MIP_POINT;
}

D3D12_TEXTURE_ADDRESS_MODE TextureAddressModeToD3D12(GEPUtils::Graphics::TEXTURE_ADDRESS_MODE InAddressMode)
{
	switch (InAddressMode)
	{
	case GEPUtils::Graphics::TEXTURE_ADDRESS_MODE::WRAP: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	case GEPUtils::Graphics::TEXTURE_ADDRESS_MODE::MIRROR: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	case GEPUtils::Graphics::TEXTURE_ADDRESS_MODE::CLAMP: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case GEPUtils::Graphics::TEXTURE_ADDRESS_MODE::BORDER: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	}
	StopForFail("Texture Address Mode not handled")
	return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
}

}
