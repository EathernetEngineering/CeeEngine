/* Decleration of CeeEngine Applications.
 *   Copyright (C) 2023-2024  Chloe Eather.
 *
 *   This file is part of CeeEngine.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>. */

#ifndef CEE_ENGINE_H
#define CEE_ENGINE_H

#include <CeeEngine/messageBus.h>
#include <CeeEngine/platform.h>
#include <CeeEngine/assert.h>
#include <CeeEngine/event.h>
#include <CeeEngine/layer.h>
#include <CeeEngine/debugLayer.h>
#include <CeeEngine/window.h>
#include <CeeEngine/debugMessenger.h>
#include <CeeEngine/renderer/renderThread.h>

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

		std::shared_ptr<Window> GetWindow() { return m_Window; }

		static CEECALL Application* Get() { return s_Instance; }

	private:
		MessageBus m_MessageBus;
		bool m_Running;
		LayerStack m_LayerStack;

		DebugLayer* m_DebugLayer;
		std::shared_ptr<Window> m_Window;

		RenderThread m_RenderThread;

		uint64_t m_AverageFrameTime;

	private:
		static Application* s_Instance;
	};
}

#endif
