/*
 Part3.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef Part3_h__
#define Part3_h__

#include "Application.h"
#include "GraphicsTypes.h"

namespace GEPUtils { namespace Graphics { class PipelineState; } }

class Part3Application : public GEPUtils::Application
{
public:
	Part3Application();

	virtual void Initialize() override;

	virtual void OnQuitApplication() override;

private:


	// Callbacks for main window mouse and keyboard events
	void OnMouseWheel(float InDeltaRot);
	void OnMouseMove(int32_t InX, int32_t InY);
	void OnLeftMouseDrag(int32_t InDeltaX, int32_t InDeltaY);
	void OnRightMouseDrag(int32_t InDeltaX, int32_t InDeltaY);
	void OnTypingKeyPressed(GEPUtils::KEYBOARD_KEY InPressedKey);
	void OnControlKeyPressed(GEPUtils::KEYBOARD_KEY InPressedKey);

	// Vertex buffer for the cube
	GEPUtils::Graphics::Resource* m_VertexBuffer;
	GEPUtils::Graphics::VertexBufferView* m_VertexBufferView;
	// Index buffer for the cube
	GEPUtils::Graphics::Resource* m_IndexBuffer;
	GEPUtils::Graphics::IndexBufferView* m_IndexBufferView;
	// Standalone Constant Buffer for the color modifier
	GEPUtils::Graphics::DynamicBuffer* m_ColorModBuffer;
	GEPUtils::Graphics::ConstantBufferView* m_ColorModBufferView;

	GEPUtils::Graphics::PipelineState* m_PipelineState;

	Eigen::Matrix4f m_MvpMatrix;

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

	const unsigned short m_IndexData[36] = {
		0, 1, 2, 0, 2, 3,
		4, 6, 5, 4, 7, 6,
		4, 5, 1, 4, 1, 0,
		3, 2, 6, 3, 6, 7,
		1, 5, 6, 1, 6, 2,
		4, 0, 3, 4, 3, 7
	};

protected:

	virtual void UpdateContent(float InDeltaTime) override;

	virtual void RenderContent(GEPUtils::Graphics::CommandList& InCmdList) override;



};
#endif // Part3_h__
