#ifndef CEE_ENGINE_H
#define CEE_ENGINE_H

#include <CeeEngine/messageBus.h>
#include <CeeEngine/platform.h>
#include <CeeEngine/assert.h>
#include <CeeEngine/event.h>
#include <CeeEngine/layer.h>
#include <CeeEngine/debugLayer.h>
#include <CeeEngine/window.h>
#include <CeeEngine/renderer.h>
#include <CeeEngine/debugMessenger.h>

#include <memory>
#include <thread>
namespace cee {

struct CEEAPI ApplicationSpec {
	CeeErrorSeverity messageLevels = (CeeErrorSeverity)(ERROR_SEVERITY_WARNING | ERROR_SEVERITY_ERROR);
	bool EnableValidation = false;
};

class CEEAPI Application {
public:
	Application(const ApplicationSpec& spec);
	virtual ~Application();

	void CEECALL Run();

	bool CEECALL OnEvent(Event& e);

	void CEECALL PushLayer(Layer* layer);
	void CEECALL PushOverlay(Layer* overlay);

	void CEECALL Close();

private:
	MessageBus m_MessageBus;
	bool m_Running;
	LayerStack m_LayerStack;

	DebugLayer* m_DebugLayer;

	std::shared_ptr<Renderer> m_Renderer;
	std::shared_ptr<Window> m_Window;

	VertexBuffer m_VertexBuffer;
	IndexBuffer m_IndexBuffer;

	std::thread m_RenderThread;

	uint64_t m_AverageFrameTime;

private:
	static Application* s_Instance;
};
}

#endif
