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
