/* Decleration of funtion submission queue.
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

#ifndef CEE_ENGINE_RENDERER_RENDER_THREAD_QUEUE_H_
#define CEE_ENGINE_RENDERER_RENDER_THREAD_QUEUE_H_

#include <cstddef>
#include <cstdint>

namespace cee {
	typedef void(*PFN_RenderCommand)(void*);

	// Not thread safe!!!
	// Using custom allocator to ensure contiguious block of memory with varying
	// size of function calls for performance.
	class RenderThreadQueue {
	public:
		RenderThreadQueue();
		~RenderThreadQueue();

		// Returns a pointer to store parameters for the function, usr std::forawrd.
		void* Submit(PFN_RenderCommand funtion, size_t size);
		void Execute();

	private:
		uint8_t* m_BufferBase;
		uint8_t* m_BufferPtr;
		size_t m_BufferSize;
		uint32_t m_SubmissionCount;
	};
}

#endif
