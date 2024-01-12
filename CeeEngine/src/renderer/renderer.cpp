/* Implementation of renderer.
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

#include <CeeEngine/renderer/renderer.h>

namespace cee {
	static size_t g_FrameIndex;

	static constexpr size_t RENDER_QUEUE_COUNT = 3;
	static RenderThreadQueue g_RenderQueues[RENDER_QUEUE_COUNT];
	static size_t g_QueueSubmissionIndex;

	void Renderer::Init() {

	}

	void Renderer::Shutdown() {

	}

	void Renderer::BeginFrame() {

	}

	void Renderer::EndFrame() {
		++g_FrameIndex %= RENDER_QUEUE_COUNT;
	}

	void Renderer::RenderThreadFucntion(RenderThread* thread) {
		while (thread->IsRunning()) {
			WaitAndRender(thread);
		}
	}

	void Renderer::WaitAndRender(RenderThread* thread) {
		thread->WaitAndSetState(RenderThread::ThreadState::KICK, RenderThread::ThreadState::BUSY);

		g_RenderQueues[GetQueueIndex()].Execute();

		thread->SetState(RenderThread::ThreadState::IDLE);
	}

	RenderThreadQueue& Renderer::GetRenderQueue() {
		return g_RenderQueues[g_QueueSubmissionIndex];
	}

	size_t Renderer::GetQueueIndex() {
		return (g_QueueSubmissionIndex + 1) % RENDER_QUEUE_COUNT;
	}

	size_t Renderer::GetQueueSubmissionIndex() {
		return g_QueueSubmissionIndex;
	}

	void Renderer::SwapQueues() {
		++g_QueueSubmissionIndex %= RENDER_QUEUE_COUNT;
	}
}
