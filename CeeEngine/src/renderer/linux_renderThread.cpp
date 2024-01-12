/* Implementation of render thead.
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

#include <CeeEngine/renderer/renderThread.h>

#include <CeeEngine/renderer/renderer.h>

namespace cee {
	RenderThread::RenderThread(ThreadingPolicy policy)
	 : m_Policy(policy), m_Thread("Render Thread"), m_State(ThreadState::IDLE)
	{
	}

	RenderThread::~RenderThread() {

	}

	void RenderThread::Run() {
		m_Running = true;
		if (m_Policy == ThreadingPolicy::MULTITHREADED)
			m_Thread.Dispatch(Renderer::RenderThreadFucntion, this);
	}

	void RenderThread::Terminate() {
		m_Running = false;
		this->Pump();

		if (m_Policy == ThreadingPolicy::MULTITHREADED)
			m_Thread.Join();
	}

	void RenderThread::SetState(ThreadState setState) {
		if (m_Policy == ThreadingPolicy::SINGLETHREADED)
			return;

		std::unique_lock lock(m_Mutex);
		m_State = setState;
		m_ConditionVariable.notify_one();
	}

	void RenderThread::WaitAndSetState(ThreadState waitState, ThreadState setState) {
		if (m_Policy == ThreadingPolicy::SINGLETHREADED)
			return;

		std::unique_lock lock(m_Mutex);
		while (m_State != waitState)
			m_ConditionVariable.wait(lock);

		m_State = setState;
		m_ConditionVariable.notify_one();
	}

	void RenderThread::Wait(ThreadState waitState) {
		if (m_Policy == ThreadingPolicy::SINGLETHREADED)
			return;

		std::unique_lock lock(m_Mutex);
		while (m_State != waitState)
			m_ConditionVariable.wait(lock);
	}

	void RenderThread::NextFrame() {
		m_AppThreadFrame++;
		Renderer::SwapQueues();
	}

	void RenderThread::BlockUntilRenderComplete() {
		if (m_Policy == ThreadingPolicy::SINGLETHREADED)
			return;

		this->Wait(ThreadState::IDLE);
	}

	void RenderThread::Kick() {
		if (m_Policy == ThreadingPolicy::MULTITHREADED) {
			this->SetState(ThreadState::KICK);
		} else {
			Renderer::WaitAndRender(this);
		}
	}

	void RenderThread::Pump() {
		this->NextFrame();
		this->Kick();
		this->BlockUntilRenderComplete();
	}
}
