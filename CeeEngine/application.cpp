#include <CeeEngine/application.h>
#include <CeeEngine/input.h>
#include <CeeEngine/renderer2D.h>
#include <CeeEngine/renderer3D.h>

#include <cstdio>
#include <cstring>

#include <chrono>
#include <iostream>

namespace cee {
	Application* Application::s_Instance = nullptr;

	Application::Application()
	 : m_LayerStack(&m_MessageBus) {
		if (s_Instance) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Application already exits...\tExiting...\t");
			std::exit(EXIT_FAILURE);
		}
		s_Instance = this;

		auto start = std::chrono::high_resolution_clock::now();

#ifndef NDEBUG
		m_DebugLayer = new DebugLayer;
		m_LayerStack.PushLayer(m_DebugLayer);
#endif

		WindowSpecification windowSpec = {};
		windowSpec.width = 1280;
		windowSpec.height = 720;
		windowSpec.title = "CeeEngine Application";
		m_Window = std::make_shared<Window>(&m_MessageBus, windowSpec);
		Input::Init(&m_MessageBus, m_Window);
		Renderer3D::Init(&m_MessageBus, m_Window);
		m_MessageBus.RegisterMessageHandler([this](Event& e){ (void)(this->OnEvent(e)); });

		char message[FILENAME_MAX];
		auto end = std::chrono::high_resolution_clock::now();
		float duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0f;
		sprintf(message, "Time to initialise engine: %.3fms", duration);
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, message);
	}

	Application::~Application() {
		for (auto& layer : m_LayerStack) {
			layer->OnDetach();
			delete layer;
		}
		Renderer3D::Shutdown();
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
		//m_RenderThread = std::thread(&Renderer::Run, m_Renderer);
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

			//m_Renderer->Clear({ 1.0f, 1.0f, 1.0f, 1.0f });
			//m_Renderer->StartFrame();
			//m_Renderer->Draw(m_IndexBuffer, m_VertexBuffer, 6);
			//m_Renderer->EndFrame();

			Renderer3D::BeginFrame();
			for (auto& layer : m_LayerStack) {
				layer->OnRender();;
			};
			Renderer3D::EndFrame();

			Window::PollEvents();
			m_MessageBus.DispatchEvents();
			m_Running = !m_Window->ShouldClose();
		}
		//m_RenderThread.join();
	}
}
