/*
 D3D12Device.cpp

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/
 
#include "D3D12Device.h"
#include "D3D12GEPUtils.h"

using namespace Microsoft::WRL;

namespace GEPUtils { namespace Graphics {

D3D12Device::D3D12Device()
{
	ComPtr<IDXGIAdapter4> adapter = D3D12GEPUtils::GetMainAdapter(false);

	m_D3d12Device = D3D12GEPUtils::CreateDevice(adapter);

#if _DEBUG
	D3D12GEPUtils::ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_DxgiDebug)));

	SetMessageBreaksOnSeverity();
#endif
}

void D3D12Device::ReportLiveObjects()
{
#if _DEBUG
	m_DxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
#endif
}

void D3D12Device::ShutDown()
{
	// The graphics interface object will be destroyed, but the D3D12Device will still be alive,
	// useful if we want to call ReportLiveObjects() after ShutDown()
	m_D3d12Device.Reset();
}

void D3D12Device::SetMessageBreaksOnSeverity()
{
#if _DEBUG
	// Taken from the answer on this thread https://stackoverflow.com/questions/46802508/d3d12-unavoidable-leak-report
	// If we don't set break on warining to false, the ReportLiveObjects() will break on every object found with active references
	ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
	{
		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
	}
#endif
}

} }