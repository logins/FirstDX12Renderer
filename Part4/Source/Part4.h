/*
 Part4.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef Part4_h__
#define Part4_h__

#include "Application.h"

class Part4Application : public GEPUtils::Application
{
public:
	Part4Application() = default;

	// Note: this static Get() member function will replace the one in Application, effectively returning a Part3Application
	static Application* Get();

	virtual void Initialize() override;
private:

	// Callbacks for main window mouse and keyboard events
	void OnMouseWheel(float InDeltaRot);
	void OnMouseMove(int32_t InX, int32_t InY);
	void OnLeftMouseDrag(int32_t InDeltaX, int32_t InDeltaY);
	void OnRightMouseDrag(int32_t InDeltaX, int32_t InDeltaY);
	void OnTypingKeyPressed(GEPUtils::Graphics::KEYBOARD_KEY InPressedKey);
	void OnControlKeyPressed(GEPUtils::Graphics::KEYBOARD_KEY InPressedKey);

	// Note: most of the following member variables should not belong to the application
	// but instead to a draw command object for the current entity being drawn.
	// In that case we would create the draw command object with references instead of raw pointers.
	// Vertex buffer for the cube
	GEPUtils::Graphics::Buffer* m_VertexBuffer;
	GEPUtils::Graphics::VertexBufferView* m_VertexBufferView;
	// Index buffer for the cube
	GEPUtils::Graphics::Buffer* m_IndexBuffer;
	GEPUtils::Graphics::IndexBufferView* m_IndexBufferView;

	// Texture for the cubemap
	GEPUtils::Graphics::Texture* m_Cubemap;
	GEPUtils::Graphics::ShaderResourceView* m_CubemapView;
	// Textures for the generated mips
	GEPUtils::Graphics::Texture* m_CubemapWithMips;
	struct GenerateMipsCB
	{
		Eigen::Vector2f Mip1Size;
	};

	GEPUtils::Graphics::PipelineState* m_PipelineState;

	Eigen::Matrix4f m_MvpMatrix;

	// Vertex data for colored cube
	struct VertexPosColor
	{
		Eigen::Vector3f Position;
		Eigen::Vector3f CubemapCoords;
	};

	const VertexPosColor m_VertexData[8] = {
		{ Eigen::Vector3f(-1.f, -1.f, -1.f),Eigen::Vector3f(-1.f, -1.f, -1.f) },	// 0
		{ Eigen::Vector3f(-1.f, 1.f, -1.f),Eigen::Vector3f(-1.f, 1.f, -1.f)  },	// 1
		{ Eigen::Vector3f(1.f, 1.f, -1.f), Eigen::Vector3f(1.f, 1.f, -1.f)   },	// 2
		{ Eigen::Vector3f(1.f, -1.f, -1.f),Eigen::Vector3f(1.f, -1.f, -1.f)  },	// 3
		{ Eigen::Vector3f(-1.f, -1.f, 1.f),Eigen::Vector3f(-1.f, -1.f, 1.f)  },	// 4
		{ Eigen::Vector3f(-1.f, 1.f, 1.f), Eigen::Vector3f(-1.f, 1.f, 1.f)  },	// 5
		{ Eigen::Vector3f(1.f, 1.f, 1.f),  Eigen::Vector3f(1.f, 1.f, 1.f)   },		// 6
		{ Eigen::Vector3f(1.f, -1.f, 1.f), Eigen::Vector3f(1.f, -1.f, 1.f)  }		// 7
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

#endif // Part4_h__
