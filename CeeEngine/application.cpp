#include "CeeEngine/assert.h"
#include "CeeEngine/debugMessenger.h"
#include "CeeEngine/renderer.h"
#include <CeeEngine/application.h>
#include <CeeEngine/input.h>
#include <CeeEngine/renderer2D.h>
#include <CeeEngine/renderer3D.h>

#include <cstring>

#include <chrono>

namespace cee {
Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpec& spec)
 : m_LayerStack(&m_MessageBus) {
	if (s_Instance) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Application already exits...\tExiting...\t");
		std::exit(EXIT_FAILURE);
	}
	s_Instance = this;
	DebugMessenger::SetReportLevels(spec.messageLevels);

	auto start = std::chrono::high_resolution_clock::now();

#ifndef NDEBUG
	m_DebugLayer = new DebugLayer;
	m_LayerStack.PushLayer(m_DebugLayer);
#endif

	WindowSpec windowSpec = {};
	windowSpec.width = 1280;
	windowSpec.height = 720;
	windowSpec.title = "CeeEngine Application";
	windowSpec.msgBus = &m_MessageBus;
	m_Window = std::make_shared<Window>(windowSpec);
	
	Input::Init(&m_MessageBus, m_Window);
	
	RendererSpec rendererSpec;
	rendererSpec.window = m_Window;
	rendererSpec.msgBus = &m_MessageBus;
	rendererSpec.enableValidationLayers = spec.EnableValidation;
	if (Renderer3D::Init(rendererSpec) != 0) {
		CEE_ASSERT(false, "Failed to initialse Renderer3D");
	}
	
	m_MessageBus.RegisterMessageHandler([this](Event& e){ (void)(this->OnEvent(e)); });

	auto end = std::chrono::high_resolution_clock::now();
	float duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0f;
	DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Time to initialise engine: %.3fms", duration);
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
