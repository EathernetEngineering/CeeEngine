#ifndef CEE_ENGINE_H
#define CEE_ENGINE_H

#include "CeeEngine/CeeEngineMessageBus.h"
#include "CeeEngine/CeeEnginePlatform.h"
#include "CeeEngine/CeeEvent.h"
#include "CeeEngine/CeeEngineLayer.h"
#include "CeeEngine/CeeEngineDebugLayer.h"
#include "CeeEngine/CeeEngineWindow.h"
#include "CeeEngine/CeeEngineRenderer.h"
#include "CeeEngine/CeeEngineDebugMessenger.h"

#include <memory>
#include <thread>

namespace cee {
	class CEEAPI Application {
	public:
		Application();
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

		std::unique_ptr<DebugLayer> m_DebugLayer;

		std::shared_ptr<Renderer> m_Renderer;
		std::shared_ptr<Window> m_Window;

		std::thread m_RenderThread;

		uint64_t m_AverageFrameTime;

	private:
		static Application* s_Instance;
	};
}

#endif
