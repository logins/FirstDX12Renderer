#ifndef Part3_h__
#define Part3_h__

#include "Application.h"

class Part3Application : public GEPUtils::Application
{
public:
	Part3Application();

	// Note: this static Get() member function will replace the one in Application, effectively returning a Part3Application
	static Application* Get();

	virtual void Initialize() override;
private:

	// Callbacks for main window mouse events
	void OnMouseWheel(float InDeltaRot);
	void OnMouseMove(int32_t InX, int32_t InY);
	void OnLeftMouseDrag(int32_t InDeltaX, int32_t InDeltaY);
	void OnRightMouseDrag(int32_t InDeltaX, int32_t InDeltaY);


	// Vertex buffer for the cube
	GEPUtils::Graphics::Resource& m_VertexBuffer;
	GEPUtils::Graphics::VertexBufferView& m_VertexBufferView;
	// Index buffer for the cube
	GEPUtils::Graphics::Resource& m_IndexBuffer;
	GEPUtils::Graphics::IndexBufferView& m_IndexBufferView;

	GEPUtils::Graphics::PipelineState& m_PipelineState;

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
	virtual void RenderContent(GEPUtils::Graphics::CommandList& InCmdList) override;

};
#endif // Part3_h__
