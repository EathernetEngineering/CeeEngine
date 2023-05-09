#ifndef CEE_ENGINE_MESSAGE_BUS_H
#define CEE_ENGINE_MESSAGE_BUS_H

#include <vector>
#include <list>
#include <functional>
#include <mutex>
#include <CeeEngine/CeeEvent.h>

namespace cee {
	class MessageBus {
		public:
			MessageBus();
			~MessageBus();

			void DispatchEvents(void);
			void Stop(void);

			void PostMessage(Event* e);
			void PostMessageImmedate(Event* e);

			void RegisterMessageHandler(std::function<void(Event&)> handler);

	private:
		std::list<Event*> m_MessageQueue;
		std::vector<std::function<void(Event&)>> m_Handlers;
	};
}

#endif
