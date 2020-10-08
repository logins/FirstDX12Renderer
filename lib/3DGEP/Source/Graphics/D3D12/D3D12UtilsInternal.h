#ifndef D3D12UtilsInternal_h__
#define D3D12UtilsInternal_h__

#include <D3D12GEPUtils.h>
#include "GraphicsTypes.h"

namespace D3D12GEPUtils {

	//using namespace Microsoft::WRL;

	void ThisIsMyInternalFunction();

	bool CheckTearingSupport();

	D3D12_COMMAND_LIST_TYPE CmdListTypeToD3D12(GEPUtils::Graphics::COMMAND_LIST_TYPE InCmdListType);

	D3D12_RESOURCE_STATES ResStateTypeToD3D12(GEPUtils::Graphics::RESOURCE_STATE InResState);

	D3D12_RESOURCE_FLAGS ResFlagsToD3D12(GEPUtils::Graphics::RESOURCE_FLAGS InResFlags);

	DXGI_FORMAT BufferFormatToD3D12(GEPUtils::Graphics::BUFFER_FORMAT InFormat);

	D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyTypeToD3D12(GEPUtils::Graphics::PRIMITIVE_TOPOLOGY_TYPE InPrimitiveTopologyType);

	D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopoToD3D12(GEPUtils::Graphics::PRIMITIVE_TOPOLOGY InPrimitiveTopo);
}
#endif // D3D12UtilsInternal_h__