/* Decleration of custom thread.
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

#ifndef CEE_ENGINE_THREAD_H_
#define CEE_ENGINE_THREAD_H_

#include <memory>
#include <string>
#include <tuple>
#include <functional>
#include <type_traits>
#include <chrono>

#include <CeeEngine/platform.h>


#if defined(CEE_PLATFORM_LINUX)
#include <pthread.h>
#elif defined(CEE_PLATFORM_WINDOWS)
#error "Windows threading not yet implemented"
#endif

namespace cee {
	class Thread {
	private:
#if defined(CEE_PLATFORM_LINUX)
		using NativeThreadHandle = pthread_t;
#elif defined(CEE_PLATFORM_WINDOWS)
		using NativeThreadHandle = HANDLE;
#endif
	public:
		class ID {
		public:
			ID()
			: m_Thread()
			{	}
			explicit ID(NativeThreadHandle h)
			 : m_Thread(h)
			{	}

		private:
			NativeThreadHandle m_Thread;

#if __cpp_lib_three_way_comparison
			friend ::std::strong_ordering operator <=>(Thread::ID lhs, Thread::ID rhs) noexcept;
#endif
			friend bool operator ==(Thread::ID lhs, Thread::ID rhs) noexcept;
			friend bool operator <(Thread::ID lhs, Thread::ID rhs) noexcept;

			friend class Thread;
			friend struct std::hash<ID>;
		};
	public:
		Thread();
		Thread(const std::string& name);
		~Thread();

		template<typename F, typename... Args>
		void Dispatch(F&& function, Args&&... args) {
			static_assert(std::is_trivially_destructible_v<F>, "Objects must be trivially destructible.");

			using Wrapper = CallWrapper<F, Args...>;
			m_StartThread(std::unique_ptr<StateBase>(new State<Wrapper>(std::forward<F>(function), std::forward<Args>(args)...)), nullptr);
		}

		void SetName(const std::string& name);
		const std::string& GetName() const { return m_Name; }

		void Join();

		ID GetID() const { return m_ID; }

	private:
		template<size_t... T>
		struct IndexTuple {};

		template<size_t N, size_t... T>
		struct GenerateIndexTuple : public GenerateIndexTuple<N - 1, N - 1, T...> {};

		template<size_t... T>
		struct GenerateIndexTuple<0, T...> { typedef IndexTuple<T...> type; };

		template<size_t P>
		using GenerateIndexTupleVal = typename GenerateIndexTuple<P>::type;

		template<typename T>
		struct Invoker {
			template<typename... Args>
			Invoker(Args&&... args)
			 : m_Tuple(std::forward<Args>(args)...)
			{	}

			T m_Tuple;

			template<size_t... I>
			void Invoke(IndexTuple<I...>) {
				return std::invoke(std::get<I>(std::move(m_Tuple))...);
			}

			void operator()() {
				using indices = GenerateIndexTupleVal<std::tuple_size_v<T>>;
				return Invoke(indices());
			}
		};

		template<typename... T>
		using CallWrapper = Invoker<std::tuple<typename std::decay<T>::type...>>;

	public:
		struct StateBase {
			virtual ~StateBase();
			virtual void Run() = 0;
		};

	private:
		template<typename F>
		struct State : public StateBase {
			F m_Function;

			template<typename... Args>
			State(Args&&... args)
			 : m_Function(std::forward<Args>(args)...)
			{}

			void Run() override { m_Function(); }
		};

	private:
		void m_StartThread(std::unique_ptr<StateBase> state, void(*)());

	private:
		std::string m_Name;
		ID m_ID;
	};

#if __cpp_lib_three_way_comparison
	inline ::std::strong_ordering operator <=>(Thread::ID lhs, Thread::ID rhs) noexcept {
		return lhs.m_Thread <=> rhs.m_Thread;
	}
#endif
	inline bool operator==(Thread::ID lhs, Thread::ID rhs) noexcept {
		return lhs.m_Thread == rhs.m_Thread;
	}
	inline bool operator!=(Thread::ID lhs, Thread::ID rhs) noexcept {
		return !(lhs == rhs);
	}
	inline bool operator<(Thread::ID lhs, Thread::ID rhs) noexcept {
		return lhs.m_Thread < rhs.m_Thread;
	}
	inline bool operator>(Thread::ID lhs, Thread::ID rhs) noexcept {
		return rhs < lhs;
	}
	inline bool operator<=(Thread::ID lhs, Thread::ID rhs) noexcept {
		return !(lhs > rhs);
	}
	inline bool operator>=(Thread::ID lhs, Thread::ID rhs) noexcept {
		return !(lhs < rhs);
	}

	namespace ThisThread {
		Thread::ID GetID();
		void Yield();

		template<typename Rep, typename Period>
		void SleepFor(const std::chrono::duration<Rep, Period>& duration);

		template<typename Clock, typename Duration>
		void SleepUntil(const std::chrono::time_point<Clock, Duration>& atTime);
	}
}

#endif
