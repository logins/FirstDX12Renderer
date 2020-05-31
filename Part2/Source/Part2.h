#ifndef Part2_h__
#define Part2_h__

#include <Windows.h>
#include <D3D12GEPUtils.h>

class Part2
{
public:
	~Part2() { m_Instance = nullptr; }
	static Part2* Get();
	void Initialize();
	void Start();
private:
	// Singleton : Default constructor, copy constructor and assingment operators to be private
	Part2() {};
	Part2(const Part2&);
	Part2& operator=(const Part2&);
private:
	// WndProc functions need to be static, so we have to create a wrapper to transfer the event to the singleton's internal wndProc
	static LRESULT CALLBACK MainWndProc(HWND InHWND, UINT InMsg, WPARAM InWParam, LPARAM InLParam) {
		return Get()->MainWndProc_Internal(InHWND, InMsg, InWParam, InLParam);
	};
	LRESULT CALLBACK MainWndProc_Internal(HWND InHWND, UINT InMsg, WPARAM InWParam, LPARAM InLParam);
	void OnMainWindowPaint();

	D3D12GEPUtils::D3D12Window m_MainWindow;

	bool m_IsInitialized = false;
	static Part2* m_Instance; //Note: This is just a declaration, not a definition! m_Instance must be explicitly defined
	bool m_VSyncEnabled = false;
};
#endif // Part2_h__
