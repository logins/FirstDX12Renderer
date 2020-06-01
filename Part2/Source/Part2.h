#ifndef Part2_h__
#define Part2_h__

#include <Windows.h>
#include <wrl.h>
#include <D3D12GEPUtils.h>

using namespace Microsoft::WRL;

class Part2
{
public:
	~Part2() { m_Instance = nullptr; }
	static Part2* Get();
	void Initialize();
	void Run();
private:
	// Singleton : Default constructor, copy constructor and assingment operators to be private
	Part2() {};
	Part2(const Part2&);
	Part2& operator=(const Part2&);
	void QuitApplication();
private:
	// Update and Render scene content and resources
	void Update();
	void Render();

	// WndProc functions need to be static, so we have to create a wrapper to transfer the event to the singleton's internal wndProc
	static LRESULT CALLBACK MainWndProc(HWND InHWND, UINT InMsg, WPARAM InWParam, LPARAM InLParam) {
		return Get()->MainWndProc_Internal(InHWND, InMsg, InWParam, InLParam);
	};
	LRESULT CALLBACK MainWndProc_Internal(HWND InHWND, UINT InMsg, WPARAM InWParam, LPARAM InLParam);
	void OnMainWindowPaint();

	static const uint8_t m_NumCmdAllocators = 3;

	ComPtr<ID3D12CommandQueue> m_CmdQueue;
	ComPtr<ID3D12Fence> m_Fence;
	HANDLE m_FenceEvent;
	D3D12GEPUtils::D3D12Window m_MainWindow;
	ComPtr<ID3D12GraphicsCommandList> m_CmdList;
	ComPtr<ID3D12CommandAllocator> m_CmdAllocators[m_NumCmdAllocators];

	bool m_IsInitialized = false;
	static Part2* m_Instance; //Note: This is just a declaration, not a definition! m_Instance must be explicitly defined
	bool m_VSyncEnabled = false;
};
#endif // Part2_h__
