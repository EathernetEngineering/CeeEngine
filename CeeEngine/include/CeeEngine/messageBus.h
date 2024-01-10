/* Decleration of internal application messaging.
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

#ifndef CEE_ENGINE_MESSAGE_BUS_H
#define CEE_ENGINE_MESSAGE_BUS_H

#include <vector>
#include <list>
#include <functional>
#include <mutex>

#include <CeeEngine/event.h>

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
