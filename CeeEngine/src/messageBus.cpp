/* Implementation of internal application messaging.
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

#include <CeeEngine/messageBus.h>

#include <CeeEngine/debugMessenger.h>

namespace cee {
	MessageBus::MessageBus() {
	}

	MessageBus::~MessageBus() {

	}

	void MessageBus::DispatchEvents(void) {
		while (!m_MessageQueue.empty()) {
			for (auto& handler : m_Handlers) {
				Event& e = *m_MessageQueue.front();
				handler(e);
			}
			delete m_MessageQueue.front();
			m_MessageQueue.pop_front();
		}
	}

	void MessageBus::PostMessage(Event* e) {
		m_MessageQueue.push_back(e);
	}

	void MessageBus::PostMessageImmedate(Event* e) {
		for (auto& handler : m_Handlers) {
			Event& event = *e;
			handler(event);
		}
	}

	void MessageBus::RegisterMessageHandler(std::function<void(Event&)> handler) {
		m_Handlers.push_back(handler);
	}
}
