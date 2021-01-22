/*
 D3D12UtilsInternal.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef D3D12UtilsInternal_h__
#define D3D12UtilsInternal_h__

#include <D3D12GEPUtils.h>
#include "GraphicsTypes.h"
#include "DirectXTex.h"

namespace D3D12GEPUtils {

	void ThisIsMyInternalFunction();

	bool CheckTearingSupport();

	D3D12_COMMAND_LIST_TYPE CmdListTypeToD3D12(GEPUtils::Graphics::COMMAND_LIST_TYPE InCmdListType);

	D3D12_RESOURCE_STATES ResourceStateTypeToD3D12(GEPUtils::Graphics::RESOURCE_STATE InResState);

	D3D12_RESOURCE_FLAGS ResFlagsToD3D12(GEPUtils::Graphics::RESOURCE_FLAGS InResFlags);

	D3D12_HEAP_TYPE HeapTypeToD3D12(GEPUtils::Graphics::RESOURCE_HEAP_TYPE InHeapType);

	DXGI_FORMAT BufferFormatToD3D12(GEPUtils::Graphics::BUFFER_FORMAT InFormat);

	GEPUtils::Graphics::BUFFER_FORMAT BufferFormatToEngine(DXGI_FORMAT InFormat);

	DirectX::TEX_DIMENSION TextureTypeToD3D12(GEPUtils::Graphics::TEXTURE_TYPE InFormat);

	GEPUtils::Graphics::TEXTURE_TYPE TextureTypeToEngine(DirectX::TEX_DIMENSION InFormat);

	D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyTypeToD3D12(GEPUtils::Graphics::PRIMITIVE_TOPOLOGY_TYPE InPrimitiveTopologyType);

	D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopoToD3D12(GEPUtils::Graphics::PRIMITIVE_TOPOLOGY InPrimitiveTopo);

	D3D12_FILTER SampleFilterToD3D12(GEPUtils::Graphics::SAMPLE_FILTER_TYPE InFilterType);

	D3D12_TEXTURE_ADDRESS_MODE TextureAddressModeToD3D12(GEPUtils::Graphics::TEXTURE_ADDRESS_MODE InAddressMode);
}

#endif // D3D12UtilsInternal_h__
