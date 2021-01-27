/*
 Part2.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef Part2_h__
#define Part2_h__

#include <Windows.h>
#include <wrl.h>
#include <D3D12Window.h>
#include <Eigen/Core>

namespace GEPUtils { namespace Graphics { class Device; } }

using namespace Microsoft::WRL;

class Part2
{
public:
	~Part2() { m_Instance = nullptr; }
	static Part2* Get();
	void Initialize();
	void Run();
private:
	// Singleton : copy constructor and assingment operators to be private
	Part2() { }
	Part2(const Part2&);
	Part2& operator=(const Part2&);
	// Orders the application to quit
	void QuitApplication();
	// Deallocates graphics resources
	void OnApplicationQuit();
	struct MouseWheelEventArgs { float WheelDelta; };
	struct MouseButtonEventArgs { int32_t X; int32_t Y; uint8_t Button; bool DragDetected; }; // 0=Left, 1=Middle, 2=Right
	void OnMouseWheel(MouseWheelEventArgs& InEvent);
	void OnMousePressed(MouseButtonEventArgs& InEvent);
	void OnMouseReleased(MouseButtonEventArgs& InEvent);
	bool m_PaintStarted = false;
	void OnLeftMouseDrag(int32_t InDeltaX, int32_t InDeltaY);
	void OnRightMouseDrag(int32_t InDeltaX, int32_t InDeltaY);
private:
	// Load graphics resources for the colored cube
	void LoadContent();
	// Update and Render scene content and resources
	void Update();
	void Render();

	void SetFoV(float InFoV);
	void SetAspectRatio(float InAspectRatio);

	// WndProc functions need to be static, so we have to create a wrapper to transfer the event to the singleton's internal wndProc
	static LRESULT CALLBACK MainWndProc(HWND InHWND, UINT InMsg, WPARAM InWParam, LPARAM InLParam) {
		return Get()->MainWndProc_Internal(InHWND, InMsg, InWParam, InLParam);
	};
	LRESULT CALLBACK MainWndProc_Internal(HWND InHWND, UINT InMsg, WPARAM InWParam, LPARAM InLParam);
	void OnMainWindowPaint();
	bool m_MouseLeftDrag = false;
	bool m_MouseRightDrag = false;
	POINT m_PrevMouseCoords = {};
	static const uint8_t m_NumCmdAllocators = 3;

	ComPtr<ID3D12Device2> m_GraphicsDevice;

	std::unique_ptr<D3D12GEPUtils::D3D12CommandQueue> m_CmdQueue;

	std::unique_ptr<D3D12GEPUtils::D3D12Window> m_MainWindow;

	// Vertex buffer for the cube
	ComPtr<ID3D12Resource> m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	// Index buffer for the cube
	ComPtr<ID3D12Resource> m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
	// Root Signature
	ComPtr<ID3D12RootSignature> m_RootSignature;
	// Pipeline State Object
	ComPtr<ID3D12PipelineState> m_PipelineState;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	float m_FoV = 0.f;
	float m_ZMin = 0.f, m_ZMax = 0.f;
	float m_AspectRatio = 0;
	// Model, View, Projection Matrices
	Eigen::Matrix4f m_ModelMatrix;
	Eigen::Matrix4f m_ViewMatrix;
	Eigen::Matrix4f m_ProjMatrix;

	// Vertex data for colored cube
	struct VertexPosColor
	{
		Eigen::Vector3f Position;
		Eigen::Vector3f Color;
	};

	const VertexPosColor m_VertexData[8] = {
		{ Eigen::Vector3f(-1.f, -1.f, -1.f), Eigen::Vector3f(0.f, 0.f, 0.f) }, // 0
		{ Eigen::Vector3f(-1.f, 1.f, -1.f), Eigen::Vector3f(0.f, 1.f, 0.f) }, // 1
		{ Eigen::Vector3f(1.f, 1.f, -1.f), Eigen::Vector3f(1.f, 1.f, 0.f) }, // 2
		{ Eigen::Vector3f(1.f, -1.f, -1.f), Eigen::Vector3f(1.f, 0.f, 0.f) }, // 3
		{ Eigen::Vector3f(-1.f, -1.f, 1.f), Eigen::Vector3f(0.f, 0.f, 1.f) }, // 4
		{ Eigen::Vector3f(-1.f, 1.f, 1.f), Eigen::Vector3f(0.f, 1.f, 1.f) }, // 5
		{ Eigen::Vector3f(1.f, 1.f, 1.f), Eigen::Vector3f(1.f, 1.f, 1.f) }, // 6
		{ Eigen::Vector3f(1.f, -1.f, 1.f), Eigen::Vector3f(1.f, 0.f, 1.f) }  // 7
	};

	const WORD m_IndexData[36] = {
		0, 1, 2, 0, 2, 3,
		4, 6, 5, 4, 7, 6,
		4, 5, 1, 4, 1, 0,
		3, 2, 6, 3, 6, 7,
		1, 5, 6, 1, 6, 2,
		4, 0, 3, 4, 3, 7
	};
	bool m_IsInitialized = false;
	static Part2* m_Instance; //Note: This is just a declaration, not a definition! m_Instance must be explicitly defined
	
};
#endif // Part2_h__
