#ifndef Application_h__
#define Application_h__

#include "Window.h"
#include "CommandQueue.h" // TODO can we forward declare CommandQueue and Device?
#include "Device.h"
#include "GraphicsTypes.h"
#include <Eigen/Core>

namespace GEPUtils
{
/*!
 * \class Application
 *
 * \brief Represents the whole application. 
 * This is intended to be derived to implement proper functionality for each project part.
 * Application acts as a main hub to generate most of the other objects required to run each example.
 *
 * \author Riccardo Loggini
 * \date July 2020
 */
	class Application
	{
	public:
		~Application() { m_Instance = nullptr; }
		static Application* Get();

		virtual void Initialize();

		void Run();
		void QuitApplication();

	protected:
		// Singleton : Default constructor, copy constructor and assingment operators to be private
		Application();

		static Application* m_Instance; //Note: This is just a declaration, not a definition! m_Instance must be explicitly defined

		virtual void RenderContent(Graphics::CommandList& InCmdList) {}

		void SetAspectRatio(float InAspectRatio);
		void SetFov(float InFov);

		void OnMainWindowUpdate();
		void OnWindowPaint();

		std::unique_ptr<Graphics::Device> m_GraphicsDevice;

		std::unique_ptr<Graphics::CommandQueue> m_CmdQueue;

		std::unique_ptr<Graphics::Window> m_MainWindow;

		std::unique_ptr<Graphics::Rect> m_ScissorRect = nullptr;

		std::unique_ptr<Graphics::ViewPort> m_Viewport = nullptr;

		bool m_IsInitialized = false;
	

		float m_Fov = 0.f;
		float m_ZMin = 0.f, m_ZMax = 0.f;
		float m_AspectRatio = 0;

		// Model, View, Projection Matrices
		// Note: View and Projection matrices would belong to a camera class if there was one
		// Model matrix would belong to a specific entity in the scene
		Eigen::Matrix4f m_ModelMatrix;
		Eigen::Matrix4f m_ViewMatrix;
		Eigen::Matrix4f m_ProjMatrix;

		bool m_PaintStarted = false;

	private:
		Application(const Application&) = delete; // We do not want Application to be copiable
		Application& operator=(const Application&) = delete; // We do not want Application to be copy assignable

		void Update();

		void Render();

	};

}
#endif // Application_h__
