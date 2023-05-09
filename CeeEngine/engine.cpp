#include <CeeEngine/CeeEngine.h>

#include <cstdio>
#include <cstring>

namespace cee {
	Application* Application::s_Instance = nullptr;

	Application::Application()
	 : m_LayerStack(&m_MessageBus) {
		if (s_Instance) {
			DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Application already exits...\tExiting...\t");
			std::exit(EXIT_FAILURE);
		}
		s_Instance = this;

#ifndef NDEBUG
		m_DebugLayer = std::make_unique<DebugLayer>();
		m_LayerStack.PushLayer(m_DebugLayer.get());
#endif

		WindowSpecification windowSpec = {};
		windowSpec.width = 1280;
		windowSpec.height = 720;
		windowSpec.title = "CeeEngine Application";
		m_Window = std::make_shared<Window>(&m_MessageBus, windowSpec);
		RendererCapabilities rendererCapabilites = {};
		rendererCapabilites.applicationName = "CeeEngine Editor";
		rendererCapabilites.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		rendererCapabilites.maxIndices = 10000;
		rendererCapabilites.maxFramesInFlight = 3;
		m_Renderer = std::make_shared<Renderer>(&m_MessageBus, rendererCapabilites, m_Window);
		if (m_Renderer->Init() != 0) {
			DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR,
											 "Failed to initialise renderer... Aborting...");
			std::exit(EXIT_FAILURE);
		}
		m_MessageBus.RegisterMessageHandler([this](Event& e){ (void)(this->OnEvent(e)); });
	}

	Application::~Application() {
		s_Instance = nullptr;
	}

	bool Application::OnEvent(Event& e)
	{
		(void)e;
		return true;
	}

	void Application::PushLayer(Layer *layer)
	{
		m_LayerStack.PushLayer(layer);
	}

	void Application::PushOverlay(Layer *overlay)
	{
		m_LayerStack.PushOverlay(overlay);
	}

	void Application::Run()
	{
		m_RenderThread = std::thread(&Renderer::Run, m_Renderer);
		Timestep ts, start, end;
		GetTime(&start);
		memset(&ts, 0, sizeof(Timestep));
		memset(&end, 0, sizeof(Timestep));
		uint64_t frameIndex = 0;
		m_Running = true;
		while (m_Running) {
			GetTime(&end);
			GetTimeStep(&start, &end, &ts);
			GetTime(&start);
			m_AverageFrameTime *= frameIndex++;
			m_AverageFrameTime += ts.nsec + ts.sec * 1000000000;
			m_AverageFrameTime /= frameIndex;
			for (auto& layer : m_LayerStack) {
				layer->OnUpdate(ts);
			};
			Window::PollEvents();
			m_MessageBus.DispatchEvents();
			m_Running = !m_Window->ShouldClose();
		}
		m_Renderer->Stop();
		m_RenderThread.join();
	}
}
