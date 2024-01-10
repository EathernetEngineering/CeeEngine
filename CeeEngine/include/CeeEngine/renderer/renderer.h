/* Decleration of Renderer.
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

#ifndef CEE_ENGINE_RENDERER_H_
#define CEE_ENGINE_RENDERER_H_

#include <CeeEngine/application.h>

namespace cee {

	class VulkanContext;

	class Renderer {
	public:
		static void Init();
		static void Shutdown();

		static std::shared_ptr<VulkanContext> GetContext() { return Application::Get()->GetWindow()->GetContext(); }
	};
}

#endif
