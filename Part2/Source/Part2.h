#ifndef Part2_h__
#define Part2_h__

#include <Windows.h>
#include <wrl.h>
#include <D3D12Window.h>
#include <Eigen/Core>

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

	// Vertex buffer for the cube
	ComPtr<ID3D12Resource> m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	// Index buffer for the cube
	ComPtr<ID3D12Resource> m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
	// DepthStencil buffer
	ComPtr<ID3D12Resource> m_DSBuffer;
	// DS buffer views need to be contained in a heap even if we use just one
	ComPtr<ID3D12DescriptorHeap> m_DSVHeap;
	// Root Signature
	ComPtr<ID3D12RootSignature> m_RootSignature;
	// Pipeline State Object
	ComPtr<ID3D12PipelineState> m_PipelineState;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	float m_FoV;

	// Model, View, Projection Matrices
	Eigen::Matrix4f m_ModelMatrix;
	Eigen::Matrix4f m_ViewMatrix;
	Eigen::Matrix4f m_ProjMatrix;

	bool m_IsInitialized = false;
	static Part2* m_Instance; //Note: This is just a declaration, not a definition! m_Instance must be explicitly defined
	
};
#endif // Part2_h__
