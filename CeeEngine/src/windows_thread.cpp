/* implemntation of custom thread.
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

namespace cee {
	Thread::Thread()
	: Thread("Unnamed Thread")
	{
	}

	Thread::Thread(const std::string& name)
	: m_Name(name), m_Thread(0)
	{
	}

	Thread::~Thread() {

	}

	template<typename F, typename... Args>
	void Thread::Dispatch(F function, Args&&... args) {

	}

	void Thread::SetName(const std::string& name) {

	}

	void Thread::Join() {

	}
}
