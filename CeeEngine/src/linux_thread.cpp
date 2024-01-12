/* Implementaion of custom thread.
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

#include <CeeEngine/thread.h>

#include <CeeEngine/debugMessenger.h>
#include <CeeEngine/assert.h>
#include <type_traits>

namespace cee {
	extern "C" {
		static void* ExecuteRoutine(void* ptr) {
			std::unique_ptr<Thread::StateBase> state{ static_cast<Thread::StateBase*>(ptr) };
			state->Run();
			return nullptr;
		}
	}

	Thread::StateBase::~StateBase() = default;

	Thread::Thread()
	: Thread("Unnamed Thread")
	{
	}

	Thread::Thread(const std::string& name)
	: m_Name(name), m_ID()
	{
	}

	Thread::~Thread() = default;

	void Thread::m_StartThread(std::unique_ptr<StateBase> state, void (*)()) {
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setschedpolicy(&attr, SCHED_RR);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

		int errnum = pthread_create(&m_ID.m_Thread, &attr, &ExecuteRoutine, state.get());
		CEE_VERIFY(errnum == 0, "Failed to create thread \"%s\", returned with \"%s\"", m_Name.c_str(), strerror(errnum));

		state.release();
	}


	void Thread::SetName(const std::string& name) {
		CEE_ASSERT(m_ID.m_Thread != 0, "Thread::SetName called at an invalid time");
		m_Name = name;
		pthread_setname_np(m_ID.m_Thread, name.c_str());
	}

	void Thread::Join() {
		void* ret;
		pthread_join(m_ID.m_Thread, &ret);
	}

	namespace ThisThread {
		Thread::ID GetID() {
			return Thread::ID(pthread_self());
		}

		void Yield() {
			sched_yield();
		}

		template<typename Rep, typename Period>
		void SleepFor(const std::chrono::duration<Rep, Period>& duration) {
			if (duration <= duration.zero())
				return;
			struct ::timespec ts = {
				static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(duration).count()),
				static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count())
			};
			while (::nanosleep(&ts, &ts) == -1 && errno == EINTR);
		}

		template<typename Clock, typename Duration>
		void SleepUntil(const std::chrono::time_point<Clock, Duration> atTime) {
			static_assert(std::chrono::is_clock_v<Clock>, "Invalid parameters");
			auto now = Clock::now();
			if (Clock::is_steady) {
				SleepFor(atTime - now);
				return;
			}
			while (atTime > now) {
				SleepFor(atTime - now);
				now = Clock::now();
			}
		}
	}

}

