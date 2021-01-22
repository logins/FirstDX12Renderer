/*
 GraphicsAllocator.cpp

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/
 
#include "GraphicsAllocator.h"

#ifdef GRAPHICS_SDK_D3D12

#include "D3D12GraphicsAllocator.h"

#endif

namespace GEPUtils { namespace Graphics {

	std::unique_ptr<GEPUtils::Graphics::GraphicsAllocatorBase> GraphicsAllocator::m_Instance;

	GEPUtils::Graphics::GraphicsAllocatorBase* GraphicsAllocator::Get()
	{
		if (!m_Instance)
		{
#ifdef GRAPHICS_SDK_D3D12
			m_Instance = std::make_unique<GEPUtils::Graphics::D3D12GraphicsAllocator>();
#endif
		}
		return m_Instance.get();
	}

}
}
