#include "D3D12UtilsInternal.h"



void D3D12GEPUtils::ThisIsMyInternalFunction()
{
	std::cout << "This is My D3D12GEP Internal Function" << std::endl;
}

bool D3D12GEPUtils::CheckTearingSupport()
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
