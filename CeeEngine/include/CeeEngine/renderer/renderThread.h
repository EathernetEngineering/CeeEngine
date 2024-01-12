/* Decleration of render thread.
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

#ifndef CEE_ENGINE_RENDERER_RENDER_THREAD_H_
#define CEE_ENGINE_RENDERER_RENDER_THREAD_H_

#include <CeeEngine/thread.h>
#include <condition_variable>
#include <atomic>

namespace cee {
	class RenderThread {
	public:
		enum class ThreadingPolicy {
			SINGLETHREADED = 0,
			MULTITHREADED = 1
		};

		enum class ThreadState {
			IDLE = 0,
			BUSY = 1,
			KICK = 2
		};

	public:
		RenderThread(ThreadingPolicy policy);
		~RenderThread();

		void Run();
		bool IsRunning() { return m_Running; };
		void Terminate();

		void SetState(ThreadState setState);
		void WaitAndSetState(ThreadState waitState, ThreadState setState);
		void Wait(ThreadState waitState);

		void NextFrame();
		void BlockUntilRenderComplete();
		void Kick();
		void Pump();

	private:
		ThreadingPolicy m_Policy;

		Thread m_Thread;

		bool m_Running;

		ThreadState m_State;
		std::mutex m_Mutex;
		std::condition_variable m_ConditionVariable;

		std::atomic<uint32_t> m_AppThreadFrame;
	};
}

#endif

