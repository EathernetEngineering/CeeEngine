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
#include <CeeEngine/renderer/renderThreadQueue.h>
#include <CeeEngine/renderer/renderThread.h>
#include <type_traits>

namespace cee {

	class VulkanContext;

	class Renderer {
	public:
		static void Init();
		static void Shutdown();

		static std::shared_ptr<VulkanContext> GetContext() { return Application::Get()->GetWindow()->GetContext(); }

		static void BeginFrame();
		static void EndFrame();

		template<typename F>
		static void Submit(F&& function) {
			static_assert(std::is_trivially_destructible_v<F>, "Objects must be trivially destructible.");
			auto wrapper = [](void* pFunction){
				(*(F*)pFunction)();
			};
			void* functionAddress = GetRenderQueue().Submit(wrapper, sizeof(function));
			new (functionAddress) F(std::forward<F>(function));
		}

		static void RenderThreadFucntion(RenderThread* thread);
		static void WaitAndRender(RenderThread* thread);
		static RenderThreadQueue& GetRenderQueue();
		static size_t GetQueueIndex();
		static size_t GetQueueSubmissionIndex();

		static void SwapQueues();


	private:
		Renderer* s_Instance;
	};
}

#endif
