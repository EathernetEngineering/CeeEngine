/* Decleration of input system.
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

#ifndef _CEE_ENGINE_INPUT_H
#define _CEE_ENGINE_INPUT_H

#include <CeeEngine/messageBus.h>
#include <CeeEngine/keyCodes.h>
#include <CeeEngine/window.h>

#include <map>
#include <memory>

namespace cee {
	namespace Input {
		void Init(MessageBus* messageBus, std::shared_ptr<Window> window);
		void Shutdown();
		int GetKeyState(KeyCode keycode);
	}
}

#endif
